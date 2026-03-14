# Passive Container Segmented Sieve -- GC-60

## ✨ Overview

This folder contains an **experimental implementation** of a segmented wheel sieve based on the **GC-60 structural model**, built around a fundamentally different segmentation philosophy compared to classical approaches.

> **Status**: Active experimentation — scalability limits are still being explored.

### The Core Idea: Passive Container

In classical segmented sieves, the sieving structure adapts as the range grows — the screening list, the segment metadata, and often the memory footprint all increase with √n.

This implementation introduces a different concept: a **passive container**.

- The bitmask is **fixed in size** — always `dimensione_maschera × 16` slots
- The container does **not grow**, does not transform — it is simply **reset and reused**
- The screening list (`lista_primi`) grows only as strictly necessary, adding primes in the interval `(√segment_N, √segment_N+1]` — nothing more
- The world **flows through the container**, not the other way around

This separation between the passive structure and the active flow of segments is the architectural distinction from classical segmented sieves, including approaches like Sorenson's interval sieve — which achieves memory control through complex data structures. Here the same property emerges naturally from the fixed GC-60 wheel geometry.

---

## 🔬 Mathematical Foundation

### The GC-60 Wheel

All primes > 5 are coprime with 2, 3, and 5. Modulo 60, there are exactly **16 residues** with this property:

```
r ∈ {1, 3, 7, 9, 13, 19, 21, 27, 31, 33, 37, 39, 43, 49, 51, 57}
```

Every candidate prime is represented as:

```
n = 60k + 10 + r
```

This reduces the candidate space to **16/60 ≈ 26.7%** of all integers — by construction, before any sieving begins.

### Memory Complexity

The screening list size grows as **O(√n / ln √n)** by the prime number theorem, while the range covered grows linearly. The passive container itself is O(1) — constant regardless of how far the sieve extends.

|Cycles|Range covered|Primes in list|List size|
|---|---|---|---|
|100|~98M|1,224|~7 KB|
|1,000|~983M|3,377|~20 KB|
|10,000|~9.83B|9,518|~56 KB|

The container holding ~9.8 billion numbers needs only **56 KB** for its screening list.

---

## 📊 Performance

Tested on a standard desktop CPU, single-threaded, unoptimized builds.

|Cycles|Range|Total primes|C++ time|Julia time|
|---|---|---|---|---|
|100|~98M|5,669,763|~405 ms|349 ms|
|1,000|~983M|50,031,385|4,607 ms|4,326 ms|
|10,000|~9.83B|447,709,896|85,537 ms|74,435 ms|

**Scaling**: Linear — no degradation observed at any tested scale.

✅ **Correctness**: Validated against `nextprime` sequential verification and independently confirmed with `primesieve` (difference of 1 is structural — the passive container always completes the full 16-residue block at segment boundaries, while `primesieve` stops exactly at the given limit).

---

## 🏗️ Architecture

### The Cycle Discovery (`ricerca_ciclo`)

For each sieving prime `p` and each segment, the function computes the **periodic pattern** of `p`'s multiples across the 16 residues of that segment. This pattern repeats with period `p`, so the sieve loop simply strides through the container adding `p` to each offset on each pass.

### Self-feeding Screening List

```
Segment 0:  bootstrap list → sieve → extract primes [7 .. √(segment_1)]
            → replace bootstrap entirely

Segment N:  screening list → sieve → extract primes (√(segment_N), √(segment_N+1)]
            → append only the strictly necessary primes
```

The bootstrap (hardcoded primes up to 1087) is used **only once** and then discarded.

### Segment Flow

```
for each segment:
    1. reset bitmask to zero          ← passive container cleared
    2. compute radice (√ of segment)
    3. sieve with lista_primi
    4. extract new primes → update lista_primi
```

---

## 💻 Implementations

Three reference implementations are provided, all producing identical results:

| File                     | Language | Notes                                    |
| ------------------------ | -------- | ---------------------------------------- |
| `ruota_sperimentale.py`  | Python   | Prototype — used for logic validation    |
| `ruota_sperimentale.cpp` | C++      | Reference implementation                 |
| `ruota_sperimentale.jl`  | Julia    | Alternative — counts all primes in range |

### Bulid C++

```bash

Recommended: **Visual Studio 2022** with AVX2 enabled for best performance.

1. Open the `.cpp` file in Visual Studio 2022
2. Select **Release** configuration, **x64** platform
3. Right-click project → Properties → C/C++ → Code Generation
4. Set **Enhanced Instruction Set** to `/arch:AVX2`
5. Build and run
```
### Run Python

```bash
python ruota_sperimentale.py
```

### Run Julia

```bash
julia ruota_sperimentale.jl
```

---

## 🔭 Current Experimental Status

This implementation is a **research prototype**. The following are open questions currently under investigation:

- **Upper scalability limit** — linear scaling confirmed to ~9.8B; limits not yet found
- **Optimal segment size** — current `dimensione_maschera = 16385` fits comfortably in L2 cache; L1/L2 boundary effects not yet characterized
- **Comparison with Sorenson** — structural similarity in memory control, but achieved through geometric simplicity rather than algorithmic complexity
- **Integration with MicroPrime** — planned use as screening foundation for gap-based prime search

---

## 🔗 Relation to M60_7

This project is **complementary** to the main [GC-60 Segmented Sieve (M60_7)](https://github.com/Claugo/segmented-sieve-wheel-m60-7):

||M60_7|Passive Container|
|---|---|---|
|**Goal**|Speed|Scalability|
|**Optimization**|AVX2, prefilter p=7|None (prototype)|
|**Memory model**|Cache-optimized segments|Passive fixed container|
|**Screening list**|Fixed bootstrap|Self-feeding, minimal growth|
|**Status**|Reference implementation|Active experimentation|

Both are built on the same GC-60 wheel geometry — they explore different dimensions of the same structural model.

---

## 📄 License

MIT License — see LICENSE file.

**Copyright (c) 2026 Claugo**