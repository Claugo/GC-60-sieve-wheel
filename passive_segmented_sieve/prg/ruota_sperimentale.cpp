// ruota_sperimentale.cpp
// Autore: Govi Claudio
// Anno di creazione: 2026
// Mese di creazione: Marzo
// Version: V1.0.0
// ============================================================
//  ruota_sperimentale.cpp
//  Sieve segmentato con ruota mod-60
// ------------------------------------------------------------
//  PRINCIPIO:
//  Tutti i numeri primi > 5 sono coprimi con 2, 3 e 5.
//  Mod 60 esistono esattamente 16 residui con questa proprietà:
//  {1,3,7,9,13,19,21,27,31,33,37,39,43,49,51,57}
//  Il sieve lavora SOLO su questi candidati, ignorando il 73%
//  dei numeri per costruzione.
//
//  STRUTTURA:
//  - La memoria è divisa in segmenti di dimensione fissa
//    (dimensione_maschera × 60 numeri per segmento)
//  - Per ogni primo p, ricerca_ciclo() calcola il pattern
//    periodico dei suoi multipli sui 16 residui del segmento
//  - La maschera_bit viene resettata ad ogni segmento
//
//  LISTA PRIMI (autoalimentata):
//  - Segmento 0: usa il bootstrap hardcoded, poi lo sostituisce
//    con i primi estratti dalla maschera fino a radice1
//  - Segmenti successivi: aggiunge alla lista solo i primi
//    nell'intervallo (radice_precedente, radice_nuovo]
//  - Il bootstrap non serve mai più dopo il primo segmento
//
//  SCALABILITÀ:
//  Verificato corretto fino a 9.8 miliardi di numeri coperti
//  (10000 cicli, 9518 primi, radice 99157) — scala linearmente
// ============================================================
#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <vector>
#include <unordered_map>
#include <cmath>
#include <chrono>
#include <fstream>
#include <string>

//************************************************
//*******     lista primi boot strap      ********
//************************************************

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

//************************************************
//*******          crea maschere          ********
//************************************************

const long long dimensione_maschera = 16385;

const int sottolista_base[16] = { 1, 3, 7, 9, 13, 19, 21, 27, 31, 33, 37, 39, 43, 49, 51, 57 };

std::unordered_map<int, int> mappa;

//************************************************
//******* creazione ciclo sul numero primo********
//************************************************

void ricerca_ciclo(long long p, long long riferimento,
    std::vector<long long>& lista, std::vector<int>& numero)
{
    long long start;
    if (riferimento == 0) {
        start = p * p;
    }
    else {
        long long A = riferimento * 60+10;
        start = A - (A % p);
        if (start % 2 == 0)
            start += p;
        else
            start += 2 * p;
    }

    lista.clear();
    numero.clear();

    long long prodotto = start;
    long long p_lista = (prodotto - 10) / 60 - riferimento;
    int p_numero = (int)((prodotto - 10) % 60);

    auto in_base = [&](int v) -> bool {
        return mappa.count(v) > 0;
        };

    if (in_base(p_numero)) {
        lista.push_back(p_lista);
        numero.push_back(p_numero);
    }
    else {
        for (int i = 0; i < 5; i++) {
            prodotto += 2 * p;
            p_lista = (prodotto - 10) / 60 - riferimento;
            p_numero = (int)((prodotto - 10) % 60);
            if (in_base(p_numero)) {
                lista.push_back(p_lista);
                numero.push_back(p_numero);
                break;
            }
        }
    }

    int termina = p_numero;

    for (int i = 0; i < 1000000; i++) {
        prodotto += p * 2;
        p_numero = (int)((prodotto - 10) % 60);
        if (p_numero == termina)
            break;
        if (in_base(p_numero)) {
            p_lista = (prodotto - 10) / 60 - riferimento;
            lista.push_back(p_lista);
            numero.push_back(p_numero);
        }
    }
}

//************************************************
//*******           main                  ********
//************************************************

