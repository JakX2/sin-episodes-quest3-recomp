# Phase 499: original C_SoundscapeSystem object constructor and vtable field contract

## Why this matters for startup

The earlier phases independently reconstructed source method contracts:
Phase 489 Shutdown, Phase 494 original manifest/file filesystem ABI,
Phase 495 Init true-return and map-resource lifecycle,
Phase 496 the 61-byte ConVar cache and Phase 498 an opt-in real
owner-retail read-only ConVar lookup test. None can safely operate
on an actual C_SoundscapeSystem object without knowing which
fields were originally constructed, initialized or left opaque.

Phase 499 authenticates its original object constructor and exact
vtable and models only observed field writes. This is a tangible
object-lifecycle prerequisite, not a claim of completed game boot.

## Private SHA-original evidence

The owner's exact original 2006 client.dll:
SHA256 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d.

| Original structure | VA | Bytes | Complete SHA256 |
| --- | --- | ---: | --- |
| C_SoundscapeSystem constructor | 0x240F3BC0 | 120 | 0d9e47028fda189671b406f07549c147f3bfebebd24291e4460221c210ee96f0 |
| Original system vtable | 0x24293C34 | 48 (12 DWORDs) | 049c45c9002710aedfaef967c4c4aaebbc81bbc645528e4a5dea7c2735ed02b8 |
| Original vtable slot7 method | 0x240F3CA0 | 12 | f3271ecc3bb1ddf88123e9cdf1e011d9495c1790230626c6d67c1df782795565 |

The constructor at 0x240F3BC0 has these authenticated DWORD writes:

| Original this-offset | Written value |
| --- | --- |
| +0x00 | vtable VA 0x24293C34 |
| +0x04 | 0xFFFFFFFF |
| +0x08..+0x2C, DWORD stride | ten zero DWORDs |
| +0x30 | embedded subobject vtable VA 0x2428A9B0 |
| +0x9C | 0xFFFFFFFF |
| +0xA0..+0xC4, DWORD stride | ten zero DWORDs |

No evidence from the constructor alone justifies zeroing every
other field. In particular, +0x94 is NOT assigned by this 120-byte
method, and the class extends beyond +0xC4. The original update
routine reads fields +0xD0 and +0xD4, so the actual object
allocation/layout must not be equated to the model's 0xC8-byte
known span.

Original system vtable (48 bytes):

| Slot | Implementation VA | Proven limited behavior |
| --- | --- | --- |
| 0 | 0x240F4160 | 343-byte original Init, Phases 494/495 |
| 1 | 0x240F3EE0 | 267-byte original Shutdown, Phase 489 |
| 2 | 0x240F3DF0 | Virtual wrapper/destructor-like chain; needs complete lifetime analysis |
| 3 | 0x240F3C40 | 61-byte two-ConVar cache, Phase 496 |
| 4 | 0x240F3C80 | Single RET instruction |
| 5 | 0x240F3E10 | Tail jump to original 0x240F3D10 |
| 6 | 0x240F3C90 | Single RET instruction |
| 7 | 0x240F3CA0 | 12-byte external-global+4 to this+4 DWORD copy |
| 8 | 0x240F3CB0 | Single RET instruction |
| 9 | 0x240F3CC0 | Single RET instruction |
| 10 | 0x240F4D30 | Complex update routine (not reconstructed here) |
| 11 | 0x240F3E20 | Cleanup/deallocation-like chain; requires lifetime analysis |

Slot 7 reads a pointer from original global 0x242FA458;
reads its +0x04 DWORD; stores that value at this+0x04
and returns. The method has **no null guard**. The original
semantic name of the global and this+0x04 field is not yet
proven, so this report deliberately calls it a *global+4 copy*,
not a guessed level token, soundscape ID or frame counter.

Likewise, slots 4/6/8/9 have complete bodies consisting
of RET; their canonical Source SDK lifecycle names are
not inferred from apparent no-op behavior.

## Proven early guards for original per-frame vtable slot10

This phase also verified the *entry guard only* of the much larger
method at original 0x240F4D30, vtable slot10:

- At 0x240F4D36 it compares **BYTE this+0xD0** with zero;
  if zero it jumps directly to 0x240F50B7, skipping update work.
- Otherwise at 0x240F4D43 it loads the **DWORD pointer global**
  at 0x24338CF0; if null it jumps to the same return path.
- Only when both conditions permit entry does the method
  proceed into its much larger vector/query/update logic.

The owned CheckOriginalSlot10EarlyGate helper reproduces
ONLY these two ordered conditions with caller-supplied independent
booleans. It cannot call the original method, does not dereference
the retail global, and does not claim the remaining update has
been reconstructed. Its private original-opcode audit verifies
six instruction windows and the exact branch destinations;
its fabricated public self-test rejects corrupted guards.

## Independently authored code

client_soundscape_original_object_ctor.h defines:
- Exact original metadata for 12 vtable entries, two
  complete method SHA-pinned VAs and the embedded vptr.
- KnownDwordModel: an **independent** 50-DWORD storage span
  corresponding to source-observed offsets 0x00..0xC4.
- ApplyOriginalConstructor: writes ONLY the twenty zero
  DWORDs, two -1 sentinel DWORDs and two vtable pointer
  metadata words. It preserves every unknown simulated field.
- ApplyOriginalSlot7GlobalPlus4: takes an **already trusted
  independent DWORD** and copies it to model offset +0x04.
  It does not dereference an original global, parse an
  engine-owned object or call real game methods.
- OriginalVtableSlotIsSingleRet: source-authenticated stub
  classification only, not a claim that any engine callback
  should be invoked or skipped.

Native MSVC x86 fixture
test_client_soundscape_original_object_ctor.cpp
checks offsets/sentinels/vptr values, all 20 exact source zero
writes, preservation of opaque fields, slot7 write isolation,
reconstruction repeat and four genuine one-byte-return slots.

Python SHA auditor
audit_sin_original_soundscape_object_ctor.py
has a private read-only --client/--archive mode validating
whole original client SHA, full original constructor SHA,
all 12 vtable pointer values, vtable SHA, slot7 entire SHA,
selected instruction windows and four one-byte RET methods.
Its public CI --self-test fabricates code windows and refuses
mutated constructor/slot7/vtable witnesses. No proprietary
DLL method bodies are committed.

## Still outstanding

This is a precise *known-field initialization model*, not a
binary-compatible C++ reconstructed original class:
- actual total object size and alignment need independent proof;
- unknown embedded subobject ownership/lifetime must be traced;
- full slot2/5/10/11 virtual method effects remain unfinished;
- normal engine ConVar callback registration, HUD, entity,
  sound event and audio playback interactions still need work;
- regular standalone VClient011::Init remains fail-closed;
- actual owner-observed retail Windows checkpoint remains
  0x53494E74, not 0x53494E76 or 0x53494E77.

The Phase 498 optional ConVar test is available separately
in the latest standalone one-click diagnostic, but has not
been confirmed in a real owner-run process at the time of
this phase. Android/Quest 3 gameplay remains downstream
of Windows startup and renderer/engine support.
