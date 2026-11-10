# Phase 168: Original ScatterGun flak spawning and entity origin/parent transform

10 November 2026. Original native 2006 SiN Episodes: Emergence `server.dll` analysis, stacked on Phase167 PR #219. Both owner-supplied Windows PE32 game DLLs remain private.

## Native code provenance and important correction

The original ScatterGun secondary-fire pellet loop at `0x2231CA0B` calls `0x2231B9F0` once for each iteration, then calls `0x22141A00` on its returned pointer at `0x2231CA5C`. The latter is **a networked entity absolute-origin update with parent-relative transformation**, **not** a projectile damage, trace, or hit-registration method. Phase167's tentative 'final shot processing' wording is superseded by this stronger direct disassembly evidence.

- Original server.dll SHA256: `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`.
- Original client.dll SHA256: `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.
- Original **0x2231B9F0, 134 bytes**, exact whole-function SHA256 `3b8c5a4ad9f86b45224b4e4f06a346254818f1137cf9ae375a69f0ea19315b86` (ends in RET at 0x2231BA75).
- Original **0x22141A00, 307 bytes**, exact whole-function SHA256 `c5a9e2bb9cef0f468373a3a7205a17a1fefe297c1ddcbcebc140349c873d5c4f` (ends in RET 4 at 0x22141B30).
- Original `flak\0` classname bytes at `0x2247A1D8` are directly referenced by `0x2231B9F4`. This is real native game evidence of the spawned entity classname. It is not an assumed Source SDK prefab name.
- Chained original-DLL auditor verifies both full-function digests, **62 sparse native opcode witnesses**, **10 original direct CALL rel32 sites within these two functions**, and **both original ScatterGun caller edges**. Neither original binary bytes nor a full proprietary disassembly is committed.

## Original flak creation function, VA 0x2231B9F0

1. Pass original classname literal `"flak"` and integer `-1` to `0x22266B00`, retaining the resulting entity pointer. Native code assumes creation succeeded and does not test this pointer for null.
2. Call `0x22285AB0` with newly created entity, original first vector/argument pointer, and zero.
3. Read optional original third argument, representing another engine object. If null, skip binding resolution entirely.
4. If nonnull, invoke its virtual method `+0xFC` and test the result. If null, skip binding assignment. Otherwise invoke **the same virtual again** (not necessarily identical result), call `0x222AA640` with the new result, and test returned pointer. If null, write `-1` to the created flak entity's DWORD `+0x794`. If nonnull, invoke its virtual `+0x8`, dereference the resulting pointer to read a DWORD, and write that raw DWORD to flak `+0x794`.
5. Invoke newly created entity's virtual method `+0x50` with the original optional source pointer (even if null).
6. Call `0x221439C0` with the entity and second vector/argument pointer; the precise downstream engine effects are not reconstructed.
7. Invoke entity virtual `+0x64`, and return the original newly created entity pointer.

The host-neutral reference retains this exact order and conditional writes. It models the original assumption that the factory succeeds and does **not** invent missing native allocator/factory implementation.

## Original shared entity absolute-origin setter, VA 0x22141A00

1. Compare all three float32 coordinates in instance fields `+0x250`, `+0x254`, `+0x258` to the incoming vector via x87 `FUCOMPP/FNSTSW/TEST AH,0x44`. If all compare numerically equal, return immediately with no notification. Signed zero compares equal; NaN/unordered coordinates force the changed path, even for identical NaN payload bits.
2. On change, call `0x2214B620` with integer `4` (effect remains an engine callback); clear bit `0x10` of entity byte `+0xB9` and copy exactly three raw incoming DWORDs into absolute world origin fields `+0x250..+0x258`, retaining NaN and signed-zero payloads.
3. Read entity parent-handle DWORD `+0xF0`. `-1` means unassigned. Otherwise resolve through the original entity-handle array at global `0x224F5620`, splitting **12 low index bits** and upper serial bits, validating the serial and nonnull entry. Invalid or absent parent follows direct local-origin assignment.
4. Without a valid parent, call native `0x2213E1A0` on entity `+0x2C8` with the incoming vector and return.
5. With valid parent, inspect parent DWORD `+0xB8`. If **bit 12** set, call `0x2213D750` on parent before reading its world origin. Then subtract parent coordinates `+0x250..+0x258` from new world coordinates to form the intermediate 3D delta.
6. If **bit 11** set, call `0x2213FA60` on parent. Pass the intermediate delta and parent matrix/transform at `+0x268` through `0x22268E90`. Exact engine matrix conventions are retained behind a callback, not fabricated from the portable reference.
7. Call `0x2213E1A0` to store the resulting three-coordinate local origin at entity `+0x2C8`. Parent updates can affect transform results, so callback order matters.

## C++17 reconstruction and validation

- `include/sin_compat/original_server_flak_origin.hpp`: pure host-neutral `original_spawn_flak` and `original_set_world_origin`, raw three-float copies, x87-equivalent equality gates, conditional flak binding, original parent flags and callback order.
- `tests/test_original_server_flak_origin.cpp`: creation and owner-lookup order, null/valid association, both parent routes, parent refresh-before-delta, no-parent and unchanged fast path, signed zeros, and NaN payload retention.
- `research/tools/audit_sin_original_server_flak_origin.py`: exact original binary evidence validator chained through Phase167, including both full original DLL SHA256 checks.
- `research/tests/test_sin_original_server_flak_origin.py`: independent **400-case** vector/parent state matrix and sparse instruction/call/classname mutation tests, with original whole-body digest checks explicitly requiring the private DLLs.
- CTest target `sin_original_server_flak_origin_tests`.
- Recovery atlas **83 to 85 manually reviewed original methods**, **35 server / 50 client**, these two conservatively `machine_code_verified` rather than full ABI or engine-runtime-verified. Reviewed count is not a decompilation percentage.

Owner-only original audit (private files stay private):

```sh
python3 research/tools/audit_sin_original_server_flak_origin.py --client /private/client.dll --server /private/server.dll
python3 -m unittest discover -s research/tests -p test_sin_original_server_flak_origin.py -v
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --target sin_original_server_flak_origin_tests
ctest --test-dir build -R sin_original_server_flak_origin_tests --output-on-failure
```

## Remaining work

The executable game still depends on unreconstructed native entity factory/allocator, virtual +0x50/+0x64 setup, CBaseEntity parent handles and transformation matrix, 3D collision detection, projectile physics and damage processing. Future original-code targets are `0x22266B00` (actual entity factory), `0x221439C0` (spawn vector application), and the flak entity's virtual callbacks/impact handlers. No working independent Windows replacement DLL pair, Android gameplay runtime, or Quest 3 playable port is claimed. Leave the stacked PR **draft and unmerged**.
