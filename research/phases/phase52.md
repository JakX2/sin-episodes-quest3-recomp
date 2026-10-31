> **Archival research record (phase52).** Transcribed unchanged below from `SiN_Phase52_Weapon_Deployment_Holster_Reverse_Engineering.zip` (`README.md`). Its test and verification claims describe work performed in the original phase, NOT new tests run by this repository. Original disassembly excerpt files are not republished here; references to `evidence/`, `disassembly/`, or local scripts may point to material only in the original phase archive. All function addresses are build-specific. No original DLL or game asset is included.

---

# SiN Episodes: Emergence | Phase 52
## Original weapon draw, deploy, holster and weapon-specific entry paths

**Source:** original x86 Windows `server.dll` / `client.dll` from the user-provided `binsin.zip`. This is static disassembly and a clean-room reference model, not Ritual's source code or a functioning Quest 3 port. The `binhl2.zip` archive is used for file-provenance SHA-256 checks. Neither proprietary archive is redistributed.

### Verified results

**Draw and holster activity IDs.** Literal Source activity registration in `server.dll` maps `0x9B` to `ACT_VM_DRAW` and `0x9C` to `ACT_VM_HOLSTER`. This build also contains the literal name `ACT_VM_DRAW2`, resolved dynamically at draw time rather than assigned a fixed ID by this analysis.

**Shared animation-based deployment mechanism.** The shared helper at `0x222AC300` invokes weapon virtual slot 256, slot 230 (draw activity selector), slot 255, and slot 254 to construct the arguments for slot **207**. Slot 207 is `0x222AEBD0` for all three firearms. On its eligible, player-valid path, it invokes weapon slot **199** (model/offset setup) **before** slot **197** (request animation), then derives action timing using the current sequence duration. It updates a player timing function and timing fields at weapon `+0x49C` and `+0x4A0`. Other guarded paths exist and are not equivalent to this one.

**Conditional alternate draw.** Shared slot **230** points to `0x222CB7F0`, which can return `ACT_VM_DRAW` (`0x9B`), or resolve `ACT_VM_DRAW2` by name and use that result under state, timing and random conditions. An unsuccessful or ineligible alternate path jumps to the `ACT_VM_DRAW` getter at `0x222AC340`. The actual alternate activity number, its occurrence rate in live gameplay and whether each weapon's models provide a matching sequence are **not established**.

**Zoom-aware holster.** Shared slot **210** points to `0x222CBDB0`, which invokes `0x222CBBC0` (zoom out) before `0x222AED80` (shared holster implementation). The latter requests `ACT_VM_HOLSTER` through virtual slot 197, checks the resulting activity and uses its sequence duration as applicable. After a nonzero base result the wrapper writes byte `+0x56C = 1` and stores the current time to `+0x574`. It does not update those two fields on its explicit failure branch. Note that zoom exit happens **before** the result is checked.

**Firearm-specific slot 209 prelude.** Magnum inherits shared slot 209, `0x222CC340`. Assault rifle override `0x2230AE50` sets `+0x584 = 0` and `+0x57C = 0` before jumping to shared code. Scattergun override `0x2231BB00` copies `+0x4EC` to `+0x57C` before jumping to shared code. Same numeric offset does **not** imply identical semantic meaning across different classes. Shared slot 209 is also able to request a lowered-state animation (`0xB9`) under a player flag, or fall through to the draw/deploy helper; exact player flag meaning is not identified here.

### Verified slot identity (all original RTTI-identified vtables)

| Slot | Base / Magnum | Assault rifle | Scattergun | Observed operation |
|---:|---|---|---|---|
| 197 | `0x222B07C0` | same | same | Activity request dispatcher |
| 199 | `0x222AD5E0` | same | same | Viewmodel model/origin setup |
| 207 | `0x222AEBD0` | same | same | Guarded deployment/activity setup |
| 209 | `0x222CC340` | `0x2230AE50` | `0x2231BB00` | Entry behavior before common draw/lowered logic |
| 210 | `0x222CBDB0` | same | same | Zoom-aware holster wrapper |
| 230 | `0x222CB7F0` | same | same | Draw activity selector |

### Simplified, evidence-oriented call graphs

```
Common / default deployment route:
weapon shared slot 209 0x222CC340 [conditional branches]
   --> 0x222AC300 [collect model/activity configuration]
      --> weapon virtual slot 207 0x222AEBD0
         --> guards: owner / player and other state
         --> slot 199 0x222AD5E0 [viewmodel model + origin]
         --> slot 197 0x222B07C0 [request activity]
         --> 0x220EEDE0 [sequence duration]
         --> 0x22040B10 [player timing] + weapon time fields

Weapon holster:
weapon virtual slot 210 0x222CBDB0
   --> 0x222CBBC0 [zoom exit]
   --> 0x222AED80 [shared holster]
       --> slot 197 request activity 0x9C
       --> activity/duration-related logic
   --> on successful base result: write +0x56C and +0x574
```

### Port integration implications

- Keep ADS state transitions, viewmodel draw/holster activities and controller-driven physical pose independent.
- Do not force `ACT_VM_DRAW` during every selection: the original draw selector has a dynamically resolved alternative.
- Holstering while zoomed exits zoom first. If holster subsequently fails, the zoom-exit request has already happened.
- Implement the rifle's deploy-related field resets separately from Scattergun's counter/state copy.
- Use actual model sequence durations when they become available. No fixed draw or holster seconds have been recovered in this phase.
- Avoid shrinking VR headset FOV to match flat-screen weapon zoom.

### Caveats / next research

The complete weapon selection timeline (which exact engine inventory path triggers slot 209 and which branches actually occur in specific campaign scenes) remains only partially recovered. The current archives contain DLLs, **not** the model assets and weapon scripts needed for per-gun numerical origin offsets, `.mdl` activities or exact ironsight geometry. C++ unit tests validate the documented narrow model with synthetic inputs; they do not execute the original code.

### How to verify

```bash
./REPRODUCE.sh /path/to/binsin.zip /path/to/binhl2.zip
```

Prerequisites: Python 3, `c++` with C++17, optionally `objdump` if recreating assembly excerpts. The HL2 argument is optional. The verifier enforces hashes, 27 fixed machine-code signatures, 4 RTTI-identified weapon vtables, viewmodel RTTI, literal activity strings and 10 anchored direct call/jump targets. `verification.json` records the checked evidence.

`src/sin_weapon_deploy_reference.hpp` is a deliberately narrow C++17 behavioral reference with explicit unresolved inputs. `tests/test_phase52.cpp` checks branch ordering, alternate-draw lookup fallback, per-weapon slot209 prelude, deployment timing and zoom-before-holster behavior. The 48,013 assertions are synthetic reference tests, **not** tests against live gameplay.
