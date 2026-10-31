> **Archival research record (phase48).** Transcribed unchanged below from `SiN_Phase48_ADS_Animation_Timing_and_Viewmodel_Origin.zip` (`Phase48/README.md`). Its test and verification claims describe work performed in the original phase, NOT new tests run by this repository. Original disassembly excerpt files are not republished here; references to `evidence/`, `disassembly/`, or local scripts may point to material only in the original phase archive. All function addresses are build-specific. No original DLL or game asset is included.

---

# SiN Episodes: Emergence | Reverse engineering Phase 48

## ADS animation deadline and viewmodel origin offset

**Research scope:** The original `binsin.zip` and `binhl2.zip` supplied by the user, both containing x86 Windows `client.dll` and `server.dll`. This archive includes **research evidence, verifiers and clean-room C++ behavior models only**. It does not redistribute the original proprietary game binaries, contain reconstructed original source code, or provide a working Quest 3 port.

### Results verified in the original SiN binaries

**1. The three weapons share an animation-completion check**

- `CBaseHLCombatWeapon`, `CWeaponMagnum`, `CWeaponAssaultRifle`, and `CWeaponScatterGun` have RTTI-verified vtables with virtual slot **200** at `0x222AC8D0` and slot **201** at `0x222AE720`.
- Slot 200 compares global engine current time (from `0x22572190`, member `+0xC`) against a float deadline at `weapon+0x4A4`. The x87 code returns true when `currentTime > animationDeadline`, **strictly greater**, not when the two values are equal. The code should not be interpreted as the HMD or render-frame time.
- Zoom-enabled animation maintenance (`0x222CB960`, vtable slot **219**) recognizes `ACT_VM_ZOOMED_IDLE` (`0xC0`), `ACT_VM_ZOOMED_PRIMARYATTACK` (`0xC1`), and `ACT_VM_ZOOMED_SECONDARYATTACK` (`0xC2`) as its zoom-related activity group. It returns without changing the activity if the deadline has not passed; otherwise, it requests zoomed idle via slot **197**. An unrelated activity in the zoom branch can also cause the same request without the deadline check.
- This evidence improves upon the Phase 47 hypothesis: slot 200 is not an unknown animation callback anymore, but an **elapsed-animation-deadline comparison**.

**2. Activity request path is tied to a resolved sequence and deadline**

- Vtable slot 197 (`0x222B07C0`, forwarding to `0x222AFC10`) calls `0x220EEBD0` to map a requested activity to a sequence. If the mapping returns `-1`, the routine returns false before writing activity/sequence fields.
- On the normal success path it stores the requested activity into `weapon+0x4C4`, resolved sequence into `weapon+0x4C0`, and normally also the current activity into `weapon+0x4BC`. Another special branch can set `+0x4BC` to activity `2`, so those two fields must not be assumed universally identical.
- Later it calls `0x220EEDE0` to retrieve a sequence duration, adds engine current time, then invokes slot 201 to store the deadline at `+0x4A4`.
- This path does **not** guarantee a sequence exists for each of the three specific weapon models. That requires the model assets.

**3. SiN client has an explicit viewmodel-origin offset**

- RTTI class `C_BaseViewModel` has vtable at `0x24285DF4` and slot 200 at **`0x2409C970`**.
- Its receive-property registration (`0x2409CD35` onward) labels `m_viewmodelOriginOffset` as a **12-byte field** at client object offset **`0x7A8`** (3 floats).
- At `0x2409C970`, the client uses view-angle basis vectors and these three floats to change the position. The observed coordinate convention is `viewmodelOrigin = inputOrigin + right * offset.x + forward * offset.y + up * offset.z`. The routine includes additional camera/viewmodel conditions after forming the transformed point; this formula represents only the directly verified offset composition.
- A separately named, 12-byte server class field is registered at object offset **`0x4B0`**. This is not proof by itself of precisely how server-side replication sends this value.
- The two **specific provided HL2 DLLs** have no exact `m_viewmodelOriginOffset` string (nor the three `ACT_VM_ZOOMED_*` strings). This does not mean an HL2-derived implementation could not support it after source changes.

### Scope limits, in order of importance

The original weapon **model and script assets have not been supplied**. Consequently the following are **not** established: precise Magnum/rifle/Scattergun iron-sight alignment, per-model animation sequence numbers, the numerical `ZoomFOV` config values, and whether pressing ADS modifies `m_viewmodelOriginOffset` at runtime. The offset might be used for other viewmodel positioning. Also not tested: weapon animation playback in a running PC game, Quest 3 controllers, hand tracking, stereoscopic comfort or ARM64 compilation of the Source engine.

### Implementation guidance for the Quest 3 prototype

Maintain three independent concepts:

1. **ADS gameplay intent** and SiN server zoom state (`weapon+0x578`, from Phase 44–46).
2. **Weapon animation activity and deadline**. Keep zoomed attack playback from being overwritten until the deadline passes. Treat missing animations as a handled failure, not as evidence that a sequence exists.
3. **Physical aiming alignment**. Derive gun pose from controller/head pose and model calibration; do not drive the HMD FOV using flatscreen `ZoomFOV` and do not guess per-gun sight coordinates from the existence of `m_viewmodelOriginOffset`.

### Reproduction

```bash
./REPRODUCE.sh /path/to/binsin.zip /path/to/binhl2.zip
```

Python 3 standard library plus any C++17 compiler is required. The second HL2 archive is optional. The verifier checks the **exact original SHA-256 hashes**, **33 fixed instruction signatures**, **four weapon vtables** and **one RTTI-confirmed client viewmodel vtable**, relevant exact strings and optionally the specific HL2 comparison. `tests/test_phase48.cpp` runs 1,924 assertions against a *clean-room approximation of only the described paths*; it is not proof of the original source's complete semantics.

### Files

- `phase48_verify.py`: PE-virtual-address and RTTI verifier against the original supplied archives.
- `verification.json`: recorded output of verifier run.
- `evidence/*.asm`: bounded original disassembly excerpts for each cited code path.
- `src/sin_viewmodel_contract.hpp`: reference math and animation-guard logic.
- `tests/test_phase48.cpp`: isolated test corpus.
- `REPRODUCE.sh`: execute verifier plus C++ tests.
- `SHA256_MANIFEST.json`: hashes of archive members (excluding manifest itself).

### Proposed Phase 49

Inspect original SiN weapon scripts and viewmodel `.mdl`, `.vvd`, and `.vtx` assets, if provided, to map `ACT_VM_ZOOMED_*` to each gun's actual sequence and sight-to-controller transform. Otherwise, trace server-side updates to the `m_viewmodelOriginOffset` field and check for ADS-specific call paths before assigning it an aiming role.
