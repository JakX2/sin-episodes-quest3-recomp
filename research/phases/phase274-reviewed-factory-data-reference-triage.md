# Phase 274: reviewed factory references into data sections

## Reason for this step

Phase 273 grouped adjacent relocated pointers into 2,170 read-only
**candidate arrays** across the original PE32 client and server DLLs,
but relocation-only records cannot prove that a particular array
belongs to `VClient011`, `ServerGameDLL003` or another engine class.

The next necessary evidence is **how a reviewed interface factory
references data**, including possible static interface objects and
global instance pointers. This phase adds a Ghidra 12.2 Java
`ExportReviewedFactoryDataReferences.java` postScript, followed by a
fail-closed metadata-only Python reconciliation tool.

## Ghidra collector

For every seed from the independently reviewed 55 client + 10 server
static factory registration entries:

1. Resolve the exact factory entrypoint after Phase 271's bounded
   function seeding. Record unresolved entries without guessing starts.
2. Enumerate disassembled instructions **only inside that function's
   inferred body**, with a 20,000-instruction limit per factory.
3. Export references from those instructions into **non-executable**
   memory blocks, including address of the referring instruction and
   referenced data target. Ignore call/jump edges, executable data,
   and unresolved external targets.
4. Deduplicate identical instruction-target-type references. Keep
   at most 1,000 data references per factory and fail closed if the
   limit is exceeded.
5. Write only `<module>_factory_data_ref_status.csv` and
   `<module>_factory_direct_data_refs.csv`.

No decompiled C, original bytes, strings found at target addresses,
instruction operands, disassembly or class implementations are copied.

## Automated reconciliation

`research/tools/summarize_factory_data_references.py` joins the
Ghidra metadata with reviewed static interface registration CSVs.
It rejects mismatched factory addresses, unknown interfaces,
duplicate or missing outcomes, malformed hexadecimal addresses,
inconsistent reference counts, unsafe free-form fields and
fabricated provenance.

The resulting `factory_global_data_reference_review.json` gives
prioritized entries for:

- `VClient011`
- `VClientPrediction001`
- `VClientEntityList003`
- `ServerGameDLL003`
- `ServerGameClients003`
- `ServerGameEnts001`
- `GameMovement001` and `GameClientExports001` where present

The queue presents **direct references to data**, not identified
objects. A factory may return or read a static global object, but
that still requires proof from data-flow, constructor writes, and
object-return analysis. Even a data address in `.data` must never
be treated as a confirmed `this` pointer.

## CI and owner execution

The PR Ghidra smoke program is harmless synthetic C with a volatile
global variable. It deliberately removes and reseeds a known
function, then checks that Ghidra's new Java collector finds a
direct function-to-data reference. Synthetic Python tests enforce
the summary schema and evidence tiers.

The authenticated owner-original GitHub job already stages and
SHA-checks both retail PE32 modules. This phase enables the factory
reference collector through `SIN_GHIDRA_FACTORY_XREFS=1` **only in
that SHA-gated owner branch**, and publishes the CSV and JSON as
metadata-only artifacts. The original binaries and private C-like
pseudocode remain on runner scratch and are deleted on cleanup.

The feature cannot run on the original binaries through the existing
PR smoke. Its first owner-original run requires integration into the
repository's default branch.

## What has NOT been demonstrated

- No class object allocation or static object identity confirmed.
- No factory has yet been associated with a particular virtual table.
- No Windows x86 calling convention or original vtable slot has been
  verified by this phase.
- No new whole-body SHA-verified original methods, engine-loaded
  gameplay methods or playable retail Windows boot.

Next: use the factory-to-data references to determine which global
objects are returned by the factory, identify constructor-time vptr
writes into those objects, and match those writes against the
read-only candidate code-pointer arrays from Phase 273. Only then
implement real ABI-compatible interface methods in the independent
Windows x86 DLLs.
