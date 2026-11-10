# Phase 155: original CHudBreathTimer and breath client/server property

**10 November 2026.** Continued original SiN Episodes: Emergence (2006) native Windows PE32 client.dll/server.dll reverse engineering, stacked on Phase 154. The directly inspected originals were SHA-256 identified as:

- `client.dll` = `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`
- `server.dll` = `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`

No original DLLs, executable machine-code dumps, assets, or licensed engine source are committed.

## Original class, two vtables, four method hashes

MSVC RTTI TypeDescriptor at `0x24320D34` names **`CHudBreathTimer`**. Its primary vtable begins at `0x242C78CC`; the secondary `vgui::Panel` subobject at displacement `+0x14` uses vtable `0x242C769C`. Their COL and RTTI references are checked directly against the original binary. Both original `CHudBreathTimer` and the previously recovered `CHudPoisonTimer` derive from `CHudElement` and `vgui::Panel`; shared field `+0xA6` is read by both, but its original source-level name and all writers are **not** yet established.

| Original VA | Virtual entry | Function length | Full original SHA-256 |
|---|---|---:|---|
| `0x24230A50` | primary slot2, reset | 11 | `8cb95062e758c4f0a32de92459dee9830edf87cd39c1c601676cdd6a51a75eb5` |
| `0x24230A60` | primary slot3, bind resources | 51 | `e80f0d9011daa95ee7bcd3a71cc86856219588ec61cff3d6ea773bf4c2ee8ef7` |
| `0x24230BB0` | secondary panel slot93, update | 224 | `1a04ae5f33db4ca385e92b0328ad3b280e6dd0994f4c5ec467717b2ce4eb6114` |
| `0x24230C90` | secondary panel slot109, paint | 233 | `0b18199261f6fb6a0477a08022d60bb0791cd5379184ec0f9fcc14022b91a43b` |

### Behavioral reconstruction

- Original primary slot2, `0x24230A50`: stores float **1.0** at `HUD+0x100` and returns. The portable `reset_slot` preserves this one-field behavior.
- Original primary slot3, `0x24230A60`: resolves two strings stored at `HUD+0xBC` and `HUD+0xDD` through original `0x241442C0`, and stores handles at `+0x104` and `+0x108`. The clean `bind_two_resources` uses an abstract host resolver and preserves call order.
- Original secondary panel slot109, `0x24230C90`: calls inherited paint `0x240053A0`, then proceeds to rendering **only if** byte `HUD+0xA6` is nonzero and both original handles at `+0xF0` and `+0xF4` are nonnull. The subsequent native VGUI resource usage, geometry and x87-to-integer coordinates are *not* implemented; this is only the verified paint gate.
- Original secondary panel slot93, `0x24230BB0`: returns immediately on a null local-player pointer or if new `player+0x11F0` is equal (ordered floating comparison) to cached `HUD+0xEC`. Otherwise it takes the branch sequence below, optionally calls the original sound singleton virtual method at byte offset `+0x24` and helper `0x240104B0`, and finally stores the **unmodified** incoming float at `HUD+0xEC`. Sound events are invoked before this write. Unordered NaNs skip all sound paths but still update.

### Four verified breath-event string branches

Original native string literals: `BreathTimeFadeIn`, `BreathTimeLow`, `BreathTimeEmpty`, `BreathTimeFadeOut`. Exact original x87 branch order:

1. If input is strictly below **1.0** and original HUD status `+0xA6` is **zero**, emit **FadeIn**.
2. Otherwise, if input is strictly below **0.25**, emit **Low**, independent of status.
3. Otherwise, if input equals **0.0**, emit **Empty**.
4. Otherwise, if input equals **1.0** and status `+0xA6` is **nonzero**, emit **FadeOut**.
5. Otherwise, emit no event.

