> **Archival research record (phase47).** Transcribed unchanged below from `SiN_Phase47_ADS_Weapon_Activity_Reverse_Engineering.zip` (`SiN_Phase47/README.md`). Its test and verification claims describe work performed in the original phase, NOT new tests run by this repository. Original disassembly excerpt files are not republished here; references to `evidence/`, `disassembly/`, or local scripts may point to material only in the original phase archive. All function addresses are build-specific. No original DLL or game asset is included.

---

# SiN Episodes: Emergence | Reverse Engineering Phase 47

## ADS weapon activity names, firearm-specific fire activities, and viewmodel animation maintenance

**Research scope:** original user-supplied 32-bit Windows SiN `server.dll`/`client.dll`, compared with the user's HL2 `server.dll`/`client.dll`. Original proprietary binaries **are not redistributed**. This package is a static binary analysis and clean-room behavior-reference package, **not** Ritual's source code, a game mod, or a Quest 3 build.

### Summary of verified findings

1. The SiN `server.dll` contains TWO independent code registration sites mapping each of these numeric IDs to actual symbolic activity names, shown in `verification.json`:

   | Decimal / hex | Name |
   |---|---|
   | 157 / `0x9D` | `ACT_VM_IDLE` |
   | 164 / `0xA4` | `ACT_VM_PRIMARYATTACK` |
   | 165 / `0xA5` | `ACT_VM_SECONDARYATTACK` |
   | 184 / `0xB8` | `ACT_VM_IDLE_TO_LOWERED` |
   | 185 / `0xB9` | `ACT_VM_IDLE_LOWERED` |
   | 186 / `0xBA` | `ACT_VM_LOWERED_TO_IDLE` |
   | 187 / `0xBB` | `ACT_VM_RECOIL1` |
   | 188 / `0xBC` | `ACT_VM_RECOIL2` |
   | 189 / `0xBD` | `ACT_VM_RECOIL3` |
   | 192 / `0xC0` | `ACT_VM_ZOOMED_IDLE` |
   | 193 / `0xC1` | `ACT_VM_ZOOMED_PRIMARYATTACK` |
   | 194 / `0xC2` | `ACT_VM_ZOOMED_SECONDARYATTACK` |

2. Phases 44/46 recovered calls from weapon zoom-enter `0x222CBB60` to `0xC0` (zoomed idle) and a *conditional* zoom-exit call at `0x222CBBC0` to `0x9D` (normal idle). Phase 47 resolves these formerly unnamed values using **original SiN registration code**.

3. Shared fire-activity selectors read `weapon+0x578` (the zoom flag):

   - Slot **228**, base function `0x222CB700`: `return zoom ? 0xC1 : 0xA4`.
   - Slot **229**, base function `0x222CB720`: `return zoom ? 0xC2 : 0xA5`.
   - The disassembly uses a branchless integer sequence `neg al ; sbb eax,eax ; and eax,0x1D ; add eax,0xA4/A5`. Thus **0x1D (29)** is the verified delta between unzoomed and zoomed primary/secondary attack IDs.

4. The **assault rifle overrides slot 228** at `0x22313880`. When zoomed it returns `ACT_VM_ZOOMED_PRIMARYATTACK` regardless of the field at `weapon+0x57C`. When not zoomed, it selects according to the field `weapon+0x57C`:

   | Field value | Activity |
   |---|---|
   | below 2 | `ACT_VM_PRIMARYATTACK` |
   | 2 | `ACT_VM_RECOIL1` |
   | 3 | `ACT_VM_RECOIL2` |
   | 4 or above | `ACT_VM_RECOIL3` |

   The underlying semantic source of `+0x57C` requires further investigation. We refer to it as an **integer firing-state/counter**, not as a confirmed number of bullets fired.

5. Four independently RTTI-verified weapon vtables show:

   | Class | Slot 219 activity maintenance | Slot 228 primary selector | Slot 229 secondary selector |
   |---|---|---|---|
   | `CBaseHLCombatWeapon` | `0x222CB960` | `0x222CB700` | `0x222CB720` |
   | `CWeaponMagnum` | `0x222CB960` | `0x222CB700` | `0x222CB720` |
   | `CWeaponAssaultRifle` | `0x222CB960` | **`0x22313880`** | `0x222CB720` |
   | `CWeaponScatterGun` | `0x222CB960` | `0x222CB700` | `0x222CB720` |

6. Activity-maintenance routine `0x222CB960` checks zoomed activity `+0x4BC` against `0xC0`, `0xC1`, and `0xC2`. For those three values it consults a virtual call at vtable byte offset `+0x320` before requesting zoomed idle. If that call returns **zero**, the routine skips replacing the activity with zoomed idle. This is consistent with preserving an in-progress zoomed animation; the exact name/meaning of the virtual callee is still unproven.

7. Neither of the **specific provided HL2** DLLs contains any of the three literal `ACT_VM_ZOOMED_*` activity names (verified exact NUL-terminated byte search). This supports SiN-specific activity registration; it does not prove what an HL2 branch can or cannot support after modification.

### Implications for Quest 3

- Model **gameplay zoom state**, **requested weapon viewmodel activity**, and **physical VR aiming pose** separately.
- Preserve the unzoomed assault rifle recoil animation selection, and let zoom override the rifle's primary-attack activity as the original does.
- The shared secondary activity selector must **not** be mistaken for proof that every firearm has the same functional secondary attack or button mapping.
- Preserve zoomed attack activity until the original animation-maintenance condition calls for zoomed idle; don't replace it every rendered frame.
- **Do not narrow the HMD view FOV** when reproducing flatscreen zoom.

### Not established by the supplied binaries

Actual gun-model animation sequence numbers, `ACT_VM_ZOOMED_*` sequence availability per model, viewmodel/bone transforms, sight-to-eye poses, and weapon-script `ZoomFOV` numbers. These generally require the shipped game `.mdl`/`.vpk`/GCF-extracted weapon assets and scripts, not merely gameplay DLLs. **No Quest headset test has occurred.**

### Reproduction

```bash
./REPRODUCE.sh /path/to/binsin.zip /path/to/binhl2.zip
```

Prerequisites: Python 3 standard library, a C++17 compiler, and the original two ZIPs. The HL2 comparison archive is optional; the SiN archive is required. `phase47_verify.py` checks exact original SHA-256s, 25 separate machine-code signatures, two code registration sites for 13 activities, four RTTI-verified vtable layouts, and optionally both HL2 digests and literal-name absence. The standalone C++17 tests run 2,293 assertions on a *modeled* selection algorithm. The model is not a substitute for an engine runtime test.

### Files

- `phase47_verify.py`: deterministic original-binary verifier, original activity registration and vtable audit.
- `verification.json`: verified evidence data and disassembly locations.
- `src/sin_ads_activity.hpp`: clean-room C++17 selectors and narrow maintenance helper.
- `tests/test_activity.cpp`: C++ activity selection test corpus.
- `evidence/*.asm`: bounded original SiN server disassembly excerpts from fixed addresses.
- `SHA256_MANIFEST.json`: SHA-256s of package contents (excluding itself).
