> **Archival research record (phase55-automation).** Transcribed unchanged below from `SiN_Phase55_Reload_Mode_Audit_Weapon_VR_Bridge.zip` (`README.md`). Its test and verification claims describe work performed in the original phase, NOT new tests run by this repository. Original disassembly excerpt files are not republished here; references to `evidence/`, `disassembly/`, or local scripts may point to material only in the original phase archive. All function addresses are build-specific. No original DLL or game asset is included.

---

# SiN Episodes: Emergence → Quest 3 | Phase 55
## Reload mode activation audit and a tested weapon/VR adapter contract

**Status:** Partial binary investigation, **completed reproducible clean-room integration slice**. The current run did not have the original SiN or HL2 DLL archives. It reused and audited the preserved disassembly from the Phase 52–54 packages. This is **not** a compiled game, Source SDK plugin, Android binary, or Quest build.

## Findings from preserved prior binary evidence

### A. `weapon+0x508` activation remains unresolved

The shared constructor at `0x222AE408` sets byte `+0x508` to zero. Magnum's constructor at `0x223168C0` explicitly does the same. The assault rifle constructor at `0x22313C10` calls `0x2230B350`, which calls the shared constructor. Scattergun's constructor at `0x2231BA90` also calls the shared constructor. These facts were already established in Phase 54 and are not presented as new discoveries.

A bounded scan of the **preserved excerpts** found four unique direct `+0x508` references: two constructor writes that store zero, and two reads. This is not a full-DLL result; see `preserved_mode508_refs.json`.

This package adds an **optional direct-memory-reference scanner** (`tools/scan_mode508.py`) for the full original `.text` section, with SHA-256 validation and read/write classification. The scan **was not run on the original DLL** this time because the original ZIP was unavailable. Therefore no claim is made that runtime code never sets `+0x508`, or that any particular firearm enables it.

### B. Shared reload entry has a primary-reserve pre-gate

In the preserved excerpt for `0x222AF240`, at `0x222AF280` the function loads the **primary ammo type** from `weapon+0x4E4`, calls reserve-ammo lookup `0x222AA770` at `0x222AF289`, and branches into the rest of reload eligibility only when the returned value is **positive** (`0x222AF290`). Only later does it test primary clip eligibility (virtual offset `+0x438`) and secondary clip eligibility (`+0x43C`).

**Precise scope:** this is a property of this shared reload-start entry path. It does not mean secondary ammunition cannot be reloaded in every possible game path. It also does not establish which ammo types the three named firearms actually configure.

### C. Bulk ammo transfer handles both clips separately

The preserved `0x222AF660` shared slot-222 disassembly confirms two independent branches after owner resolution:

| Channel | Clip | Ammo type | Capacity query | Eligibility query | Reserve lookup | Reserve deduction |
|---|---|---|---|---|---|---|
| Primary | `+0x4EC` | `+0x4E4` | vtable `+0x404` | vtable `+0x438` | `0x222AA770` | `0x222AA670` |
| Secondary | `+0x4F0` | `+0x4E8` | vtable `+0x408` | vtable `+0x43C` | `0x222AA770` | `0x222AA670` |

The basic transfer amount is `min(capacity - loaded, reserve)` under normal nonnegative game-state assumptions. A positive amount updates the clip, and reserve deduction is issued. The end of this routine checks `+0x508` and conditionally clears `+0x4AC`. The exact game-level event that invokes slot 222 for each gun remains to be traced.

All three firearm vtables were reported in Phase 53 to share slot 222. **This does not prove** that all three have two usable clips, or their capacities, reserve values, or timing.

### D. One-round reload is a supported conditional path, not an assigned Scattergun feature

The shared frame routine reads `+0x508`. In its enabled branch, after owner, reload-active and deadline guards, the attack mask `0x801` and a positive primary clip count can terminate the reload without another insertion. Otherwise a one-round insertion path deducts one primary reserve round and requests the next stage through virtual slot 224. We preserve this option in code but **default it off for Magnum, assault rifle and Scattergun**.

