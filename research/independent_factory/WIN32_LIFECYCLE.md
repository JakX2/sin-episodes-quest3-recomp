# Independent Win32 x86 loader lifecycle batch

This is a **synthetic-only** native loader test. It does not load, proxy, forward
to, or reproduce original gameplay DLLs. The DLL exposes only synthetic
interface identifiers. This test exercises a Windows PE32 process loading
and unloading a standalone C++17 module, with an undecorated `CreateInterface`
export and `__cdecl` caller contract.

The native test runs 32 load/lookup/negotiation/unload cycles, validates that
the export address belongs to the loaded module, checks stable interface
object identity, rejects unsupported names, verifies an extra LoadLibrary
reference keeps the module alive, and checks that a nonexistent DLL cannot
be loaded. These tests cover loader mechanics, **not** retail engine ABI or
CRT/gameplay initialization.

## Evidence tiers (unchanged)

- SHA-reviewed original function bodies: **403** (256 client, 147 server),
  from the existing research catalog, not authenticated again here.
- Automated potential original entrypoints: **64,586** (candidates, not
  independently verified function bodies).
- Independently load-tested *original* functions: **0**.
- Original retail gameplay boot: **not demonstrated**.
- Original bytes independently authenticated in this batch: **0**.

Next: use SHA-pinned original PE export metadata and Ghidra's verified
factory/interface/vtable evidence to identify the retail engine handshake,
then implement an independently owned interface implementation and test
the actual engine's requests. Keep original binaries private.
