> **Archival research record (phase55).** Transcribed unchanged below from `SiN_Phase55_Reload_Config_Clip_Accounting_Reverse_Engineering.zip` (`PHASE55_REPORT.md`). Its test and verification claims describe work performed in the original phase, NOT new tests run by this repository. Original disassembly excerpt files are not republished here; references to `evidence/`, `disassembly/`, or local scripts may point to material only in the original phase archive. All function addresses are build-specific. No original DLL or game asset is included.

---

# Phase 55 technical report | SiN Episodes: Emergence

## Provenance and scope

Examined the user-provided Windows x86 `binsin.zip` (`server.dll`, `client.dll`) and verified SHA-256 digests from Phases 53–54. Optionally reverify the user-supplied HL2 reference archive `binhl2.zip`. All virtual addresses below are **build-specific**. This work is static machine-code analysis and a reference test harness, not original C++ source recovered verbatim and not an operational Quest build. There are no original DLL bytes in the distribution.

## 1. Weapon script capacity configuration is explicit

The server weapon-information parser at `0x222B45B1–0x222B462D` requests integer values associated with the following embedded strings:

| Script string | String VA | Stored WeaponInfo offset | Default from parser |
|---|---|---|---|
| `clip_size` | `0x2245C774` | `+0x160` | `-1` |
| `clip2_size` | `0x2245C768` | `+0x164` | `-1` |
| `default_clip` | `0x2245C758` | `+0x168` | `clip_size` |
| `default_clip2` | `0x2245C748` | `+0x16C` | `clip2_size` |

The shared weapon capacity getters at `0x222ABF50` and `0x222ABF70` read `WeaponInfo+0x160` and `+0x164`, respectively. All the named firearm vtables share these getters in slots 257 and 258, and the corresponding clip-existence checks in slots 270/271. Consequently **shared code does not force shared ammunition capacities**. We have *not* recovered numerical values for the Magnum, assault rifle or Scattergun; those are data in the shipped weapon scripts, not the DLL text analyzed here.

Evidence: `01_weapon_script_clip_parser.asm`, `04_clip_capacity_accessors.asm`.

## 2. Script `MeleeWeapon` is not the one-round reload flag

The string at `0x2245C6D0` is exactly `MeleeWeapon`. The server parser calls an integer lookup with default zero, converts nonzero to a boolean, and stores it at WeaponInfo `+0x6C8` at `0x222B474F`. The client parser performs the corresponding store at `0x241E68DF` and has the same string at `0x242AA460`. The shared weapon getter at `0x222ABFF0` reads this byte.

The shared reload code reads `WeaponInfo+0x6C8` at `0x222AF8E6`. In reload cleanup (`0x222AF810`), an eligible nonempty primary clip resets the reload stage `weapon+0x4A8`; otherwise the code reads `MeleeWeapon`, and if it is nonzero it also resets the stage. The weapon attack path at `0x222AF094` consults the same flag. This is a **script-level melee flag, not an alias for `weapon+0x508`**.

Evidence: `02_weapon_script_mode_parser.asm`, `03_weapon_script_client_parser.asm`, `05_melee_weapon_accessor.asm`, `08_reload_melee_stage_reset.asm`.

## 3. The per-instance +0x508 flag still has no verified affirmative activation for the three guns

The shared weapon constructor `0x222AE390` zeros `weapon+0x508` at `0x222AE408`. The Magnum constructor's direct zero store at `0x223168C0`, and the assault-rifle/Scattergun constructor chains through the shared base, are confirmed. `CWeaponNPCPistol` also has a zero store at `0x22317D7E` and is **not** one of the target three firearms. The one-round branch reads `+0x508` at `0x222AF457`; the bulk-transfer branch checks it again at `0x222AF7F5`.

A bounded disassembly-reference inventory identified no `mov byte ptr [esi+0x508],1` or similarly obvious affirmative byte store in the examined firearm constructor paths. **Absence of an obvious write is not proof that the flag is never enabled.** Code may use a different object alias or register, copy saved state, load a map/script entity, or write through a pointer. `+0x508` is an offset and might refer to an unrelated field in other class layouts. Do not assume the Scattergun reloads per-shell on this evidence.

