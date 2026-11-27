# Phase 1639: Source-engine to GameEnts retirement handshake

Stacked on Phase 1638 PR #1646, starting from the corrected native compiler checkpoint `005b882aee045ee01ea4571d8bff8c20bcc1e9f1`.

## Concrete integration

The synthetic, **unregistered** native MSVC x86 seven-slot `ServerGameEnts001` fixture now invokes its slot-3 FreeContainingEntity through the independently reconstructed original-engine retirement core. An owner-array validator takes a short metadata-only slot-5 access lease to authenticate the exact 0x14-byte synthetic edict, releases that lease, then the server dispatch independently acquires the slot-3 lease. The server cleanup callbacks detach the entity and tombstone its generation. On return, the engine-side helper checks that the original edict remains owner-authorized and only then writes the retired serial 0xFFFFFFFF at +0x04 and zero at +0x10. This verifies the actual native call chain, callback order, registry token balancing and serial poisoning using a single C++ fixture rather than testing independent halves only.

The engine helper now **revalidates owner edict storage after the server callback** before executing its final writes. If the callback revoked ownership, it returns `PostCallbackUnauthorized` with serial/free-link untouched. This is a conservative safety guard, not an original 2006 engine return status; previous callback effects cannot be undone. The focused engine-retire test simulates successful server cleanup followed by owner revocation and requires that failure status and absence of engine epilogue writes.

The complete GameEnts fixture additionally verifies that, once the owner explicitly forgets the retired edict, a repeated engine-side retirement is rejected without running cleanup or touching the poisoned serial.

## Limits

This is a non-retail synthetic engine-to-server roundtrip using real compiled MSVC x86 virtual calls, not the original engine allocator. The owner adapter and entity memory are part of the test fixture, and the registry does not pin physical pages by itself. The live original game client/server remain unbooted, and `ServerGameEnts001` registration remains disabled. No Source-equivalent generic physics, surfaces, models or prop rendering work.

## Explicit original virtual call

The server-side bridge reads the actual compiled MSVC x86 vtable function pointer at slot 3 (+0x0C) and invokes that `__thiscall` thunk. It does not rely on a compiler potentially devirtualizing a pointer to a known final diagnostic class, while the enclosing `CHECK_X86_STACK` asserts that the engine-to-server roundtrip preserves the real x86 caller stack pointer.

## Hardening the owner allocation registry

The owner registry now checks each of the four enrolled allocation intervals for internal overlap and for overlap with every still-owned region of another slot, rather than comparing only corresponding pointer values. A malicious edict beginning one byte inside another owned edict, or an unknown-interface pointer aliasing an entity in another slot, is rejected before any client of the registry can read those addresses. Pointer-range endpoint overflow is rejected before addition, and adjacent 0x14-byte engine edict records remain valid. Enrollment generations must also be globally monotonic, even when a prior slot record was overwritten after retirement. The test adds cross-field aliases, partial overlap, self-overlap, overflow, and older global-generation reuse after a fresh address allocation. These checks do not replace authentication from the real engine allocator.

## Before-virtual unknown/edict provenance

A complete GameEnts owner now requires a separate valid_unknown_pair(context,edict,unknown) callback. The bounded dispatcher validates this relationship **before** `IServerUnknown::GetBaseEntity` virtual slot +0x14, so another live but unrelated unknown object cannot execute code merely because both pointers are individually trusted. The concrete owner registry implements `ValidUnknownPair` under the same thread-bound lease; the complete x86 fixture additionally checks the actual synthetic edict serial. A missing callback makes the diagnostic vtable unbound, and tests assert that a crosswired owned unknown receives **zero** virtual calls during conversion, release and touch. This is a conservative safety guard, not a new original retail ABI method.

## Correction to the original edict memory model

Inspection of the authenticated original `engine.dll` allocation and reuse code reveals that `edict+0x10` is operated on as a floating-point reuse-time value. At VA 0x20110EF8, `fld dword ptr [esi+0x10]` reads it; at VA 0x20111004, `fstp dword ptr [esi+0x10]` stores it after time calculations. Previous comments incorrectly called it a free-list pointer. Tests and engine-core constants now label this 32-bit storage `reuse_time_bits`/`kEngineOriginalEdictReuseTime`; the original zero-on-retirement behavior remains unchanged. The read-only auditor SHA-pins the two three-byte x87 instruction windows and corruption-tests each, without including proprietary DLLs. Precise reuse timing conditions remain a future original-guided implementation task.
