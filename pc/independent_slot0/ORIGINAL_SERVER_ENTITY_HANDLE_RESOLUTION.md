# Original SiN server: entity handle resolution in contact cleanup

## Original evidence

Read-only inspection of the SHA-pinned original retail Windows x86 server
(`568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`)
establishes a shared handle-resolution pattern inside the two cleanup
routines previously attributed to `PhysicsRemoveTouchedList` and
`PhysicsRemoveGroundList` (RVAs `0x21FFD0` and `0x220190`).

Both routines examine contact-node handles. The first routine's lookup
is visible around RVA `0x220024`; the second around `0x2201D4`.
Both reject `0xFFFFFFFF`, extract a 12-bit entry index, compare the
remaining 20-bit serial against the indexed entity-list entry, and
use its entity pointer only on equality. The original x86 registry
stride is 16 bytes, with entity pointer at entry +0 and serial at +4.
An unmatched serial becomes null even if the entry has been reused.

These observations agree with the owner's Source SDK 2004 archive:

- `src_mod/public/const.h`: MAX_EDICT_BITS is 11, NUM_ENT_ENTRY_BITS is 12,
  NUM_ENT_ENTRIES is 4096, INVALID_EHANDLE_INDEX is 0xFFFFFFFF.
- `src_mod/public/basehandle.h`: entry and serial extraction.
- `src_mod/game_shared/entitylist_base.h`: `CEntInfo` layout (pointer,
  serial, previous and next registry links), serial-checked lookup.

The SDK stores an `IHandleEntity*`; the independent view intentionally
stores a `CBaseEntity*` already supplied by its own registry, not an
unchecked cast from arbitrary third-party object memory. The original
global address is not read by the independent implementation.

## Implemented and tested

`server_entity_handle_core.h` provides an independent registry view and
matching generation-checked handle resolver. It rejects the invalid
sentinel, stale serials, empty entries, and absent storage; it also
checks the supplied array bound (an independent safety restriction).
On MSVC x86, compile-time checks enforce the observed record stride
and first two member offsets. Other targets use native pointer sizes,
not the original x86 raw memory layout.

`test_server_entity_handle_core.cpp` tests an occupied entry, a stale
serial after reuse, an empty entry, entry zero, the highest indexed
entry, invalid sentinel even with a matching table record, and
short/absent storage. Registered in the independent Windows CTest suite.

## Limits / next work

This is a shared entity-lifetime primitive, not a reconstructed full
contact registry. The original touched-list and ground-list loops also
have distinct presence-bit tests and linked-list traversal directions,
call partner unlink helpers, update counters, free nodes, and remove
per-entity data objects. Those behaviors, callbacks, the ground handle
reset in the first routine, and immediate deletion remain separate
reconstruction targets. The independent resolver is not yet hooked
into the release core or exported through `ServerGameEnts001`.

The seven-slot server interface remains unavailable to the retail
engine; neither client nor server initialization is yet complete.
