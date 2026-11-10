# Phase 167: Original ScatterGun secondary fire and fire-vector helper

10 November 2026. Continues Phase166. Examined owner-held original Windows 2006 PE32 `server.dll` and `client.dll`; no original binaries, executable instruction dumps, or assets are committed.

## Original native executable provenance

- Original server SHA256: `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`.
- Original client SHA256: `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.
- **CWeaponScatterGun secondary fire** at original VA `0x2231C810`, **612 bytes**, complete SHA256 `323253a226cad936b4bc6c1e56c349b39eae8391b5414e203d86ba725fe16d5e`. Ends at native RET `0x2231CA73`. Prior Phase165 RTTI/vtable checks establish ScatterGun ownership.
- **Shared firing-vector acquisition helper** at VA `0x222CBC20`, **225 bytes**, complete SHA256 `ee1c0a4bf7a3913881d5809a637847b1d3d2ee0251bfb27d0f1df630532f91f3`. Ends in native RET 8 at `0x222CBCFE`.
- Chained owner-only audit checks prior Phase166 and both full DLL SHA256s, **54** sparse original opcode witnesses, **18** exact original direct CALL rel32 edges, **3** original data constants and both complete native function-region fingerprints. Sparse synthetic fixtures cannot pass the complete-region checks.

## Original secondary-fire branch/control-flow reconstruction

1. Call original weapon owner getter `0x222AD370`; return if null. Call owner virtual `+0x120` and retain its boolean result for a *later* animation/ammunition branch.
2. If owner DWORD `+0x298 == 3`, send weapon virtual `+0x3C4(0,0)`, read the game clock, add original double-precision literal `0.2` from `0x223D8FD0`, and write tracked weapon timer `+0x4A0` using original setter `0x221732A0`. Return without firing or acquiring vectors.
3. Otherwise acquire two firing vectors with native helper `0x222CBC20`. If it returns false, exit before changing fire state or timers.
4. Query weapon virtual `+0x43C`. If true and weapon secondary-clip DWORD `+0x4F0` is **nonpositive as a signed int32**, invoke weapon virtual `+0x380` and return. If `+0x43C` is false, this clip gate is not applied.
5. Send weapon `+0x3C4(3,0)`, obtain a selection from weapon `+0x394`, and dispatch selection with weapon `+0x314`. Obtain a float interval from original `0x220EEDE0` using raw weapon field `+0x3EC`. Perform **two distinct game-clock reads**, writing interval+clock to weapon tracked timers `+0x49C` and `+0x4A0` in that order through earlier Phase166 original setters. Retain the exact original equal/NaN notification semantics of those setters.
6. Call owner virtual `+0x3D4` unconditionally. If the owner `+0x120` result saved at entry was true, call owner virtual `+0x484(5)` and **recheck** weapon `+0x43C`. If true, call original `0x2231C740` to decrement secondary ammo, including unsigned 32-bit wrap behavior. Otherwise call original `0x222AA670` to consume one unit of alternative ammo type `weapon+0x4E8`. Query weapon virtual `+0x450`, pass its result to original owner ammo-count helper `0x222AA770`, and store result in weapon `+0x584`. Call weapon virtual `+0x3A0` and original `0x222AAFE0(6)`.
7. **Regardless** of saved animation eligibility, run original setup call `0x22033250` and exactly **eight** native pellet iterations. Each calls `0x22033280` using float 1.0 and data address `0x2252E888`, `0x22269E50`, owner getter `0x222AD370`, and `0x2231B9F0`, then performs three x87 coordinate multiplies by the original float **1500.0** at `0x2247A180` and invokes `0x22141A00`. Exact vector math, raycasting, hit registration and engine-side effects are **not** implemented in the portable model: this iteration stays an explicit host callback.

### Original 225-byte vector helper

- Find owner via `0x222AD370`, return false if missing.
- Call owner virtual `+0x120`. If true, copy exactly three raw DWORDs from owner virtual `+0x358` into first vector, and obtain the second from owner virtual `+0x514` using native float-bit literal `0x3DB27EB6` (approximately 0.08715574).
- Otherwise query fallback entity via `0x22139F60`; return false and **preserve both vector outputs unchanged** if none exists.
- If a fallback exists, copy the three-word owner `+0x358` vector to first output, then get second via fallback virtual `+0x734`. Three-word copies retain exact payload bits.

## Reference implementation and validation

- `include/sin_compat/original_server_scattergun_secondary_fire.hpp`: C++17 host-neutral portable branch/state model with typed vector snapshots, owner/weapon callbacks, signed-clip gate, Phase165 consumption, Phase166 tracked-float updates, separate clock reads and eight opaque pellet callbacks.
- `tests/test_original_server_scattergun_secondary_fire.cpp`: missing-owner and missing-vector paths, special state, signed ammo gates, two timer writes, both ammo routes, repeated vtable test, no owner animation side effects, loop count and callback order.
- `research/tools/audit_sin_original_server_scattergun_secondary_fire.py`: original-only complete SHA gate and sparse native witnesses, chained through Phase166.
- `research/tests/test_sin_original_server_scattergun_secondary_fire.py`: mutation tests for 54 original opcode witnesses, 18 CALL edges and 3 data constants, plus independent 384-case outcome matrix.
- Added root CTest target `sin_original_server_scattergun_secondary_fire_tests`.
- Original-function recovery ledger **81 to 83** (33 server, 50 client). Both new methods are conservatively marked `machine_code_verified`, rather than full native ABI or engine-runtime verification. The tally is not a completion percentage.

Local owner-only original audits require original privately held game DLLs:

```sh
python3 research/tools/audit_sin_original_server_scattergun_secondary_fire.py --client /private/client.dll --server /private/server.dll
python3 -m unittest discover -s research/tests -p test_sin_original_server_scattergun_secondary_fire.py -v
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --target sin_original_server_scattergun_secondary_fire_tests
ctest --test-dir build -R sin_original_server_scattergun_secondary_fire_tests --output-on-failure
```

## Limitations and next work

This is **verified real original game code recovery**, not a complete ABI-compatible replacement for original 2006 Win32 gameplay DLLs. Game engine virtual callbacks, actual pellet trajectory geometry, ammo networking, traces and damage are intentionally host-owned. Neither a playable independent Windows binary nor an Android/Quest3 gameplay port has been demonstrated. Next decompilation targets: original `0x2231B9F0` projectile dispatch helper, `0x22141A00` final shot processing and its player/trace dependencies. Keep this **stacked draft** on Phase166 PR218; do not merge.