### E. Weapon switching and aiming remain distinct

The holster wrapper exits zoom before the shared holster routine. Active reload triggers cleanup before stage reset and holster animation. ADS eligibility tests reload-active. None of this establishes an exact physical VR gesture or a model-specific animation time.

## New deliverables

- `src/sin_weapon_runtime.hpp`: C++17 engine-independent reference for shared reload-start gating, staged activity fallback, clip/reserve transfer, conditional one-round processing, rifle timing hook, ADS gate and holster order.
- `src/sin_vr_bridge_contract.hpp`: a compilable interface for a future SourceVR host. A VR reload gesture **requests** gameplay reload; only an authoritative event commits bulk ammo. Controller motion does not directly change clip counts.
- `tests/test_phase55.cpp`: exhaustive small-state ammo-conservation tests plus targeted timing, interruption, holster, ADS and rifle tests.
- `tests/test_bridge.cpp`: fake-host test of the event bridge.
- `tools/scan_mode508.py`: optional full original-binary direct-reference scan for `+0x508` (requires original archive; not run on this pass).
- `tools/scan_preserved_refs.py` and `preserved_mode508_refs.json`: bounded enumeration of the four direct references visible in retained excerpts.
- `tests/test_scanner.py` and `tests/test_scanner_integration.py`: scanner fixture and real GNU assembler/objdump tests.
- `tools/audit_preserved_evidence.py`: checks 32 instruction anchors in 11 preserved excerpt files; outputs `evidence_audit.json`.
- `tools/phase53_verify.py` and `tools/phase54_verify.py`: previous original-binary verifiers included for convenience, usable if the original ZIP is available.
- `tools/phase55_binary_verify.py`: 14 additional fixed instruction signatures for the shared reload-start gate and primary/secondary transfer; only runnable with the original ZIP.
- `EVIDENCE_MATRIX.json`: per-claim evidence scope and unresolved questions.
- `SHA256_MANIFEST.json` and `verify_package.py`: ZIP contents integrity.

## How to reproduce

On a Linux host with Python 3, GNU C++17 compiler, GNU assembler and objdump:

```bash
./REPRODUCE.sh
```

To additionally verify against the **original user-owned** SiN ZIP and scan the actual DLL for direct `+0x508` references:

```bash
./REPRODUCE.sh /path/to/binsin.zip
```

`REPRODUCE.sh` puts temporary build outputs and scan JSON in a temporary directory, not inside the immutable package. The optional full scan verifies the expected DLL SHA-256. A candidate writer from the scanner must still be inspected manually for function boundaries, register identity, reachability and actual weapon state.

## Known limitations of the reference code

The reference uses caller-provided clip capacities, eligibility flags and sequence durations. It is deliberately **not** a faithful reimplementation of every undocumented engine-side guard. In particular, the host decides when the authoritative bulk-transfer event occurs, whether to preserve the rifle's earlier secondary attack deadline, and how a completed staged reload is acknowledged. The one-round model conservatively prevents duplicate same-frame insertions by requiring a new positive sequence duration; this is an adapter safety rule, not a recovered original engine rule. The synthetic tests cannot establish original in-game timing or correct Quest behavior.

## Next priority: Phase 56

1. If the original `binsin.zip` is available, run the full `+0x508` scan, then inspect any write candidates and script/config/save-restore paths. Trace exact call sites to slot 222 and model event scheduling.
2. Add the original weapon script and model sequence data to establish clip capacities, `ACT_VM_RELOAD_START/END` availability and actual transfer times.
3. Wire the tested `VrWeaponBridge` contract into an actual SourceVR weapon host with authoritative game-state synchronization. Keep VR pose/ADS visual presentation independent of the original flat-screen FOV logic.
4. Once a playable weapon slice exists, connect the previously recovered SinTEK enemy AI, dynamic difficulty and vehicle state contracts through separate host adapters, testing each in a real build.
