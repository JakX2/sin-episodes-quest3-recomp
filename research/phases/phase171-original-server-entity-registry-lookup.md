# Phase 171: Original SiN entity registry vtable, class lookup and provider callbacks

11 November 2026. Continues stacked Phase170 draft PR #222. Direct x86 analysis of owner-held original *SiN Episodes: Emergence* (2006) PE32 `server.dll`, with both exact original DLL fingerprints. Proprietary game DLLs, instruction dumps and game assets remain private.

## Verified native provenance

- Original `server.dll` full SHA256: `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`.
- Original `client.dll` full SHA256: `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.
- Original registry vtable at **0x22456324**, verified as four DWORD entries:
  1. `+0x00 -> 0x2228B190`, unmodeled native destructor entry.
  2. `+0x04 -> 0x22285550`, **creation registry resolver**.
  3. `+0x08 -> 0x22285580`, **destruction registry resolver**.
  4. `+0x0C -> 0x2228B010`, **internal classname-to-registered-provider lookup**.
- The original registry constructor at `0x2228AFB3` writes this exact vtable; the Phase170 factory entrypoints use the `+0x04` virtual slot at original instructions `0x22266B3E` and `0x22266B6E`.

| Original entry VA | Complete native bytes | Complete original SHA256 | Reviewed function |
|---|---:|---|---|
| `0x22285550` | 46 | `58f62479791e357d5b220d60803a1a72e2cfc003944000c50f667f30730854f1` | Registry creation resolver, calls provider `+0x00` |
| `0x22285580` | 50 | `83eb972b13abd551e76ba781d13eed5ab614cd013c0d7f8c133020f7a2b2a5db` | Registry destruction resolver, calls provider `+0x04` |
| `0x2228B010` | 94 | `ba7b2d918b3b72235c9457731f50ebcfb3b6f001c699d981c788d7fed8c9abf5` | Internal symbol-indexed 16-byte registration record lookup |

The new owner-only auditor hashes **all 190 native bytes** in the three functions, independently checks **41 sparse x86 opcode witnesses**, both direct `CALL rel32` instructions in the internal lookup, all four full vtable pointers, both original caller virtual dispatch opcodes, and both complete unknown-entity diagnostics. The auditor calls the entire Phase170 audit first, including its chained dual-DLL fingerprint checks. Sparse original-free fixtures deliberately cannot reproduce the whole-region hashes.

## Original 94-byte internal lookup at 0x2228B010

1. Receives the raw classname pointer; initializes a 16-bit stack symbol handle to `0xFFFF`.
2. On registry member `this+0x20`, calls original `0x2228F7A0`, supplying the caller's classname and the address of the 16-bit result. This is the game's symbol-interning/lookup pipeline; exact hash and memory management are engine-owned.
3. Reads the resulting symbol word; invokes original `0x2228A760` on registry member `this+0x04` with the symbol word pointer and literal integer zero.
4. If the returned 16-bit record index is `0xFFFF`, return a **null pointer**. It is the record index, **not the symbol handle**, that gates this branch.
5. On hit, load the registry's original record storage pointer from `this+0x08`, compute `index * 16`, load the **DWORD at record +0x0C**, and return it as the registered provider pointer. The 16-byte record stride and DWORD position are verified directly by the original `SHL 4` and `MOV [EDX+ECX+0x0C]` instructions. The record's remaining fields and index algorithm are not implemented here.

Two exact direct internal native call edges: `0x2228B02E -> 0x2228F7A0` and `0x2228B044 -> 0x2228A760`. The recovered C++ control-flow model keeps symbol interning, record-index lookup, and record storage read as independent host callbacks.

## Registry virtual +0x04: creation resolver at 0x22285550

1. Call registry virtual `+0x0C` with the classname. On this actual registry object, that dispatches to the 94-byte lookup above.
2. If no provider is found, pass classname and the original literal at **0x22455F50** to the game's diagnostic function: `Attempted to create unknown entity type %s!` Return zero without any provider callback.
3. Otherwise invoke the **registered provider's virtual `+0x00`**, with the **same classname pointer**, and return its result. Thus Phase170's `0x22266B00` resolves the provider and calls provider `+0x00` **before** later invoking the returned factory object's virtual `+0x24`. Do not confuse the registry provider pointer with the factory pointer returned by provider `+0x00`.

## Registry virtual +0x08: destruction resolver at 0x22285580

1. Call the same registry virtual `+0x0C(classname)` internal lookup.
2. If missing, print distinct original diagnostic literal at **0x22455F80**: `Attempted to destroy unknown entity type %s!` Then return without invoking provider.
3. On a hit, call provider virtual `+0x04` with the caller's **second opaque pointer/word**, not the classname, and return its value. Native function uses `RET 8` versus creation wrapper's `RET 4`. The deeper provider virtual implementation remains unresolved.

## Code, validation and integration

- `include/sin_compat/original_server_entity_registry_lookup.hpp`: portable C++17 model of lookup/result, missing index `0xFFFF`, native record stride 16 and provider at +12, separate diagnostics and provider `+0`/`+4` effects.
- `tests/test_original_server_entity_registry_lookup.cpp`: host callback order and exact arguments, missing record and null provider gates, differing diagnostics and virtual dispatch, and **end-to-end integration with Phase170** `original_create_entity_by_name` including edict-context visibility during provider lookup and clearing before final factory `+0x24`.
- `research/tools/audit_sin_original_server_entity_registry_lookup.py`: chained private original whole-DLL/whole-function SHA checks, 41 independent sparse opcode witnesses, two exact direct CALLs, four native vtable entries, two Phase170 original virtual-slot callsites and two diagnostic literals.
- `research/tests/test_sin_original_server_entity_registry_lookup.py`: original-free native mutation tests, independent 128-case record index/provider/creation/destruction outcomes, wrong-base and full-hash anti-fake tests.
- Root CTest target `sin_original_server_entity_registry_lookup_tests`.
- Reviewed evidence ledger rises **93 -> 96 manually reviewed functions** (**46 server, 50 client**). The three methods are marked `machine_code_verified`, not claimed runtime ABI-ported. The reviewed-function tally is not a whole-game recovery percentage.

Private original-only audit:

```sh
python3 research/tools/audit_sin_original_server_entity_registry_lookup.py --client /private/client.dll --server /private/server.dll
python3 -m unittest discover -s research/tests -p test_sin_original_server_entity_registry_lookup.py -v
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --target sin_original_server_entity_registry_lookup_tests
ctest --test-dir build -R sin_original_server_entity_registry_lookup_tests --output-on-failure
```

## Remaining true native dependencies

`0x2228F7A0` classname symbol interning, `0x2228A760` 16-bit registration index search, provider `+0x00/+0x04` implementations, native record allocator/registration, factory object virtual `+0x20/+0x24` and all flak/gameplay engine objects require further work. The portable recovery does **not** create an independently playable 2006 Windows DLL pair, Android port or Quest 3 port. Keep this phase stacked as an **unmerged draft** above Phase170 PR #222; Windows PC playback verification remains first.