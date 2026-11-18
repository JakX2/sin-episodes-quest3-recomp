# Independent backing particle storage

This change is stacked on native-CI-verified PR #410. It moves beyond a
callback-only allocation fixture by supplying a concrete owned bucket backend
and contiguous 96-byte record storage with independent free-list bookkeeping.

Source evidence: Phase 87 authenticates the 48-byte bucket allocation and
96-byte constructor argument, but not the entire original allocator or its
capacity. The test capacity 2048 is explicitly a trial configuration only.
The limit of 65536, alignment, free-list order, generation handles, clearing
and ownership policy are independent design choices, not retail observations.

The pool handles full allocation, reuse, exhaustion, invalid/stale/foreign
handles, shutdown and reinitialization. The backend works with the existing
ParticleBucketOwner boundary. It does not expose any guessed Source object
layout, call the real engine, simulate particle effects or integrate into
the shipped diagnostic DLL. No original method coverage or retail startup
credit is claimed. The next retail test still is Phase 348.
