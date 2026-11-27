# Phase 1633: One complete native ServerGameEnts001 x86 diagnostic vtable

## Why this phase

All seven retail ServerGameEnts001 vtable entries were SHA-verified during Phase 1626. Later phases implemented their behaviors *as separate independent cores*: `ServerGameEntsConversionCore` (debug edict, CBaseEntity+0x16c, edict+0x0c unknown virtual+0x14), `MarkEntitiesAsTouchingWithContactGraph` (touch entry and reciprocal links), `TryReleaseContainingEntity` (detach then touch/ground/immediate removal) and `CheckTransmitEntry` (original 20-byte PE32 candidate records and recipient/bitset policy). Phase 1632's end-to-end synthetic touch->release lifecycle passed 238/238 native Win32 CTest, but these still were **not methods on one real virtual object**.

The new `pc/independent_slot0/server_gameents_bounded_dispatch.h` provides a concrete `ServerGameEntsBoundedDispatch final : ServerGameEntsAbi` on native MSVC x86. Its destructor plus six `__thiscall` methods form the seven-slot Source 2004 interface. This is a **host-injected diagnostic object only**; it is neither exported through the independent DLL `CreateInterface` nor returned to the retail engine.

## Original ABI provenance

Authenticated commercial `server.dll` SHA-256: `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`. Original singleton `0x22510654`; original vtable `0x22443030`; seven complete function-pointer words SHA-256 `802c3d045ace1464abe8259852c6654c2c4a2327b56be820085cca8e609038ae`. Original slot order (including MSVC deleting destructor):

| Slot | Original function | New diagnostic dispatch |
| --- | --- | --- |
| `+0x00` | `0x221e9360` | MSVC virtual deleting destructor for owned diagnostic object |
| `+0x04` | `0x221e82d0` | `SetDebugEdictBase`, stores independent pointer only |
| `+0x08` | `0x221e9380` | `MarkEntitiesAsTouchingWithContactGraph` |
| `+0x0c` | `0x221e82e0` | `TryReleaseContainingEntity` |
| `+0x10` | `0x221e94b0` | `ServerGameEntsConversionCore::BaseEntityToEdict` |
| `+0x14` | `0x221e82f0` | `ServerGameEntsConversionCore::EdictToBaseEntity` |
| `+0x18` | `0x221e94d0` | `CheckTransmitEntry` and shared transmit policy |

All original function identities were already recovered and are *not counted again*. This phase is ABI composition and native regression only, not a byte-for-byte decompilation of the retail object.

## Engine-owned lifecycle and security boundaries

`GameEntsBoundedOwner` is an immutable callback configuration provided at construction. The diagnostic vtable is considered **bound** only if all owner services are present: full touch/contact graph, release cleanup stages, transmit table/recipient/bitsets/entity/ShouldTransmit/PVS/SetTransmit and original byte-extent providers. Missing any required dependency causes every operation to fail closed without work.

Before reading CBaseEntity+0x16c, calling an edict's unknown interface, or accessing native edicts, the instance requires separate host `valid_entity`/`valid_edict` callbacks to attest live owner memory and minimum original PE32 extents. Returned conversion pointers are authorized *again* before crossing back to the caller. `CheckTransmit(CCheckTransmitInfo*,ushort*,signed count)` requires a host `transmit_extent` callback to authorize and bound its info buffer and index array before using the existing PE32 entry adapter. This does not claim that raw pointers from an unmodified engine are safe by default: a genuine runtime must implement, prove and test these ownership callbacks.

All original void signatures remain void and preserve x86 calling convention. A nonvirtual `Report()` records last slot, refusal/result status and counters for diagnostic tests without modifying the original ABI contract. A per-instance reentrancy guard refuses nested virtual calls invoked from callbacks, avoiding reentering halfway-mutated contacts, transmission state or deletion. A reentrant method does not overwrite the outer method's result. The bridge does **not** export the original retail singleton or write global Source memory.

## Native regression

New `independent_server_gameents_bounded_dispatch_native` CTest:

- Constructs a fully bound native x86 object and examines all seven non-null virtual slots. In addition to calling through `ServerGameEntsAbi*`, it casts the six operational entries from the **actual MSVC x86 vtable** to original `__thiscall` signatures, invokes each pointer, and checks unchanged `ESP` across each call. It deletes the instance via its virtual destructor.
- Validates exact original CBaseEntity edict+0x16c and edict unknown+0x0c GetBaseEntity virtual+0x14 conversion, including unauthorized foreign memory and retired entity pointers without dereference.
- Executes original-guided edict touch initialization, midpoint publication and reciprocal two-node creation; verifies publication refusal before allocations and successful repeat-contact calls.
- Executes original candidate table and selected-bit processing through virtual CheckTransmit, including valid index array, undersized physical index array before bit mutation and an attempted callback reentrancy.
- Executes virtual FreeContainingEntity after touch creation and network transmission; checks edict detachment, remote EndTouch, both node releases, ground cleanup, UpdateOnRemove, synthetic entity tombstone and registry invalidation.
- Confirms a retired edict cannot be released a second time, resolved into its old entity or generate new contact links. An incompletely bound instance refuses operations without side effects.

The fixture deliberately uses stable **owned** PE32 memory and synthetic entity destruction (tombstones) to test logical identity without any dangling dereference. The previous separate Phase 1632 test uses real heap deletion inside the destruction callback.

## Explicit remaining blockers

This is a native seven-slot **diagnostic** implementation, not evidence that the actual retail engine owns these callbacks or that the independent `server.dll` can safely expose `ServerGameEnts001`. Required future work: real Source edict/networkable storage ownership and generation checks; original global trace publication and contact allocator; live engine transmit table/recipient/bitset resolution; stateful entity removal and stage callbacks; interface factory registration, engine init ordering, retail Windows startup and client-side services. The Android and Quest 3 engine ABI will require separate native adaptation; the exact MSVC x86 layout is not portable.

No proprietary executable bytes, binary copies, Source physics, surface/material, prop, model-loading or rendering work are added. `main` untouched.
