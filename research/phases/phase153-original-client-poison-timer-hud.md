# Phase 153: original client.dll CHudPoisonTimer state and rendering predicates

**10 November 2026.** Unlike Phases 148–151, which focused on original `server.dll` difficulty statistics, this phase recovers a new **client presentation** path from the real SiN Episodes: Emergence 2006 Windows x86 `client.dll`. Owner-held original binaries were directly inspected; no commercial machine-code bytes or game assets are committed.

## Original binary identity

- `client.dll` SHA-256: `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.
- `server.dll` SHA-256: `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`.

Original MSVC RTTI TypeDescriptor `0x24320EA8` spells **`CHudPoisonTimer`**; the primary CompleteObjectLocator at `0x242EFAD4` and secondary at `0x242EFA80` both point to it. Primary virtual table `0x242C91C4` has 13 known pointer slots. Secondary UI/panel table `0x242C8F94`, for a subobject displaced by `0x14` bytes, has 139 known slots. Both identities are confirmed from the original class metadata, not guessed from strings near assembly.

## Four original method-body hashes

| Original method | Vtable slot | Code length | Exact original region SHA-256 |
|---|---|---:|---|
| `0x242360B0`: reset | primary 2 | 11 | `4dfd88bca790581954a5a77ef16d80c8cd3e50c95525a01731d0998c0cce4151` |
| `0x242360C0`: bind three resources | primary 3 | 74 | `b3e2c15990cc1a70df2faeeaca0bc09be2e224a43fa71484115c1fc1a030f06d` |
| `0x24236120`: HUD render path | secondary 109 | 243 | `ca1b8cf8c14c650150eb8b2c8258deaaba3c712888bf6b6f495f0a458972d0e4` |
| `0x24236320`: local player/HUD update | secondary 93 | 203 | `921a342df76c159e6121789f111bcfaf66c4154cb47e4afd8b76fa2c06297150` |

The auditor checks exact original x86 method hashes, **24 field/branch opcode anchors**, eight original RTTI/vtable pointers, six rel32 direct calls and three float constants, on the **two complete pinned original DLL files**.

### Verified behavior and fields

- `0x242360B0`: one 32-bit zero store at `this+0x120`, followed by `RET`. No other state is touched. The field's original source identifier is unknown.
- `0x242360C0`: invokes original helper `0x241442C0` three times, **in order**, using string arrays at original offsets `+0xBC`, `+0xDD` and `+0xFE`; stores each resulting 32-bit handle in `+0x124`, `+0x128` and `+0x12C`. The helper's full lifetime and concrete engine interface are not reconstructed. The portable C++ model takes a host resolver.
- `0x24236120`: invokes inherited paint `0x240053A0`, compares float `this+0x10C` against **0.0**, selects the pointer at `+0x114` if it is strictly greater than zero, or `+0x118` otherwise. The x87 comparison's `TEST AH,0x41` includes equal, negative and unordered values in the second path. It skips subsequent drawing when 32-bit `+0x110` is zero or the selected pointer is null. The original then invokes `0x24154AB0` and `0x24154D50` for rendering; **actual geometry, font API, original event scheduling and screen output are not recovered here**.
- `0x24236320`: checks a global local-player pointer at `0x24338CF0`. When nonnull, reads float `player+0x11EC` and eventually writes `float(1.0 - player_value)` to `HUD+0x10C`. This uses the original double-precision literal **1.0** at `0x242DE320` through x87 `FSUBR`. There are additional floating comparisons and sound-dispatch branches referring to literal `PoisonTimeFadeIn` at `0x242C8E50` and `PoisonTimeFadeOut` at `0x242C8E3C`. Their *full* condition-to-sound effects and engine virtual callback signatures still need CFG recovery. Do not treat the portable projection helper as a complete 203-byte implementation.

### Why it matters

These original HUD routines are not ordinary stock Source Engine code inferred from a modern SDK. The concrete original class, its two virtual tables, four complete x86 method bodies, the player data offset, both branch targets and the resource names have been tied back to **this exact 2006 game DLL**. We now have a clean portable reference for state changes, resource lookup order and the native branch selection rule, plus an auditable queue to reconstruct the actual drawing and fade-in/out calls.

## Authored code, verification and coverage

- `include/sin_compat/original_client_poison_timer_hud.hpp`: host-neutral C++17 state, one-field reset, ordered triple resource resolver, precise positive vs nonpositive/unordered paint selection, and optional local-player `1.0-value` projection. **Not a 2006 object layout, client prediction module, VGUI implementation, or full audio emulator.**
- `tests/test_original_client_poison_timer_hud.cpp`: strict-warning CTest with untouched reset fields, resolver call ordering, zero/nonzero resource gates, positive/negative/zero/NaN/infinity threshold cases, absent local player, and timer value projection into paint selection.
- `research/tools/audit_sin_original_client_poison_timer_hud.py`: original-only SHA-pinned PE32 static verifier, metadata-only output.
- `research/tests/test_sin_original_client_poison_timer_hud.py`: synthetic fixtures that mutate every pointer, opcode, direct-call edge, string and constant and require fail-closed verification.
- `research/catalog/original_dll_verified_methods.json`: **four new client entries**; the 11-byte reset and 74-byte resource resolver are recorded as `behavior_reconstructed`, while the 243-byte paint and 203-byte player-update methods are `machine_code_verified` because their engine-dependent side effects are not fully rebuilt. The actual recovered render and timer portions remain documented and tested, but are not inflated into whole-method completion.
- Root CMake/CTest includes the client HUD regression.

Owner-only audit:

```bash
python3 research/tools/audit_sin_original_client_poison_timer_hud.py \
  --client /private/client.dll --server /private/server.dll \
  --output /private/phase153-client-poison-hud.json
```

Portable tests:

```bash
python3 -m unittest discover -s research/tests -p 'test_sin_original_client_poison_timer_hud.py' -v
cmake -S . -B build && cmake --build build --target sin_original_client_poison_timer_hud_tests
ctest --test-dir build -R '^sin_original_client_poison_timer_hud_tests$' --output-on-failure
```

## Next client DLL recovery work

1. Recover the complete `0x24236120` paint-to-VGUI argument graph, including x87 integer conversion and resource setup.
2. Decode all conditional audio predicates and virtual-dispatch methods in `0x24236320` with independently verified x87 condition flags before implementing fades.
3. Crosswalk the player `+0x11EC` field to the verified original RecvTable properties and server SendTable send proxies; do not assume it is a 2006 game-source identifier just because this HUD reads it.
4. Expand the Phase 152 method ledger with older client methods, prioritizing prediction, weapon state and HUD. Keep all inherited vtable slot identities separate from game-specific overrides.

**Boundary:** verified original client decompilation of limited functions and branch behavior; no engine-compatible rebuilt `VClient011` or playable Windows/Android/Quest client DLL demonstrated.
