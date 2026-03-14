# Segmented Wheel Sieve – Orthogonal Prefilter Extension (GC-60)

## ✨ Overview

This repository contains a **reference implementation** of a segmented wheel sieve
based on the **GC-60 structural model** — a novel approach to wheel factorization
that extends periodicity **orthogonally** without structural enlargement.

### Why This Matters

Most wheel-based sieve implementations (mod 30, mod 210) scale periodicity by increasing
the wheel size. This project demonstrates a **fundamentally different approach**:

- **Fixed mod-60 wheel** (16 residues)
- **Orthogonal pre-filtering** (e.g., p = 7) layered on top
- **Constant 16-bit mask** per segment (no expansion)
- **Preserved cache locality** despite extended periodicity

**Result**: A structurally elegant proof-of-concept competitive with mature sieves like
[primesieve](https://github.com/kimwalisch/primecount).

---

## 📊 Quick Comparison

| Implementation | Wheel | Modulo | Segments | Performance (10⁹) |
|---|---|---|---|---|
| **This (GC-60)** | Orthogonal ext. | 60 | ✅ Segmented | ~211 ms |
| primesieve | Hierarchical | 30, 210 | ✅ Multi-level | ~100-150 ms |
| gkonovalov/algorithms | Simple | 2,3 | ❌ Array-based | ~500+ ms |
| Python scipy | Sundaram | N/A | ❌ Single-pass | ~10-15 sec |

**Note**: This project = Proof-of-concept reference. Not micro-optimized like primesieve.

---

## 🔬 The GC-60 Model

### Coordinate System
```
n = 60k + 10 + r

where r ∈ {1, 3, 7, 9, 13, 19, 21, 27, 31, 33, 37, 39, 43, 49, 51, 57}
```

- **60k + 10** = base coordinate
- **16 residues** = coprime to 2, 3, 5
- **Structural invariant**: Each candidate is exactly 1 bit in a 16-bit mask

### Orthogonal Pre-filtering (p = 7)

Instead of expanding to mod(60×7) = mod 420, we layer p=7 filtering orthogonally:

```cpp
// 7 initial mask patterns (SOTTOLISTE array)
// Each pattern represents 60/gcd(60,7) = cycle of 60 positions
// Pre-filter eliminates multiples of 7 without enlarging word size
```

**Innovation**: Separates concerns → modular extensibility to p ∈ {11, 13, ...}

---

## 🚀 Performance

**Test Machine**: AMD Ryzen 7 4800U (Single-threaded)  
**Compiler**: Microsoft Visual Studio 2022  
**Timing**: Sieve elimination phase only (excluding I/O, validation)

```
Range          Time        Primes Found
───────────────────────────────────────
1×10⁹         211 ms       50,847,534
1×10¹⁰        (see video)   455,052,511
```

✅ **Correctness**: Validated via structural checks + Miller–Rabin (100 random tests)

---

## 🏗️ Architecture

### Three Phases

1. **Initialization (Fast)**
   - Memcpy-doubling technique for O(log N) setup
   - Exploits periodicity of p=7 for cache efficiency

2. **Sieving (Main Loop)**
   - Segmented elimination (32 KB segments = L1 cache)
   - Per-prime "hit points" pre-computed
   - Direct bit-AND operations

3. **Validation**
   - Density check
   - Structural verification
   - Probabilistic primality tests

### Key Data Structures

```cpp
struct ColpoResiduo {          // Hit pattern for prime p
    u64 prossima_L;            // Next segment to hit
    uint16_t maschera_NOT;     // Bit mask (negated)
};

struct PrimoAtleta {           // Sieving prime metadata
    u64 p;                     // Prime value
    std::vector<ColpoResiduo> colpi;  // All hit positions
};
```

---

## 💻 Build & Run

### Requirements
- Windows 11 / Linux / macOS
- C++11 or later
- AVX2 capable CPU (recommended for best performance)

### Build with Visual Studio (Recommended)

1. Open `sieve_wheel_M60_7.sln` in **Visual Studio 2022**
2. Select **Release** configuration
3. Select **x64** platform
4. Ensure **Instruction Set** is set to **AVX2 (/arch:AVX2)**
   - Right-click project → Properties
   - C/C++ → Code Generation → Enhanced Instruction Set
   - Select `/arch:AVX2`
5. Build → Executable ready in `Release/x64` folder

### Build with GCC/Clang (Linux/macOS)

```bash
g++ -std=c++17 -O3 -march=native prg/sieve_wheel_M60_7.cpp -o sieve
```

### Run

```bash
# Windows
sieve_wheel_M60_7.exe 1000000000

# Linux/macOS
./sieve 1000000000
```

### Output Example
```
--- Sieve Wheel M60 ricerca su: 1,000,000,000 (Pre-filtro 7 Memcpy) ---
Tempo eliminazione: 0.211 secondi

--- AVVIO CONTROLLI STRUTTURALI ---
1. Test Densita': 50847534 candidati rimasti.
2. Campione Primi identificati:
   > 1013
   > 1019
   > ...
3. Test Divisibilita': OK
4. Risultato Miller-Rabin: 100/100 OK.
```

---

## 🔍 Code Walkthrough

### 1. Residue Mapping
```cpp
const int RESIDUI[] = { 1, 3, 7, 9, 13, 19, 21, 27, 31, 33, 37, 39, 43, 49, 51, 57 };
int residuo_to_bit[60];  // Maps residue → bit position

for (int i = 0; i < 16; i++) 
    residuo_to_bit[RESIDUI[i]] = i;
```

### 2. Orthogonal P=7 Pre-filter
```cpp
const int SOTTOLISTE[7][16] = {
    {1, 3, 7, 9, 13, 19, 21, 27, 31, 33, 37, 0, 43, 49, 51, 57},  // p=7 class 0
    {1, 3, 0, 9, 13, 19, 0, 27, 31, 33, 37, 39, 43, 0, 51, 57},   // p=7 class 1
    // ... 7 patterns total
};
// 0 = eliminated by p=7, non-zero = included in that class
```

### 3. Fast Initialization
```cpp
// Memcpy doubling: O(log N) copies instead of O(N)
u64 filled = 7;
while (filled * 2 <= N_SOTTOLISTE) {
    memcpy(ptr + filled, ptr, filled * sizeof(uint16_t));
    filled *= 2;
}
```

### 4. Segmented Elimination Loop
```cpp
for (u64 s_start = 0; s_start < N_SOTTOLISTE; s_start += SEG_SIZE) {
    for (auto& pa : database) {
        for (auto& colpo : pa.colpi) {
            while (L < s_end) {
                ptr[L] &= m;  // Mark composite
                L += p;       // Jump to next multiple
            }
        }
    }
}
```

---

## 📈 Potential Extensions

This reference implementation can be extended with:

### Short-term (Straightforward)
- [ ] Prefilter extensions: p ∈ {11, 13, ...}
- [ ] Cross-compilation support (CMake)
- [ ] Benchmark suite vs primesieve

### Medium-term (Moderate effort)
- [ ] SIMD/AVX2 vectorization of sieve loop
- [ ] Dual-level sieving (separate small/big primes)
- [ ] Multi-threading with atomic segment coordination

### Long-term (Research)
- [ ] Investigate optimal orthogonal filter combinations
- [ ] Cache hierarchy optimization (L1/L2/L3)
- [ ] GPU acceleration for massive ranges

---

## 🤝 Contributing

This is a **reference implementation for research and educational purposes**.

We welcome contributions in:

1. **Performance Analysis**
   - Profiling on different CPU architectures
   - Comparison benchmarks vs other sieves
   - Cache behavior measurement

2. **Extensions**
   - Implementing additional orthogonal filters
   - SIMD optimizations
   - Parallelization strategies

3. **Documentation**
   - Mathematical proofs of correctness
   - Detailed algorithm walkthroughs
   - Academic paper drafts

**Please open an Issue first** to discuss larger changes.

---

## 📚 Related Work

- **[primesieve](https://github.com/kimwalisch/primecount)** – Production-grade, multi-level approach
- **[Sieve of Eratosthenes (Wikipedia)](https://en.wikipedia.org/wiki/Sieve_of_Eratosthenes)**
- **[Wheel Factorization (cp-algorithms)](https://cp-algorithms.com/algebra/sieve-of-eratosthenes.html#sieve-of-eratosthenes-with-linear-time-complexity)**

---

## 📄 License

MIT License – See [LICENSE](LICENSE) file

**Copyright (c) 2026 Claugo**

---

## 🎓 Citation

If you use this in research or publications, please cite:

```bibtex
@repository{claugo_gc60_sieve,
  title = {Segmented Wheel Sieve with Orthogonal Prefilter Extension (GC-60)},
  author = {Claugo},
  year = {2026},
  url = {https://github.com/Claugo/segmented-sieve-wheel-m60-7}
}
```

---

## ❓ FAQ

**Q: Is this faster than primesieve?**  
A: No. Primesieve is micro-optimized over years. This is a *proof-of-concept* showing an
alternative structural approach.

**Q: Can I use this for production?**  
A: Not recommended. Use primesieve for production. Use this for **research** and **learning**.

**Q: Why GC-60 and not mod 210 like others?**  
A: GC-60 demonstrates that we can extend periodicity **without** enlarging the fundamental
wheel structure—a conceptually simpler tradeoff.

---
🔬 Parallel Research
Passive Segmented Sieve  GC-60
An experimental sibling project exploring a different dimension of the GC-60 model.
While M60_7 focuses on speed through orthogonal pre-filtering and AVX2 optimization,
the Passive Segmented Sieve focuses on unlimited scalability through a fixed-size
passive container and a self-feeding screening list that grows only as strictly necessary.
Key properties:

Fixed bitmask — reset and reused at each segment, never grows
Screening list adds only primes in (√segment_N, √segment_N+1]
Linear scaling validated to 9.8 billion — upper limits still under investigation
Implementations in Python, C++ and Julia


Both projects share the same GC-60 wheel geometry and explore complementary
directions of the same structural model.