# Phase 258: independent PE32 dependency boundary

This phase extends the Phase 257 static import audit with delay-load import descriptors and forwarded export targets. The Windows x86 workflow now rejects a synthetic replacement DLL if these PE metadata mechanisms name `client.dll` or `server.dll`. Synthetic fixture regression tests cover RVA/VA delay names, forwarders, malformed addresses and architecture rejection.

This is **not** proof that a DLL cannot dynamically call `LoadLibrary`/`GetProcAddress`, or that its code is a faithful implementation of the original game. It does not claim retail engine ABI compatibility. The scanner does not execute or redistribute original DLL bytes. Malformed PE metadata causes a fail-closed error.

## Coverage evidence (unchanged)

- Whole-body SHA-reviewed original functions: **403** (256 client, 147 server).
- Automated original-PE candidate entrypoints: **64,586** (26,290 client, 38,296 server). Candidate != verified function.
- Independently load-tested *original* functions: **0**.
- Synthetic Win32 DLL factory loader: previously passed CI; no retail gameplay initialization demonstrated.
- New original bytes authenticated in this phase: **0**.

## Next high-value batch

Authenticate both owner original PE32 SHA256 values, execute the hosted Ghidra atlas against them, and prioritize exports, class/vtable layout, engine-to-DLL factory ABI, and gameplay initialization edges. Build a non-forwarding Windows x86 replacement against that **verified** ABI and test a real engine load before pursuing Android ARM64 and Quest 3.