**Important original control-flow quirk:** for ordinary ordered IEEE-754 floats, step 3 cannot be reached at input zero because step 2 takes the **Low** branch first. The branch and literal are nevertheless present in the original executable and preserved as a distinct dispatch case in the source. This is *not* a license to silently reorder the conditions to improve the game logic. The independent x87 status-flag test checks 1,040 combinations of old/incoming floats, NaN, infinity, status bytes and presence of the local player; no combination reaches the `Empty` branch.

Unlike the Phase154 poison HUD, which writes `float(1.0 - m_poisonTimer)` and requires an exact `+0xA6 == 0xFF` FadeOut condition, breath stores the raw `m_breathTimer` and takes FadeOut for **any nonzero** `+0xA6` byte. The two routines should *not* be combined into a generic timer state machine without preserving these differences.

## Networked field provenance

The original `DT_HL2Local` `RecvProp` registers **`m_breathTimer`** at nested offset `+0x28`, original client site `0x2421AD35`, SHA256 of its 26-byte bounded instruction sequence `85fa43b87b76f50680eff732bf0b851a9424f707fddcd2ebe22925905c34c1e1`.

The original server `SendProp` registers the *same* `m_breathTimer` nested offset `+0x28`, original server site `0x223A2928`, SHA256 of its 35-byte bounded code `8790ee840f413d633501963f9b0a68147048a02cfe21a58dc9c366ac02dd2456`.

Phase153 already verified that `m_HL2Local` is embedded at original **client player +0x11C8** and **server player +0x137C**. Therefore client absolute `0x11C8 + 0x28 = 0x11F0`, matching the literal read in the breath HUD assembly, while server absolute `0x137C + 0x28 = 0x13A4`. These are **original 2006 x86 offsets**, never Android/Quest ARM64 layout promises.

## Added clean-room code and tests

- `include/sin_compat/original_client_breath_timer_hud.hpp`: portable C++17 state/reset, ordered resource resolution, paint gate, ordered sound-token selection and callback-before-update.
- `include/sin_compat/original_breath_timer_network_contract.hpp`: actual nested client/server original offsets with static assertions, building on the existing verified `m_HL2Local` contract.
- `tests/test_original_client_breath_timer_hud.cpp`: reset scope, resource ordering, panel paint gate, all reachable cue branches, equality/no-player, NaN/infinity and two original player layouts.
- `research/tools/audit_sin_original_client_breath_timer.py`: original owner-only SHA chain, RTTI, both original vtables, four full HUD function bodies and both bounded network initializer sequences, original instruction/call/string/constant witnesses.
- `research/tests/test_sin_original_client_breath_timer.py`: synthetic mutation tests and independently expressed x87 C0/C2/C3/PF control-flow model across 1,040 cases.
- Root CMake and CTest registration.
- The Phase152 manually reviewed recovery ledger adds **four previously untracked original client methods**. Reset and resource binding are `behavior_reconstructed`; the more extensive update and paint methods remain `machine_code_verified` because original sound/VGUI engine side effects are not fully recreated.

Run owner-only original-code audit from repository root:

```sh
python3 research/tools/audit_sin_original_client_breath_timer.py \
 --client /private/client.dll --server /private/server.dll \
 --output /private/phase155-breath.json
```

Original-free regression:

```sh
python3 -m unittest discover -s research/tests -p 'test_sin_original_client_breath_timer.py' -v
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --target sin_original_client_breath_timer_hud_tests
ctest --test-dir build -R '^sin_original_client_breath_timer_hud_tests$' --output-on-failure
```

## Remaining unknowns

The shared inherited `HUD+0xA6` byte's meaning and all its writers, actual audio singleton at `0x2437F8A0`, engine callback `+0x24`, helper `0x240104B0`, and VGUI paint/number layout remain unresolved. The portable model does **not** implement a native `VClient011` engine module, actual sound playback, full original gameplay or Android/Quest VR execution.

**Next:** trace real writers to `+0xA6` in the relevant class hierarchy (distinguish unrelated classes whose members happen to share that displacement); recover the sound helper and complete the breath/poison HUD paint arguments; then continue broader client prediction and weapon-state decompilation.
