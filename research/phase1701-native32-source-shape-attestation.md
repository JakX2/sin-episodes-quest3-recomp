# Phase 1701: source-exact native32 receive-table attestation

## Scope and unchanged retail boundary

This phase extends Phase 1700's host-owned 32-bit little-endian **token image**. It is not a PE32 pointer exporter, network decoder, gameplay boot, or replacement for the user's original `client.dll`. The real original engine's last owner test remains the deliberately guarded `VClient011.slot5` exit. No game DLLs were replaced for this phase.

The source facts came from the existing SHA-pinned original 2006 PE32 client and engine audits in Phases 1696–1700, not from invented newer SDK assumptions. Original client DLL SHA256: `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.

## Safety issue closed by this phase

Previously `WalkRoots` validated that a simulated RecvProp native type was in the valid range 0..5 and a type-5 child was another *owned* table token. That permitted a valid-but-wrong type (e.g. original source type 0 silently changed to 1), a valid-but-wrong nested child table, an altered but still bounded count, a pointer to another owned descriptor array, or a mutated generated child `+44` byte offset/name. It also permitted the host traversal to mistake that different graph for the exact original 2006 graph, even though no native memory corruption was needed.

The new `Image::AuditVisitedSourceTable` is called **before** marking and appending each encountered table. It compares every reached table and all of its properties against authenticated immutable source metadata:

- Each original static table's source-identical property-array token and exact 211-table property count.
- All encountered static native property type DWORDs against the exact 1,110-element original type-tag map, rather than only 0..5 validity.
- All source type-5 static child table links against the exact 192 authenticated parent/slot/child triples, and all 14 generated array parent/child links against the source array-shape manifest. A valid but wrong owned child is rejected.
- The one specialized overlay descriptor must retain the **unresolved null child**. This does not claim an implemented overlay RecvProxy.
- Each generated child table's original count and descriptor-array token; all 691 native type 0/1 child tags, generated three-digit names and **child-relative +44 offsets** starting at zero.

The audit is **root-scoped**, not a blanket 211-table scan. That matches Phase 1700's evidence: original 191-class startup roots visit **224 tables and 1,796 descriptors**. The all-211 static inventory scan visits **225 tables and 1,801 descriptors**. The one five-property static `DT_Animationlayer` remains absent from original class-root traversal. Mutating a valid but source-wrong tag in that unreachable auxiliary table must leave startup traversal unchanged and cause only full-inventory validation to fail.

### Non-goals and remaining unsafe fields

The Phase 1700 simulated image still does not contain all original static property names, flags, RecvProxies, class creator callbacks, full overlay descriptors, network serialization, mutable allocator ownership, or a verifiably executable original engine ClientClass ABI. This check is a source-consistency and memory-integrity regression, **not** a substitute for reconstructing those missing fields.

`Image::CanExposeToRetailEngine()`, `WalkReport::CompleteRetailRecursion()` and upstream source `CanExposeToRetailEngine()` remain **false**.

## Regression tests

The production phase1700 host image test now corrupts an otherwise accepted property type to *another valid native type*, changes a source child pointer to another *valid owned static table*, lowers a count while remaining under the source limit, redirects a table to another *valid owned property array*, changes a generated child-relative offset, mutates one generated three-digit name, and attempts to resolve the overlay with an existing valid table token. Every change must be rejected by the source-root traversal. The unreachable auxiliary property mutation must be rejected only by the exhaustive inventory audit. Each mutated field is restored and both original traversal totals checked again.

Existing Linux strict C++17 + ASan/UBSan, Windows MSVC Win32 x86 and Android ARM64/x86_64 Bionic CI execute the augmented existing `test_original_client_native32_recvtable_image.cpp` rather than a new unregistered test file. No owner-private executable bytes are committed.

## Follow-on original-engine boundary

The next material retail-compatibility step remains recovery of **complete original static RecvProp flags, names and proxy function ABI**, native table/string memory ownership, the overlay builder's full child semantics, and executable creator/proxy callbacks. Only after independent native integration checks should the deliberately blocked `VClient011.slot5` be reconsidered. No physics, surfaces, props/models or renderer reimplementation is reopened.
