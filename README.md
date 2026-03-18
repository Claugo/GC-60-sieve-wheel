# Passive Container Segmented Sieve  GC-60
### Version V3.0.0

---
## ⚠️ Note on segment size and bootstrap list

The hardcoded prime list (`lista_primi`) serves as the bootstrap for marking
composites in **segment 0**. This list must contain all primes up to the
square root of the segment 0 upper bound:
```
radice_0 = sqrt(dimensione_maschera × 60) + 1
```

If the bootstrap list is too short, composites whose smallest prime factor
lies above the list's last entry will not be marked in segment 0 — causing
a **silent overcounting of primes**.

If you change `dimensione_maschera`, update the bootstrap list accordingly:

| `dimensione_maschera` | `radice_0` | Bootstrap must reach |
|---|---|---|
| 65.536 | 1.983 | 1.979 (296 primes) |
| **131.072** | **2.805** | **2.803 (406 primes — current)** |
| 262.144 | 3.966 | 3.947 (545 primes) |
| 524.288 | 5.609 | 5.591 (735 primes) |
| 1.048.576 | 7.932 | 7.927 (998 primes) |

The bootstrap list can be generated with any prime sieve or verified with
`sympy.isprime`. The formula is deterministic no guesswork required.

## GC-60 Sieve Wheel: Conceptual and Architectural Diversity

The **Passive Container Segmented Sieve GC-60** project was conceived as a perspective differentiated from traditional sieves, with the intent of leveraging **resource efficiency**, **data independence**, and **structural stability**.

The four fundamental differences are described below, formalized mathematically where relevant.

---

## 1. Structural Translation: The Anchor at 10

The primary difference does not lie in the modulus $M=60$, but in its **reference system**.

### Traditional Approach (Standard Wheel)

The wheel is centered at the origin $0$. Candidates are generated as:

```math
n = 60k + r, \quad k \ge 0, \quad r \in R
```

Where $R$ is the set of residues coprime with $60$:

```math
R = \{1, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 49, 53, 59\}
```

The periodicity is mathematically sound but abstract with respect to the decimal system.

### GC-60 Approach (Translated Wheel)

The structure is anchored at $10$. Candidates are generated as:

```math
n = 60k + 10 + r, \quad k \ge 0, \quad r \in R'
```

Where $R'$ is the set of translated offsets:

```math
R' = \{1, 3, 7, 9, 13, 19, 21, 27, 31, 33, 37, 39, 43, 49, 51, 57\}
```

**Significance:** The translation $\rho(n) = (n - 10) \pmod{60}$ does not change the set of primes, but stabilizes the correspondence between memory index and residue relative to a decimal base. It transforms the wheel from a computational tool into a **fixed coordinate grid**.

---

## 2. Elimination Dynamics: Propagation vs Interrogation

The way composite numbers are identified defines the philosophy of the algorithm.

### Traditional Approach (Active Propagation)

The algorithm actively propagates divisibility information. For each prime $p$, it marks multiples along a linear arithmetic progression:

```math
m = p^2 + 2kp, \quad k \ge 0
```

- **Logic:** "For each prime $p$, find its multiples and cross them off."
- **State:** Segments are coupled; the position of the next multiple is propagated from segment $i$ to segment $i+1$.

### GC-60 Approach (Passive Interrogation)

The algorithm treats the segment as a **passive structure** that receives exclusion patterns. Elimination occurs through internal composition within the structured domain $A$:

```math
n = a \cdot b, \quad a, b \in A, \quad a \le b
```

- **Logic:** "I have a fixed structure (the container). I apply logical masks to determine which positions are hit by the divisors."
- **State:** **Radical Independence.** Each segment $S_i$ is computed as:

```math
S_i = \text{Sieve}(Range_i,\ \sqrt{Max})
```

Without depending on the final state of $S_{i-1}$. The cycle function `ricerca_ciclo(p, riferimento)` is recomputed for each segment, trading CPU time for logical isolation.

---

## 3. Memory Management: Segmentation State vs Divisor List

### Traditional Approach

To save CPU cycles, classical segmented sieves globally store the **propagation state** for each sieving prime up to $\sqrt{N}$ (e.g. position of the next multiple to mark).

- **Memory cost:** $O(\pi(\sqrt{N}))$ for the prime list **+** $O(\pi(\sqrt{N}))$ for the global state (offsets/maps).
- **Limitation:** For $\sqrt{N} \approx 3\ \text{billion}$, the RAM required to store maps or complex states for each prime can become prohibitive (tens or hundreds of GB).

