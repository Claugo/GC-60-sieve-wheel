using Dates

# Funzione per formattare i numeri con i punti delle migliaia
function format_num(n)
    return replace(string(n), r"(?<=\d)(?=(\d{3})+(?!\d))" => ".")
end

# Funzione di ricerca ciclo - Modello GC-60
function ricerca_ciclo(p, riferimento, sottolista_base)
    if riferimento == 0
        start = p * p
    else
        A = riferimento * 60 + 10
        start = A - (A % p)
        if start % 2 == 0
            start += p
        else
            start += 2 * p
        end
    end

    lista = Int64[]
    numero = Int64[]
    prodotto = start
    p_lista = fld(prodotto - 10, 60) - riferimento
    p_numero = (prodotto - 10) % 60
    
    if p_numero in sottolista_base
        push!(lista, p_lista)
        push!(numero, p_numero)
    else 
        for i in 1:5
            prodotto += 2 * p
            p_lista = fld(prodotto - 10, 60) - riferimento
            p_numero = (prodotto - 10) % 60
            if p_numero in sottolista_base
                push!(lista, p_lista)
                push!(numero, p_numero)
                break
            end
        end
    end

    termina = p_numero
    for i in 1:1000000
        prodotto += p * 2
        p_numero = (prodotto - 10) % 60
        if p_numero == termina
            break
        end
        if p_numero in sottolista_base
            p_lista = fld(prodotto - 10, 60) - riferimento
            push!(lista, p_lista)
            push!(numero, p_numero)
        end
    end
    return lista, numero  
end

function main()
    #****************************
    # Parametri di configurazione
    #****************************
    dimensione_maschera = 16385
    quanti_cicli = 10000 
    sottolista_base = [1, 3, 7, 9, 13, 19, 21, 27, 31, 33, 37, 39, 43, 49, 51, 57]
    mappa = Dict(v => i-1 for (i, v) in enumerate(sottolista_base))

    start_time = time_ns()

    # Primi di bootstrap
    lista_primi = Int64[7,11,13,17,19,23,29,31,37,41,43,47,53,59,61,67,71,73,79,83,89,97,
    101,103,107,109,113,127,131,137,139,149,151,157,163,167,173,179,181,
    191,193,197,199,211,223,227,229,233,239,241,251,257,263,269,271,277,
    281,283,293,307,311,313,317,331,337,347,349,353,359,367,373,379,383,
    389,397,401,409,419,421,431,433,439,443,449,457,461,463,467,479,487,
    491,499,503,509,521,523,541,547,557,563,569,571,577,587,593,599,601,
    607,613,617,619,631,641,643,647,653,659,661,673,677,683,691,701,709,
    719,727,733,739,743,751,757,761,769,773,787,797,809,811,821,823,827,
    829,839,853,857,859,863,877,881,883,887,907,911,919,929,937,941,947,
    953,967,971,977,983,991,997,1009,1013,1019,1021,1031,1033,1039,1049,1051,1061,1063,1069,1087]

    radice_massima_totale = isqrt(dimensione_maschera * quanti_cicli * 60) + 1
    
    conteggio_primi_totale = 3 
    ultimo_primo_trovato = 5

    for cicli in 0:(quanti_cicli-1)
        maschera_bit = falses(16 * dimensione_maschera)
        riferimento = dimensione_maschera * cicli
        radice_corrente = isqrt(dimensione_maschera * (cicli + 1) * 60) + 1
        
        for p in lista_primi
            if p > radice_corrente break end
            
            lista, numero = ricerca_ciclo(p, riferimento, sottolista_base)
            
            stop = false
            for ii in 0:10000000000
                for i in eachindex(lista)
                    indice = lista[i] + p * ii
                    if indice < 0 continue end
                    if indice > dimensione_maschera - 1
                        stop = true
                        break
                    end
                    residuo = mappa[numero[i]]
                    maschera_bit[(indice << 4) + residuo + 1] = true
                end
                if stop break end
            end
        end

        if riferimento == 0
            empty!(lista_primi)
            push!(lista_primi, 7)
        end

        for i in eachindex(maschera_bit)
            if !maschera_bit[i]
                idx_0based = i - 1
                somma = (div(idx_0based, 16) * 60 + 10) + (riferimento * 60) + sottolista_base[(idx_0based % 16) + 1]
                
                if somma > 7
                    conteggio_primi_totale += 1
                    ultimo_primo_trovato = somma
                    
                    if somma <= radice_massima_totale && (riferimento == 0 || somma > (riferimento * 60))
                        if somma != 7 push!(lista_primi, somma) end
                    end
                end
            end
        end
    end

    durata_ms = (time_ns() - start_time) / 1e6

    println("-"^45)
    println("           RISULTATI MODELLO GC-60")
    println("-"^45)
    println("Radice massima utilizzata: ", format_num(radice_massima_totale))
    println("Numero di segmenti:        ", format_num(quanti_cicli))
    println("Totale primi trovati:      ", format_num(conteggio_primi_totale))
    println("Numero massimo raggiunto:   ", format_num(ultimo_primo_trovato))
    println("Tempo di elaborazione:     ", round(durata_ms, digits=2), " ms")
    println("-"^45)
end

main()