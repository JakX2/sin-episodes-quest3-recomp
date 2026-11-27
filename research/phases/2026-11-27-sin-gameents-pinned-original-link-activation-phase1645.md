# Phase 1645: Pinned reciprocal network-property link attestation at CBaseEntity activation

**Stack:** Phase 1644 PR #1653 at final head e17062cee42e4782aa5605651b16635abf518cbb, MSVC Win32 native 244/244 and Android ARM64/x86_64 synthetic CI green. Retail factory registration is still disabled.

## Why this follows Phase 1644

Phase 1644 records the 0x14-byte engine edict and globally monotonic host epoch inside the allocation owner's physical pin. It intentionally does not invent an entity before construction. But its later entity activation still accepted **trusted metadata addresses only**: it had not verified the actual 2006 PE32 network-property fields that connect the newly constructed CBaseEntity and edict.

The Phase 1643 original-server SHA-pinned audit (SHA-256 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650) authenticated all three reciprocal 32-bit pointer fields in this original 2006 path:

- edict+0x0C points at CBaseEntity, used as the exact-base IServerUnknown alias;
- CBaseEntity embeds CServerNetworkProperty at +0x160; property+0x08 (entity+0x168) is the owning outer CBaseEntity pointer;
- property+0x0C (entity+0x16C) stores the same edict pointer, used by ServerGameEnts001::BaseEntityToEdict.

No additional original-game binary contents or unverified vtable behavior are claimed in this phase.

## Independent host-bound implementation

New header `server_gameents_pinned_original_link_attacher.h` adds `GameEntsPinnedOriginalLinkAttacher::Attach` for a source-backed, bounded three-link readback and subsequent `ActivateReservedEntity` call.

A **trusted host** must acquire a simultaneous exclusive physical pin on edict, entity and touch backing, then return an authenticated snapshot with PE32 addresses and reserved host epoch. The attacher rejects missing callbacks, malformed extents, overflow, zero or mismatched pointer tokens, shifted unknown identities, wrong entity/edict/touch extents, wrong epoch, and callbacks that revoke a pin. It validates the pin before reading PE32 bytes and again after reading. Only when all three pointers match does it activate the Phase 1644 reserved identity **while the pin is still held**. A third post-activation validation detects a broken host pin after metadata mutation and permanently quarantines the attacher; failure of the exactly-once unpin also quarantines it. Reentrant and competing-thread calls fail Busy, and a failed lease/registry activation does not report success.

On native Win32 the PE32 tokens must equal actual 32-bit addresses. On x64 test fixtures they are **synthetic tokens** representing an original 2006 PE32 address space; this is not a functional x64 port or a claim that x64 pointers match original game memory. The implementation uses bounded little-endian 32-bit reads and does not dereference retail objects or call original vtables.

New `test_server_gameents_pinned_original_link_attacher.cpp` exercises correct end-to-end reservation→pinned readback→deferred identity activation; three individual reciprocal-pointer corruptions and clean retries; missing and truncated memory extents; stale epochs and shifted/foreign metadata; denied or revoked pins; permanent quarantine on failure after activation; registry lease conflicts; duplicate activations; reentrant callback and competing thread; and successful exactly-once releases. CMake registers this as an additional independent native CTest case (total 245, subject to completed CI). Tests are fully original-free.

## Unresolved retail interface

The actual unmodified 2006 retail engine does not expose a proven exclusive edict/entity/touch pin contract. This attacher cannot manufacture one. A real Windows host must own the physical backing, recognize any array reallocation and entity invalidation, and guarantee the snapshot throughout both readback and registry mutation. The true memory/lifetime nature of the original touch backing is not verified. An owner-original runtime experiment is necessary before any retail ServerGameEnts001 registration or playable Windows, Android or Quest milestone is claimed.

Do not implement generic HL2-equivalent physics, surfaces, props or model loading for this phase.
