# Phase 1639: Source-engine to GameEnts retirement handshake

Stacked on Phase 1638 PR #1646, starting from the corrected native compiler checkpoint `005b882aee045ee01ea4571d8bff8c20bcc1e9f1`.

## Concrete integration

The synthetic, **unregistered** native MSVC x86 seven-slot `ServerGameEnts001` fixture now invokes its slot-3 FreeContainingEntity through the independently reconstructed original-engine retirement core. An owner-array validator takes a short metadata-only slot-5 access lease to authenticate the exact 0x14-byte synthetic edict, releases that lease, then the server dispatch independently acquires the slot-3 lease. The server cleanup callbacks detach the entity and tombstone its generation. On return, the engine-side helper checks that the original edict remains owner-authorized and only then writes the retired serial 0xFFFFFFFF at +0x04 and zero at +0x10. This verifies the actual native call chain, callback order, registry token balancing and serial poisoning using a single C++ fixture rather than testing independent halves only.

The engine helper now **revalidates owner edict storage after the server callback** before executing its final writes. If the callback revoked ownership, it returns `PostCallbackUnauthorized` with serial/free-link untouched. This is a conservative safety guard, not an original 2006 engine return status; previous callback effects cannot be undone. The focused engine-retire test simulates successful server cleanup followed by owner revocation and requires that failure status and absence of engine epilogue writes.

The complete GameEnts fixture additionally verifies that, once the owner explicitly forgets the retired edict, a repeated engine-side retirement is rejected without running cleanup or touching the poisoned serial.

## Limits

This is a non-retail synthetic engine-to-server roundtrip using real compiled MSVC x86 virtual calls, not the original engine allocator. The owner adapter and entity memory are part of the test fixture, and the registry does not pin physical pages by itself. The live original game client/server remain unbooted, and `ServerGameEnts001` registration remains disabled. No Source-equivalent generic physics, surfaces, models or prop rendering work.