### GC-60 Approach

The cycle map (`ricerca_ciclo`) is recomputed for each segment, eliminating the need to store any global propagation state.

- **Memory cost:** $O(\pi(\sqrt{N}))$ for the prime list **+** $O(1)$ for the segmentation state (local buffer).
- **Advantage:** The operational memory per segment is **constant** and independent of $N$. With no global maps to maintain in RAM, the bottleneck shifts from memory capacity to computing power.
- **In practice:**

```math
\text{RAM}_{\text{operational}} \approx \text{SegmentSize} + \text{ThreadOverhead}
```

Independent of the complexity of the divisor maps.

### Real Comparison

Both approaches must store the sieving prime list ($O(\pi(\sqrt{N}))$), but GC-60 eliminates the additional overhead tied to **preserving propagation state** (global maps/offsets), allowing operation at theoretical scales ($N \to 10^{19}$) with a minimal active memory footprint — accepting a deliberate trade-off: **CPU time in exchange for RAM space**.

---

## 4. Structural Density and Bitmasking

### Structural Compression

The system compresses 60 natural numbers into a 16-bit word (`uint16_t`). The information density $\rho$ is:

```math
\rho = \frac{16\ \text{bit}}{60\ \text{numbers}} \approx 0.266\ \text{bit per number}
```

### Comparison with Standard Methods

| Method | Bits per number |
|:---|:---|
| Classical bit-sieve | 1 bit |
| **GC-60** | **~0.266 bit** |
| **Density gain** | **~3.75×** |

The elimination operation is an atomic logical instruction on the CPU:

```math
\text{ptr}[L] \leftarrow \text{ptr}[L]\ \&\ \sim\text{mask}
```

This allows processing 16 candidates simultaneously at processor clock speed.

---

## Comparative Summary

| Feature | Industrial Sieves (e.g. primesieve) | GC-60 Sieve Wheel |
|:---|:---|:---|
| **Primary Goal** | Speed (Throughput) | Memory Efficiency and Logic |
| **Generator Equation** | $n = 60k + r$ | $n = 60k + 10 + r$ |
| **Segment Management** | Propagated state (Coupled) | Recomputed state (Independent) |
| **RAM Complexity** | $O(\pi(\sqrt{N}))$ (Growing) | $O(1)$ (Constant) |
| **Philosophy** | Active hunt for multiples | Interrogation of passive containers |
| **Output** | Immediate result | Persistent and regenerable archive |

---

## Background and History

This project started as a computational experiment on prime number distribution, with an initial Python prototype to validate the core idea. The evolution followed three stages:

- **Python** — conceptual prototype, logic validation
- **sieve_wheel_M60_7** — first performant C++ implementation, used as reference benchmark
- **V3.0.0** — optimized rewrite in C++ and Julia, with AI-assisted low-level optimization (Claude and Gemini)

The use of AI tools is stated transparently: the algorithmic intuitions and architectural decisions remain the author's, while AI accelerated the implementation of critical low-level optimizations.

---

## The Core Concept: Passive Container

The defining characteristic of this sieve is what it does **not** do.

In a classical sieve, the algorithm actively interrogates each candidate: *"is this number a multiple of p?"*

In this implementation, the segment is a **passive container** — it receives exclusion patterns without ever being interrogated. For each sieving prime `p`, the algorithm pre-computes a fixed pattern of 16 residues (the mod-60 multiples of `p`) and **blindly translates** that pattern onto the segment. No individual candidate is ever checked. The container simply accumulates marks from all primes, and what remains unmarked at the end is prime.

**Concrete example with p = 7:**

All numbers follow the form $n = 60k + 10 + r$ where $r \in R'$.
For $p = 7$, the multiples land on a fixed subset of these 16 residue positions. This pattern repeats identically on every segment with period $60 / \gcd(60, 7) = 60$. The algorithm computes this pattern once and translates it across the entire segment in a single pass, with no conditional logic per candidate.

---

## Benchmark Results

### C++ — 16 threads, Ryzen 7 4800MHz

| Range | Cycles | Primes to √n | Total primes | Time |
|:---|:---|:---|:---|:---|
| 10,003,415,040 | 1,272 | 9,590 | 455,200,781 | 1.12 s |
| 100,057,743,360 | 12,723 | 27,297 | 4,120,334,844 | 12.51 s |
| 1,000,616,755,200 | 127,235 | 78,520 | 37,630,234,099 | 177.75 s |

### Julia — 8 threads, same hardware

