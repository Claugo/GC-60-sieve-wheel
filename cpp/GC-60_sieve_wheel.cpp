// ruota_sperimentale.cpp
// Autore: Govi Claudio
// Anno di creazione: 2026
// Mese di creazione: Marzo
// Version: V2.0.0
// ============================================================
//  sieve_wheel_M60_v2.cpp
//  Sieve segmentato con ruota mod-60 — versione ottimizzata
// sieve_wheel_M60_conta_turbo.cpp
// Autore: Govi Claudio
// ============================================================
//  Sieve segmentato con ruota mod-60 — Versione Multi-Thread 
//  OTTIMIZZAZIONE MASSIMA: Bit-Packing e Hardware Popcount
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
#include <bit> // Fondamentale per il conteggio hardware dei bit (C++20)

std::string fmt(long long n) {
    std::string s = std::to_string(n);
    int ins = (int)s.size() - 3;
    while (ins > 0) { s.insert(ins, "."); ins -= 3; }
    return s;
}

void stampa_header() {
    std::cout << "================================================\n";
    std::cout << "******* Programma a liste traslate    ********\n";
    std::cout << " Lista impostata a 131.072:\n\n";
    std::cout << " 10   miliardi  -  1.272 cicli \n";
    std::cout << " 100  miliardi  -  12.723 cicli \n";
    std::cout << " 1000 miliardi  -  127.235 cicli \n";
    std::cout << "================================================\n\n";
}

std::vector<long long> lista_primi = {
    7,11,13,17,19,23,29,31,37,41,43,47,53,59,61,67,71,73,79,83,89,97,
    101,103,107,109,113,127,131,137,139,149,151,157,163,167,173,179,181,
    191,193,197,199,211,223,227,229,233,239,241,251,257,263,269,271,277,
    281,283,293,307,311,313,317,331,337,347,349,353,359,367,373,379,383,
    389,397,401,409,419,421,431,433,439,443,449,457,461,463,467,479,487,
    491,499,503,509,521,523,541,547,557,563,569,571,577,587,593,599,601,
    607,613,617,619,631,641,643,647,653,659,661,673,677,683,691,701,709,
    719,727,733,739,743,751,757,761,769,773,787,797,809,811,821,823,827,
    829,839,853,857,859,863,877,881,883,887,907,911,919,929,937,941,947,
    953,967,971,977,983,991,997,1009,1013,1019,1021,1031,1033,1039,1049,
    1051,1061,1063,1069,1087
};

const long long dimensione_maschera = 131072;
const int sottolista_base[16] = { 1, 3, 7, 9, 13, 19, 21, 27, 31, 33, 37, 39, 43, 49, 51, 57 };
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
    }
    else {
        long long A = riferimento * 60 + 10;
        start = A - (A % p);
        if (start % 2 == 0) start += p;
        else start += 2 * p;
    }

    long long prodotto = start;
    long long p_lista = (prodotto - 10) / 60 - riferimento;
    int       p_numero = (int)((prodotto - 10) % 60);

    if (lookup[p_numero] >= 0) {
        out.lista[out.len] = p_lista;
        out.numero[out.len] = p_numero;
        out.len++;
    }
    else {
        for (int i = 0; i < 5; i++) {
            prodotto += 2 * p;
            p_lista = (prodotto - 10) / 60 - riferimento;
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
        p_numero = (int)((prodotto - 10) % 60);
        if (p_numero == termina) break;
        if (lookup[p_numero] >= 0) {
            p_lista = (prodotto - 10) / 60 - riferimento;
            out.lista[out.len] = p_lista;
            out.numero[out.len] = p_numero;
            out.len++;
        }
    }
}

