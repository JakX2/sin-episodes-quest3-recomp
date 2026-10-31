> **Archival research record (phase53).** Transcribed unchanged below from `SiN_Phase53_Reload_Activities_Ammo_Transfer_Reverse_Engineering.zip` (`README.md`). Its test and verification claims describe work performed in the original phase, NOT new tests run by this repository. Original disassembly excerpt files are not republished here; references to `evidence/`, `disassembly/`, or local scripts may point to material only in the original phase archive. All function addresses are build-specific. No original DLL or game asset is included.

---

# SiN Episodes: Emergence | Reverse Engineering Phase 53
## Weapon reload activities, state transitions and ammunition transfer

**Scope:** compiled x86 Windows `server.dll` and `client.dll` supplied by the user in `binsin.zip`, plus file provenance check of `binhl2.zip`. The addresses in this report are **virtual addresses valid only for the SHA-256-verified DLL build**. This is static disassembly, not recovered original C++ source, an engine plug-in, or a functioning VR port. The actual gun models and script data are not in the supplied ZIPs. Neither original DLL nor any Valve/Ritual binary is redistributed in this package.

## Findings with anchored evidence

### 1. Activity `ACT_VM_RELOAD` is 166 (`0xA6`)

Original activity registration at `server.dll` VA `0x220E78B6` pushes integer `0xA6` alongside the string `ACT_VM_RELOAD`, at VA `0x223D8154`. Bounded excerpt: `evidence/01_reload_activity_registration.asm`. The runtime also refers by name to `ACT_VM_RELOAD_START` (VA `0x2245AF88`) and `ACT_VM_RELOAD_END` (VA `0x2245AF74`); their numerical identifiers, when dynamically resolved, are *not* established by the registration excerpt.

### 2. Shared staged reload choice (`0x222AF3D0`, weapon virtual slot 224)

On a non-3 reload stage, the selector writes `weapon+0x4A8 = 2` and looks up `ACT_VM_RELOAD_START`. For stage 3 it looks up `ACT_VM_RELOAD_END`. A failed lookup (`-1`) writes `weapon+0x4A8 = 1`, choosing generic activity `ACT_VM_RELOAD` (`0xA6`) instead. It sends the resulting activity, clip-capacity data and ammo-type arguments to `0x222AF240` (the shared reload start/request helper); when successful it then performs another helper call `0x222AAFE0` with argument 4. The byte-level `CALL` targets have been verified. `evidence/02_shared_reload_stage_selection.asm`.

**Caveat:** The dynamic activity string lookup does not prove each model provides a corresponding successful sequence. Exact stage semantics, conditional fallbacks and sequence durations must be validated against the original assets or runtime.

### 3. Reload start, attack timing and active state (`0x222AF240`)

The shared helper validates the owner/player path and the player's available ammo for the primary ammo type. It also checks clip capacity versus clip count for enabled clip paths, and tests secondary ammo rules if configured. On a successful path it invokes weapon virtual slot 197 to request the chosen viewmodel activity, asks for its sequence duration through `0x220EEDE0`, and updates player and weapon attack timing. It marks `weapon+0x4AC = 1` on a successful path, returning success. A failure path returns false without reaching that final store. `evidence/03_shared_reload_start_eligibility.asm` and `04_reload_start_animation_and_timing.asm`.

**Caveat:** We have not recovered a fixed numerical reload duration. The implementation uses the model sequence duration, not a recovered universal constant.

### 4. Per-frame reload processing (`0x222AF450`, weapon virtual slot 221)

The shared routine has branches depending on `weapon+0x508`, and checks whether the reload-active flag at `+0x4AC` is set. In one branch, once the attack-time condition is satisfied, it checks buttons (`player+0x938`, mask `0x801`) and a nonempty primary clip `+0x4EC` to allow an early exit from the processing branch. It also contains an insertion branch: compare the primary clip to its capacity, update one round, call `0x222AA670` to decrement player reserve ammo by one, and invoke weapon virtual slot `0x380` (slot 224). Another branch transitions stage `+0x4A8` from 2 to 3 and dynamically requests the reload-end activity. `evidence/05_reload_state_mode_active.asm`, `06_shell_insert_and_abort.asm`, `07_reload_end_phase.asm`.

