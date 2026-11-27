# Phase 1628: Consolidate duplicate Minor +0x398 recovery and index original identities

## Proven duplication

Phase 1613 and Phase 1625 both describe original server.dll SHA-256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`, Recombinant Minor vtable +0x398 at VA `0x2237e8c0`, 56 bytes, method SHA-256 `e08a5b91b9f7e3411976303123a218fb28e6d3ff2df0690a1e8df6525b714ff7`. This is one original method, not two. The two versions diverged on positive infinity: the Phase 1613 native code required isfinite(damage), but Phase 1625 uses the documented original ordered greater-than comparison. The original FCOMP/AH mask rejects equality, below-threshold and unordered results; positive infinity is ordered greater than the finite threshold. This consolidation follows that documented comparison without claiming a fresh original-byte audit in this phase.

## Implementation

The canonical implementation is in native_server_recombinant_minor_damage_gate.cpp. The original damage-gate export and the later metric-gate export retain their distinct public version-1 structs and function signatures; the latter maps fields explicitly into and out of the canonical implementation. No struct aliasing or const-discarding cast is used. The duplicate body and linker export are removed from native_server_recombinant_minor.cpp. Existing loaded-DLL regressions remain, and the damage-gate regression now compares both ABI outcomes, callback counts and result fields for finite values, NaN, infinities, vetoes, invalid extents, wrong actor type and failed owner services.

The semantic difference for positive infinity in the older ABI is intentionally corrected. All other documented outcomes and both public ABI layouts are preserved. No new original function is claimed.

## Recovery identity ledger

The existing 403-entry catalog omitted this method and several recently recovered entries. Four distinct original addresses are added: Minor +0x398 and +0x1d4, and ServerGameEnts001 +0x08 and +0x18. This produces 407 ledger entries, not 407 newly recovered functions or a coverage percentage. The older evidence remains the primary citation for +0x398; the Phase 1625 research is additional evidence.

Run from the repository root:
```sh
python research/tools/check_original_recovery_ledger.py --self-test
python research/tools/check_original_recovery_ledger.py --lookup server 0x2237E8C0
```

The guard validates DLL hashes, normalized VA keys, region hashes and extents; duplicate keys fail even when their hashes or lengths differ. It is included in the original PE32 authentication CI. A lookup miss only means absent from the incomplete ledger; prior phase notes, existing implementation files, and original evidence still need checking before any new recovery is called novel. The script does not authenticate original machine bytes by itself.

No original executable bytes committed; no generic Source physics, surface/material, prop/model loading or rendering changes. Native tests are synthetic host/DLL tests, not original-engine gameplay. The retail ServerGameEnts001 factory remains disabled.
