# Phase 170: Original server entity factory, classname registry and lazy singleton

11 November 2026. Stacked on Phase169 Draft PR #221. Original SiN Episodes: Emergence (2006) Windows x86 PE32 `server.dll` and `client.dll` remain private. This phase examines the **real game-side entity factory**, directly called by the previously recovered ScatterGun `flak` spawn routine.

## Exact original 2006 native-code evidence

Original complete DLL SHA256s, retained by the chained owner-only audit:

- `server.dll`: `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`.
- `client.dll`: `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.

Four **complete** original native-function regions were independently SHA256-pinned to the original `server.dll`, together totaling **262 native bytes**:

| Original VA | Size | Original whole-function SHA256 | Meaning |
|---|---:|---|---|
| `0x22266B00` | 89 bytes | `3325571079cbbca426a5bbf82b3107161480a5084af1865ced302e7a2f2d236b` | CreateEntityByName-style classname lookup, optional edict allocation, factory dispatch |
| `0x22266B60` | 29 bytes | `0ca3b1d05299ea46641fd1e12e7c9c415d649a07be32acd90af6da9c0b44a9db` | Secondary classname registry dispatch through a different factory virtual method |
| `0x2228B110` | 48 bytes | `19b2485323f81a818354e47c14ed282e982f371827a7af4c1556a5293a49d558` | Lazy global classname-registry singleton |
| `0x2228AFB0` | 96 bytes | `7fd117f38076655eb4b0e4c2911d8ba3954986ec2488250b1efb4987b009faf6` | Registry storage constructor and map/index initializer |

Owner-only audit `research/tools/audit_sin_original_server_entity_factory.py` verifies all four whole-code digests, **49 original sparse opcode witnesses**, **six native CALL rel32 edges**, the real ScatterGun `0x2231B9F9 -> 0x22266B00` caller, and the original warning string beginning `CreateEntityByName(` at `0x224542A4`. All original native bodies remain private; only hashes, partial instruction witnesses and independently authored models are committed.

## Original game factory entry point at 0x22266B00

1. Original native arguments are a classname pointer and a **signed 32-bit edict index**. A value of `-1` skips explicit engine edict creation, without changing the temporary global edict context beforehand.
2. If the index differs from `-1`, read engine interface pointer from `0x22572198` and call its virtual `+0x54(index)`. Store the resulting edict pointer, including null, to original global `0x22558764`.
3. On null edict, call the original imported warning/diagnostic function using the original `CreateEntityByName( %s, %d ) - CreateEdict failed.` literal. **There is no early return**. Classname resolution and later factory invocation still proceed.
4. Call registry singleton `0x2228B110`; invoke its virtual `+0x04(classname)` to obtain an opaque registered factory pointer. **Clear the original global edict context to null immediately after lookup, regardless of lookup result.** The lookup itself can observe the context value from step 2 (or any previous value on the `-1` path).
5. If registry lookup returned null, return null. Otherwise **tail jump** to factory virtual `+0x24` and return its result. Actual original allocator/object ABI and callbacks remain unreconstructed engine behavior.

Native callsites: direct CALL `0x22266B34 -> 0x2228B110`; indirect engine virtual `+0x54` at `0x22266B18`; registry virtual `+0x04` at `0x22266B3E`; factory tail JMP `+0x24` at `0x22266B56`; original global context clear at `0x22266B44`. The original `flak` function calls this entry at `0x2231B9F9` using the literal classname and index `-1`.

## Alternate dispatch 0x22266B60

The 29-byte native entry obtains the same lazy registry singleton, uses the same registry virtual `+0x04(classname)` lookup, checks for null, and **tail jumps to factory virtual `+0x20`**, **not** `+0x24`. Unlike the primary entry point, it does not allocate an edict, print the failed-edict diagnostic, or modify the global edict context. The result of `+0x20` is not labeled as a created entity without additional engine/vtable evidence. There were no direct CALL rel32 references to this entry in the original whole `.text` census; it is nevertheless a complete observed machine-code function.

## Lazy classname-registry singleton 0x2228B110

Original guard byte at `0x22590004` is checked for bit 0. On first access, the native code **ORs bit0 into the DWORD guard before calling the constructor**. This matters for reentrant calls, which see a partially constructed but already guarded instance instead of constructing twice. The constructor runs on fixed native storage at `0x2258FFB0`; then the game registers a destructor callback pointer at `0x223CEA10` using `0x223A5639`. Subsequent calls return the same fixed registry storage immediately. Existing unrelated guard bits are preserved.

Original direct CALLS: `0x2228B128 -> 0x2228AFB0` and `0x2228B132 -> 0x223A5639`.

## Native registry constructor 0x2228AFB0

The 96-byte original constructor:

- Writes native vtable pointer `0x22456324` at `this+0x00` and callback function pointer `0x222868C0` at `this+0x04`.
- Clears `this+0x08` and `this+0x10`; sets `this+0x0C` to `0x80`.
- Allocates **0x800 bytes** using original `0x2226E390`, stores the returned allocation at `this+0x08`.
- Sets 16-bit sentinel fields `this+0x14` and `this+0x18` to `0xFFFF`, clears 16-bit fields `this+0x16` and `this+0x1A`, and aliases the allocation at `this+0x1C`.
- Calls native `0x2228F360` on member `this+0x20` with the original argument sequence **(0, 0x80, 1)**, then returns the same object. The actual symbol map/list and memory allocator implementations are still native engine dependencies, not fabricated inside the portable host model.

## Implementation, regression, and exact scope

- `include/sin_compat/original_server_entity_factory.hpp`: host-neutral C++17 control-flow reconstruction for the two factory entry points, registry singleton and original constructor fields. All original virtuals, allocator, diagnostic, and destructor registration remain explicit host callbacks.
- `tests/test_original_server_entity_factory.cpp`: reentrant constructor guard, exact initialization order and fields, default/explicit edict selection, failed edict warning that does not prevent dispatch, context visibility during classname lookup, context clearing before primary factory dispatch, null lookup behavior, and the alternate virtual `+0x20` preserving context.
- `research/tools/audit_sin_original_server_entity_factory.py`: original-only SHA gate chained through Phase169 and both original game DLL hashes, 49 opcode witnesses, seven real CALL edges including incoming flak, literal data check.
- `research/tests/test_sin_original_server_entity_factory.py`: original-free mutations of **all 49 opcode witnesses, seven relative CALLs and diagnostic literal**, independent 64-case factory gate matrix, image-base gate and proof sparse fixtures cannot satisfy full original function SHA checks.
- CTest target: `sin_original_server_entity_factory_tests`.
- Manually reviewed evidence atlas rises **89 -> 93 original functions** (**43 server, 50 client**). These four are conservatively `machine_code_verified` with host-reconstructed branch behavior rather than ABI-compatible game implementations. Function count does **not** represent percentage of game completion.

Local original-only audit, with exact private original DLL files:

```sh
python3 research/tools/audit_sin_original_server_entity_factory.py --client /private/client.dll --server /private/server.dll
python3 -m unittest discover -s research/tests -p test_sin_original_server_entity_factory.py -v
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --target sin_original_server_entity_factory_tests
ctest --test-dir build -R sin_original_server_entity_factory_tests --output-on-failure
```

**Not claimed:** A working original MSVC Win32 gameplay DLL pair, Source game allocator/factory ABI, functional entity/spawn ownership, exact native classname symbol table, flak collision/impact simulation, or Android/Quest3 gameplay. Next high-value original code: registry virtual `+0x04` lookup implementation and the two distinct registered factory virtuals `+0x20/+0x24` (identify registration and object construction). The PC Windows game must be validated before Android and Quest 3. Keep this PR **DRAFT**, stacked on Phase169, **unmerged**.