**Important:** This is evidence of support for a *one-round-at-a-time* reload-processing pathway. It is **not proof** of which individual gun has mode `+0x508` enabled in gameplay. We would need a trustworthy weapon-script or runtime-path trace to assign it to the Scattergun definitively.

### 5. Ammo transfer routine (`0x222AF660`, weapon virtual slot 222)

The shared routine resolves the owning player, then independently checks weapon vtable offsets `0x438` and `0x43C` for primary and secondary clip-based ammo eligibility. It computes missing capacity against clip counts at `+0x4EC` (primary) and `+0x4F0` (secondary), limits transferred quantity by the player's reserve ammo using `0x222AA770`, writes the new clip counts and deducts corresponding reserve ammo with `0x222AA670`. In a conditional end path involving `+0x508`, it clears reload-active `+0x4AC`. `evidence/08_ammo_transfer_primary.asm` and `09_ammo_transfer_secondary.asm`.

### 6. Reload-state/timing cleanup (`0x222AF810`, weapon virtual slot 223)

This routine invokes virtual offset `0x3C8` with arguments 6, 7, and 8, clears `+0x4AC`, normalizes attack-time fields `+0x49C`/`+0x4A0`, and conditionally zeros stage `+0x4A8`. The precise semantics of those three vtable calls and every conditional branch are not fully recovered. `evidence/10_reload_state_reset.asm`.

### 7. Rifle-specific staged reload wrapper (`0x22314000`, weapon slot 224)

Magnum and Scattergun inherit shared slot 224 (`0x222AF3D0`). **Assault rifle overrides slot 224** at `0x22314000`, saves prior `weapon+0x4A0`, calls shared `0x222AF3D0`, and conditionally restores the prior attack deadline after success. Its complete reason for restoring timing is not established. `evidence/11_rifle_reload_override.asm`.

## Exact RTTI-verified virtual slots

| Class | Slot 221 state | Slot 222 transfer | Slot 223 cleanup | Slot 224 activity |
|---|---|---|---|---|
| CBaseHLCombatWeapon | `0x222AF450` | `0x222AF660` | `0x222AF810` | `0x222AF3D0` |
| CWeaponMagnum | `0x222AF450` | `0x222AF660` | `0x222AF810` | `0x222AF3D0` |
| CWeaponAssaultRifle | `0x222AF450` | `0x222AF660` | `0x222AF810` | **`0x22314000`** |
| CWeaponScatterGun | `0x222AF450` | `0x222AF660` | `0x222AF810` | `0x222AF3D0` |

## Quest 3 compatibility guidance

- Separate reload request, activity/sequence playback, timed lockout and clip/reserve updates; the compiled code has separate mechanisms for them.
- Preserve a stage-driven option for START/END activities, with safe fallback to `ACT_VM_RELOAD` when an activity cannot be resolved.
- Never deduct reserve ammunition merely because the player animated a virtual hand near the gun. Reproduce the original timing and transfer conditions, then attach VR interactions to the compatible gameplay events.
- Keep secondary ammunition independent of primary ammunition. This routine's support for a secondary clip does **not** establish the physical secondary-fire behavior of each SiN weapon.
- The assault rifle's slot-224 wrapper must not be silently replaced by the base implementation: its attack-time handling is observably different.
- Use the user's gun models and scripts later to measure actual reload sequence lengths, weapon-specific ammo capacities and whether one-round reload mode is in use.

## Evidence and reproducibility

Run in the package directory:

```bash
./REPRODUCE.sh /path/to/binsin.zip /path/to/binhl2.zip
```

The HL2 archive is optional. Prerequisites: Python 3 (stdlib), C++17 compiler, `objdump` only if you want to recreate bounded disassembly files independently. `phase53_verify.py` checks both archive hashes, 35 frozen machine-code signatures, four RTTI-identified weapon vtables, three literal activity strings, and 12 anchored direct-call targets. `src/sin_reload_reference.hpp` is an intentionally narrow model; `tests/test_phase53.cpp` performs synthetic invariant/regression tests (not live game tests). `SHA256_MANIFEST.json` protects files distributed in this research package.

## Next recommended work

**Phase 54:** Resolve which input and weapon-specific constructors/scripts select `+0x508` mode; trace how slot 221 is scheduled in frame update and whether holster/ADS cancel the flag; capture model sequence availability and precise ammo loading event from shipped model files when those assets are provided. The priority is accuracy, not a large assertion count.
