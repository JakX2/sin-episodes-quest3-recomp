# Phase 127: Source-style entity target resolution reference

**Status:** Independently authored C++17 resolver and tests, stacked on Phase 126. This is a pinned public Source SDK 2013 behavioral reference, not a verified original SiN 2006 resolver or a SourceVR engine adapter.

## Why this is a useful gameplay milestone

Phase 73 verified 32 selected original SiN server.dll entity-factory registrations, including logic_auto, logic_relay, trigger_once, trigger_changelevel, trigger_transition, info_landmark, npc_maker, scripted_sequence and func_door. That establishes class registration only; it does not establish how the original DLL's event queue resolves targets.

Phase 74 deliberately left target resolution behind IEntityOutputHost. Phase 127 supplies a reusable engine-neutral resolver that an eventual lawful engine adapter can back with real entity-list iteration and input dispatch.

## Pinned reference behavior

Reference revision: ValveSoftware/source-sdk-2013 at e1f1fd42271805072f052051c26c21caa2c87b04.

- CEventQueue::ServiceEvents in src/game/server/cbase.cpp first iterates every targetname match through CGlobalEntityList::FindEntityByName.
- It next dispatches an optional direct entity target.
- Only when neither path found a target does it iterate FindEntityByClassname and dispatch the classname matches.
- FindEntityByName in src/game/server/entitylist.cpp preserves entity-list iteration order and returns one match per call, allowing duplicate targetnames to receive the same input.
- Procedural names in the pinned FindEntityProcedural implementation are !player, !pvsplayer, !activator, !caller, !picker and !self.
- NamesMatch in src/game/server/baseentity.cpp performs ASCII case-insensitive comparison. Its implementation checks for '*' at the first query mismatch/end position and returns true there. Although the adjacent source comment describes trailing '*', the actual code short-circuits at the first '*' and ignores any remaining query suffix.

Pinned public references:
- https://github.com/ValveSoftware/source-sdk-2013/blob/e1f1fd42271805072f052051c26c21caa2c87b04/src/game/server/cbase.cpp
- https://github.com/ValveSoftware/source-sdk-2013/blob/e1f1fd42271805072f052051c26c21caa2c87b04/src/game/server/entitylist.cpp
- https://github.com/ValveSoftware/source-sdk-2013/blob/e1f1fd42271805072f052051c26c21caa2c87b04/src/game/server/baseentity.cpp

No Valve implementation source was copied. The C++ here is independently authored from the observed behavior.

## Implemented

- Targetname lookup over the host-supplied engine entity order.
- All duplicate targetname matches, preserving that order.
- ASCII case-insensitive exact/prefix matching.
- Procedural targets !player, !pvsplayer, !activator, !caller, !picker and !self.
- Direct-entity dispatch after targetname/procedural lookup.
- Classname fallback only when targetname/procedural and direct-entity lookup found nothing.
- Generation-safe filtering of stale entity-list entries and direct references.
- A Phase 74 IEntityOutputHost integration test that exercises named duplicates, classname fallback and !activator through the delayed event scheduler.

The directory adapter must implement player slot 1, PVS client lookup and picker lookup using the real host. It must also preserve actual engine entity-list iteration order.

## Evidence boundary

This is not yet proven original SiN 2006 parity. The Phase 73 original DLL evidence confirms selected factory registrations, not the event-queue resolver. The original owner-supplied PE32 DLL bytes were not available for fresh disassembly in this run. Source SDK 2013 is a comparison reference only; do not silently assume the SiN 2006 fork or SourceVR 0.5.5 uses identical semantics.

This resolver does not parse BSP entity lumps, create engine entities, execute real AcceptInput handlers, implement variant_t conversion, save/restore pending events, perform level transitions, or provide a SourceVR native game module.

## Verification

CMake adds the resolver to the portable static library and a dedicated CTest executable. Tests cover duplicate ordering, wildcard behavior, procedural names, direct-target ordering, classname fallback, stale generations and integration through the existing Phase 74 scheduler.

No proprietary DLLs, APKs, original ELF, BSPs, models, maps, scripts or game assets are included. No placeholder native module is produced.
