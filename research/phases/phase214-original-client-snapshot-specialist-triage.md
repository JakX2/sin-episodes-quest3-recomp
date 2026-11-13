# Phase 214: deterministic triage of original client snapshot serializer callees

13 November 2026. **Stacked draft on Phase213 PR #267.** This phase is an investigation aid, NOT a newly decompiled native function or a runtime-compatible game DLL.

## Evidence and count

The original 2006 `client.dll` prediction record router at `0x241996F0` has **39 instruction-boundary verified direct x86 CALL sites reaching 36 distinct targets** (Phase212). Four helper targets were fully SHA256-reviewed in Phase213: `0x24198A10`, `0x24198A40`, `0x24198A70`, and `0x24198B40`. The fifth reviewed destination is the **recursive router itself**, not another newly recovered function.

After cross-referencing the Phase213 243-method catalog: **5 distinct call destinations reviewed, 31 still unreviewed**. Of 39 native call sites, **5 reach reviewed targets and 34 reach unreviewed targets**. Those 31 are a *router-local target queue*, not the number of functions remaining across either DLL or the percentage of SiN reconstructed.

## Priority for the next original-binary review

| Priority | Distinct unreviewed destinations | Why |
| --- | ---: | --- |
| P0 | **6** | Complete the native type 1 and type 2 specialist chains (three unreviewed targets in each case) |
| P1 | **3** | Shared helper targets that are directly called from multiple original field cases |
| P2 | **22** | Remaining single-callsite native specialist targets |

P0 direct native destinations:

- Type 1: `0x24198AA0`, `0x24197CA0`, `0x24198470`.
- Type 2: `0x24198B80`, `0x24197D00`, `0x241984A0`.

P1 repeated native destinations:

- `0x24197A90`: cases 8 and 9.
- `0x24197B60`: cases 5 and 8.
- `0x24198360`: cases 8 and 9.

All addresses above are independently witnessed **CALL destinations**, not newly recovered function boundaries or verified implementation details.

## Reproduce without commercial DLLs

From the repository root:

```sh
python3 research/tools/build_sin_original_snapshot_specialist_queue.py --output /tmp/sin-snapshot-queue.json
python3 -m unittest discover -s research/tests -p 'test_sin_original_snapshot_specialist_queue.py' -v
```

The queue reads `research/catalog/original_dll_verified_methods.json` and existing Phase212/213 Python evidence constants. It checks both original retail DLL SHA provenance values in the ledger, all five previously SHA-reviewed router/helper metadata entries, switch-table shape, 39 callsite metadata entries, and unique target counts. Cases are inferred from the *already-authenticated* native switch destinations, not from a new linear sweep of machine code. Every target records case indices, field types, original CALL sites, and priority. Synthetic-only unit tests prove ordering, duplicate handling, evidence drift refusal, case grouping, and how a *hypothetically* reviewed next target would leave the queue.

**Crucial limitation:** This script does not open the owner's original binaries or itself verify any x86 method, function size, caller argument semantics or Windows engine behavior. It must not increment the reviewed 243-function catalog. Its output should guide a future original-private, SHA-gated binary audit of one P0 target, followed by clean-room implementation, regression tests and a genuine Windows x86 engine loading gate.

Roadmap remains Windows PC → Android ARM64 → Quest 3. Proprietary binaries and original full code remain excluded from the repository.
