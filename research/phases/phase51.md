> **Archival research record (phase51).** Transcribed unchanged below from `SiN_Phase51_Weapon_Animation_Viewmodel_Lifecycle.zip` (`sin_phase51/README.md`). Its test and verification claims describe work performed in the original phase, NOT new tests run by this repository. Original disassembly excerpt files are not republished here; references to `evidence/`, `disassembly/`, or local scripts may point to material only in the original phase archive. All function addresses are build-specific. No original DLL or game asset is included.

---

# SiN Episodes: Emergence | Reverse Engineering Phase 51

## Weapon animation lifecycle: verified model and origin setup path

**Primary new result:** the shared weapon-animation request path can call the previously recovered `viewmodelOriginOffset` transfer routine **before sending the animation sequence**. This is not an ADS-only initialization: entering zoom happens to use the same shared sequence machinery used by other valid weapon animations.

**Source of evidence:** the original Windows x86 `server.dll` and `client.dll` in the user-provided `binsin.zip`; `binhl2.zip` is used for SHA-256 provenance checking only. The original game DLLs and game assets are **not** included in this research ZIP. All C++ is clean-room behavioral *reference* code, not Ritual's recovered C++ or a running port.

### Proven call chain, with guards

| Step | Original binary evidence | What it establishes |
|---|---|---|
| 1 | `server.dll` `0x222CBB60`, call at `0x222CBBA7` | Enter zoom requests `ACT_VM_ZOOMED_IDLE` (activity `0xC0`) via weapon vtable slot **197** (`+0x314`); this request occurs regardless of whether the nearby player/FOV guard succeeds, provided the weapon wasn't already zoomed |
| 2 | Slot 197 for all four checked classes → `0x222B07C0` | Trampoline jumps to `0x222AFC10` |
| 3 | `0x222AFC10`, call `0x220EEBD0` | Converts requested activity into a model sequence; a sequence result of **`-1` returns failure before sequence-sending and therefore before slot 199** |
| 4 | `0x222AFCC7` | On the accepted activity path, calls weapon slot **198** (`+0x318`) with the resulting sequence index |
| 5 | Slot 198 → `0x222AD470` | Checks nonnegative sequence, weapon owner and player validity, and viewmodel availability |
| 6 | `0x222AD4DF` | Calls weapon slot **199** (`+0x31C`) to configure live viewmodel |
| 7 | Slot 199 → `0x222AD5E0`, call `0x222AD655` | Uses weapon slot **254** (`+0x3F8`) to retrieve model-name data from weapon-info record |
| 8 | Slot 254 → `0x222ABED0` | Retrieves `WeaponInfo + 0xA6` as the model-name pointer |
| 9 | `0x222AD65E` → viewmodel slot **185** (`+0x2E4`) → `0x222412B0` | Associates weapon with viewmodel and changes model when different |
| 10 | `0x222AD664` through `0x222AD67E` | Looks up same weapon-info record, supplies `WeaponInfo + 0x6CC` triplet to viewmodel slot **186** (`+0x2E8`) |
| 11 | Viewmodel slot 186 → `0x222412A0` → `0x22240EA0` | Applies change-aware triplet to the viewmodel's networked offset field `+0x4B0` |
| 12 | Back in slot 198 at `0x222AD4EA` | Invokes viewmodel slot **184** (`+0x2E0`) to send the requested sequence |

**Zoom exit:** `0x222CBBC0` conditionally requests `ACT_VM_IDLE` (activity `0x9D`) via slot 197 when the current activity is `0xC0`. It can therefore enter the same chain under the same sequence and owner/viewmodel guards.

### Vtable checks

RTTI-identified vtables, all original `server.dll`:

| Class | Slot 197 | Slot 198 | Slot 199 | Slot 254 |
|---|---|---|---|---|
| `CBaseHLCombatWeapon` | `0x222B07C0` | `0x222AD470` | `0x222AD5E0` | `0x222ABED0` |
| `CWeaponMagnum` | same | same | same | same |
| `CWeaponAssaultRifle` | same | same | same | same |
| `CWeaponScatterGun` | same | same | same | same |

The RTTI-identified `CBaseViewModel` vtable at `0x2244F2B4` contains slots 184, 185 and 186 pointing to `0x22241330`, `0x222412B0`, and `0x222412A0`, respectively.

### What we learned about equip / weapon switching

The offset transfer is **not constrained to a dedicated equip function**. The same path is reachable during a valid animation request. The model-selection setter at `0x222412B0` compares the requested model against the stored current model and only invokes its entity-model-setting call if changed. The offset setter at `0x22240EA0` likewise compares the components and only dirties network state for a change.

Thus weapon switching can refresh both the model and the script offset through this shared route; **exactly when the game first calls it during an actual equip event, and whether other equip-specific hooks exist, remains unresolved**. The original animation path alone does not establish a reliable runtime frame-by-frame timeline.

### What this does **not** prove

- Pressing ADS does **not** necessarily modify the configured script offset. It reuses a path that can *apply the existing* per-weapon script offset. Changing to the zoomed animation is a distinct operation.
- This does not reveal numerical offsets, weapon models, attachment/sight transforms or animation content. Real `scripts/weapon_*.txt` and model assets are needed for those.
- A vtable slot's function address is supported by RTTI plus 4-byte pointer reads; class names assigned to function *purposes* are interpretations from their instruction behavior, not debug symbols.
- The original DLLs have not been executed in the headset, and we haven't built an ARM64 game module. The reference tests exercise only our clean-room model.
- A runtime viewmodel sequence and a VR controller's physical weapon transform should remain separately owned; do not replace the user's actual headset FOV with SiN's flatscreen zoom FOV.

### Porting advice

For a clean-room compatibility layer, use a common animation dispatcher for all three guns. On each accepted sequence request, ensure the viewmodel is bound to the current weapon, refresh model metadata if different, apply a change-aware default viewmodel offset, then send the sequence. Use physically tracked controllers to determine the VR muzzle/sight orientation, not the original 2D animation or HMD FOV reduction. Unknown model/asset alignment should be calibrated separately.

### Reproducibility

```bash
./REPRODUCE.sh /path/to/binsin.zip /path/to/binhl2.zip
```

The second archive is optional. The Python 3 verifier checks **29 fixed instruction signatures**, both original SiN SHA-256 values, four RTTI-identified weapon vtables across slots 197/198/199/254, viewmodel RTTI/vtable slots 184/185/186, selected direct call-site census, and optional original HL2 SHA-256. The independent C++17 test checks synthetic activity failure, model/origin transfer ordering, idempotence on repeated animation requests, switching between two synthetic weapons, changed offset values and guarded null/invalid paths. **40,030 reference assertions passed**.

`evidence/*.asm` has bounded x86 disassembly excerpts generated by `objdump -d -Mintel` against the original DLLs. These are illustrative; the fixed-byte verifier is the reproducible source-of-truth check. No proprietary binaries are bundled.

### Next research target, Phase 52

Recover the **actual weapon script values and relevant viewmodel animation activity sequences** from game assets, and identify the first animation/model setup calls during `Deploy` / weapon selection. Do not assert precise ADS sight alignment from current DLL evidence.