Evidence: `07_reload_one_round_interrupt.asm`, `09_base_constructor_mode_default.asm`, `10_magnum_constructor_mode_default.asm`, `11_rifle_and_scattergun_constructor.asm`, `12_scattergun_constructor.asm`, `14_npc_pistol_mode_default.asm`, and Phase 54 constructor-chain report.

## 4. Dual-clip transfer rules and shared vtables

The shared slot-222 routine `0x222AF660`:

- Resolves the owning player; checks clip-availability virtual slots.
- For primary: obtains max clip (slot 257, `0x404` vtable byte offset), subtracts `weapon+0x4EC`, compares with player reserve via `0x222AA770` using ammo index `weapon+0x4E4`, and updates the clip count. It deducts transferred reserve using helper `0x222AA670` at `0x222AF74F`.
- For secondary: repeats with max clip slot 258 (`0x408` byte offset), `weapon+0x4F0`, ammo index `weapon+0x4E8`, and deduction call at `0x222AF7F0`.
- In one-round mode, the separate branch can add one primary round at a time, with a check against held attack buttons `0x801` and nonempty primary clip; this is conditional on `weapon+0x508 != 0` and the existing state/time/owner guards.

Virtual table comparison:

| Virtual slot | Function | Magnum | Assault rifle | Scattergun |
|---:|---|---|---|---|
| 221 | Reload-state processing | shared `0x222AF450` | same | same |
| 222 | Ammo transfer | shared `0x222AF660` | same | same |
| 223 | Reload cleanup | shared `0x222AF810` | same | same |
| 224 | Staged reload selection | `0x222AF3D0` | `0x22314000` override | `0x222AF3D0` |
| 257/258 | Script clip capacity getters | shared `0x222ABF50/70` | same | same |
| 270/271 | Presence of each clip channel | shared `0x222ABFD0/C010` | same | same |

Evidence: `04_clip_capacity_accessors.asm`, `06_reload_primary_secondary_bulk.asm`, `07_reload_one_round_interrupt.asm`, `13_rifle_reload_override.asm`; virtual pointers validated from RTTI-identified vtables.

## Implications for VR reconstruction

Use script-driven per-weapon `clip_size` / `clip2_size` / default clips and separate primary/secondary ammo accounts. Represent `reloads_singly` as a separate instance flag whose activation remains **unknown**, not a guessed Scattergun constant. Preserve the rifle-specific staged timing override. Preserve ADS/holster interruption behavior from Phases 52–54. Place physical magazine/shell gestures above the original gameplay-side ammo transfer and timer rules, not in place of them.

## Reproduction and constraints

`phase55_verify.py` checks exact provided DLL hashes, 38 anchored machine-code signatures, six literal script strings, eight relative calls and 4 RTTI vtables across 7 slots plus the slot-224 exception. `src/sin_reload_phase55_reference.hpp` and `tests/test_phase55.cpp` validate conservative arithmetic and interruption invariants. Those tests are **synthetic**. They do not verify engine callbacks, command prediction, network messages, audio cues, model events, savegame compatibility or a Quest port.

### Still unresolved

1. Whether a runtime write actually enables `weapon+0x508` for a named firearm, including aliases or save/restore.
2. The actual values of `clip_size`, `clip2_size`, `default_clip`, `default_clip2`, animation lengths and reload events in shipped weapon assets.
3. Which particular player interactions invoke the dual-channel bulk transfer, or how a weapon with specialized attacks maps to each ammo channel.
4. Integrated game-module compilation, ARM64/SourceVR interface compatibility, and runtime playback tests.

Recommended Phase 56: continue the save/restore/datamap and runtime-writer investigation for `weapon+0x508`, or, if original weapon scripts are obtained, recover their exact clip/reload configuration and model-sequence timing. Avoid decompiling disconnected methods indefinitely: implement one compatible weapon lifecycle subsystem and test it against original runtime behavior.
