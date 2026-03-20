# Window Exploration  Extended Range Prime Analysis

**GC-60 Passive Container Sieve  MicroPrime Studio** _March 2026 — Govi Claudio_

---

## Overview

The standard GC-60 sieve counts all primes up to N, storing only the sieving primes up to √N in memory. This is efficient for contiguous ranges starting from 2, but it implies a hard architectural limit: the target N must fit in a 64-bit integer (`uint64_t`), capping the reachable range at approximately 1.8 × 10¹⁹.

This folder documents an extension of the GC-60 concept that decouples **where** you sieve from **how far** you sieve. Instead of counting all primes from 2 to N, the algorithm targets an arbitrary window [A, A + L] positioned anywhere in the number line, including far beyond 10¹⁹.

The key insight is that the passive container model naturally supports this: a segment does not need to know its absolute position to receive exclusion patterns. It only needs to know which divisors reach it.

---

## Why the Passive Container Enables This

In a classical segmented sieve, reaching a window at position A requires propagating the state of every sieving prime from segment 0 all the way to the segment containing A. This is computationally and structurally expensive.

In the GC-60 model, each segment is **stateless and independent**. The cycle function `ricerca_ciclo(p, riferimento)` computes the entry point of any prime `p` into any segment directly from its reference offset, without needing the result of any previous segment. This property makes arbitrary window placement essentially free in terms of architectural complexity.

The algorithm does not "travel" to position A. It simply asks:

> _"Which primes p ≤ √(A + L) have at least one multiple inside [A, A + L]?"_

Those primes — called **useful divisors** — are the only ones applied to the window. All others are discarded without ever touching the window array.

---

## Algorithm Structure

### Phase 1 — Divisor Generation (parallel, GC-60 segments)

The GC-60 sieve runs from 0 to √(A + L), identifying all primes up to the square root of the window's upper bound. For each prime found, a filter test determines whether it is a **useful divisor**:

```
first_odd_multiple(p) = next odd multiple of p ≥ A
useful if: first_odd_multiple(p) < A + L
```

Primes that pass this test are collected into an archive. All others are discarded immediately.

Two distinct categories emerge from this phase:

|Category|Definition|Role|
|---|---|---|
|**Stored primes** (≤ √√(A+L))|Bootstrap primers, kept in RAM throughout|Drive the GC-60 sieve segments|
|**Interrogated primes** (≤ √(A+L))|Found during sieving, not all kept in RAM simultaneously|Tested once for usefulness, then released|

For a window at 10²⁵, the stored primes reach ~10⁶ (~78,000 primes), while the interrogated primes reach ~3.16 × 10¹² (~108 billion primes, never all in RAM at once).

### Phase 2 — Passive Window Sieve

A boolean array of L+1 elements is initialized to `true` (all candidates presumed prime). Each useful divisor projects its multiples onto the array:

```
offset = A mod p
for idx = offset; idx ≤ L; idx += p:
    window[idx] = false
```

This is the passive container mechanism applied to an offset window: the array receives marks without any knowledge of A's absolute value.

### Phase 3 — Survivor Collection

Positions still marked `true` after Phase 2, after a final check against small factors (2, 3, 5, 7), are the primes in [A, A + L].

---

## Implementation Notes

The implementation (`microprime_studio.cpp`) uses `__int128` (GCC/Clang) to represent A_TARGET, allowing targets up to ~3.4 × 10³⁸. All internal sieve arithmetic remains in `long long`, only the window position and the divisibility test use 128-bit arithmetic.

**Compilation (GCC / Code::Blocks with MinGW):**

```bash
g++ -O3 -std=c++20 -fopenmp -o microprime_studio microprime_studio.cpp
```

**MSVC note:** `__int128` is not supported by MSVC. Use GCC or Clang. A `uint128` emulation struct is available for MSVC builds but involves a performance overhead on the divisor filter loop.

---

## Benchmark Results

All runs use a fixed window of **10,000,000** numbers. Hardware: Ryzen 7, 16 threads (OpenMP). All results verified against `nextprime` sequence, no errors detected across all 7 runs.

|Target|Cycles GC-60|Useful divisors|Primes found|Phase 1 (s)|Phase 2 (s)|Total (s)|
|---|---|---|---|---|---|---|
|10²¹|4,023|2,595,933|206,889|5.16|0.30|5.80|
|10²²|12,717|2,827,186|197,136|17.54|0.31|18.20|
|10²³|40,212|3,050,169|188,571|61.15|0.33|61.85|
|10²⁴|127,158|3,264,030|180,298|234.6|0.35|235.4|
|10²⁵|402,106|3,467,578|173,428|983.9|0.37|984.7|
|10²⁶|1,271,567|3,662,703|167,584|4,002.5|0.38|4,003.4|
|10²⁷|4,021,045|3,853,716|161,025|17,599.3|0.49|17,600.2|

