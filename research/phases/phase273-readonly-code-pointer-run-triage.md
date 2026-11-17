# Phase 273: candidate virtual-method pointer table triage from authenticated Ghidra relocation data

## Goal

Progress from arbitrary function starts to **possible C++ virtual-method pointer arrays** without claiming original class layouts. The original Ghidra atlas already exports `client.dll_relocated_code_pointers.csv` and `server.dll_relocated_code_pointers.csv`, each containing relocated 32-bit code pointers found in data sections. This phase groups **contiguous read-only relocations** into length-stratified review queues, and builds a metadata-only JSON report.

## Results on the authenticated 2026-11-17 owner-original Ghidra atlas

Source: GitHub Actions Ghidra atlas run **43991323338**, artifact `sin-ghidra-12-2-atlas-43991323338` (original DLLs SHA-checked upstream). The analyzer was executed locally against its downloaded metadata, with **zero original DLL bytes** or pseudocode in the report.

| Metric | client.dll | server.dll |
|---|---:|---:|
| Relocated code pointer CSV records | 63,577 | 126,446 |
| Distinct relocation pointer locations | 63,547 | 126,390 |
| Identical duplicate records deduplicated | 30 | 56 |
| .rdata pointer runs with four or more distinct code targets and at least four adjacent slots | **1,162** | **1,008** |
| Ranked review entries (up to 8 per size bucket) | 32 | 40 |

Runs are bucketed by 4–16, 17–64, 65–128, 129–256, and 257+ slots, rather than ranking solely by massive tables. Pointer addresses in `.data` (including legal x86 packed relocations at non-word-aligned addresses) are reported in totals but excluded from this read-only table queue. Exact Ghidra inferred function-start matches are counted separately from pointers into executable code that Ghidra has not recognized as functions.

Examples of read-only candidate arrays in the actual original atlas:
- client: `0x242A6828`, 12 pointer slots, 10 Ghidra exact function-start matches.
- server: `0x22450858`, 12 pointer slots, 12 exact function-start matches.
- client: longer 301-slot arrays occur in families near `0x242C2FF4`.
- server: 500+ pointer arrays occur in `.rdata`, with hundreds of exact function-start matches.

**No array above has been proven to be a vtable**, no slot is associated with an engine interface or C++ class by this phase, and no new whole-body SHA verified game methods or retail-engine tests are credited.

## Implementation and testing

`research/tools/build_relocated_pointer_table_queue.py` checks CSV schema, hex PE32 addresses, pointer-source uniqueness and conflicting duplicates, exact target-function association, plausible .rdata alignment, and bounded output. Identical duplicate relocation records are collapsed; unexpected different targets at a reused source address fail closed. Runs are maximal contiguous four-byte sequences within the same memory block; require at least four unique executable target addresses. The output includes slot count, source VA, Ghidra-recognized function-start count, distinct targets, a preview of **addresses only** and explicit `not_verified_as_vtable: true`.

Twelve independent synthetic unit tests pass and run on PR CI. After this PR reaches the default branch, the existing real owner-original Ghidra workflow will attach `relocated_pointer_table_review_queue.json` to its **metadata-only** artifact. No raw executable bytes, instructions or pseudocode are stored.

## Next step

For selected candidate arrays, use Ghidra's cross-reference graph (constructors, writes of vptrs into allocated objects, and return objects of reviewed `CreateInterface` factories) to link an array to a **specific interface**. Only then start reconstructing precise Win32 x86 virtual method positions and calling conventions in the independent DLL. This phase improves the ranking of actual original-game analysis targets, **not Windows gameplay boot**.
