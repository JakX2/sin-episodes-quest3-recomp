> **Archival research record (phase50).** Transcribed unchanged below from `SiN_Phase50_Weapon_Script_to_Viewmodel_Transfer.zip` (`SiN_Phase50/README.md`). Its test and verification claims describe work performed in the original phase, NOT new tests run by this repository. Original disassembly excerpt files are not republished here; references to `evidence/`, `disassembly/`, or local scripts may point to material only in the original phase archive. All function addresses are build-specific. No original DLL or game asset is included.

---

# SiN Episodes: Emergence | Reverse Engineering Phase 50

## Weapon-script viewmodel origin: verified server-side transfer chain

**New finding:** the `viewmodelOriginOffset` triplet parsed from weapon scripts **is passed to the live server `CBaseViewModel`** on a guarded weapon virtual-method path. This resolves the main missing dataflow link from Phase 49. It is **not** proof that toggling ADS modifies the triplet, or evidence of exact iron-sight geometry.

**Sources:** the original, user-supplied `binsin.zip` (32-bit Windows SiN `server.dll` and `client.dll`) and optionally `binhl2.zip` for file-identity comparison. These ZIPs are **not** distributed here. All executable examples are clean-room reference implementations, not source recovered from Ritual's game.

### Evidence chain

| Stage | Original evidence | Interpretation |
|---|---|---|
| 1 | `server.dll` weapon routine `0x222AD5E0` | Validates weapon owner's entity handle and accesses the player's viewmodel by index; skips if guards fail |
| 2 | `0x222AD664` / `0x222AD66E` | Reads the weapon's script-info index at `weapon+0x51C`, calls `0x222B5BD0` to find its weapon-info object |
| 3 | `0x222AD676` | Adds **`0x6CC`** to the weapon-info pointer and pushes the address of its script origin offset |
| 4 | `0x222AD67E` | Calls the resolved viewmodel virtual method via vtable **byte offset `0x2E8`**, method slot **186** |
| 5 | `CBaseViewModel` RTTI/vtable at `0x2244F2B4` | Slot 186 points to `0x222412A0` |
| 6 | `0x222412A0` | Adds `0x4B0` to the viewmodel `this` pointer and jumps to `0x22240EA0` |
| 7 | `0x22240EA0` | Compares all three floats and, if changed, updates the live viewmodel vector with network dirty marking |
| 8 | Client `C_BaseViewModel` registration | Client receives `m_viewmodelOriginOffset` at entity `+0x7A8` (12 bytes) and applies it in client position code (`0x2409C970`) |

The original weapon-class vtables show **all three firearms use the same weapon virtual slot 199**, pointing to `0x222AD5E0`:

- `CWeaponMagnum`
- `CWeaponAssaultRifle`
- `CWeaponScatterGun`

`CBaseHLCombatWeapon` also uses this method. Two verified call sites at `0x222AD4DF` and `0x222AD56F` use weapon vtable slot **199** (`0x31C` byte offset). The exact gameplay lifecycle triggers for those functions have *not* been established.

### Extra finding: weapon configuration copy on the client

The client-side weapon-info copy function at `0x2413F5B0` copies `ZoomFOV` at info offset `+0x1BC`, and at `0x2413F72C` explicitly copies all 12 bytes of the info vector at `+0x6CC..+0x6D4`. This is copying weapon-*metadata*, not writing the live viewmodel entity. The latter has a distinct field at `+0x7A8`.

### A correction to Phase 49's open question

Phase 49 identified a change-aware **scalar-argument** setter at `0x22241420` with only one direct near-call found, from the viewmodel constructor. Phase 50 identified the different **vector-pointer** setter at `0x22240EA0` reached through a viewmodel virtual-function thunk. Both paths can update the same live viewmodel field. The earlier near-call finding was true but was not an exhaustive accounting of write paths.

### Boundaries of the evidence

- The path **does not prove** that `+zoom` or `-zoom` rewrites the viewmodel origin offset. The ADS enter/exit routines at `0x222CBB60`/`0x222CBBC0` explicitly deal with FOV transition and viewmodel activity. A link between ADS state and changes of the origin vector has not been demonstrated.
- No game assets were supplied: real Magnum, rifle or Scattergun `viewmodelOriginOffset` values, model animation sequences and sight axes remain unknown.
- There is no observed in-game execution; the finding is based on original binary disassembly, fixed-byte checks, RTTI/vtable validation and call-graph inspection.
- The network property registration and client consumption are established statically, not a live round-trip replication test.
- `binhl2.zip` is hash-checked for consistency, but **no equivalence between SiN and HL2 implementations is asserted here**.
- No Android, ARM64, or Quest 3 executable was built. HMD projection must not be narrowed by flatscreen `ZoomFOV`.

### Reproduction

```bash
./REPRODUCE.sh /path/to/binsin.zip /path/to/binhl2.zip
```

The second argument is optional. Python 3 stdlib and a C++17 compiler are required. The verifier checks **21 fixed instruction signatures**, original binary SHA-256, four RTTI-identified weapon vtables/slot 199, `CBaseViewModel` RTTI/slot 186, and selected near-call scans. The C++ test suite independently checks a **clean-room subset** of the guarded transfer behavior, with **28,035 assertions**. The original game's machine code is never executed.

### Contents

- `phase50_verify.py`: independent original-binary verifier, with fixed signatures
- `verification.json`: result recorded on the supplied original archives
- `src/sin_weapon_viewmodel_transfer.hpp`: C++17 clean-room viewmodel transfer adapter reference
- `tests/test_phase50.cpp`: reference-model regression tests
- `evidence/*.asm`: bounded disassembly excerpts showing the source chain
- `REPRODUCE.sh`: verification and test command
- `SHA256_MANIFEST.json`: generated content hashes for package files

**Suggested Phase 51:** Identify the lifecycle invocations of weapon virtual slot 199 and how the model/sequence-setting virtual slot 185 (`+0x2E4`) fits into equip/switch. Once model assets and weapon scripts are available, extract the actual per-gun origin offset values and sight poses.
