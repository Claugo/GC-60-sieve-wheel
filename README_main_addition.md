## Extended Range: Window Exploration

The passive container model is not limited to counting primes from 2 to N.

Because each GC-60 segment is structurally independent — it does not require
the propagated state of any previous segment, the sieve can be aimed at an
**arbitrary window [A, A + L]** positioned anywhere on the number line,
including far beyond the 64-bit limit of ~1.8 × 10¹⁹.

The `window_exploration` folder documents this extension:

- Targets verified from **10²¹ to 10²⁷** with a fixed window of 10,000,000
- Phase 1 scales linearly with the number of GC-60 cycles (~×10 per order of magnitude)
- Phase 2 (passive window sieve) runs in ~0.3 seconds regardless of target magnitude
- All results verified against the `nextprime` sequence — no errors detected
- No upper bound has been encountered; `__int128` supports targets to ~3.4 × 10³⁸

> primesieve and similar tools are limited to values below ~1.8 × 10¹⁹.
> All runs in the window exploration table exceed that limit.

→ **[See window_exploration/README.md](window_exploration/README.md)**
