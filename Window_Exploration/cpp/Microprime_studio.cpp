// microprime_2.cpp
// Autore: Govi Claudio
// Mese: 03
// Anno: 2026
// ============================================================
//  MicroPrime Studio — Finestra Passiva GC-60
//
//  Base: sieve_wheel_M60_conta.cpp (bit-packing + OpenMP)
//  Estensione: logica finestra passiva da ruota_sperimentale_8C_4.jl
//
//  Funzionamento:
//    Fase 1 — Setaccio GC-60 fino a sqrt(A_TARGET + L_FINESTRA)
//             Filtra solo i divisori che colpiscono la finestra
//    Fase 2 — Finestra booleana passiva: proietta i divisori
//             e raccoglie i sopravvissuti (= numeri primi)
//    Fase 3 — Buffer circolare da 25 primi, pronto per analisi
// ============================================================

#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <vector>
#include <cmath>
#include <chrono>
#include <fstream>
#include <string>
#include <cstring>
#include <algorithm>
#include <omp.h>
#include <bit>       // std::popcount (C++20)

// ============================================================
// __int128: supporto stampa e operazioni
// ============================================================
using i128 = __int128;

// Stampa un __int128 come stringa decimale
std::string i128_to_str(i128 n) {
    if (n == 0) return "0";
    bool neg = (n < 0);
    if (neg) n = -n;
    std::string s;
    while (n > 0) {
        s = char('0' + (int)(n % 10)) + s;
        n /= 10;
    }
    if (neg) s = "-" + s;
    return s;
}

// Formatta con punti (es: 10.000.000)
std::string fmt128(i128 n) {
    std::string s = i128_to_str(n);
    int ins = (int)s.size() - 3;
    while (ins > 0) { s.insert(ins, "."); ins -= 3; }
    return s;
}

std::string fmt(long long n) {
    std::string s = std::to_string(n);
    int ins = (int)s.size() - 3;
    while (ins > 0) { s.insert(ins, "."); ins -= 3; }
    return s;
}

// isqrt per __int128
i128 isqrt128(i128 n) {
    if (n < 0) return 0;
    if (n == 0) return 0;
    i128 x = (i128)sqrtl((long double)n);
    // correzione fine
    while (x * x > n) x--;
    while ((x+1)*(x+1) <= n) x++;
    return x;
}

// ============================================================
// PARAMETRI FINESTRA TARGET
// Modifica A_TARGET e L_FINESTRA per esplorare zone diverse
// ============================================================
const i128 A_TARGET    = (i128)10000000000000000000ULL * 100000000ULL;
const i128 L_FINESTRA  = 10000000;   // 10 milioni

// ============================================================
// Parametri GC-60
// ============================================================
const long long DIMENSIONE_MASCHERA = 131072;
const int SOTTOLISTA_BASE[16] = { 1, 3, 7, 9, 13, 19, 21, 27, 31, 33, 37, 39, 43, 49, 51, 57 };
int lookup[60];

struct CicloRisultato {
    long long lista[16];
    int       numero[16];
    int       len;
};

void ricerca_ciclo(long long p, long long riferimento, CicloRisultato& out) {
    out.len = 0;
    long long start;
    if (riferimento == 0) {
        start = p * p;
    } else {
        long long A = riferimento * 60 + 10;
        start = A - (A % p);
        if (start % 2 == 0) start += p;
        else                 start += 2 * p;
    }

    long long prodotto = start;
    long long p_lista  = (prodotto - 10) / 60 - riferimento;
    int       p_numero = (int)((prodotto - 10) % 60);

    if (lookup[p_numero] >= 0) {
        out.lista[out.len] = p_lista;
        out.numero[out.len] = p_numero;
        out.len++;
    } else {
        for (int i = 0; i < 5; i++) {
            prodotto += 2 * p;
            p_lista  = (prodotto - 10) / 60 - riferimento;
            p_numero = (int)((prodotto - 10) % 60);
            if (lookup[p_numero] >= 0) {
                out.lista[out.len] = p_lista;
                out.numero[out.len] = p_numero;
                out.len++;
                break;
            }
        }
    }
    if (out.len == 0) return;
    int termina = p_numero;

    for (int i = 0; i < 1000000; i++) {
        prodotto += p * 2;
        p_numero  = (int)((prodotto - 10) % 60);
        if (p_numero == termina) break;
        if (lookup[p_numero] >= 0) {
            p_lista = (prodotto - 10) / 60 - riferimento;
            out.lista[out.len] = p_lista;
            out.numero[out.len] = p_numero;
            out.len++;
        }
    }
}

