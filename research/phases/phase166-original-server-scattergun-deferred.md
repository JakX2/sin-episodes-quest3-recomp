# Phase 166: Original ScatterGun deferred action and float time propagation

10 November 2026. This phase continues original SiN Episodes: Emergence (2006) server.dll reverse engineering, building directly on Phase165's server-side CWeaponScatterGun ammo snapshot observer. Original DLL files remain private. No full proprietary function bytes are committed.

## Original owner-supplied executable evidence

- server.dll SHA256: 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650
- client.dll SHA256: 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d

| Native original server VA | Code bytes | Whole original region SHA256 | Recovery |
|---|---:|---|---|
| 0x2231C2F0 | 240 | a1150899b9ecb8e31f9c5b93d749cc6c93abe7529fc5a2ccdbf0f6749efc8916 | Deferred ScatterGun action state machine |
| 0x2203D190 | 64 | afa46015b3a6f73bdab83b3095e84c7e0fe6aa42419e92cc5c5529c6dbf87357 | Player/owner+0x60C tracked-float update |
| 0x22173260 | 64 | d95240a043eb30317c36583e375a0cadcde6bc95f8856e5ccbdacb635ec7f8c3 | Weapon+0x49C tracked-float update |
| 0x221732A0 | 64 | a0544a00be1504ca79951ac0e73096c194eab280690c5359884646a68f84819e | Weapon+0x4A0 tracked-float update |

The complete region fingerprints were calculated from the exact original 2006 PE32 server.dll. The SHA-pinned owner-only audit verifies all four whole regions, 59 sparse instruction anchors, six original CALL edges, and the original incoming tail jump. Phase165's original DLL SHA and ScatterGun RTTI checks are chained. Synthetic unit fixtures contain only sparse witnesses and cannot satisfy the original whole-function hash tests.

## Original 240-byte deferred ScatterGun handler at 0x2231C2F0

This function is reached by the native ammo-update vtable slot216 through original tail JMP 0x2231C441.

1. Read pending byte at this+0x589; if zero, return without asking for game time.
2. Read the float at global game-clock pointer 0x22572190+0x0C. Compare float this+0x49C to the clock using x87 FCOMP and TEST AH,0x41. When the saved time is strictly greater than the clock, return. Equal, less and unordered/NaN pass the deadline gate.
3. Require positive **signed** 32-bit ammo count at this+0x4EC. Zero or raw values with bit31 set suppress the action, leaving pending intact.
4. Require byte this+0x4AC to be zero; otherwise leave pending intact.
5. Call original owner getter 0x222AD370. If null, leave pending intact.
6. Clear the pending byte **before** calling original virtual +0x3C4 with native arguments (13,0).
7. If secondary tracking byte this+0x578 is nonzero at this moment, call 0x220ECB40 using original data address 0x2247AAF0 and use its returned integer. Otherwise use literal 0x126. Pass that value to original virtual +0x314. Its actual engine meaning is not established, so the reference calls it an opaque selection.
8. Read original timing argument this+0x3EC; call 0x220EEDE0 to obtain a float. Add the game clock read **a second time**, after the preceding external calls. The two clock samples may differ.
9. Call original setter 0x2203D190 on player+0x60C with the computed deadline.
10. Read back the resulting stored player timer (which may not have changed if numerically equal) and pass it to original setters 0x22173260 for weapon+0x49C and 0x221732A0 for weapon+0x4A0, in that order.

The standalone C++ reference models these branches and call ordering with explicit host callbacks for the owner getter, virtual methods, selector resolver, timing provider and clock. It does not invent engine-side firing animation, sounds or network events.

## Three original x87 float setter bodies

All three implementations use FLD / FUCOMPP / FNSTSW / TEST AH,0x44 / JNP and perform no update when float32 values compare numerically equal. This includes -0.0 and +0.0 with different bit patterns. Unordered/NaN comparisons always take the changed path even if NaN payload bits match.

On change:
- If the containing owner/weapon mode byte +0x1F0 is nonzero: set dirty byte +0x200 to 1 and assign the new float.
- Otherwise, if pointer +0x16C is nonnull: OR bit0 into the pointed-to DWORD and assign the new float.
- If that pointer is null, still assign the float without notification.
- If float values compare equal, leave existing bits and dirty flags untouched.

Original method addresses and relative offsets are independently verified, although a common portable C++17 helper implements their identical observable behavior.

## Exact direct native call graph

- 0x2231C33C to 0x222AD370: owner lookup.
- 0x2231C370 to 0x220ECB40: tracked selection ID.
- 0x2231C390 to 0x220EEDE0: timing value.
- 0x2231C3AE to 0x2203D190: owner float update.
- 0x2231C3C0 to 0x22173260: weapon first timer update.
- 0x2231C3D6 to 0x221732A0: weapon second timer update.
- Original incoming tail jump 0x2231C441 to 0x2231C2F0.

Original virtual method offsets +0x3C4 and +0x314 are verified by sparse opcode anchors but their full engine effects remain unresolved.

## Code, regression, and remaining work

- include/sin_compat/original_server_scattergun_deferred.hpp: callback-driven C++17 host state, signed ammo gate, two clock samples, three float setters and two-mode state-change notifications.
- tests/test_original_server_scattergun_deferred.cpp: early exits, unsigned-bit representation of signed ammo, NaN deadline, signed zero, player absence, changed tracking mode after event, separate dirty/flag routes, exact callback order and Phase165 integration.
- research/tools/audit_sin_original_server_scattergun_deferred.py: private original DLL SHA gate, four original region digests, 59 opcode witnesses and seven call/tail-jump relationships.
- research/tests/test_sin_original_server_scattergun_deferred.py: synthetic witness mutation tests and independent x87 C0/C2/C3 status-word, 720-case gate model and 882-case notifier matrix.
- Root CMake/CTest target.
- Reviewed-function recovery ledger rises from 77 to 81 (31 server, 50 client). Three shared float setters are behavior-reconstructed; the 240-byte deferred handler is machine-code-verified with an external-callback behavior model. The evidence ledger is NOT a count of all native functions or a percentage of game completion.

Owner-only verifier:

    python3 research/tools/audit_sin_original_server_scattergun_deferred.py --client /private/client.dll --server /private/server.dll

Original-free regression:

    python3 -m unittest discover -s research/tests -p test_sin_original_server_scattergun_deferred.py -v
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
    cmake --build build --target sin_original_server_scattergun_deferred_tests
    ctest --test-dir build -R sin_original_server_scattergun_deferred_tests --output-on-failure

Next high-value targets are the full ScatterGun secondary-fire method 0x2231C810, weapon virtual callbacks and projectile/hit computation, and the shared weapon update 0x222CD050. The project does not yet build a verified playable original Windows replacement server/client DLL pair, nor standalone Android or Quest 3 gameplay.
