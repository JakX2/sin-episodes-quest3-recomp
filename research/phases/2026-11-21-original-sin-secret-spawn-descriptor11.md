# Phase 527: CTriggerSecret::Spawn is a different game event from its Touch

Source of truth: exact owner-authenticated original SiN Episodes:
Emergence 2006 `server.dll`
SHA256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`.
Original `client.dll` partner SHA256
`6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.

The previous Phase526 implemented player-only Touch of
`trigger_dynamic_difficulty` and `trigger_secret`, including
`game` descriptor **12** for secret discovery upon player touch.
The original `CTriggerSecret` also has an independent **Spawn**
implementation, 99 bytes at original server VA `0x221D0790`
(RTTI-verified vtable slot 25). Its exact full-function SHA256
is `561260115c2318a26943c49e29d43494f7ef6ceea2108f12acafde9f27d8c329`.

## Authenticated native control flow

1. Read original subsystem global `0x22572914`, then call its virtual
   byte offset `+0x88`. If the returned low byte is nonzero, call
   queued removal helper `0x22286BF0(this)` and return immediately.
   It does NOT execute base Spawn or publish a secret event on this path.
2. Resolve `targetname` from `this+0xC0` if present and nonempty.
   Otherwise choose `classname` from `this+0x0C`, else a source
   fallback string. The exact original fallback string pointer is a
   local original constant, never an inferred Source generality.
3. Call native `CBaseTrigger::Spawn` helper at `0x221CFCF0`.
4. Read the LOW BYTE at `this+0x120`, shift it right three places and
   mask bit zero: equivalent to `(flags & 0x08) != 0`.
   On that branch only, call original game-event writer `0x222B63B0`
   with **descriptor 11** and resolved name.
5. Return with no additional explicit side effects.

Original direct relative calls are fully validated at:
- `0x221D07A6 -> 0x22286BF0` remove;
- `0x221D07D2 -> 0x221CFCF0` base Spawn;
- `0x221D07E8 -> 0x222B63B0` game event;
with the source BYTE/shift/branch and literal descriptor-11 witnesses
validated in `research/tools/audit_sin_original_gameplay_triggers.py`,
in addition to the existing complete method SHA check.

**Crucial:** Spawn descriptor **11** is not Touch descriptor **12**.
One secret can legitimately publish descriptor11 on spawn and later
descriptor12 if touched by the player. Coalescing or treating one as
a duplicate would alter original campaign secret tracking behavior.

## Independent implementation

`OriginalTriggerEntityFactory::spawn` still attaches a portable
behavior instance to an existing native-host entity and is **not**
equivalent to `CTriggerSecret::Spawn`.

The NEW `run_secret_spawn(EntityRef,TriggerSpawnInput)` explicitly models
the genuine native engine lifecycle callback, with two separately
provided inputs from the future engine host:

- original global virtual +0x88's boolean `engine_requests_removal`;
- original entity field +0x120's `native_flags_0x120`.

The host must supply genuine `initialize_native_base_trigger()`
behavior, plus previously existing typed `game` event publisher and
deferred `queue_remove` callbacks. The adapter refuses missing base
spawn, never invents a native Source entity pointer, and never
creates a fake collision trigger. If global removal is requested,
the original trigger is consumed and queued with no base Spawn/event.
If flag bit3 is present, it emits an original `game` event with
descriptor11, owner=-1 and targetname. A later player Touch emits
descriptor12 using Phase526 behavior.

Conservative independent safety differences are explicit:
bounded game host lifetime, same-generation entity handles, one-shot
spawn state latch, fail-closed base callback and queued-remove retry
without repeated event publication. Native method lifecycle and
save/restore integration still must be built on top of a real engine.

## Tests

The existing `sin_original_gameplay_triggers_tests` native CTest now
also checks: descriptor11/descriptor12 distinction and order, exact
flag bit3, suppression by global +0x88, no base call/event in suppression,
deferred removal failure/retry, base Spawn failure/retry, event publisher
failure with no re-emission, and rejection of dynamic triggers passed
to the secret-only native Spawn stage.

This is gameplay reconstruction. No HL2-common physics, model or prop
reader is rebuilt, and the original PE32 DLLs/assets are never
published. The independently compiled `server.dll` still fails closed
before normal `ServerGameDLL003::DLLInit`, and this adapter has not
executed inside a real SiN map or on Quest 3.
