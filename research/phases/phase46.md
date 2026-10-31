> **Archival research record (phase46).** Transcribed unchanged below from `SiN_Phase46_Original_DLL_Verification_and_ADS_Input.zip` (`SiN_Phase46/README.md`). Its test and verification claims describe work performed in the original phase, NOT new tests run by this repository. Original disassembly excerpt files are not republished here; references to `evidence/`, `disassembly/`, or local scripts may point to material only in the original phase archive. All function addresses are build-specific. No original DLL or game asset is included.

---

# SiN Episodes: Emergence → Quest 3 | Phase 46

## Original DLLs recovered: press-edge zoom and shared weapon animation dispatch

**Scope:** original 2006 Windows x86 SiN Episodes `server.dll` and `client.dll`, plus a 2023 HL2 comparison pair that was inventoried but not used as evidence for SiN-specific findings. This is reverse-engineering research, **not** original Ritual source, an Android build, or a VR playtest. The distributed package contains **no proprietary DLLs**.

### Provenance

- SiN `server.dll`: **6,049,792 bytes**, SHA-256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`.
- SiN `client.dll`: **3,592,192 bytes**, SHA-256 `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.
- Both hashes exactly match the saved Phase 44 evidence. The original Phase 44 binary verifier passed **22 machine-code signature checks**, 4 shared getter vtables, plus the original ZoomFOV/FOV checks.
- This package's **new** verifier passes **19 fixed-address instruction checks**, all 4 weapon RTTIs/vtables at slots 197 and 272, two relative branch targets, and the original DLL digests.
- The uploaded HL2 archive also passed ZIP CRC checks. It is not needed to prove the following SiN facts.

### Finding A: the server receives a *newly pressed* zoom bit

At `server.dll:0x2223D240`, a button-state update computes:

```cpp
previous = current;             // +0x944 <- old +0x938
changed  = previous ^ incoming;
pressed  = changed & incoming;  // +0x93C
released = changed & ~incoming;// +0x940
current  = incoming;           // +0x938
```

At `server.dll:0x222CCDCB`, the zoom decision executes `testb $0x08, 0x93E(%esi)`. Since `0x93E` is the third byte of `+0x93C`, this corresponds to `pressed & 0x00080000`.

At `client.dll:0x2416269D`, the original command builder ORs the matching `0x00080000` bit into the player command whenever the zoom key is active.

**Conclusion:** for its normal eligible input path, the original server **toggles** zoom on a *new press*; it does **not** repeatedly toggle every frame the button is held. This resolves the Phase 45 uncertainty about held versus press-edge input. The button-state update establishes how the field is produced; the decision routine establishes how it is consumed.

**Important exception:** the existing Phase 45 automatic restoration branch can enter zoom when `weapon+0x579` is set and eligibility returns, without a newly pressed zoom bit. This is not a pure button-only state machine.

### Finding B: three firearms use the same animation dispatch function

The zoom-enter helper at `0x222CBB60` calls the weapon virtual method at vtable byte offset `+0x314` (x86 **slot 197**) with integer `0xC0`. The zoom-exit helper at `0x222CBBC0` calls that slot with `0x9D`, but only when its current activity field at `weapon+0x4BC` equals `0xC0`.

| Weapon | Primary vtable | Slot 197 | Slot 272 (ZoomFOV) |
|---|---|---|---|
| `CBaseHLCombatWeapon` | `0x22459194` | `0x222B07C0` | `0x222AC030` |
| `CWeaponMagnum` | `0x2247875C` | `0x222B07C0` | `0x222AC030` |
| `CWeaponAssaultRifle` | `0x224776AC` | `0x222B07C0` | `0x222AC030` |
| `CWeaponScatterGun` | `0x2247A1E4` | `0x222B07C0` | `0x222AC030` |

RTTI validates all four vtable identities. `0x222B07C0` is a direct jump thunk to `0x222AFC10`. The latter calls `0x220EEBD0` with the integer argument and branches if no sequence is found (`-1`); on success it writes a sequence-like selection at weapon `+0x4C0`, the requested activity argument at `+0x4C4`, and in one branch the current activity at `+0x4BC`, then performs additional virtual function calls. This is compelling evidence of **shared activity/sequence dispatch**, though the precise engine names of every callee have not been proven.

**Do not confuse shared activity dispatch with identical animations.** Different gun models can resolve the same numeric activity ID to different sequences, or have no matching sequence. Actual sight positions and the symbolic Source enum names for `0xC0` and `0x9D` are **still unverified**.

### VR implementation implications

Preserve two separate layers:

1. **Input/gameplay:** an ADS button press edge toggles SiN's zoom state when eligible. Honor interruption and automatic restoration logic; do not conflate it with independent secondary fire or grenades.
2. **Physical presentation:** aim the VR weapon using the tracked hands/controller and appropriate muzzle orientation. Do not apply the original flatscreen narrowing of headset FOV directly. Per-weapon sight alignments, gun models and numerical `ZoomFOV` values are still to be extracted from the game's script/model assets.

### Next research targets

- Identify symbolic activity values `0xC0`/`0x9D` using the matching Source-engine activity tables, without speculative naming.
- Locate per-weapon model sequence mappings, ADS poses and actual gun-script `ZoomFOV` entries in the original game assets.
- Trace network `m_bZooming`/`m_bZoomScope` assignments, and the ADS interaction with reload, sprint, and weapon switches.
- Separate evidence obtained from the original DLLs from assumptions made for Quest controls.

### Verification commands

```sh
python3 phase46_verify.py /path/to/binsin.zip --json verified.json
c++ -std=c++17 -O2 -Wall -Wextra -Werror -Isrc tests/test_button_edges.cpp -o /tmp/sin_phase46_button_test
/tmp/sin_phase46_button_test
c++ -std=c++17 -O2 -Wall -Wextra -Werror -Isrc tests/test_zoom_transition_model.cpp -o /tmp/sin_phase46_zoom_test
/tmp/sin_phase46_zoom_test
```

`evidence/*.asm` contains bounded `objdump` excerpts from the verified originals; `verified_original_binsin.json` is a reproducible machine-code evidence manifest. Source/test files are clean-room behavior models only.
