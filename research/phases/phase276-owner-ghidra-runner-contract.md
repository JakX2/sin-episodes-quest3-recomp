# Phase 276: owner-original Ghidra workflow contract repair

## Confirmed blocker

The Phase 275 owner-original GitHub Actions workflow invokes
`research/tools/run_ghidra_original.sh` with **five positional arguments**.
That script requires three arguments (`GHIDRA_HOME`, original PE32 staging
directory, output metadata directory) and accepts only one **optional fourth
mode**, either `metadata-only` or `emit-c`. It does not accept reviewed seed
and private pseudocode directories as positional arguments.

Therefore, after integrating the previous stacked research phases into
`main`, the original owner-hosted Ghidra job would fail before analyzing
the retail DLLs. Earlier pull-request CI passed because its synthetic Ghidra
smoke doesn't call the *owner-original* runner command.

## Corrected contract

The workflow now calls the runner with exactly three positional arguments.
It transmits the separately reviewed seed input, direct factory xref toggle
and private C output directory via its already-exported job environment:

- `SIN_GHIDRA_SEED_DIRECTORY`
- `SIN_GHIDRA_FACTORY_XREFS=1`
- `SIN_GHIDRA_TARGETED_PRIVATE_DIRECTORY`
- `SIN_PRIVATE_DECOMP_ACK=1`

Those settings are emitted **only after** the existing full-file
SHA-256 authentication of the original `client.dll` and `server.dll`
from the owner's archive. The C-like pseudocode remains exclusively under
runner-private temporary storage, never in the publicly uploaded metadata
artifact.

The unconditional cleanup `if: always()` now also removes the reviewed
seed scratch and the private Ghidra decompilation scratch, in addition to
original ZIP, staged PE32 and metadata scratch folders.

## Regression coverage

`research/tests/test_owner_ghidra_workflow_contract.py` is deliberately
run inside the synthetic-only PR CI job. It tests:

1. Real owner-job command has exactly the expected three positional
   arguments, with reviewed factory settings passed by environment.
2. Adding back a fourth or fifth directory argument fails CI.
3. Omission of private pseudocode scratch cleanup fails CI.
4. Removal of reviewed seed environment fails CI.
5. Disabling the default-branch-only owner-original guard fails CI.
6. Accidental inclusion of private C-like output in artifact globs fails CI.

No original DLL bytes are required for these checks; they validate the
YAML workflow and shell runner's call boundary rather than gameplay.

## Evidence and constraints

This is a deployment repair, not new reverse engineering of an original
method. It does **not** assert successful original-game factory xrefs,
retail vtables, PE32 engine initialization, independent game boot,
Android ARM64 or Quest 3 gameplay.

The fix is stacked on Phase 275 and remains a draft until integrated.
Do not infer that the enhanced owner's original-DLL analysis is running
from `main` before the corresponding PRs are merged.
