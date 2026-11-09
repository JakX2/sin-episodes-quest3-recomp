# Phase 141: original automatic vehicle-view permission synchronization

## Verified original behavior

The preserved Phase 24 static-analysis report for the owner's 2006 SiN `server.dll` records three automatic vehicle-update paths in addition to the map-facing `SetView` input:

- `0x22331D7D` then `0x22331D8C`: set the player's selected view, then copy the selected 100-byte view record's raw permission byte at `+0x55`.
- `0x22331E1B` then `0x22331E24`: set the player's selected view, then explicitly write zero to the permission field.
- `0x22331F59` then `0x22331F71`: set the player's selected view, then copy the selected record's raw permission byte.

The original player fields are identified by the same Phase 24 report: selected view at player `+0x9B0`, permission byte at `+0x9B4`. The view-record permission is a byte, not a C++ bool.

## Implementation

`OriginalVehicleRuntime::apply_automatic_view_transition` implements only the confirmed setter pair and call order. The caller must choose `CopyViewRecordByte` or `ForceZero` based on its own verified engine-side transition branch. The code does not guess what event causes each branch, does not change the actual vehicle camera/view, and does not reuse x86 offsets as ARM64 layout.

Regression coverage checks both modes, exact raw-byte preservation, setter order, stale player rejection and invalid view indices. The existing vehicle-runtime CTest target owns these tests, so no parallel synthetic audit or extra test target was added.

## Evidence boundary

Evidence source: Library-held `phase24_report.md`, derived from static analysis of the owner's original 2006 `server.dll` / `client.dll`. The original DLL bytes were not materialized or freshly disassembled during this pass. No original code was executed.

## Remaining integration work

The real SourceVR vehicle update path must identify the triggering transition branches and call this helper with the appropriate mode. The native host still owns player references, view application, send-table replication, and client camera/render behavior. This is not a native Source game module, a playable campaign, or Quest 3 execution.