// ============================================================
// FILTRO "DIVISORE UTILE"
// Restituisce true se p ha almeno un multiplo nella finestra
// [A_TARGET, A_TARGET + L_FINESTRA]
// ============================================================
bool e_divisore_utile(long long p, i128 a_target, i128 fine_finestra) {
    // Primo multiplo dispari di p >= a_target
    i128 st = a_target - (a_target % (i128)p);
    if (st % 2 == 0) st += (i128)p;
    else              st += (i128)(2 * p);
    return (st < fine_finestra);
}

// ============================================================
// BUFFER CIRCOLARE DA 25 PRIMI
// Struttura pronta per analisi strutturali future
// ============================================================
struct PrimeWindow {
    static const int CAPACITY = 25;
    i128 data[CAPACITY];
    int  count = 0;   // quanti slot sono occupati (0..25)
    int  head  = 0;   // indice del più vecchio (wraparound)

    bool full()  const { return count == CAPACITY; }
    bool empty() const { return count == 0; }

    // Inserisce un primo; se pieno, scarta il più vecchio
    void push(i128 p) {
        int slot;
        if (count < CAPACITY) {
            slot = (head + count) % CAPACITY;
            count++;
        } else {
            // sovrascrive il più vecchio, avanza head
            slot = head;
            head = (head + 1) % CAPACITY;
        }
        data[slot] = p;
    }

    // Accesso ordinato: element(0) = più vecchio
    i128 element(int i) const {
        return data[(head + i) % CAPACITY];
    }

    // Analisi strutturale — da espandere in futuro
    // Per ora solo placeholder commentato
    // void analyze(Stats& s) { ... }
};

// ============================================================
// FASE 2: FINESTRA PASSIVA
// Proietta i divisori utili su un array booleano e raccoglie
// i sopravvissuti nel buffer circolare
// ============================================================
std::vector<i128> applica_finestra_passiva(
    const std::vector<long long>& divisori,
    i128 a_target,
    i128 l_finestra,
    PrimeWindow& window)
{
    i128 fine_finestra = a_target + l_finestra;

    std::cout << "\n======= AVVIO SCREMATURA FINESTRA PASSIVA =======\n";
    std::cout << "Finestra: " << fmt128(L_FINESTRA) << "\n";
    std::cout << "Divisori applicati: " << fmt((long long)divisori.size()) << "\n";

    auto t_start = std::chrono::high_resolution_clock::now();

    // Array booleano: indice i = numero a_target + i
    long long sz = (long long)l_finestra + 1;
    std::vector<uint8_t> finestra(sz, 1);  // 1 = presunto primo

    // Cancella i multipli di ogni divisore nella finestra
    for (long long p : divisori) {
        // Primo multiplo di p >= a_target
        i128 resto = a_target % (i128)p;
        i128 offset_iniziale = (resto == 0) ? 0 : ((i128)p - resto);
        long long idx = (long long)offset_iniziale;

        for (; idx <= l_finestra; idx += p)
            finestra[idx] = 0;
    }

    auto t_end = std::chrono::high_resolution_clock::now();
    double sec = std::chrono::duration<double>(t_end - t_start).count();

    // Raccolta sopravvissuti con check finale su 2, 3, 5, 7
    std::vector<i128> sopravvissuti;
    sopravvissuti.reserve(200000);

    for (long long i = 0; i <= l_finestra; i++) {
        if (finestra[i]) {
            i128 n = a_target + (i128)i;
            if (n % 2 != 0 && n % 3 != 0 && n % 5 != 0 && n % 7 != 0) {
                sopravvissuti.push_back(n);
                window.push(n);
            }
        }
    }

    std::cout << "-----------------------------------------\n";
    std::cout << "Primi trovati:  " << fmt((long long)sopravvissuti.size()) << "\n";
    std::cout << "Tempo scrematura: " << sec << " s\n";
    std::cout << "-----------------------------------------\n";

    return sopravvissuti;
}

