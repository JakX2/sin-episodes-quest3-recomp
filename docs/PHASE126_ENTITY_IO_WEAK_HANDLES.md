# Phase 126: Entity I/O weak caller/activator lifetime

**Status:** Independently authored C++17 compatibility correction. It improves the portable entity-I/O scheduler using a public Source SDK 2013 behavior reference. It is not a SourceVR engine adapter and does not claim original SiN 2006 binary parity.

## Why this change was needed

Phase 74 previously removed pending output events whenever their source entity was removed. That conflated two different lifetimes:

- The entity-owned output connection list.
- The global event queue entries already created by firing those outputs.

The public Valve Source SDK 2013 reference keeps these separate. Its `EventQueuePrioritizedEvent_t` stores `EHANDLE` values for both activator and caller. The queue services due events and passes the handles through target resolution and `AcceptInput`. A removed entity therefore becomes a null handle; the queued event itself is not automatically discarded. The queue exposes explicit `CancelEvents(caller)`, and `logic_relay`'s `CancelPending` input invokes it.

Reference revision: `e1f1fd42271805072f052051c26c21caa2c87b04`.

- [eventqueue.h: weak activator/caller handles](https://github.com/ValveSoftware/source-sdk-2013/blob/e1f1fd42271805072f052051c26c21caa2c87b04/src/game/server/eventqueue.h)
- [cbase.cpp: queue servicing, explicit cancellation, save/restore](https://github.com/ValveSoftware/source-sdk-2013/blob/e1f1fd42271805072f052051c26c21caa2c87b04/src/game/server/cbase.cpp)
- [logicrelay.cpp: CancelPending input](https://github.com/ValveSoftware/source-sdk-2013/blob/e1f1fd42271805072f052051c26c21caa2c87b04/src/game/server/logicrelay.cpp)

These files were consulted as behavioral references only. No Valve source implementation was copied into this repository.

## Implemented

- `remove_source` now removes only the entity-owned output definitions.
- Pending events retain their caller and activator handles.
- At delivery, a stale generation is converted to the null `EntityRef{}` sentinel rather than causing the event to be dropped.
- Target resolution still occurs at delivery time and receives the current live-or-null context.
- Each target delivery revalidates the weak handles, because an earlier input callback can remove or recycle an entity.
- New `cancel_events_from_caller` explicitly cancels queued events for a caller and returns the number removed. This is the portable counterpart of an explicit `CancelPending` operation, not automatic entity destruction.

## Regression coverage

The entity-I/O tests now verify:

1. A delayed event survives source removal and delivers to a live named target with a null caller.
2. A recycled entity index does not become the old caller.
3. Explicit caller cancellation removes pending events.
4. Output definitions are removed independently of already queued events.
5. Zero-delay relay-loop budgeting and the existing scheduling tests remain intact.

## Evidence boundary

This is a **Source SDK 2013 reference behavior**, not proof that the original SiN 2006 engine fork or the shipped SourceVR 0.5.5 native engine uses byte-for-byte identical queue semantics. The original SiN DLL and SourceVR ELF bytes were not available for fresh disassembly in this run. The new behavior must be validated against the chosen host before being described as exact SourceVR or SiN runtime parity.

No proprietary DLL, APK, ELF, BSP, model, map, script or game asset is included. No native game module or placeholder APK is produced.