| Range | Cycles | Primes to √n | Total primes | Time |
|:---|:---|:---|:---|:---|
| 10,003,415,041 | 1,272 | 9,590 | 455,200,781 | 7.63 s |
| 100,057,743,349 | 12,723 | 27,297 | 4,120,334,844 | 115.28 s |
| 1,000 billion | 127,235 | — | — | aborted |

> The 1 trillion test in Julia was aborted — execution time was out of scale compared to C++. Julia's scaling degrades significantly at very large ranges with 8 threads, likely due to memory management and JIT overhead on data structures of this size.

### Scalability (C++)

```
10  billion  →    1.12 s
100 billion  →   12.51 s   (11.2× — near linear)
1   trillion →  177.75 s   (14.2× — expected degradation)
```

The degradation at 1 trillion is explained by the growth of the sieving prime list:

```math
\pi(\sqrt{10^{10}}) \approx 9{,}590 \quad \rightarrow \quad \pi(\sqrt{10^{12}}) \approx 78{,}520
```

Each segment performs approximately $8\times$ more sieving operations.

---

## Architectural Comparison: V3.0.0 vs sieve_wheel_M60_7 — 1 trillion

| Implementation | Time | Memory (sieving primes) |
|:---|:---|:---|
| sieve_wheel_M60_7 | 456.5 s | ~37 billion elements in RAM |
| **V3.0.0 C++** | **177.8 s** | **~78,000 elements in RAM** |
| **Speedup** | **2.6×** | **~500,000×** |

**Structural difference:**
- **M60_7** — parallelizes over primes within each segment (shared mask contention)
- **V3.0.0** — assigns full independent segments to each thread (private mask, zero contention)

---

## Build Instructions

### C++

Recommended: Visual Studio, Console project, Release x64.

Enable the following compiler options:
- OpenMP support (`/openmp`)
- AVX2 instruction set (`/arch:AVX2`)
- C++20 standard (required for `std::popcount` and `<bit>`)


### Julia

```bash
julia --threads=8 sieve_wheel_M60.jl
```

---

## Collaboration

This is an experimental project with a solid and verified base. Contributions are welcome, particularly around:

- Further parallelization strategies
- Memory layout optimizations
- Validation against independent prime counting references
- Port to other languages (Rust, Go)

Open an issue on GitHub if you are interested in collaborating.

---

## Conclusion: The Value of the Passive Container

The **GC-60 Sieve Wheel** project does not position itself as a replacement for optimized industrial libraries such as `primesieve`, nor does it aim to compete on raw execution speed. Classical algorithms benefit from decades of low-level optimizations (assembly, prefetching, vectorization) carried out by expert teams on specific hardware.

The goal of this work is different: to demonstrate that the **Passive Container** concept represents a valid starting point for **rethinking the interpretation of the traditional wheel**.

### 1. A Conceptual Alternative, not a Performance Claim

While classical sieves treat the data structure as an active buffer to fill and propagate, the **GC-60 Sieve Wheel** proposes a static view:
- The structure (the container) exists independently of the computation.
- Divisibility information is **projected** onto the structure via logical masks.
- Primality emerges as a **residual state** — what is not marked — rather than as the active result of a search.

This divergence does not make the algorithm "better" in terms of throughput, but makes it **conceptually alternative**. It offers a search logic where segment independence and constant memory occupancy take priority over minimizing clock time.

### 2. Sustainability and Scalability

The practical value of this approach lies in its sustainability under different resource constraints:

```math
\text{RAM}_{\text{GC-60}} \approx O(1) \quad \text{vs} \quad \text{RAM}_{\text{traditional}} \approx O(\pi(\sqrt{N}))
```

This characteristic allows operation at theoretical scales ($N \to 10^{19}$) with a minimal active memory footprint, making the **GC-60 Sieve Wheel** a genuine alternative for scenarios where memory is the primary bottleneck rather than computation speed.

### 3. Future Perspective

The **GC-60 Sieve Wheel** does not seek to break speed records, but to **expand the solution space**:
- For **theoretical research**, it offers a model where structural periodicity (translation +10) can be studied as a fixed environment.
- For **software engineering**, it demonstrates that persistent, independent archives can be built without relying on global computation state.

The project is therefore proposed as a **complementary tool** in the ecosystem of computational number theory: not to replace existing tools, but to accompany them by offering a passive container logic that makes the structure of prime numbers readable through a different lens — one where efficiency is measured in architectural stability and resource savings, as well as in speed.