// ============================================================
// Bootstrap primers (fino a ~2803, coprono il segmento 0)
// ============================================================
std::vector<long long> lista_primi_bootstrap = {
    7,11,13,17,19,23,29,31,37,41,43,47,53,59,61,67,71,73,79,83,89,97,
    101,103,107,109,113,127,131,137,139,149,151,157,163,167,173,179,181,191,193,197,199,211,
    223,227,229,233,239,241,251,257,263,269,271,277,281,283,293,307,311,313,317,331,337,347,
    349,353,359,367,373,379,383,389,397,401,409,419,421,431,433,439,443,449,457,461,463,467,
    479,487,491,499,503,509,521,523,541,547,557,563,569,571,577,587,593,599,601,607,613,617,
    619,631,641,643,647,653,659,661,673,677,683,691,701,709,719,727,733,739,743,751,757,761,
    769,773,787,797,809,811,821,823,827,829,839,853,857,859,863,877,881,883,887,907,911,919,
    929,937,941,947,953,967,971,977,983,991,997,1009,1013,1019,1021,1031,1033,1039,1049,1051,
    1061,1063,1069,1087,1091,1093,1097,1103,1109,1117,1123,1129,1151,1153,1163,1171,1181,1187,
    1193,1201,1213,1217,1223,1229,1231,1237,1249,1259,1277,1279,1283,1289,1291,1297,1301,1303,
    1307,1319,1321,1327,1361,1367,1373,1381,1399,1409,1423,1427,1429,1433,1439,1447,1451,1453,
    1459,1471,1481,1483,1487,1489,1493,1499,1511,1523,1531,1543,1549,1553,1559,1567,1571,1579,
    1583,1597,1601,1607,1609,1613,1619,1621,1627,1637,1657,1663,1667,1669,1693,1697,1699,1709,
    1721,1723,1733,1741,1747,1753,1759,1777,1783,1787,1789,1801,1811,1823,1831,1847,1861,1867,
    1871,1873,1877,1879,1889,1901,1907,1913,1931,1933,1949,1951,1973,1979,1987,1993,1997,1999,
    2003,2011,2017,2027,2029,2039,2053,2063,2069,2081,2083,2087,2089,2099,2111,2113,2129,2131,
    2137,2141,2143,2153,2161,2179,2203,2207,2213,2221,2237,2239,2243,2251,2267,2269,2273,2281,
    2287,2293,2297,2309,2311,2333,2339,2341,2347,2351,2357,2371,2377,2381,2383,2389,2393,2399,
    2411,2417,2423,2437,2441,2447,2459,2467,2473,2477,2503,2521,2531,2539,2543,2549,2551,2557,
    2579,2591,2593,2609,2617,2621,2633,2647,2657,2659,2663,2671,2677,2683,2687,2689,2693,2699,
    2707,2711,2713,2719,2729,2731,2741,2749,2753,2767,2777,2789,2791,2797,2801,2803
};

