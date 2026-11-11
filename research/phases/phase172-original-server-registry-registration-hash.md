# Phase 172: Original server classname registration and signed-byte hash

11 November 2026. Continues *canonical* Phase171 draft PR #223 (not superseded duplicate #224). Inspected the exact original 2006 Windows PE32 *SiN Episodes: Emergence* `server.dll`. The full 2006 original game DLLs are private; no proprietary executable bodies, assets, or disassemblies are committed.

## Original binary provenance

Chained private audit requires the original `server.dll` SHA256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650` and original `client.dll` SHA256 `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.

| Native original entry VA | Original length | Complete original function SHA256 | New evidence |
|---|---:|---|---|
| `0x2228AEC0` | 60 | `22925b759c92d8e17fea6181e5eb1ac9c6fbb3c68cb081b731592ff10e72c8ec` | Symbol intern then existing index lookup |
| `0x2228B140` | 79 | `a66c1b2ab5b3ff865fe672c5b02aa2ab10858ed3f867589593c3f6d537b09259` | Embedded registry subobject, indirect DWORD provider registration |
| `0x2228B190` | 78 | `cc1922a54b06dc0518f57e6d20c30b0d7bf18fcfea58180fcfdcefbd2de79752` | Registry vtable +0x00 direct DWORD provider registration |
| `0x2228B1E0` | 40 | `1cbb4f8f3651d9dbfae4235ec60dce656bbf53569dca30ad8af027799571403a` | Original signed-byte DJB2-style classname hash |

All four complete original regions total **257 real native bytes** and have independently verified full SHA256 fingerprints from the owner's game binary. Owner-only code checks **58 sparse original x86 opcode witnesses**, **six exact internal CALL rel32 edges**, **one verified incoming original hash CALL** at `0x222C7A13`, and the complete four-DWORD registry vtable at `0x22456324`. A synthetic fixture cannot pass full-region SHA checks. The auditor calls the entire Phase171 auditor first and therefore also checks all chained original DLL SHA256s.

## Correction: original registry vtable +0x00 is registration, not destruction

Original registry vtable `0x22456324` has the following exact four pointers:

- `+0x00 -> 0x2228B190`: **register a classname and provider DWORD**. This is *not* a destructor. The previously unrecovered slot gets its precise purpose here.
- `+0x04 -> 0x22285550`: Phase171 create resolver: native `+0x0C` record lookup followed by registered provider virtual `+0x00` on hit.
- `+0x08 -> 0x22285580`: Phase171 destroy resolver: same record lookup followed by provider virtual `+0x04` on hit.
- `+0x0C -> 0x2228B010`: Phase171 16-byte record pointer lookup.

The native original singleton's separately registered destructor callback at `0x223CEA10` from Phase170 should not be confused with this registry vtable `+0x00` entry.

## 0x2228B190: native registry class-provider registration

This 78-byte routine accepts a classname pointer and a raw second 32-bit **provider word**. It computes the registry's embedded table pointer at `this+0x04`, interns the classname via original `0x2228F7A0`, then calls original **find-or-insert** `0x2228B0A0` to produce a **16-bit** registration index. It loads the record storage pointer from registry `+0x08` and stores the passed raw provider word at `storage + (index * 16) + 0x0C`. It does not first check for `0xFFFF` or for allocation failure. The host-neutral C++ implementation delegates table mutation and memory safety to external host callbacks, rather than reproducing unsafe raw original pointer arithmetic.

Exact internal CALLs: `0x2228B1AB -> 0x2228F7A0` and `0x2228B1C1 -> 0x2228B0A0`. The return retains the 16-bit registration index in AX, with native `RET 8`.

## 0x2228B140: registration through embedded index subobject

This 79-byte routine acts on the embedded index/record subobject (its `ECX` points at the registry `+0x04`, not the full registry). It again interns the original classname and calls the same **find-or-insert** function `0x2228B0A0`. Its second source argument is **a pointer to a DWORD**, not a raw provider DWORD as in the vtable method above. The routine dereferences that pointer **after the insertion/index lookup**, then writes the result to `subobject storage pointer + index*16 + 0x0C`.

