using .Threads  # Attiva il supporto multi-core
using Dates
using Base: summarysize
# Funzione per formattare i numeri con il punto delle migliaia
function format_num(n)
    return replace(string(n), r"(?<=\d)(?=(\d{3})+(?!\d))" => ".")
end

println("*******   Programma a liste traslate    ********\n")
println(" Lista impostata a 131.072:")
println()
println(" 10   miliardi  -  1.272 cicli ")
println(" 100  miliardi  -  12.723 cicli")
println(" 1000 miliardi  -  127.235 cicli")
println("================================================")


# Funzione di ricerca ciclo basata sul codice originale
function ricerca_ciclo(p, riferimento, sottolista_base)
    if riferimento == 0
        start = p * p
    else
        A = riferimento * 60 + 10
        start = A - (A % p)
        # Allineamento al modulo
        start += (start % 2 == 0) ? p : 2 * p
    end

    lista = Int64[]
    numero = Int64[]
    prodotto = start
    # fld (floor division) emula // di Python
    p_lista = fld(prodotto - 10, 60) - riferimento
    p_numero = (prodotto - 10) % 60
    
    # Ricerca iniziale del residuo nella sottolista base
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

    # Espansione del ciclo divisore-centrico
    termina = p_numero
    for i in 1:1000000
        prodotto += p * 2
        p_numero = (prodotto - 10) % 60
        if p_numero == termina break end
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
    dimensione_maschera = 131072
    quanti_cicli = 1272

    sottolista_base = [1, 3, 7, 9, 13, 19, 21, 27, 31, 33, 37, 39, 43, 49, 51, 57]
    mappa = Dict(v => i-1 for (i, v) in enumerate(sottolista_base))

    println("Lancio del calcolo su $(nthreads()) thread...")
    start_time = time_ns()

    # Primi bootstrap forniti
    lista_primi = Int64[7,11,13,17,19,23,29,31,37,41,43,47,53,59,61,67,71,73,79,83,89,97,101,103,107,109,113,127,131,137,139,149,151,157,163,167,173,179,181,191,193,197,199,211,223,227,229,233,239,241,251,257,263,269,271,277,281,283,293,307,311,313,317,331,337,347,349,353,359,367,373,379,383,389,397,401,409,419,421,431,433,439,443,449,457,461,463,467,479,487,491,499,503,509,521,523,541,547,557,563,569,571,577,587,593,599,601,607,613,617,619,631,641,643,647,653,659,661,673,677,683,691,701,709,719,727,733,739,743,751,757,761,769,773,787,797,809,811,821,823,827,829,839,853,857,859,863,877,881,883,887,907,911,919,929,937,941,947,953,967,971,977,983,991,997,1009,1013,1019,1021,1031,1033,1039,1049,1051,1061,1063,1069,1087]

    radice_massima_totale = isqrt(dimensione_maschera * quanti_cicli * 60) + 1
    
    # Variabili Thread-Safe per conteggi globali
    conteggio_totale = Atomic{Int64}(3) # Parte da 3 per i primi 2, 3, 5
    max_primo = Atomic{Int64}(5)

    # FASE 1: Segmento 0 (Seriale) - Costruzione archivio divisori
    maschera_0 = falses(16 * dimensione_maschera)
    radice_0 = isqrt(dimensione_maschera * 60) + 1
    
    for p in lista_primi
        if p > radice_0 break end
        l, n = ricerca_ciclo(p, 0, sottolista_base)
        for ii in 0:1000000, i in eachindex(l)
            idx = l[i] + p*ii
            if idx < 0 continue end
            if idx > dimensione_maschera-1 break end
            maschera_0[(idx << 4) + mappa[n[i]] + 1] = true
        end
    end
    
    # Conteggio veloce di tutti i sopravvissuti nel Segmento 0
    tutti_i_vuoti_0 = count(!, maschera_0)
    atomic_add!(conteggio_totale, tutti_i_vuoti_0)
    
    # Trova il massimo nel Segmento 0
    ultimo_idx_0 = findlast(!, maschera_0)
    if ultimo_idx_0 !== nothing
        max_0 = (div(ultimo_idx_0-1, 16) * 60 + 10) + sottolista_base[(ultimo_idx_0-1)%16 + 1]
        atomic_max!(max_primo, max_0)
    end

    # Estrazione mirata: ci fermiamo appena raggiungiamo la radice utile
    empty!(lista_primi)
    push!(lista_primi, 7)
    
    for i in eachindex(maschera_0)
        if !maschera_0[i]
            somma = (div(i-1, 16) * 60 + 10) + sottolista_base[(i-1)%16 + 1]
            if somma > radice_massima_totale 
                break # Interrompe l'estrazione, la base informativa è completa
            end
            if somma > 7
                push!(lista_primi, somma)
            end
        end
    end

    # FASE 2: Restanti Segmenti (Parallelo su Core multipli)
    @threads for cicli in 1:(quanti_cicli-1)
        riferimento = dimensione_maschera * cicli
        maschera_bit = falses(16 * dimensione_maschera)
        radice_corrente = isqrt(dimensione_maschera * (cicli + 1) * 60) + 1
        
        for p in lista_primi
            if p > radice_corrente break end
            l, n = ricerca_ciclo(p, riferimento, sottolista_base)
            stop = false
            for ii in 0:1000000
                for i in eachindex(l)
                    idx = l[i] + p * ii
                    if idx < 0 continue end
                    if idx > dimensione_maschera - 1 
                        stop = true; break 
                    end
                    maschera_bit[(idx << 4) + mappa[n[i]] + 1] = true
                end
                if stop break end
            end
        end

        # Conteggio puramente passivo dei risultati
        primi_nel_segmento = count(!, maschera_bit)
        
        if primi_nel_segmento > 0
            atomic_add!(conteggio_totale, primi_nel_segmento)
            
            ultimo_idx = findlast(!, maschera_bit)
            somma_max = (div(ultimo_idx-1, 16) * 60 + 10) + (riferimento * 60) + sottolista_base[(ultimo_idx-1)%16 + 1]
            atomic_max!(max_primo, Int64(somma_max))
        end
    end

    durata_s = (time_ns() - start_time) / 1e9

    # Output finale formattato
    println("-"^45)
    println("      RISULTATI TURBO GC-60 (Multi-Thread)")
    println("-"^45)
    println("Thread attivi:             ", nthreads())
    println("Radice massima utilizzata: ", format_num(radice_massima_totale))
    println("Numero di segmenti:        ", format_num(quanti_cicli))
    println("Totale primi alla radice:  ", format_num(length(lista_primi)))
    println("Totale primi globale:      ", format_num(conteggio_totale[]))
    println("Numero massimo raggiunto:  ", format_num(max_primo[]))
    println("Tempo di elaborazione:     ", round(durata_s, digits=3), " s")
    
    # Calcolo stima memoria e salvataggio
    ultimo_primo = isempty(lista_primi) ? 0 : lista_primi[end]
    cifre_medie = floor(Int, log10(max(1.0, Float64(ultimo_primo)))) + 1
    bytes_stimati = length(lista_primi) * (cifre_medie + 1)
    kb = bytes_stimati / 1024.0
    mb = kb / 1024.0

    println("\nSalvataggio lista divisori su disco:")
    print("Dimensione stimata: ", format_num(bytes_stimati), " bytes")
    if mb >= 1.0
        println(" (", round(mb, digits=2), " MB)")
    else
        println(" (", round(kb, digits=2), " KB)")
    end

    print("\nVuoi salvare i divisori alla radice per verifica? (s/n): ")
    risposta = readline()
    if lowercase(strip(risposta)) == "s"
        open("lista_primi_verifica.txt", "w") do f
            for p in lista_primi
                println(f, p)
            end
        end
        println("Salvato in lista_primi_verifica.txt")
    end
    println("-"^45)
end

main()