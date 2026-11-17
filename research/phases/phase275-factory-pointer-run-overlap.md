# Phase 275: exact factory-data-reference overlap with pointer-array candidates

This is a **metadata-only** follow-up to Phase 274, stacked on its reviewed
factory-to-data references and Phase 273's relocated code-pointer arrays.

The joiner reads authenticated owner-run Ghidra metadata only:

- `client.dll_factory_seeds.csv` / `server.dll_factory_seeds.csv`
- `*_factory_data_ref_status.csv` and `*_factory_direct_data_refs.csv`
- `*_relocated_code_pointers.csv`

For each module, it independently rechecks reviewed seed provenance, matching
factory entry VAs, inferred-function status, complete reference counts, and
relocation source uniqueness. It rebuilds contiguous, aligned `.rdata`
code-pointer runs of four or more slots with four or more distinct targets.

A factory data-reference target **exactly inside** such a run is reported
with factory VA, instruction VA, referenced data VA, candidate run bounds,
slot index, total candidate slots and count of exact Ghidra function starts.
References that merely fall near a candidate, or reference a different
global, are not promoted.

This overlap is a stronger *investigation lead*, not a verified C++ vtable:
data xrefs can arise from incidental constants, global dispatch arrays,
indirect structures, or non-vptr use. Establish the factory's returned
object address and a constructor vptr write before assigning class identity.
Do not infer method calling conventions from pointer relocation alone.

The output `factory_pointer_run_overlap_review.json` is a compact review
queue. No game DLL, instruction bytes, disassembly, C-like pseudocode, or
game assets are included. Six synthetic-only regression tests cover positive
overlap, unrelated references, mismatched factory VA, reference-count
mismatch, conflicting pointer relocations and missing status.

The new owner job runs this join after both original PE32 files pass their
pinned full-file SHA256 checks and after Ghidra has generated both kinds of
metadata. PR jobs run synthetic tests only. Do not repeat an unchanged
full original scan solely to regenerate this report: reuse a successful
authenticated artifact containing both inputs, or wait until a new
owner-original extraction has run after Phase 274/275 integration.

**Current evidence tier:** exact address overlap of *inferred* factory
data references and *candidate* pointer arrays. New SHA-reviewed complete
methods: zero. New verified vtables: zero. New engine-loaded original
methods: zero. Retail Windows gameplay boot: not demonstrated.