int main()
{
    auto t0 = std::chrono::high_resolution_clock::now();

    // inizializza mappa
    for (int i = 0; i < 16; i++)
        mappa[sottolista_base[i]] = i;

    //***************************************************************
    // Selezionare i cicli in base al numero che si vuole raggiungere
	// ogni ciclo è un numero pari a dimensione_maschera * 60
    const int quanti_cicli = 10000;
    //***************************************************************
    
    long long radice1 = (long long)std::sqrt((double)(dimensione_maschera * (quanti_cicli + 1) * 60)) + 1;

    std::vector<int> maschera_bit(16 * dimensione_maschera);

    for (int cicli = 0; cicli < quanti_cicli; cicli++) {

        // reset maschera
        std::fill(maschera_bit.begin(), maschera_bit.end(), 0);

        long long riferimento = dimensione_maschera * cicli;
        long long radice = (long long)std::sqrt((double)(dimensione_maschera * (cicli + 1) * 60)) + 1;

        //************************************************
        //*******           scrematura            ********
        //************************************************

        for (long long p : lista_primi) {
            if (p > radice) {
                break;
            }

            std::vector<long long> lista;
            std::vector<int> numero;
            ricerca_ciclo(p, riferimento, lista, numero);

            if (lista.empty()) continue;
            int ciclo_len = (int)lista.size();

            bool stop = false;
            for (long long ii = 0; ; ii++) {
                for (int i = 0; i < ciclo_len; i++) {
                    long long indice = lista[i] + p * ii;
                    if (indice < 0) continue;
                    if (indice > dimensione_maschera - 1) {
                        stop = true;
                        break;
                    }
                    int residuo = mappa[numero[i]];
                    maschera_bit[(indice << 4) + residuo] = 1;
                }
                if (stop) break;
            }
        }

        //************************************************
        //*******    aggiornamento lista_primi    ********
        //************************************************

        if (riferimento == 0) {
            // sostituisce il bootstrap
            lista_primi.clear();
            lista_primi.push_back(7);
            for (long long i = 0; i < (long long)maschera_bit.size(); i++) {
                if (maschera_bit[i] == 0) {
                    long long somma = (i / 16) * 60 + 10 + riferimento * 60 + sottolista_base[i % 16];
                    if (somma > radice1) break;
                    if (somma > 7) lista_primi.push_back(somma);
                }
            }
        }
        else {
            // aggiunge solo i nuovi primi
            for (long long i = 0; i < (long long)maschera_bit.size(); i++) {
                if (maschera_bit[i] == 0) {
                    long long somma = (i / 16) * 60 + 10 + riferimento * 60 + sottolista_base[i % 16];
                    if (somma > radice1) break;
                    lista_primi.push_back(somma);
                }
            }
        }
    }

    auto t1 = std::chrono::high_resolution_clock::now();
    double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

    long long numero_totale = (long long)dimensione_maschera * quanti_cicli * 60;

    std::cout << "\n";
    std::cout << "================================================\n";
    std::cout << "  Cicli eseguiti    : " << quanti_cicli << "\n";
    std::cout << "  Numero totale     : " << numero_totale << "\n";
    std::cout << "  Radice massima    : " << radice1 << "\n";
    std::cout << "  Primi in lista    : " << lista_primi.size() << "\n";
    std::cout << "  Tempo esecuzione  : " << ms << " ms\n";
    std::cout << "================================================\n";

    // stima occupazione su disco
    // ogni numero occupa in media log10(p)+1 cifre + newline ≈ stimiamo dai dati reali
    long long ultimo = lista_primi.empty() ? 0 : lista_primi.back();
    int cifre_medie = (int)std::log10((double)(ultimo > 0 ? ultimo : 1)) + 1;
    long long bytes_stimati = (long long)lista_primi.size() * (cifre_medie + 1); // +1 per \n
    double kb = bytes_stimati / 1024.0;
    double mb = kb / 1024.0;

    std::cout << "\n  Salvataggio lista_primi su disco:\n";
    std::cout << "  Dimensione stimata: " << bytes_stimati << " bytes";
    if (mb >= 1.0)
        std::cout << "  (" << mb << " MB)\n";
    else
        std::cout << "  (" << kb << " KB)\n";
    std::cout << "\n  Vuoi salvare la lista_primi per verifica? (s/n): ";

    std::string risposta;
    std::cin >> risposta;
    if (risposta == "s" || risposta == "S") {
        std::ofstream f("lista_primi_verifica.txt");
        f << 7 << "\n";
        for (long long p : lista_primi)
            if (p > 7) f << p << "\n";
        f.close();
        std::cout << "  Salvato in lista_primi_verifica.txt\n";
    }

    return 0;
}