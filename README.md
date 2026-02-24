# Segmented Wheel Sieve – Orthogonal Prefilter Extension (GC-60)

## Overview

This repository contains a **reference implementation** of a segmented wheel sieve
based on the **GC-60 structural model** — a mod-60 scheme shifted by 10 — and
explores *orthogonal periodic extensions* (example: prefiltering `p = 7`)
without increasing the structural word size.

This is not intended as a production-grade optimized sieve.
It is a structural experiment demonstrating that wheel periodicity
can be extended without enlarging the 16-bit mask representation
or degrading cache locality.

---

## Motivation

Most wheel-based sieve implementations extend periodicity
(e.g., mod 210) by enlarging the wheel.

However, increasing the wheel size often:

- increases structural complexity
- reduces memory locality
- worsens cache behavior

This project keeps:

- a fixed **mod-60 base**
- a constant **16-bit mask representation**
- segmented memory layout

and integrates additional periodic filtering *orthogonally*
instead of structurally enlarging the wheel.

---

## Structural Model

Numbers are represented using the GC-60 coordinate system:
n = 60k + 10 + r

where:
r ∈ {1, 3, 7, 9, 13, 19, 21, 27, 31, 33, 37, 39, 43, 49, 51, 57}

The implementation demonstrates:

- segmented elimination
- orthogonal periodic filtering (example: p = 7)
- preservation of 16-bit mask per block
- no expansion to mod 210 structure

---

## Performance

**Machine:** AMD Ryzen 7 4800U  
**Mode:** Single-threaded  
**Compiler:** `g++ -O3 -march=native`  
**Timing:** Sieve phase only (counting excluded)

| Range            | Time      |
|------------------|----------|
| 1,000,000,000    | ~211 ms  |
| 10,000,000,000   | see video |

Prime density and correctness validated via structural checks
and Miller–Rabin random certification.

---

## Demonstration Video

Execution on multiple ranges (1e9, 1e10) is shown here:

**YouTube:**  
https://youtu.be/jG0l9Rhujj0

---

## Build

This project was developed and tested using:

- Windows 11
- Microsoft Visual Studio 2022
- Configuration: Release | x64
- Optimization: /O2
- Instruction Set: AVX2 enabled
- Single-threaded execution

To build:

1. Open `sieve_wheel_M60_7.sln` in Visual Studio.
2. Select **Release** configuration.
3. Select **x64** platform.
4. Ensure **Instruction Set** is set to **AVX2 (/arch:AVX2)**.
5. Build the solution.

The executable is generated locally during compilation.

## Run

The program accepts a range limit `N` as a command-line argument:
./sieve_wheel_M60_7 1000000000

Output includes:

-   sieve elimination time
    
-   candidate density check
    
-   sample prime verification
    
-   Miller–Rabin validation
-
 ## Notes

-   This repository focuses on **structural representation**, not micro-optimization.
    
-   The implementation serves as a proof-of-concept.
    
-   Extensions to additional periodic filters (11, 13, ...) are possible.
    
-   Contributions and optimization improvements are welcome.
