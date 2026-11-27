# Phase 1643: Original CBaseEntity / IServerUnknown exact-base owner alias

**Stack:** Phase 1642 PR #1651, tested Win32 x86 244/244. Builds on original-independent allocation pin and lifespan registry; factory registration remains disabled.

## Original 2006 PE32 evidence

Original owner server.dll SHA-256 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650. The following source-binary facts were verified in original-private local analysis and committed as read-only SHA-pinned audit metadata, not executable bytes:

- CBaseEntity constructor VA 0x2214671F forms this+0x160 and calls CServerNetworkProperty constructor VA 0x2227EFE0. The property constructor installs the source-verified first vtable at 0x22454F00 and a second interface vptr at +4.
- In the property edict binding function VA 0x2227F070, original 0x2227F08C writes edict into property+0x0C, calls its virtual +0x24 at 0x2227F08F, then original 0x2227F095 writes the returned outer CBaseEntity pointer to edict+0x0C. The authenticated property vtable+0x24 points to VA 0x2227F040, which returns [property+0x08].
- CBaseEntity vtable VA 0x2240AC2C has +0x10 = VA 0x22001AD0, returning this+0x160, and +0x14 = VA 0x22001AE0, returning this.
- ServerGameEnts001::EdictToBaseEntity VA 0x221E82F0 loads unknown pointer from edict+0x0C and invokes virtual +0x14, which for this original CBaseEntity path returns the same entity pointer. Its inverse VA 0x221E94B0 returns entity+0x16C: the property edict field +0x0C, consistent with the embedded offset +0x160.

This establishes that the original CBaseEntity itself is the valid IServerUnknown address in this path. Strict disjointness between entity and unknown in the previous independent registry was incompatible with authentic original memory relationships. The property subobject is at +0x160, NOT the unknown pointer loaded from edict. Other subobject identities or touch allocations remain unverified.

## Registry correction and tests

The WellFormed owner registry validation now allows ONE precisely bounded exception: unknown == entity and unknown_bytes <= entity_bytes. This is safe only when the trusted host has independently attested the entity allocation. All edict/unknown, edict/entity, edict/touch, entity/touch and unknown/touch overlaps are still forbidden; shifted, partially overlapping, out-of-bounds aliases and aliases into a different live entity are rejected.

The owner registry C++ native regression now checks authentic exact-base aliases, shifted and oversized aliases, cross-entity attacks, alias expiration under slot +0x0C retirement, retained edict prefix and re-enrollment with a new globally monotonic generation. The pinned-engine allocation regression now passes returned owner epochs (not original uint32 serials) into two adjacent edict identities enrolled with the source-backed entity==unknown alias, verifying original pair authorization and crosswired refusal under owner lease. The native CTest total remains 244.

The new original-free audit script audit_sin_original_gameents_entity_unknown_alias.py verifies **nine** full original 2006 server PE32 SHA-256 instruction/data windows and **20** short opcode/vtable witnesses; --server accepts only the SHA-pinned owner-original binary, and --self-test corrupts every witness, truncates each window and rejects fabricated original SHA evidence. Both modes passed locally. This audit is wired into the synthetic decomp GitHub Actions workflow.

## Remaining work

These are source-backed memory identity and lifetime contracts, not actual retail process attachment. Physical allocator pin and atomically committed count/epoch still rely on a separately implemented trusted host. The authenticity of the original touch subobject/backing lifetime and a live Windows engine ownership adapter remain unresolved. Do not register ServerGameEnts001 with retail CreateInterface or claim a playable Windows, Android or Quest build. No generic HL2 physics/surfaces/models/props work has been undertaken.