### Key observations

**Linear scaling.** Phase 1 time scales by approximately ×10 per order of magnitude in the target, exactly matching the ×10 growth in GC-60 cycles. No degradation, no unexpected bottlenecks.

**Constant Phase 2.** The passive window sieve runs in ~0.3–0.5 seconds regardless of target magnitude. The window array (~10 MB) fits entirely in L3 cache; its cost is independent of where it sits on the number line.

**Decreasing prime density.** Primes found per window decrease from 206,889 at 10²¹ to 161,025 at 10²⁷,  consistent with the prime number theorem (density ~1/ln N).

**No overflow.** All 7 runs completed without arithmetic errors. `__int128` provides headroom to ~3.4 × 10³⁸.

### Comparison with primesieve

`primesieve`, the current reference implementation for prime counting, operates on `uint64_t` and is therefore limited to values below ~1.8 × 10¹⁹. All 7 runs in this table exceed that limit by factors ranging from 100× (10²¹) to 100,000,000× (10²⁷). Direct numerical comparison is not possible above 10¹⁹; correctness was instead verified via the `nextprime` sequence.

---

## Algorithm Flow

```
INPUT: A_TARGET, L_FINESTRA
         |
         v
   Compute R = sqrt(A_TARGET + L)
   Compute RR = sqrt(R)
   Compute cycles = R / (131072 × 60)
         |
         v
┌─────────────────────────────────────┐
│  PHASE 1 — GC-60 segments (parallel)│
│                                     │
│  Stored primes (≤ RR)               │
│    → bootstrap, kept in RAM         │
│                                     │
│  Interrogated primes (RR < p ≤ R)   │
│    → tested for usefulness          │
│    → useful: saved to archive       │
│    → not useful: discarded          │
└─────────────────────────────────────┘
         |
         v
┌─────────────────────────────────────┐
│  PHASE 2 — Passive window sieve     │
│                                     │
│  Boolean array[L+1] = all true      │
│  For each useful divisor p:         │
│    mark multiples of p in window    │
│  Cost: ~0.3s, fits in L3 cache      │
└─────────────────────────────────────┘
         |
         v
┌─────────────────────────────────────┐
│  PHASE 3 — Survivor collection      │
│                                     │
│  Collect true positions             │
│  Final check: not div by 2,3,5,7    │
│  → primes in [A_TARGET, A_TARGET+L] │
└─────────────────────────────────────┘
         |
         v
OUTPUT: sopravvissuti.txt
        divisori_utili.txt
        registro_microprime.txt (append)
```

---

## Files in this folder

| File                    | Description                                    |
| ----------------------- | ---------------------------------------------- |
| `microprime_studio.cpp` | Main implementation  C++20, OpenMP, `__int128` |
| `README.md`             | This document                                  |
| `LICENSE`               |                                                |

---

## Relationship to the main GC-60 sieve

The window exploration mode is a **targeted application** of the same passive container principle, not a separate algorithm. The GC-60 segments in Phase 1 are structurally identical to those in the main sieve, the only difference is what happens to the primes they produce: instead of being counted, they are tested for relevance to the window and either archived or discarded.

The main sieve counts. This extension explores.

---

## Experimental Status and Manual Configuration

`microprime_studio.cpp` is currently in **experimental form**. Exploring different windows and magnitudes requires manual editing of two constants directly in the source code:

cpp

```cpp
// ============================================================
// PARAMETRI FINESTRA TARGET
// Modifica A_TARGET e L_FINESTRA per esplorare zone diverse
// ============================================================
const i128 A_TARGET   = (i128)10000000000000000000ULL * 100000000ULL;
const i128 L_FINESTRA = 10000000;
```

`A_TARGET` defines the starting position of the window. Because values above ~1.8 × 10¹⁹ exceed the 64-bit integer limit, the target must be constructed as a product of `__int128` literals. `L_FINESTRA` can be set freely; 10,000,000 is the value used in all benchmark runs above. After editing, recompile and run.

The use of **Code::Blocks with MinGW (GCC)** was necessary because `__int128` is a GCC/Clang extension not supported by MSVC (Visual Studio). Recommended settings: compiler flags `-O3 -fopenmp`, linker flag `-fopenmp`, standard C++20, build target **Release** (Debug mode disables `-O3` and produces runtimes ~30× slower).

The transparency of this documentation is an invitation to collaborate — to extend and improve the exploration capability of this tool. Contributions and experiments on different window positions and magnitudes are welcome.
