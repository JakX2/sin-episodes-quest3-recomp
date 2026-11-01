# Phase 89: Verified 2006 grunt input-handler behavior

**Scope:** Original 2006 owner-held Windows x86 SiN Episodes `server.dll` machine instructions, not just detached input strings. Paired with independently authored engine-neutral C++ tactical-state updates and original-free synthetic regression tests. This phase does **not** reconstruct the original enemy-acquisition/squad AI scheduler or a playable game module.

## Source provenance and executable registration

The original `server.dll` is SHA-256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`. Its PE32 image base is `0x22000000`. Phase 88 verified these five handler *names* in original DLL datamap registries; this phase additionally verifies the datamap entry's **actual function pointer** and the following instructions in the pointed-to function:

| Name | Registry external-name pointer slot | Actual handler address | Observed effect |
|---|---|---|---|
| LookOn | `0x22535520` | `0x2235DDC0` | Clears bit `0x00010000` in 32-bit field at object offset `+0xB4` if set |
| LookOff | `0x225354EC` | `0x2235F260` | Sets bit `0x00010000` in same field if unset |
| StartPatrolling | `0x22535554` | `0x22357E30` | Writes byte `1` at object offset `+0xC65` |
| StopPatrolling | `0x22535588` | `0x22357E40` | Writes byte `0` at `+0xC65` |
| ThrowGrenades | `0x225355F0` | `0x22357EA0` | Tests DVariant type `6`; sets grenade-aggression byte at `+0xC6C` from Boolean at variant `+0x8` or clears it for non-Boolean input; **both paths write zero at `+0xC70`** |

**Extra conditions observed in LookOn/LookOff:** Both skip when the requested flag state is already present. When changing the bit, they check a separate byte at `+0x1F0`, then either write `1` at `+0x200` or, if a pointer at `+0x16C` exists, OR bit `1` into that pointed-to field. The *meaning and lifetime* of those extra fields are not established here. No Source AI scheduling callbacks are inferred from them.

The `ThrowGrenades` instruction path for a wrong variant type clears the grenade-aggression byte, **not** a failure return or attempt to parse arbitrary text. The method is a typed Source entity input handler, separate from Hammer's string argument parsing.

## What changed in the independent portable component

The existing Phase 88 `GruntTacticalControls` now:

- Updates its *effective runtime spawnflag word* when LookOff/LookOn is processed, preserving all unrelated flags, rather than changing only a separate preference field. New snapshots expose `effective_spawnflags`.
- Adds `apply_typed_grenade_input(grunt, variant_type, boolean_value)`; type `6` accepts the Boolean while any other variant clears aggressive-grenade state. This is **a simulation of the verified typed handler input**, not the real Source `variant_t`.
- Snapshots expose `grenade_auxiliary_cleared`, indicating the model processed a typed grenade input and mirrored the handler's auxiliary-zero side effect. No claim is made about the original purpose of the `+0xC70` word.
- Preserves the strict user-facing `apply_input("ThrowGrenades", "true"/"false")` parser. Malformed strings are still rejected at the *host-neutral parser* rather than silently treated as an engine variant of the wrong type. This distinction is deliberate.
- Continues tracking NPC patrol requests, grenade inventory and equipment with generation-safe entity references. No pathfinding or combat decision algorithm has been added.

## Reproducible independent verification

`scripts/verify_sin_grunt_handlers.py` accepts the separately held **exact pinned** original DLL:

    python3 scripts/verify_sin_grunt_handlers.py /private/server.dll
    python3 -m unittest discover -s research/tests -p 'test_*.py' -v
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
    cmake --build build --parallel
    ctest --test-dir build --output-on-failure

It checks five datamap input name/function pointers via the existing Phase 84 independent PE32 VA mapper and **18 short original instruction signatures**, including flag masks, patrol bytes, type comparisons, grenade flag/auxiliary writes and fallback. Failure on mismatch is explicit. It prints only metadata, not a DLL dump or copyrighted assets.

Eight new **synthetic self-authored PE fixtures** cover a positive fake registry/handler image and mutations to name, pointer, flag masks, typed-variant handling, patrol writes and grenade-auxiliary resetting. A new native CTest, `sin_grunt_handler_execution_tests`, checks mutation of the actual tracked spawn-flag word, typed variant invalid-value fallback, state preservation and entity generation recycling. GitHub CI runs the full C++/Python suite and ARM64/Android build. The owner's original DLL remains private.

## Remaining gameplay work and proof boundaries

- The actual original `ChooseEnemy` / `ShouldChooseNewEnemy`, squad sight transfer, tactical scheduling, grenade targeting and the reason for the `+0xC70` zero are **not** recovered by these isolated handler instructions.
- The existing grunt model still uses abstract host-supplied idle/alert/combat state. It does not spawn real NPCs, navigate maps, or perform sensing.
- Real SourceVR engine/client-server module integration (#12), BSP/campaign execution (#20) and VR hardware testing remain out of scope. As requested, **no Quest headset testing is performed**.
- No original SiN SDK, DLL, APK, model, map, engine assets or copied source are committed to GitHub.

**Conclusion:** Phase 89 upgrades five grunt controls from *registered-name evidence* to actual, hash-pinned original binary *execution-path evidence*, and adapts portable state behavior to it. It does not yet establish original combat AI fidelity.