int main() {
    stampa_header();

    // Mostriamo i thread che verranno usati
    int n_threads = omp_get_max_threads();
    std::cout << "Lancio del calcolo C++ su " << n_threads << " thread (OpenMP)..." << std::endl;

    auto t0 = std::chrono::high_resolution_clock::now();

    memset(lookup, -1, sizeof(lookup));
    for (int i = 0; i < 16; i++) lookup[sottolista_base[i]] = i;

    //***************************************************************
    const int quanti_cicli = 127235; // Modifica per 10 miliardi, ecc.
    //***************************************************************

    long long radice_massima_totale = (long long)std::sqrt((double)(dimensione_maschera * quanti_cicli * 60)) + 1;
    long long contatore_totale = 3;
    long long total_bits = 16 * dimensione_maschera;
    long long array_size = total_bits / 64; // Numero esatto di elementi a 64-bit necessari

    // ====================================================================
    // FASE 1: SEGMENTO 0 (Costruzione archivio divisori in seriale)
    // ====================================================================
    {
        // Usa uint64_t: comprime 64 "vuoti" in un solo elemento.
        std::vector<uint64_t> maschera_0(array_size, 0);
        CicloRisultato ciclo_buf;
        long long radice_0 = (long long)std::sqrt((double)(dimensione_maschera * 60)) + 1;

        for (long long p : lista_primi) {
            if (p > radice_0) break;
            ricerca_ciclo(p, 0, ciclo_buf);
            if (ciclo_buf.len == 0) continue;

            for (long long ii = 0; ; ii++) {
                bool stop = false;
                for (int i = 0; i < ciclo_buf.len; i++) {
                    long long indice = ciclo_buf.lista[i] + p * ii;
                    if (indice > dimensione_maschera - 1) { stop = true; break; }

                    int residuo = lookup[ciclo_buf.numero[i]];
                    long long bit_idx = (indice << 4) + residuo;

                    // Magia del bit-packing hardware:
                    maschera_0[bit_idx >> 6] |= (1ULL << (bit_idx & 63));
                }
                if (stop) break;
            }
        }

        long long ones_0 = 0;
        for (uint64_t val : maschera_0) ones_0 += std::popcount(val); // Conta velocissimo gli 1
        contatore_totale += total_bits - ones_0; // Se totale-1 hai gli 0 sopravvissuti!

        lista_primi.clear();
        lista_primi.push_back(7);
        for (long long bit_idx = 0; bit_idx < total_bits; bit_idx++) {
            if ((maschera_0[bit_idx >> 6] & (1ULL << (bit_idx & 63))) == 0) {
                long long somma = (bit_idx / 16) * 60 + 10 + sottolista_base[bit_idx % 16];
                if (somma > radice_massima_totale) break;
                if (somma > 7) lista_primi.push_back(somma);
            }
        }
    }

    // ====================================================================
    // FASE 2: SEGMENTI PARALLELI (Finestra passiva e conteggio)
    // ====================================================================

    // Rimozione schedule(dynamic) per evitare colli di bottiglia tra i thread
#pragma omp parallel reduction(+:contatore_totale)
    {
        std::vector<uint64_t> maschera_bit(array_size, 0);
        CicloRisultato ciclo_buf;

#pragma omp for
        for (int cicli = 1; cicli < quanti_cicli; cicli++) {

            // Azzeriamo la memoria ultra-ridotta del thread corrente
            std::fill(maschera_bit.begin(), maschera_bit.end(), 0);

            long long riferimento = (long long)dimensione_maschera * cicli;
            long long radice = (long long)std::sqrt((double)(dimensione_maschera * (cicli + 1) * 60)) + 1;

            for (long long p : lista_primi) {
                if (p > radice) break;

                ricerca_ciclo(p, riferimento, ciclo_buf);
                if (ciclo_buf.len == 0) continue;

                for (long long ii = 0; ; ii++) {
                    bool stop = false;
                    for (int i = 0; i < ciclo_buf.len; i++) {
                        long long indice = ciclo_buf.lista[i] + p * ii;
                        if (indice > dimensione_maschera - 1) { stop = true; break; }

                        int residuo = lookup[ciclo_buf.numero[i]];
                        long long bit_idx = (indice << 4) + residuo;

                        // Setta il singolo bit
                        maschera_bit[bit_idx >> 6] |= (1ULL << (bit_idx & 63));
                    }
                    if (stop) break;
                }
            }

            // Conta tutti i bit a 1 nel blocco e sottraili al totale per ottenere i bit a 0
            long long ones_nel_segmento = 0;
            for (uint64_t val : maschera_bit) {
                ones_nel_segmento += std::popcount(val);
            }
            contatore_totale += total_bits - ones_nel_segmento;
        }
    }

    auto t1 = std::chrono::high_resolution_clock::now();
    double sec = std::chrono::duration<double>(t1 - t0).count();
    long long numero_totale = (long long)dimensione_maschera * quanti_cicli * 60;

    std::cout << "\n";
    std::cout << "================================================\n";
    std::cout << "      RISULTATI TURBO GC-60 (Multi-Thread C++)\n";
    std::cout << "================================================\n";
    std::cout << "  Cicli eseguiti    : " << fmt(quanti_cicli) << "\n";
    std::cout << "  Numero totale max : " << fmt(numero_totale) << "\n";
    std::cout << "  Radice massima    : " << fmt(radice_massima_totale) << "\n";
    std::cout << "  Primi alla radice : " << fmt((long long)lista_primi.size()) << "\n";
    std::cout << "  Primi totali      : " << fmt(contatore_totale) << "\n";
    std::cout << "  Tempo esecuzione  : " << sec << " s\n";
    std::cout << "================================================\n";

    return 0;
}