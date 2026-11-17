# Original SiN server deferred entity deletion (Phase 322)

## Original evidence

Read-only local PE32 inspection, original server.dll SHA256
568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650:

- RVA 0x286B90: deferred removal helper checks networkable, checks and
  sets kill flags through virtual methods, obtains its base entity,
  invokes UpdateOnRemove-like slot 0x17C, clears base name field +0xC0,
  then calls the queue insertion helper at RVA 0x1636D0.
- RVA 0x1636D0: obtains the networkable's entity handle, rejects the
  invalid sentinel 0xFFFFFFFF, then appends the networkable pointer.
- RVA 0x162AA0: sets a cleanup-in-progress byte, conditionally calls
  a physics-environment virtual method at byte offset 0xB8, sets the
  entity-handle access guard, loops over queued networkables in order
  invoking virtual slot byte offset 0x18, clears the guard and queue
  count, then clears the cleanup-in-progress byte.

All three are consistent with the owner's SDK 2004 dlls/util.cpp and
dlls/entitylist.cpp (UTIL_Remove, AddToDeleteList, CleanupDeleteList).
Original queue storage is a dynamic networkable-pointer container;
the original loop rechecks its count. This is not proof that all
retail callback mutation scenarios are safe.

## Independent implementation

server_deferred_entity_queue.h uses a bounded, noncopyable owned queue
of entity identity and generation handles. It marks, updates and resets
the name before enqueuing; it validates the handle before and after the
callbacks. It refuses at capacity before mutation and explicitly reports
unexpected post-update invalidation, rather than reporting a queued entity.
An adapter's refusal now propagates through TryRemoveImmediate as
DeferredRefused. The queue flush calls optional physics cleanup first,
enables the handle-access guard, performs ordered releases of current
generations, clears storage and restores state. The cleanup manager uses
its internal registry view; ordinary consumers use ResolveLiveEntity.

Fixed capacity, strict generation revalidation and rejecting reentrant
queue mutation are intentional independent safety constraints, not
claimed retail behavior. Actual engine physics, networkable lifecycle,
targetname storage, listener notifications and allocator ownership remain
external dependencies. Hooks must honor the declared object lifetime.

The synthetic Windows test uses actual independently allocated entities
and wires the deferred route of TryRemoveImmediate to the queue. It
checks mark/update/name order, refusal on capacity, no duplicate enqueue,
missing callbacks, physics-first FIFO drain, actual reclamation and
generation invalidation. A reused slot is never allowed to delete the
replacement entity using an obsolete queued handle.

This is still independent fixture integration, not an exported complete
server interface, retail frame execution or a playable build.
