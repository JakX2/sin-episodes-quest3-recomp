# Phase 165: original server ScatterGun ammunition state

10 November 2026. This phase moves from client HUD research into original SiN Episodes: Emergence (2006) server.dll gameplay behavior. The owner-held original PE32 DLLs were privately inspected and SHA-256 verified. No original executable bytes or game assets are committed.

## Original identity and machine-code provenance

Original server.dll SHA256: 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650
Original client.dll SHA256: 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d

Original MSVC RTTI TypeDescriptor at 0x2252E8A8 contains the genuine CWeaponScatterGun class name. Its COL at 0x224CF8B4 is referenced by primary vtable[-1] at 0x2247A1E0. The primary vtable begins at 0x2247A1E4. The owner-only verifier checks both original RTTI links and vtable slots **216** (0x2231C3E0) and **227** (0x2231C810), so class ownership does not rely solely on nearby disassembly.

| Server function | Original code length | Complete original function SHA256 | What was reconstructed |
|---|---:|---|---|
| 0x2231C3E0 | 104 bytes | 5e5668894afb65fd7dd45078abe9d58cae147714e302b7e5952411b9fb91af43 | Ammo snapshot and deferred-action decision logic |
| 0x2231C740 | 51 bytes | b88574dc44edf04e41c044514f9a371e8e955c9f1ca9786c24d22fc70eb0ec0e | Secondary ammo decrement and two notification modes |

Both complete original method hashes matched the private 2006 server DLL. The verifier also requires 34 deliberately sparse instruction anchors, two exact original direct CALL edges and one tail JMP. These are not guesses taken from a generic Source SDK.

## Function 0x2231C3E0: native weapon ammo update

Original CWeaponScatterGun vtable slot216 calls base routine 0x222CD050 first. After that callback, it compares the previous primary-ammo snapshot (this+0x57C) to the current primary clip (this+0x4EC). On change, it updates the snapshot; it sets pending flag byte this+0x589 to 1 only if the previous snapshot was not 0xFFFFFFFF.

It next compares previous secondary snapshot this+0x580 against secondary state this+0x584, but only processes the change while byte this+0x578 is nonzero. As with primary ammo, a previous 0xFFFFFFFF is treated as an initial reading and does not newly latch the pending flag.

When pending byte +0x589 is nonzero, the routine tail-JMPs at 0x2231C441 to 0x2231C2F0, a separate original deferred handler. It does not clear the pending byte before dispatch. The effects of the base update and deferred handler remain external host callbacks rather than invented game engine implementations.

## Function 0x2231C740: secondary-clip decrement

At original ScatterGun vtable slot227 (function 0x2231C810), the callsite 0x2231C964 invokes 0x2231C740 with ECX set to this+0x4F0. The helper decrements that DWORD without a zero guard, wrapping 0 to 0xFFFFFFFF. It checks byte at original weapon+0x1F0. If nonzero, sets byte weapon+0x200 to 1. Otherwise, it checks the DWORD pointer at weapon+0x16C and, when nonnull, ORs bit 0 into the pointed-to DWORD. It returns with RET 4 and does not read its stack argument.

The x86 comparison of the pre- and post-decrement values is necessarily not equal for modulo-32-bit subtraction by one; the host model preserves the observable notification and ammo effects without a redundant comparison. No firing permission check, projectile launch or hit registration is claimed here.

## Files and validation

- include/sin_compat/original_server_scattergun_ammo.hpp: C++17 portable behavior model with original numeric member offsets and engine callbacks.
- tests/test_original_server_scattergun_ammo.cpp: strict tests for first snapshots, ammo changes, base-call ordering, pending latch, disabled secondary tracking, both notification routes, null pointer and wrapping decrement.
- research/tools/audit_sin_original_server_scattergun_ammo.py: exact owner-only original DLL fingerprints, two complete function SHA256 hashes, original RTTI, vtable, 34 instruction anchors, two calls and one tail jump.
- research/tests/test_sin_original_server_scattergun_ammo.py: original-free mutation tests and independent 972-case snapshot and 100-case decrement/event matrices.
- Root CMake/CTest registration.
- Original recovery ledger increases from **75 to 77** selected reviewed functions, now **27 server and 50 client**. These manually reviewed entries are not the total original function count or a game decompilation percentage.

Private original binary verification:

    python3 research/tools/audit_sin_original_server_scattergun_ammo.py --client /private/client.dll --server /private/server.dll

Original-free tests:

    python3 -m unittest discover -s research/tests -p test_sin_original_server_scattergun_ammo.py -v
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
    cmake --build build --target sin_original_server_scattergun_ammo_tests
    ctest --test-dir build -R sin_original_server_scattergun_ammo_tests --output-on-failure

## Next gameplay targets and limitations

Original deferred ammo-action handler 0x2231C2F0, full secondary-firing vtable method 0x2231C810, shared weapon update 0x222CD050, and broader NPC/gameplay state remain worthwhile. No complete original Windows server.dll game ABI or playable PC/Android/Quest build has been demonstrated. Native engine and input callbacks must still be integrated.
