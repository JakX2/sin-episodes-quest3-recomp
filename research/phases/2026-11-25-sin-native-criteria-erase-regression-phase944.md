# Phase 944: portable indexed erase regression

Rename near/far locals in shared deletion template to avoid Win32 legacy macro collision. Add CTest covering actual shared Tree template for 32-bit 24-byte and 16-bit 16-byte rows, each 1,000 randomized insertion/deletion permutations (128,000 removals), checking RB invariants and payload identity after each removal. Explicit simulated near/far macros. This is portable structural regression, not retail engine integration. CI pending.
