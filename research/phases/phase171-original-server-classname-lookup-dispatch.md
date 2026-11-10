# Phase 171: Original game entity classname lookup and create/destroy dispatch

11 November 2026. Continues Phase170 draft PR #222. This phase reads the real owner-held *SiN Episodes: Emergence* (2006) Windows PE32 `server.dll` and `client.dll`, and reconstructs four complete native server functions. No original proprietary executable bodies, full disassembly or game assets are committed.

## Complete original code provenance

- `server.dll`: SHA256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`
- `client.dll`: SHA256 `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`

| Original server VA | Exact code length | Entire region SHA256 | Attribution |
|---|---:|---|---|
| `0x2228B010` | 94 | `ba7b2d918b3b72235c9457731f50ebcfb3b6f001c699d981c788d7fed8c9abf5` | Registry virtual +0x0C classname-to-factory lookup |
| `0x22285550` | 46 | `58f62479791e357d5b220d60803a1a72e2cfc003944000c50f667f30730854f1` | Registry virtual +0x04 create wrapper |
| `0x22285580` | 50 | `83eb972b13abd551e76ba781d13eed5ab614cd013c0d7f8c133020f7a2b2a5db` | Registry virtual +0x08 destroy wrapper |
| `0x2228AEA0` | 18 | `28b0a8d65b4efb61b7c2be4b21a107fa30757f2208c14612ceb98396691cde47` | Raw 16-bit record-index accessor |

Total: **208 completely fingerprinted native function bytes**. The private original-only audit independently verifies **51 sparse opcode witnesses**, **2 direct native CALL rel32 sites**, **3 exact native registry vtable pointers**, and both native warning strings. It verifies both complete DLL hashes first. A synthetic sparse opcode fixture explicitly fails whole-region SHA checks.

## Actual original vtable ownership and correction

Original registry constructor `0x2228AFB0` writes `0x22456324` as registry vtable. Reading its entries from the actual `.rdata` proves:

- `vtable +0x00` -> `0x2228B190` (not yet reconstructed)
- `vtable +0x04` -> `0x22285550` (create wrapper)
- `vtable +0x08` -> `0x22285580` (destroy wrapper)
- `vtable +0x0C` -> `0x2228B010` (classname lookup)

Phase170's two global entity factory entry points `0x22266B00` and `0x22266B60` *both* invoke the registry's **virtual +0x04 create wrapper**, then invoke different *per-class* factory methods (+0x24 or +0x20 respectively). They must **not** be conflated with the separate registry destroy wrapper at `0x22285580`, which invokes per-class virtual +0x04. The original warning strings independently confirm this distinction:

- `0x22455F50`: `Attempted to create unknown entity type %s!`
- `0x22455F80`: `Attempted to destroy unknown entity type %s!`

## Original classname-to-factory lookup `0x2228B010`

1. Accept the original classname pointer and preserve the registry object pointer in `EDI`. The registry's embedded index lies at registry+0x04, and its symbol/interner member is at registry+0x20.
2. Call original `0x2228F7A0` from `0x2228B02E` with an output 16-bit stack key slot initialized to `0xFFFF` and the classname. Read the returned pointer's first 16-bit word; it is a key/index for the next lookup. Exact engine table string normalization and hashing remain unknown, not invented.
3. Call `0x2228A760` from `0x2228B044`, with `ECX=registry+0x04`, the key's address, and integer 0. The returned `AX` is treated as a **16-bit** record slot.
4. If returned slot equals `0xFFFF`, return a null pointer. Otherwise zero-extend the slot, shift left four (multiply by 16), read registry+0x08 allocation pointer and return the DWORD from `[allocation + 16*slot + 0x0C]`.
5. The original does **not** bounds-check a non-sentinel slot before dereferencing. The host-neutral C++ reference validates a slot against host vector length as a deliberate safety departure, not an original-game behavior claim.

The complete native `0x2228F7A0` interning algorithm and `0x2228A760` index logic are outside the current reconstruction. The C++ interface models them as explicit callbacks, not an invented `std::unordered_map` substitute for original mechanics.

## Create wrapper `0x22285550`

- Receives classname pointer, calls its own virtual +0x0C (`0x2228B010`) with that classname.
- On a null factory, prints original create-warning literal and returns null.
- Otherwise invokes **per-class factory virtual +0x00**, passing the original classname pointer, and returns that callback's pointer.
- Original 46-byte body terminates with `RET 4`.

## Destroy wrapper `0x22285580`

- Receives classname and a second caller argument, calls registry virtual +0x0C with classname.
- On null factory, prints original destroy-warning literal, returns without per-class invocation.
- Otherwise invokes **per-class factory virtual +0x04** with the second argument.
- Original 50-byte body terminates with `RET 8`. This is not the same API as Phase170's second *create* entry point.

## Record accessor `0x2228AEA0`

- Zero-extends the original 16-bit parameter, computes `index << 4`, reads a dynamic allocation pointer at object+0x04, and returns `[allocation + 16*index + 12]` **as an address**. Its native `RET 4` does not check bounds and does not dereference the factory pointer.
- The portable byte-offset helper models the arithmetic independently. It does not expose unsafe native pointer arithmetic to callers.

## Implementation, testing and limitations

- `include/sin_compat/original_server_factory_dispatch.hpp`: C++17 portable lookup/create/destroy state flow and exact 16-byte slot arithmetic, with explicit external interner/map and per-class virtual callbacks.
- `tests/test_original_server_factory_dispatch.cpp`: 16-bit slot arithmetic and full callback order, create/destroy success, null factory, 0xFFFF missing slot and invalid host slot.
- `research/tools/audit_sin_original_server_factory_dispatch.py`: original-only PE32 parser and full dual-DLL SHA256 / complete four-region SHA256 checks; native witnesses, vtable ownership and diagnostic strings.
- `research/tests/test_sin_original_server_factory_dispatch.py`: independent original-free evidence mutation tests and a 12-combination create/destroy outcome matrix.
- CMake/CTest target `sin_original_server_factory_dispatch_tests`.
- Reviewed original-function evidence ledger grows **93 -> 97 (47 server, 50 client)**. All four new entries are conservatively `machine_code_verified`; the 18-byte accessor's normal arithmetic is behavior-reconstructed, but the engine allocator remains unresolved.

Owner-only run:

```sh
python3 research/tools/audit_sin_original_server_factory_dispatch.py --client /private/client.dll --server /private/server.dll
python3 -m unittest discover -s research/tests -p test_sin_original_server_factory_dispatch.py -v
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --target sin_original_server_factory_dispatch_tests
ctest --test-dir build -R sin_original_server_factory_dispatch_tests --output-on-failure
```

This **does not** provide playable original Windows DLL replacements or a live Source entity factory. Native string symbol-index code, per-class vtable callbacks, edict allocator and actual `flak` object construction, collision and damage handling remain engine-owned. Windows gameplay comes first, then Android and Quest 3. Preserve the stacked draft PR chain, do not merge.