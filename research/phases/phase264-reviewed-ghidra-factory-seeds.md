# Phase 264: reviewed static-interface factory addresses seed Ghidra discovery

## Evidence and immediate target

On 2026-11-16, an existing successful authenticated original-DLL Ghidra
metadata artifact (run 43815461835) was retrieved and inspected directly.
Its 9,676 client and 14,755 server inferred starts **omit all 12** tested
high-priority original interface factory addresses from the Phase 177
registration inventory, including `VClient011`, `ServerGameDLL003`,
`ServerGameClients003`, `ServerGameEnts001`, `VClientPrediction001`,
`VClientEntityList003`, `GameMovement001`, and others.

An absent Ghidra function start is not proof that the previous reviewed
factory pointer was incorrect. It can be the result of missed code discovery,
an overlapping inferred function, a thunk, or an undecodable boundary.
This phase introduces a **targeted candidate-recovery pass** rather than
another undirected whole-binary scan.

## Implementation

1. `prepare_reviewed_factory_seeds.py` accepts the pre-reviewed original
   registration metadata, validates both modules, counts, names and PE32 VAs,
   then emits metadata-only per-module factory seed CSV files.
2. `SeedReviewedInterfaceFactories.java` runs **after** Ghidra's default
   analysis but **before** the atlas exporter. Each reviewed factory address
   is checked against executable memory and existing functions. The script
   never destroys, clears, or splits a preexisting function. It attempts
   disassembly and function creation only at an uncovered reviewed entry.
3. Each address receives an explicit outcome: `already_inferred`,
   `seeded_candidate`, `overlaps_another_inferred_function`,
   `non_executable`, `unresolved_candidate` or an error status.
4. The ABI prioritization report now distinguishes starts that existed
   **before seeding** from those **created by seeding**, ensuring a
   manually anchored function is never misreported as independently
   Ghidra-discovered.
5. The existing hosted workflow tests the Java pass with a harmless
   synthetic non-PIE ELF and tests the seed generator with constructed
   registration metadata. On an authenticated owner-original run, the
   script uses the reviewed manifest if present in that branch, and
   archives seed outcomes alongside the metadata-only function atlas.

## Limitations

A `seeded_candidate` is only a candidate function with a reviewed
registration pointer. It is not a fully body-SHA-verified original method,
verified function extent, known vtable, or executable replacement interface.
Further original-only inspection is required to establish ABI method layouts,
calling conventions, and native engine behavior.

The default branch may not yet contain the reviewed registration manifest
because research PRs are stacked and unmerged. The workflow therefore
warns and skips owner seeding if that manifest is absent. Synthetic CI
cannot make new claims about retail binaries. Ghidra scheduled GitHub runs
are configured but not yet confirmed by a `schedule` event.

Existing method evidence remains 403 previously reviewed whole-body-SHA
entries, 64,586 candidate entrypoints, zero independently original-engine-
load-tested functions and **no real SiN Windows gameplay boot**.

Windows x86 integration and critical engine interface vtables remain the
next milestones, followed by Android ARM64 and Quest 3 only after actual
PC engine/gameplay testing succeeds.

## Synthetic test commands

```sh
python3 -m unittest discover -s research/tests -p test_prepare_reviewed_factory_seeds.py -v
python3 -m unittest discover -s research/tests -p test_build_ghidra_interface_queue.py -v
bash -n research/tools/run_ghidra_original.sh
```

No retail game files, pseudocode or original machine instructions are
included in this branch.
