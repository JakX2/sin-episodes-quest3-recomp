> **Archival research record (phase54).** Transcribed unchanged below from `SiN_Phase54_Reload_Interruptions_Frame_Scheduling.zip` (`README.md`). Its test and verification claims describe work performed in the original phase, NOT new tests run by this repository. Original disassembly excerpt files are not republished here; references to `evidence/`, `disassembly/`, or local scripts may point to material only in the original phase archive. All function addresses are build-specific. No original DLL or game asset is included.

---

# SiN Episodes: Emergence | Reverse Engineering Phase 54
## Reload mode defaults, per-frame scheduling, holster cancellation and ADS gating

**Evidence scope.** Static x86 disassembly of the user-provided `binsin.zip` (`server.dll`, `client.dll`), authenticated by SHA-256. The user-provided `binhl2.zip` is checked for provenance, but does not determine Ritual-specific weapon behavior. Addresses are **virtual addresses for this exact 2006 SiN build**, not portable offsets. This is not recovered original C++ source or a working VR port. This ZIP contains no user game binaries.

## Main finding: all three named firearm constructor paths start with one-round reload mode disabled

The shared `CBaseHLCombatWeapon` constructor at `0x222AE390` performs:

```asm
222ae3ae: xor ebx,ebx
222ae408: mov BYTE PTR [esi+0x508],bl
```

The `CWeaponMagnum` constructor (`0x223168B0`) first calls that shared constructor, then explicitly writes zero from `AL` into `+0x508` at `0x223168C0`.

The `CWeaponAssaultRifle` constructor (`0x22313C10`) calls its select-fire base constructor at `0x2230B350`, which calls `0x222AE390`. The `CWeaponScatterGun` constructor (`0x2231BA90`) calls `0x222AE390` as well. These direct calls were verified from x86 relative machine code. The four RTTI-identified weapon vtables also share reload-processing slot 221.

**Interpretation:** the +`0x508` one-round reload branch recovered in Phase 53 is **supported by shared engine code but not established as active for any of these three weapons**. The constructors set it to zero. Our bounded scan of the analyzed code did not find an affirmative assignment in these constructors; that alone cannot exclude runtime state changes, map scripting or other indirect initialization. Don't hard-code the Scattergun as using this branch until the actual weapon assets or runtime state establish it.

See `evidence/01_base_and_magnum_constructors.asm`, `02_rifle_constructor_chain.asm`, `03_scattergun_constructor.asm`.

## Frame scheduling is recovered

The shared weapon-frame routine `0x222AEED0` first resolves an owner/player and validates its player interface. It invokes the weapon's virtual offset `0x438` (a clip-eligibility check). When that returns true it invokes **vtable byte offset `0x374`, virtual slot 221** at instruction `0x222AEF6A`. The Magnum, rifle and Scattergun resolve that slot to **`0x222AF450`**, the reload state processor studied in Phase 53.

This is important for gameplay correctness: reload state evolves in the normal weapon-frame path, subject to owner and eligibility checks. The slot isn't an independent fixed-rate timer; animation deadline and time comparisons within the callee still determine when an insert or stage transition may occur.

See `evidence/04_weapon_frame_reload_dispatch.asm` and `05_reload_state_mode_entry.asm`.

## Enabled one-round mode supports an attack-button interruption

The *conditional* `+0x508 != 0` branch of `0x222AF450` tests `player+0x938` against **`0x801`** once the reload deadline condition is satisfied (`0x222AF4E7`). It then checks `clip+0x4EC > 0`. If both tests succeed, it branches to `0x222AF64E` and clears the active reload flag `+0x4AC` without passing through the single-round insertion instructions on that branch.

If that early-exit check does not fire and a round can be loaded, the routine increments the clip by one, calls `0x222AA670` with quantity one to deduct reserve ammunition, and invokes **vtable slot 224** to schedule the next reload stage. The exact semantics of `0x801` are consistent with two attack bits in Source-style input; the evidence here proves the bitmask check, not every gameplay context in which both bits can be pressed.

