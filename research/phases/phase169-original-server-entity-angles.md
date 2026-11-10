# Phase 169: Original server entity absolute angles, x87 vector comparisons and parent rotations

11 November 2026. Stacked on Phase168 Draft PR #220. This is reverse engineering of the owner-held **original SiN Episodes: Emergence (2006) Windows PE32 server.dll**, not a Source SDK guess. Original game executable bytes, full disassemblies, and assets remain private.

## Original executable provenance

Both complete original fingerprints are required by the chained private binary audit:

- `server.dll` SHA256: `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`.
- `client.dll` SHA256: `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.

Four complete function bodies were identified and independently SHA256 verified from the original 2006 `server.dll`:

| Original VA | Native length | Full function SHA256 | Function |
|---|---:|---|---|
| `0x221439C0` | 412 | `6ad0a90de973cdb3fded0dca507c5905d8c40ec3d06c9d3ee0f1cccd3b708205` | Entity absolute angle update, parent-relative transform |
| `0x2213A360` | 91 | `0b856a01ccd342b7e18d656d1ac7c8708dd318942fe53d6efd11653ab4ef4410` | 3-float x87 changed comparator |
| `0x220E2E70` | 60 | `a6ad19ca3a57bed3a25fdcddaae6a80dbac81ae489f7bba36a39ec52167e04ab` | 3-float x87 equal comparator |
| `0x22140040` | 99 | `c3fcfa2b46be4418d626a752cd75307102200c365d9d1e308cd9afec3d7710e4` | 3-float tracked networked setter |

The original functions are exactly **662 native bytes in total**. `research/tools/audit_sin_original_server_entity_angles.py` checks 96 intentionally partial native opcode witnesses, all 13 original direct CALL rel32 instructions inside the 412-byte routine, and the original flak spawn caller at `0x2231BA65 -> 0x221439C0`. The complete native regions remain owner-local. The private audit chains Phase168 and both original DLL hashes.

## Corrected original flak interpretation

In Phase168, `0x2231B9F0` creates an entity using original classname `flak`, calls native entity initialization `0x22285AB0`, associates an optional owner/source, calls created-entity virtual `+0x50`, **calls `0x221439C0` with its second vector**, and then calls virtual `+0x64`.

The Phase169 disassembly proves that `0x221439C0` sets **absolute orientation angles** at entity `+0x354..0x35C`, not an unspecified generic spawn vector. This corrects the Phase168 callback and its documentation without claiming the opaque factory/initialization routines are fully recovered.

## Full branch/callback model: 0x221439C0

1. First call native `0x2213FA60` on the entity, **even if the eventual new angle vector compares equal**. The engine's cache refresh is host-provided and may affect the comparison.
2. Compare the existing absolute three-float vector `this+0x354` with the incoming three-float pointer via native `FUCOMPP/FNSTSW/TEST AH,0x44`, axis by axis. All three numerically equal returns without further work. Signed zeros compare equal; NaN is unordered and forces change.
3. On change, call native `0x2214B620(this,2)`; clear bit `0x08` in byte `this+0xB9`, then copy three exact incoming 32-bit words to `this+0x354..0x35C`. Rebuild orientation matrix at `this+0x268` using original `0x22269F80` and update auxiliary matrix/transform at `this+0x2BC` using original `0x22269000` and integer 3.
4. Check parent handle `this+0xF0` against invalid `-1`. If valid, original `0x224F5620` handle registry is consulted with **12-bit low index** and **20-bit upper serial**; matching serial and nonnull object are required. The portable host performs the actual registry lookup.
5. With no valid parent, the proposed absolute angles become the candidate local angles at `this+0x36C`. With a valid parent, read parent `+0xB8` bit 11 (`0x800`). If set, call `0x2213FA60` on the parent before comparing absolute angles.
6. Call native **`0x220E2E70`** to compare child and parent absolute three-float angles. If equal, candidate local angles are **exact +0.0,+0.0,+0.0**. If unequal, check parent bit 11 **again** and refresh a second time if now set; call original parent inverse `0x22268F10`, child/parent matrix composition `0x22269520`, and local angle extraction `0x22268A60`. Do not fabricate these native matrix operations: the portable model exposes host callbacks preserving their order.
7. Call original **`0x2213A360`** on current local angles `this+0x36C` and candidate local angles. Crucially, this **only tests for a difference and does not assign**. If equal, skip all final updates.
8. If different, call original **`0x22140040`** on the vector at `this+0x36C`. This compares again, sets dirty byte `this+0x200` if mode byte `this+0x1F0` is nonzero, else ORs bit0 into any nonnull event DWORD pointed to by `this+0x16C`, and copies exactly three raw float32 words. Finally, read the original shared game clock at `0x22572190+0x0C` and call `0x22042D70` with that time. None of these effects occurs if local angles were already equal.

The original 91-byte changed comparator snapshots the incoming three DWORDs on its stack before comparing. The original 60-byte equal comparator reverses the boolean. The 99-byte tracked vector setter preserves original signed-zero/no-write and NaN/changed semantics. Its notifier offsets are relative to the target vector, **not** the entity base.

## Code and regression

- `include/sin_compat/original_server_entity_angles.hpp` models valid original branch decisions, direct x87-equivalent comparisons, tracked vector notifications and exact raw-value copies; its original engine cache/matrix/time/registry effects are explicit host callbacks.
- `tests/test_original_server_entity_angles.cpp` checks initial refresh before unchanged fast path, raw signed-zero and NaN behavior, owner and parent paths, parent double-refresh ordering, parent flag changes between reads, local angle change notification/time, unchanged local angle suppression, and invalid serial lookup.
- `research/tools/audit_sin_original_server_entity_angles.py` requires full owner-original function digests and verifies native opcode/call witnesses, chained to the Phase168 SHA-pinned dual-DLL audit.
- `research/tests/test_sin_original_server_entity_angles.py` uses independent x87-status-math comparisons, native opcode/call corruption fixtures, 147 vector-case and 882 tracked-notifier-case matrices, and 36 parent-handle/flag test cases. Synthetic fixtures cannot satisfy original full-function hashes.
- The root CMake/CTest target is `sin_original_server_entity_angles_tests`.
- Corrected Phase168's flak callback identifier and description to **absolute angles**.
- Manually reviewed method atlas **85 -> 89 verified original methods (39 server, 50 client)**. `0x221439C0` is conservatively `machine_code_verified` with host callbacks. The three complete x87 helpers are `behavior_reconstructed`. These numbers are not whole-game completion percentages.

Owner-only original audit:

```sh
python3 research/tools/audit_sin_original_server_entity_angles.py --client /private/client.dll --server /private/server.dll
python3 -m unittest discover -s research/tests -p test_sin_original_server_entity_angles.py -v
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --target sin_original_server_entity_angles_tests
ctest --test-dir build -R sin_original_server_entity_angles_tests --output-on-failure
```

## Not yet reconstructed

Neither the original Source entity factory `0x22266B00` nor underlying engine matrices/handle table ABI, flak creation, original flak physics/collision or damage callback execution have been fully ported. The existing Windows factory proxy, Android reference code and host-neutral unit tests do **not** form a verified playable replacement for the 2006 client/server DLLs. A functional original Windows game should precede any Android/Quest 3 gameplay port. Keep the stacked Phase169 PR **draft and unmerged**.

High-value next original-game targets: entity factory `0x22266B00`, created-flak virtual `+0x50/+0x64` binding, and original projectile collision/impact functions.
