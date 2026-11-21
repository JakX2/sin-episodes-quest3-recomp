# 2026-11-21: actual SiN campaign trigger behavior, not shared Source physics

## Why these two entities

The owner previously loaded SiN Episodes maps in a Source-compatible
runtime without the original SiN game-code DLL, producing 49 unknown
classnames. The five owner-supplied console logs contain
**88** unknown-type messages for `trigger_dynamic_difficulty` and
**44** for `trigger_secret`. These are repeated runtime log occurrences,
not a claim that the shipped BSPs contain exactly 88 and 44 instances.
Both classes are absent from stock HL2 gameplay but legitimately
registered by the original SiN 2006 server.dll.

Neither BSP/world physics, surface properties, model reading, nor prop
rendering needs recreation for this specific gameplay behavior.
The owner has already loaded the maps and existing props in HL2,
so these engine-common systems are deprioritized pending a demonstrated
gameplay blocker.

## Authenticated original inputs

The original 2006 unmodified PE32 server.dll SHA256:
`568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`.

Owner-private binary audit verifies **seven complete method bodies**
(total **698** bytes), two original classname factory registration
methods, and RTTI-backed original vtables. It emits only metadata,
not binary bytes or disassembly. The original paired client.dll
full SHA256 is also mandatory:

`6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.

| Original 2006 server VA | Length | Original method |
|---|---:|---|
| `0x221CCB70` | 137B | CTriggerDynamicDifficulty player-gated touch |
| `0x221CCC10` | 67B | CTriggerSecret player-gated touch |
| `0x221D0740` | 69B | Dynamic trigger spawn-time behavior |
| `0x221D0790` | 99B | Secret trigger spawn-time behavior |
| `0x222B64D0` | 218B | dynamic_difficulty event writer |
| `0x222B63B0` | 103B | game event writer |
| `0x222B93E0` | 5B | sets dynamic manager byte at +0x11 to one |

Class registration:
- `trigger_dynamic_difficulty`, registration `0x223BE4A0`,
  classname `0x2243CAF4`, factory object `0x2250B404`,
  RTTI-backed original vtable `0x2243B96C`;
  touch vtable slot **89** and spawn slot **25**.
- `trigger_secret`, registration `0x223BE4D0`,
  classname `0x2243CB10`, factory object `0x2250B408`,
  RTTI-backed original vtable `0x2243BC0C`;
  touch vtable slot **89** and spawn slot **25**.

### Exact dynamic trigger map KeyValues

The original class DATADESC at `0x2250B374`, initialized by method
`0x221D3040`, has **three** fields. Actual type descriptors are stored
at `0x2250CCFC`, `0x2250CD30`, `0x2250CD64`.

| External KeyValue (exact source spelling) | Internal field name | x86 offset | Original field type |
|---|---|---:|---|
| `EstimatedTime` | `m_estimatedTimeToGetHere` | +0x470 | FLOAT (1) |
| `EstimatedDifficulty` | `m_estimatedDifficulty` | +0x474 | INTEGER (5) |
| `EndOfGame` | `m_finishesPlaythrough` | +0x478 | INTEGER (5) |

**Important observed quirk:** The touch body checks **BYTE**
`this+0x478`, not a 32-bit integer compare. Thus a hypothetical
`EndOfGame=256` has a zero low byte and does *not* set the original
difficulty-manager completion flag. This independently reconstructed
behavior is tested rather than substituting a general C++ nonzero-int
check.

### Original touch action sequences

`CTriggerDynamicDifficulty`, original `0x221CCB70`:

1. Invokes touched actor virtual byte **+0x120** (likely Source IsPlayer,
   still an opaque typed engine-host predicate) and returns without
   side effects when false.
2. Resolves trigger display name using nonempty `this+0xC0` targetname,
   falling back to `this+0x0C` classname, then the original empty
   fallback.
3. If `this+0xB8 & 0x800`, calls base helper `0x2213FA60`.
   This helper has additional state changes and is NOT yet reconstructed;
   native integration must supply its real side effect through the
   explicit host callback rather than silently skip it.
4. Emits Source game event **`dynamic_difficulty`** via original helper
   `0x222B64D0`. The confirmed event fields are
   `descriptor=0`, `owner` (local player-index lookup or -1),
   `advisor=-1`, `name`, `decision` (the original
   EstimatedDifficulty integer), `value` (EstimatedTime float),
   and `x/y/z` (trigger world origin from +0x2BC).
5. If the low byte of `EndOfGame` is nonzero, uses manager getter
   `0x222BC4A0` then calls `0x222B93E0`, setting manager byte
   `+0x11` to **one**.
6. Unconditionally requests queued entity removal via
   `0x22286BF0`, including when event-manager allocation returned null.

`CTriggerSecret`, original `0x221CCC10`:
- The same touched-actor virtual player predicate.
- Resolves targetname/classname fallback.
- Emits Source game event **`game`**, with
  `descriptor=12`, `owner=-1`, `name`
  via original `0x222B63B0`.
- Requests queued entity removal even if event creation failed.

The secret's separate **spawn** method `0x221D0790` also checks an
original +0x120 bit and may emit the same `game` event with descriptor
**11**. That spawn-time branch is source-authenticated but NOT yet
implemented by the portable trigger adapter. Likewise, the real
base Trigger spawn, area touches, collision filters, special spawnflags,
game event allocation, and actual Source save/restore still belong to
the native engine host.

## Independently authored portable gameplay implementation

`include/sin_compat/original_sin_trigger_entities.hpp` and
`src/original_sin_trigger_entities.cpp` implement both real trigger
event/action sequences through `IOriginalTriggerHost`:

- The host **must first create** a real live entity from an
  engine-installed Source-compatible factory; the adapter will not
  alias classes or fabricate CBaseTrigger memory.
- Registration through the existing `IGameEntityRegistrar` supports
  both real classnames and source-backed `SpawnCallback` objects, with
  unregister rollback and lifetime handling.
- Generation-checked entity references prevent reuse of an old entity
  slot as the wrong new actor.
- Host receives typed Event payloads and independently supplies a real
  game-event-manager publisher, manager flag setter and deferred
  removal. The three real DATADESC KeyValues can be configured through
  a strict, bounded map-value parser (a **new** safety policy, not the
  2006 game's exact numeric lexer).
- Event publish failure still requests original removal; a failed
  deferred deletion remains pending for a retry without emitting the
  event again. The original may have other same-tick reentrant
  semantics; reentrancy rejection and idempotence are **independent
  safety choices**, not identical 2006 game source.
- No SourceVR/Android/Quest3 callback pointer or original x86 vtable
  pointer is invoked by this cross-platform library.

Regression `sin_original_gameplay_triggers_tests` covers real
`EstimatedTime/EstimatedDifficulty/EndOfGame` map keys, exact event
field values, non-player rejection, script-to-manager sequencing,
byte-only `EndOfGame` semantics, missing event manager, queued
removal refusal/retry, generation mismatch, reentrancy, unknown
classnames and failed registry rollback.
`sin_original_gameplay_triggers_binary_audit_synthetic` checks
synthetic mutation of all three original DATADESC field/type/name
claims and rejects untrusted DLLs.

## Honest integration milestone

This code is **new functioning portable gameplay logic** with
synthetic-host tests and original binary provenance, but the original
Windows engine has not loaded it as a real CBaseTrigger subclass.
The distributed independently compiled `server.dll` currently
fails closed before normal ServerGameDLL003::DLLInit success, and
the original HL2 map-port logs reflect an engine without these
SiN-specific C++ entities. Correct full gameplay will require an
engine-compatible native entity class registration, collision/touch,
network send tables, level scripting and save/restore. Those remain
the next integration targets, **not** shared physics or model loaders.

Run locally against private original DLLs to authenticate:

```sh
python3 research/tools/audit_sin_original_gameplay_triggers.py --client /private/client.dll --server /private/server.dll
python3 research/tools/audit_sin_original_gameplay_triggers.py --self-test
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build --target sin_original_gameplay_triggers_tests
ctest --test-dir build -R sin_original_gameplay_triggers --output-on-failure
```
