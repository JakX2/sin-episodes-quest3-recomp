# Phase 281-G: private, bounded virtual-method pseudocode from 2006 SiN DLLs

This research phase is stacked on Phase 280 PR #337. It adds a reviewable
technical step toward genuine independently reconstructed PE32 virtual
interfaces without claiming any recovered source method signatures.

## Original evidence and selection

The Phase 280 private original-DLL audit authenticated both 2006 DLLs by
whole-file SHA-256 and recovered 105 reviewed vtable pointer values across
five engine interfaces. Nine were exact starts in the old Ghidra atlas.
This phase selects **14** priority interface slots for a new, bounded
Ghidra 12.2 `DecompInterface` pass after Phase 280's function seeding:

| Module | Interface | Slots selected |
| --- | --- | --- |
| client.dll | VClient011 | 0, 1, 2, 3 |
| client.dll | GameClientExports001 | 0, 1 |
| server.dll | ServerGameDLL003 | 0, 1, 2, 3 |
| server.dll | ServerGameClients003 | 0, 1 |
| server.dll | ServerGameEnts001 | 0, 1 |

These are **original address candidates**, not established C++
signatures, method behavior, calling conventions or complete vtable layouts.

## Implementation

`ExportTargetedVtableMethodPseudocode.java` reads the previously verified
slot/address CSVs, requires an explicit owner-private opt-in, validates unique
slot identities and bounded PE32 addresses, and refuses unexpected interface
entries in the selected batch. It decompiles only functions whose *exact*
start Ghidra can identify, and applies a 25-second per-function decompiler
timeout, a 500,000-byte function-body limit, a 256 KiB C-text limit and a
maximum of eight functions per DLL.

The resulting candidate C files are created under the existing ephemeral
`RUNNER_TEMP/sin-ghidra-private/<module>_PRIVATE_VTABLE_METHOD_DECOMPILATION`
directory. This is outside the checkout and the metadata upload path.
The owner job always removes private pseudocode, original binaries, and
temporary metadata during cleanup.

The report `private_vtable_method_decomp_summary.json` is created only by
`summarize_private_vtable_method_decomp.py` after checking that all 14
selected addresses match the reviewed seed CSV and that outcomes and numeric
extents have an exact allowed schema. It includes only address, slot,
status, and inferred size; **never proprietary C text or instructions**.

Possible statuses: decompiled_candidate, missing_inferred_function,
not_executable, empty_decompilation, decompilation_failed,
oversized_decompilation, excessive_inferred_function_extent.

## CI

The synthetic Ghidra job imports a harmless C ELF, deletes a selected
factory function and a separate selected 'virtual' function boundary,
recreates both with the two seeders, and attempts actual C-like Ghidra
decompilation for both. This is separate from original retail decompilation.
Python synthetic regressions test the private status sanitizer, provenance
and output safety, while mocked shell-runner tests check that optional
vtable scripts are never invoked without the private output opt-in.

The owner-original augmented Ghidra job remains restricted to `main`
and cannot run this stage from the draft PR. After integrating the
research stack, it will authenticate the original DLLs before authorizing
private decompilation. No original pseudocode is uploaded to GitHub, so
the private C files cannot be reviewed through the public artifact.
An owner-controlled local private Ghidra run is needed for human review.

## Evidence boundary and next step

A decompiled_candidate proves only that Ghidra produced C-like text for
an inferred function; **zero original method signatures or method bodies**
are newly SHA-verified, and zero game interfaces have been independently
engine-load tested. The next requirement is private human review of selected
method bodies, ABI signature verification and an independent Windows x86
client/server interface implementation. Android and Quest 3 remain later.