// ============================================================
// MAIN
// ============================================================
int main() {
    std::cout << "================================================\n";
    std::cout << "   MicroPrime_2 03/2026 Finestra Passiva GC-60\n";
    std::cout << "================================================\n";

    i128 fine_finestra = A_TARGET + L_FINESTRA;
    i128 radice_massima = isqrt128(fine_finestra) + 1;

    // Quanti cicli GC-60 servono per coprire fino a radice_massima
    long long quanti_cicli = (long long)(radice_massima / ((i128)DIMENSIONE_MASCHERA * 60)) + 2;

    std::cout << "Target             : " << fmt128(A_TARGET) << "\n";
    std::cout << "Dimensione Finestra: " << fmt128(L_FINESTRA) << "\n";
    std::cout << "Radice massima     : " << fmt128(radice_massima) << "\n";
    std::cout << "Cicli GC-60        : " << fmt(quanti_cicli) << "\n";
    std::cout << "Thread OpenMP      : " << omp_get_max_threads() << "\n";
    std::cout << "================================================\n\n";

    memset(lookup, -1, sizeof(lookup));
    for (int i = 0; i < 16; i++) lookup[SOTTOLISTA_BASE[i]] = i;

    long long total_bits = 16 * DIMENSIONE_MASCHERA;
    long long array_size = total_bits / 64;

    // Archivio divisori utili (thread-safe: uno per ciclo parallelo)
    std::vector<long long> divisori_fase1;
    std::vector<std::vector<long long>> divisori_per_ciclo(quanti_cicli - 1);

    auto t0 = std::chrono::high_resolution_clock::now();

    // ============================================================
    // FASE 1: SEGMENTO 0 — seriale
    // ============================================================
    {
        std::vector<uint64_t> maschera_0(array_size, 0);
        CicloRisultato ciclo_buf;
        long long radice_0 = (long long)std::sqrt((double)(DIMENSIONE_MASCHERA * 60)) + 1;

        for (long long p : lista_primi_bootstrap) {
            if (p > radice_0) break;
            ricerca_ciclo(p, 0, ciclo_buf);
            if (ciclo_buf.len == 0) continue;
            for (long long ii = 0; ; ii++) {
                bool stop = false;
                for (int i = 0; i < ciclo_buf.len; i++) {
                    long long indice = ciclo_buf.lista[i] + p * ii;
                    if (indice > DIMENSIONE_MASCHERA - 1) { stop = true; break; }
                    int residuo = lookup[ciclo_buf.numero[i]];
                    long long bit_idx = (indice << 4) + residuo;
                    maschera_0[bit_idx >> 6] |= (1ULL << (bit_idx & 63));
                }
                if (stop) break;
            }
        }

        // Estrazione divisori dal segmento 0
        lista_primi_bootstrap.clear();
        lista_primi_bootstrap.push_back(7);

        for (long long bit_idx = 0; bit_idx < total_bits; bit_idx++) {
            if ((maschera_0[bit_idx >> 6] & (1ULL << (bit_idx & 63))) == 0) {
                long long p = (bit_idx / 16) * 60 + 10 + SOTTOLISTA_BASE[bit_idx % 16];
                if (p > (long long)radice_massima) break;
                if (p > 7) lista_primi_bootstrap.push_back(p);

                // Filtro utile per la finestra target
                if (e_divisore_utile(p, A_TARGET, fine_finestra))
                    divisori_fase1.push_back(p);
            }
        }
    }

    std::cout << "Avvio Scrematura Globale in Parallelo...\n";

    // ============================================================
    // FASE 2: CICLI PARALLELI — filtro divisori utili
    // ============================================================
    #pragma omp parallel for schedule(dynamic, 4)
    for (long long cicli = 1; cicli < quanti_cicli; cicli++) {
        std::vector<uint64_t> maschera_bit(array_size, 0);
        CicloRisultato ciclo_buf;

        long long riferimento = DIMENSIONE_MASCHERA * cicli;
        long long radice_c = (long long)std::sqrt((double)(DIMENSIONE_MASCHERA * (cicli + 1) * 60)) + 1;

        for (long long p : lista_primi_bootstrap) {
            if (p > radice_c) break;
            ricerca_ciclo(p, riferimento, ciclo_buf);
            if (ciclo_buf.len == 0) continue;
            for (long long ii = 0; ; ii++) {
                bool stop = false;
                for (int i = 0; i < ciclo_buf.len; i++) {
                    long long indice = ciclo_buf.lista[i] + p * ii;
                    if (indice > DIMENSIONE_MASCHERA - 1) { stop = true; break; }
                    int residuo = lookup[ciclo_buf.numero[i]];
                    long long bit_idx = (indice << 4) + residuo;
                    maschera_bit[bit_idx >> 6] |= (1ULL << (bit_idx & 63));
                }
                if (stop) break;
            }
        }

        // Spoglio: cerca i 0 (= candidati primi) e filtra quelli utili
        for (long long bit_idx = 0; bit_idx < total_bits; bit_idx++) {
            if ((maschera_bit[bit_idx >> 6] & (1ULL << (bit_idx & 63))) == 0) {
                long long p = (bit_idx / 16) * 60 + 10
                            + (long long)(DIMENSIONE_MASCHERA * cicli) * 60
                            + SOTTOLISTA_BASE[bit_idx % 16];

                if ((long long)p > (long long)radice_massima) break;

                if (e_divisore_utile(p, A_TARGET, fine_finestra))
                    divisori_per_ciclo[cicli - 1].push_back(p);
            }
        }
    }

    // Unione divisori fase1 + tutti i cicli
    std::vector<long long> archivio_utili = divisori_fase1;
    for (auto& v : divisori_per_ciclo)
        archivio_utili.insert(archivio_utili.end(), v.begin(), v.end());

    auto t1 = std::chrono::high_resolution_clock::now();
    double sec_fase12 = std::chrono::duration<double>(t1 - t0).count();

    std::cout << "\n---------------------------------------------\n";
    //std::cout << "Finestra Target: " << fmt128(A_TARGET)
              //<< " + " << fmt128(L_FINESTRA) << "\n";
    std::cout << "Divisori entrati nella Finestra: " << fmt((long long)archivio_utili.size()) << "\n";
    std::cout << "Tempo generazione e filtro: " << sec_fase12 << " s\n";
    std::cout << "---------------------------------------------\n";

    // Salva divisori utili
    {
        std::ofstream f("divisori_utili.txt");
        for (long long d : archivio_utili) f << d << "\n";
        std::cout << "Divisori salvati in: divisori_utili.txt\n";
    }

    // ============================================================
    // FASE 3: FINESTRA PASSIVA + BUFFER 25 PRIMI
    // ============================================================
    PrimeWindow window;
    std::vector<i128> sopravvissuti = applica_finestra_passiva(
        archivio_utili, A_TARGET, (long long)L_FINESTRA, window);

    // Salva sopravvissuti
    {
        std::ofstream f("sopravvissuti.txt");
        for (i128 p : sopravvissuti) f << i128_to_str(p) << "\n";
        std::cout << "Primi Trovati salvati in: sopravvissuti.txt\n";
    }

    // Mostra il primo e l'ultimo del buffer
    std::cout << "\n--- Buffer: primo e ultimo primo della finestra ---\n";
    std::cout <<"\n";
    std::cout << "  primo: " << fmt128(window.element(0)) << "\n";
    std::cout << "  ultimo: " << fmt128(window.element(window.count - 1)) << "\n";

    // Mostra gli ultimi 25 nel buffer (= ultima finestra attiva)
    //std::cout << "\n--- Buffer finale (ultimi " << window.count << " primi) ---\n";
    //for (int i = 0; i < window.count; i++)
    //    std::cout << "  [" << i << "] " << fmt128(window.element(i)) << "\n";

    auto t2 = std::chrono::high_resolution_clock::now();
    double sec_totale = std::chrono::duration<double>(t2 - t0).count();
    std::cout << "\nTempo totale: " << sec_totale << " s\n";
    std::cout << "================================================\n";

    std::cout << "\nPremi INVIO per chiudere...\n";
    std::cin.get();
    return 0;
}
