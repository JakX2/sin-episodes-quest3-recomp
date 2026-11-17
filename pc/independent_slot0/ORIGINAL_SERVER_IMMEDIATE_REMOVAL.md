# Original SiN server immediate-removal lifecycle (Phase 321)

## Verified owner-original evidence

Original Windows x86 server.dll SHA256:
568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650.
Direct local PE32 analysis of RVA 0x286C10 confirms the immediate-remove
entry tests for a null entity and extended-flags bit 0 at entity +0xB8.
It checks a global removal semaphore; a nonzero value uses the embedded
networkable at entity +0x160 and delegates to RVA 0x286B90.
The alternate immediate path sets the kill bit, clears a global update
tracking byte, invokes a virtual method at vtable byte offset 0x17C,
enables a global destruction guard, invokes virtual slot zero with
deleting-destructor flag 1, then clears the destruction guard.

The SDK 2004 dlls/util.cpp UTIL_RemoveImmediate and UTIL_Remove routines
corroborate the meanings: EFL_KILLME, s_RemoveImmediateSemaphore,
UpdateOnRemove, g_bReceivedChainedUpdateOnRemove, and
g_bDisableEhandleAccess. The deferred target at 0x286B90 uses the
networkable flag interface; after marking it obtains the base entity,
performs the same virtual update and clears a field at base +0xC0
(SDK-supported targetname attribution), then calls a delete-list helper.
That actual queue, virtual lifecycle implementations, and allocator
ownership still require reconstruction. The SDK asserts the chained
update flag in debug builds; the observed retail path has no such check.

## Independent implementation and integration

server_entity_immediate_removal_core.h implements the verified outer
lifecycle using independent flags and state, typed callbacks for
UpdateOnRemove, destruction, and deferred routing, and explicit
handle-access gating. It refuses missing dependencies or attempts
to initiate destruction from inside a destructor. These additional
guards are independent safeguards. Deferred routing requires an actual
queue adapter; this phase does not implement the retail delete list.

The immediate-removal test checks ordering, reentry, nonmutation on
missing dependencies, the destruction guard and real heap deletion
with generation invalidation. The full native x86 teardown fixture
combines existing FreeContainingEntity detachment, touch and ground
bulk removal, reciprocal partner unlink, immediate-remove flags,
UpdateOnRemove and actual deletion of a synthetic owned entity.
All four allocated contact nodes are reclaimed; the original-style
touch suppression retains the now-empty partner touch root. The old
registry handle becomes invalid and a repeated edict release is a no-op.

These are explicitly controlled synthetic objects and injected behavior,
not a complete CBaseEntity class, real physics callbacks, networking
system or exported ServerGameEnts001. No original executable code,
commercial binary or raw disassembly is included in the repository.
Retail server/game initialization remains unproven.