Exact internal CALLs: `0x2228B15A -> 0x2228F7A0`, `0x2228B170 -> 0x2228B0A0`. Native `RET 8` and 16-bit returned index.

## 0x2228AEC0: existing classname slot search

The 60-byte routine invokes original `0x2228F7A0` to intern a classname into a 16-bit key, then original `0x2228A760` with that key and literal zero, returning its 16-bit index. It does **not** allocate/register a provider record and does **not** fetch the provider pointer at record+12. It can return the native not-found `0xFFFF` sentinel, unlike the two registration writers which do not branch on it. Exact internal CALLs: `0x2228AEDA -> 0x2228F7A0`, `0x2228AEF0 -> 0x2228A760`.

## 0x2228B1E0: native classname hash

The 40-byte method begins with 32-bit seed `0x1505` (5381), reads each zero-terminated input byte, performs `SHL 5`, adds the previous accumulator to multiply by **33**, then uses **`MOVSX` signed 8-bit byte-to-DWORD** and adds it. Each iteration wraps at 32 bits:

`hash = (hash * 33 + signed_int8(byte)) mod 2^32`

High-bit bytes `0x80` through `0xFF` therefore produce *different* results from unsigned-byte DJB2. Empty classname returns `0x1505`. Independent reference inputs: `flak -> 0x7C96D683`, `abc -> 0x0B885C8B`, single `0x80 -> 0x2B525`, single `0xFF -> 0x2B5A4`. A real original native caller uses direct `CALL 0x222C7A13 -> 0x2228B1E0`, but that caller's broader role remains unresolved.

## Code and test results

- `include/sin_compat/original_server_registry_registration.hpp`: portable C++17 real observed branch/callback state of registration and indexed lookup; exact signed-byte hash with modulo-32-bit behavior and no invented record allocator.
- `tests/test_original_server_registry_registration.cpp`: golden hash vectors including negative signed bytes and 32-bit wrapping, existing slot sentinel, direct vs indirect registration, argument-load-after-lookup order, original lack of index guard, and integration with Phase170's factory-context lifecycle.
- `research/tools/audit_sin_original_server_registry_registration.py`: original-only chained PE32 SHA256 verifier for four native complete function regions, 58 sparse opcode anchors, seven verified call relationships, and original four-DWORD vtable.
- `research/tests/test_sin_original_server_registry_registration.py`: synthetic witness corruption tests, independent hash vectors for all 255 nonzero input bytes, 128-case registration matrix, invalid-original SHA fixture and wrong-PE-base tests.
- CMake/CTest target `sin_original_server_registry_registration_tests`.
- Reviewed original recovery atlas **97 -> 101 methods** (**51 server, 50 client**). The signed-byte hash routine is `behavior_reconstructed`; the three registry functions remain `machine_code_verified` with external engine allocator/index callbacks. The manually reviewed tally is **not** a measure of whole-game completion.

Owner-local evidence commands:

```sh
python3 research/tools/audit_sin_original_server_registry_registration.py --client /private/client.dll --server /private/server.dll
python3 -m unittest discover -s research/tests -p test_sin_original_server_registry_registration.py -v
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --target sin_original_server_registry_registration_tests
ctest --test-dir build -R sin_original_server_registry_registration_tests --output-on-failure
```

## Scope and next native dependency

Original `0x2228F7A0` string-key interning, `0x2228A760` existing index lookup, **`0x2228B0A0` record insertion**, registry allocation/storage reclamation, per-class provider virtuals and actual flak entity/physics logic remain engine-owned. This portable reference is **not** a playable reconstructed Windows x86 DLL, Android gameplay runtime, or Quest 3 port. Windows game loader/playability validation remains the first target; continue reviewing `0x2228B0A0` and original registration call sites, then factory virtual object creation.

Keep the phase **draft, stacked on #223, unmerged**.