The code also has other completion branches for exhausted reserve ammo, full clips, missing owners and time gates. The clean-room test intentionally models only the narrow, guarded insertion/interrupt behavior.

See `evidence/06_one_round_attack_interrupt.asm`, `07_one_round_insert_and_next_stage.asm`.

## Holster cancels active reload, and zoom exit is first

The shared holster function `0x222AED80` checks the active reload flag at `weapon+0x4AC`. If true it calls **virtual slot 223** via `vtable + 0x37C` (`0x222AF810`, the Phase 53 cleanup function). It then stores reload stage zero to `+0x4A8` and requests activity `0x9C`, `ACT_VM_HOLSTER`.

SiN's zoom-aware holster wrapper at **`0x222CBDB0`** first calls zoom exit at `0x222CBBC0` and then calls the shared `0x222AED80` holster routine. If the shared holster request fails, the wrapper can return failure even though zoom exit already happened.

**Ordering matters for Quest VR:** invoke ADS exit and the original gameplay reload cancellation before starting a new weapon's animations; avoid transferring ammunition solely because a VR interaction began.

See `evidence/08_holster_reload_cleanup.asm`, `09_zoom_holster_wrapper.asm`.

## Reload is a zoom-eligibility gate; zoom-in/out do not directly refill or cancel reload

The shared zoom decision routine at `0x222CCD40` reads `weapon+0x4AC` before permitting zoom (`0x222CCD6B`), and can invoke zoom exit at `0x222CCD98` if eligibility becomes false. The earlier `0x222CBB60` and `0x222CBBC0` zoom transitions alter FOV/animation and the zoom flag `+0x578`; their bounded disassembly contains no call to reload cleanup at `0x222AF810`.

This supports an **ADS-blocked-while-reloading** interpretation, with temporary zoom interruption handled by the Phase 45 recovery state at `+0x579`, not unconditional cancellation of all reloads when zoom is toggled. The zoom decision has other eligibility checks; `+0x4AC` is not the only gate.

See `evidence/10_zoom_reload_eligibility.asm`, `11_zoom_enter_exit.asm`.

## Reproducibility

```bash
./REPRODUCE.sh /path/to/binsin.zip [/path/to/binhl2.zip]
```

- `phase54_verify.py`: checks exact user DLL SHA-256 digests, **33 frozen instruction signatures**, eight direct relative CALL targets, one conditional jump target, and the three shared reload virtual slots of four RTTI-identified classes.
- `src/sin_reload_lifecycle.hpp`: narrow clean-room C++17 event model, deliberately not a complete source-port replacement.
- `tests/test_phase54.cpp`: tests constructor defaults, interruption behavior, ammo conservation, missing-time/owner gates, ADS eligibility, frame dispatch and holster event ordering.
- `SHA256_MANIFEST.json` and `verify_package.py`: checks packaged research file integrity. Nothing here executes the original game.

## What remains unproved

1. **Runtime `+0x508` activation for each firearm.** All examined constructors initialize it to zero. Shipped weapon scripts, model activity metadata, save/restore writes, or a live runtime trace are necessary before assigning one-round reload mode to the Scattergun or another gun.
2. **Exact model animation events and clip-transfer times.** The DLL state machine uses sequence duration; no per-model assets are present here.
3. **Interactions with individual weapon overrides.** The rifle retains its distinct slot-224 timing wrapper from Phase 53, but it does not imply that `+0x508` becomes true.
4. **Full Quest controller behavior.** Reload hand gestures, physical shells/magazines, and animation-driven haptics are design choices to layer on top of this verified gameplay core.

**Recommended Phase 55:** inspect runtime data initialization, save/restore descriptors and any script-owned reload mode fields, then map primary/secondary reload eligibility and real ammo transfer call paths per gun. Ideally add the original weapon scripts and models when available.
