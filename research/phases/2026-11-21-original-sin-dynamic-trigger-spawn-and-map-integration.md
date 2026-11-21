# Phase 528: original CTriggerDynamicDifficulty::Spawn and two-class map pipeline

## Why this is game code, not engine duplication

The user's SiN Episodes BSP maps, models and props were successfully
ported into Half-Life 2. Consequently, further reverse engineering
of shared Source engine physics/material/model/prop readers is not
justified without a specific blocker. These authentic game triggers,
their map KeyValues, events, Spawn/Touch and difficulty-state effects
are among the **missing SiN-only server gameplay code** instead.

The previous Phase526 verified original trigger dynamic/secret Touch
and three dynamic DATADESC keys. Phase527 recovered the separate
secret Spawn `game` descriptor 11 path. This phase completes the
corresponding **original dynamic trigger Spawn** control flow and
tests both entities from synthetic Source BSP data.

## Exact original server binary evidence

Original game `server.dll` SHA256:
`568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`.

Original `CTriggerDynamicDifficulty::Spawn` virtual slot25 is at
`0x221D0740`, full 69 bytes SHA256
`3309b7d25f963dbcb77ae2e1d6f8101fc97cf4ae0a1635c3e172d06d0f011c27`.

The exact method does:

1. Read original global object at `0x22572914`, invoke virtual byte
   `+0x88`; if the returned byte is nonzero, call
   `0x22286BF0(this)` to enqueue removal and immediately return.
2. Otherwise call original `CBaseTrigger::Spawn` at `0x221CFCF0`.
3. Load 32-bit `this+0xB4`, shift right 10 bits and test the low bit,
   i.e. check `0x400`. If clear, return.
4. If set, pass the exact source diagnostic
   `DD Trigger with don't destroy set` followed by newline
   to the warning import through original literal pointer
   `0x22438650`. The return is ignored, then the method returns.

Direct native call edges verified: `0x221D0756 -> 0x22286BF0`
and `0x221D0762 -> 0x221CFCF0`.
Original opcode witnesses independently pin the +0xB4 load,
10-bit shift and warning string pointer. Full original method SHA
protects all unlisted instruction bytes too.

**No `dynamic_difficulty` game event is dispatched during Spawn.**
That event is instead generated only by the actual player Touch path,
with original `EstimatedTime`, `EstimatedDifficulty`,
`EndOfGame` KeyValues validated in Phase526.

## New independent runtime work

`OriginalTriggerEntityFactory::run_dynamic_spawn` is a DISTINCT
native-engine Spawn callback from its portable classname allocation.
It uses host-provided original global +0x88 result, original
`this+0xB4` spawnflag bits and genuine
`initialize_native_base_trigger` callback. If global remove is
requested, it enqueues entity deletion without running base Spawn,
warning or Touch. It emits the verified diagnostic warning on bit10
through `IOriginalTriggerHost::report_original_warning`.

As with Phase527, a return of false or an exception from a host
callback is handled conservatively in the independent adapter;
reentrant dispatch and duplicate Spawn events are refused. Those
guards are NEW design and are not presented as original 2006 native
method equivalence.

The original-free `test_original_sin_trigger_bsp_integration.cpp`
now exercises both gameplay entities through the actual project
`parse_source_bsp_entities` and `SourceMapRuntime` using fabricated
Source BSP version20 entity text:

- Secret has genuine map `classname` and `targetname`. The test's
  synthetic engine-host supplies +0x120 flag bit3 independently of
  map KeyValues. Spawn emits `game` descriptor11; player Touch
  emits descriptor12.
- Dynamic has map `classname`, `targetname`,
  `EstimatedTime`, `EstimatedDifficulty`, `EndOfGame`.
  Its synthetic native-host +0xB4 flag bit10 triggers the real
  diagnostic warning on Spawn, but no premature difficulty event.
  Player Touch later emits the correctly populated original
  `dynamic_difficulty` event, sets the end-of-game manager flag
  and queues removal.

Synthetic tests also cover invalid KeyValue rollback, failed base
Spawn rollback, wrong class, global suppression and duplicate
callback behavior. CMake continues to link the real
`sin_original_gameplay_triggers`, `sin_source_map_runtime`,
and `sin_source_bsp_entities` components, never a fake alias
for a nonexistent Source engine-native class.

The auditor `research/tools/audit_sin_original_gameplay_triggers.py`
now verifies the two source Spawn branches, SHA-pinned methods,
call edges, field loads, bit masks and warning literal with
original-free individual mutation regressions.

## Limitations

These portable C++ tests run against synthetic in-memory Source
BSP data and a fabricated native entity host. **No original SiN map
was loaded by the reconstructed Windows server.dll**, no full
`ServerGameDLL003::DLLInit` startup is implemented, and no real
Source CBaseTrigger class allocation/engine entity registration
or Quest 3 on-device gameplay is established here.

The next serious priority is native server entity-factory integration,
original DLLInit success, engine-owned entity lifecycle, networkable
send tables and campaign map scripting, not HL2-compatible prop
or physics reimplementation.
