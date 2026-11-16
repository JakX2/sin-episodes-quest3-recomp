# Phase 260: batch ABI candidate triage (metadata only)

Stacked on Phase 259. This does **not** recover original code, validate an inferred
function boundary, or demonstrate a retail Windows gameplay boot.

The batch tool combines the Phase 252 original PE inventory's candidates,
exports and direct-call edges with optional Phase 253 Ghidra inferred starts.
It prioritizes exported CreateInterface anchors, exported methods, inferred
Ghidra overlaps, direct-call sites and relocated code pointers. The resulting
priority score is a queueing heuristic, **not** a probability or coverage metric.

Run after the original owner DLLs have been full-hash authenticated and the
upstream original PE inventory has been generated:

```sh
python3 research/tools/triage_original_abi_atlas.py \
  --inventory /private/sin-batch-output \
  --ghidra /private/sin-ghidra-output \
  --out /private/sin-abi-priority.json
```

Omit --ghidra if no authenticated original Ghidra atlas exists. The tool checks
the upstream summary's recorded SHA256 against the two known retail DLL digests,
but **does not itself rehash binaries**. Treat summary files from untrusted
sources as untrusted. Output is address/count/name metadata only, without
native instructions, full disassembly or pseudocode. No private reports are
committed.

Six constructed-input regressions cover ordering, Ghidra candidate status,
duplicate detection, invalid tiers, missing columns and SHA summary mismatch.
The CI workflow runs only synthetic fixtures, not retail binaries.

Evidence remains: 403 previously whole-body-SHA-reviewed methods (256 client,
147 server), 64,586 previously scanned entrypoint candidates, zero original
functions independently load-tested. No new original bytes authenticated in
this phase. The executable goal is still independently implementing actual
client/server engine ABI and initialization, then Windows x86 boot, followed
by Android ARM64 and Quest 3.
