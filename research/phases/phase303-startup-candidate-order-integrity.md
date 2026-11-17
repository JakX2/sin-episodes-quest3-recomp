# Phase 303: reject duplicate or reordered first-startup indirect-call sites

The Phase 299 Ghidra extractor enumerates computed call instructions in
ascending address order, with ranks 1 through 8. The Phase 301 private
validator previously checked rank continuity and bounded offsets but did
not require **strictly increasing** computed-call offsets. A malformed
private CSV could repeat the same candidate site under multiple ranks
or reorder the sites and inflate the aggregate candidate count.

The Phase 303 gate now rejects both patterns without printing owner-private
row data. Three synthetic-only tests cover duplicate call offsets, decreasing
call offsets, and distinct ascending call offsets. The JSON summary still
reports `verified_factory_calls=0`, `verified_failure_edges=0`, and
`original_sha256_authenticated_by_this_tool=false`.

This is a **metadata integrity correction**, not new original binary
evidence. It does not run Ghidra, inspect proprietary methods, establish
factory argument types, prove a conditional failure edge, or change
Windows runtime code. It can be applied to already-existing private
Phase 299 output without re-running the original PE32 scan.

For the next owner-private review, inspect the earliest candidate in
`VClient011::Init` and `ServerGameDLL003::DLLInit` only after pinned
full-file SHA256 verification. Verify the callback argument source, the
version-literal pointer reaching the call, return-value test, and the
branch target in the same complete function body. Do not promote
proximity-only observations into original-verified ABI handoff records.
