# crea mappe
from math import isqrt
import time

start_time = time.perf_counter()
#************************************************
#*******     lista primi boot strap      ********
#************************************************

lista_primi = Int64[7,11,13,17,19,23,29,31,37,41,43,47,53,59,61,67,71,73,79,83,89,97,101,103,107,109,113,127,
    131,137,139,149,151,157,163,167,173,179,181,191,193,197,199,211,223,227,229,233,239,241,251,257,263,269,271,
    277,281,283,293,307,311,313,317,331,337,347,349,353,359,367,373,379,383,389,397,401,409,419,421,431,433,439,
    443,449,457,461,463,467,479,487,491,499,503,509,521,523,541,547,557,563,569,571,577,587,593,599,601,607,613,
    617,619,631,641,643,647,653,659,661,673,677,683,691,701,709,719,727,733,739,743,751,757,761,769,773,787,797,
    809,811,821,823,827,829,839,853,857,859,863,877,881,883,887,907,911,919,929,937,941,947,953,967,971,977,983,
    991,997,1009,1013,1019,1021,1031,1033,1039,1049,1051,1061,1063,1069,1087,1091,1093,1097,1103,1109,1117,1123,
    1129,1151,1153,1163,1171,1181,1187,1193,1201,1213,1217,1223,1229,1231,1237,1249,1259,1277,1279,1283,1289,1291,
    1297,1301,1303,1307,1319,1321,1327,1361,1367,1373,1381,1399,1409,1423,1427,1429,1433,1439,1447,1451,1453,1459,
    1471,1481,1483,1487,1489,1493,1499,1511,1523,1531,1543,1549,1553,1559,1567,1571,1579,1583,1597,1601,1607,1609,
    1613,1619,1621,1627,1637,1657,1663,1667,1669,1693,1697,1699,1709,1721,1723,1733,1741,1747,1753,1759,1777,1783,
    1787,1789,1801,1811,1823,1831,1847,1861,1867,1871,1873,1877,1879,1889,1901,1907,1913,1931,1933,1949,1951,1973,
    1979,1987,1993,1997,1999,2003,2011,2017,2027,2029,2039,2053,2063,2069,2081,2083,2087,2089,2099,2111,2113,2129,
    2131,2137,2141,2143,2153,2161,2179,2203,2207,2213,2221,2237,2239,2243,2251,2267,2269,2273,2281,2287,2293,2297,
    2309,2311,2333,2339,2341,2347,2351,2357,2371,2377,2381,2383,2389,2393,2399,2411,2417,2423,2437,2441,2447,2459,
    2467,2473,2477,2503,2521,2531,2539,2543,2549,2551,2557,2579,2591,2593,2609,2617,2621,2633,2647,2657,2659,2663,
    2671,2677,2683,2687,2689,2693,2699,2707,2711,2713,2719,2729,2731,2741,2749,2753,2767,2777,2789,2791,2797,2801,2803]

#************************************************
#*******          crea maschere          ********
#************************************************
dimensione_maschera=131072 # Dimensione maschera scrematura 
#* Ogni sgmento di questa dimensione contiene 16 residui che coprono 60 numeri 

sottolista_base = [1, 3, 7, 9, 13, 19, 21, 27,31, 33, 37, 39, 43, 49, 51, 57]
mappa = {v: i for i, v in enumerate(sottolista_base)}
#maschera_bit= [0]*16*dimensione_maschera

#************************************************
#******* creazione ciclo sul numero primo********
#************************************************
def ricerca_ciclo(p,riferimento):
    if riferimento==0:
        start = p * p
    else:
        A = riferimento*60+10
        start = A -(A%p)

        if start % 2 == 0:
            start += p
        else:
            start += 2*p
    lista=[]
    numero=[]
    prodotto=start
    p_lista=(prodotto-10)//60-riferimento
    p_numero=(prodotto-10)%60
    
    if p_numero in sottolista_base:
        lista.append(p_lista)
        numero.append(p_numero)
    else: 
        for i in range(5):
            prodotto+=2*p
            p_lista=(prodotto-10)//60-riferimento
            p_numero=(prodotto-10)%60
            if p_numero in sottolista_base:
                lista.append(p_lista)
                numero.append(p_numero)
                break
    termina=p_numero
    #print(f"numero ({p_numero})  lista ({p_lista})")

    for i in range(1000000):
        prodotto+=p*2
        p_numero=(prodotto-10)%60
        if p_numero==termina:
            break
        if p_numero in sottolista_base:
            p_lista=(prodotto-10)//60-riferimento
            lista.append(p_lista)
            numero.append(p_numero)
    #print(lista)
    #print(numero)
    return lista, numero  


#************************************************
#*******           scrematura            ********
#************************************************

quanti_cicli=5
radice1=isqrt(dimensione_maschera*(quanti_cicli+1)*60)+1 # calcolo radice sul secondo segmento

for cicli in range(quanti_cicli):
    maschera_bit= [0]*16*dimensione_maschera
    riferimento=dimensione_maschera*cicli
    radice=isqrt(dimensione_maschera*(cicli+1)*60)+1 # calcolo radice sul segmento
    for primi in lista_primi:
        p=primi
        if p>radice:
            print(f"{p} > {radice}  radice massima {radice1}")
            break
        lista,numero=ricerca_ciclo(p,riferimento)
        stop=0
        for ii in range(10000000000):# attenzione a questo ciclo deve coprire la grandezza della maschera_bit
            for i in range(16):
                indice = lista[i]+p*ii
                if indice>dimensione_maschera-1:
                    stop=1
                    break
                residuo = mappa[numero[i]]
                maschera_bit[(indice << 4) + residuo] = 1
            if stop==1:
                break
        #print(maschera_bit)
    #************************************************
    #*******    crea nuova mappa primi       ********
    #************************************************
    if riferimento==0:
        lista_primi.clear()
        lista_primi.append(7)
        for i in range(len(maschera_bit)):
            if maschera_bit[i]==0:
                somma=((((i//16)*60+10)+riferimento*60+(sottolista_base[i%16])))
                if somma>radice1:
                    break
                else:
                    lista_primi.append(somma)

    if riferimento>0:       
        for i in range(len(maschera_bit)):
            if maschera_bit[i]==0:
                somma=((((i//16)*60+10)+riferimento*60+(sottolista_base[i%16])))
                if somma>radice1:
                    break
                else:
                    lista_primi.append(somma)


end_time = time.perf_counter()
print(f"Tempo: {(end_time - start_time) * 1000:.2f} ms")
