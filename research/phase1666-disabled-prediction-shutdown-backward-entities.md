# Phase 1666: original disabled-prediction predictable-entity shutdown

## Provenance and recovered source

This phase is a read-only audit of owner-supplied unchanged 2006 PE32
SiN Episodes files and an **entirely original-free** native Windows
MSVC x86 regression. It stacks on Phase1665 PR #1674, which proved
that disabled `cl_predict` in original
`CPrediction::PreEntityPacketReceived` calls
`0x24195720`, whereas disabled
`PostEntityPacketReceived` does not.

Original unchanged file SHA256 gates from the parent original audit:

- `client.dll` complete SHA256
  `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.
- `engine.dll` complete SHA256
  `f3add5b41300e6d709fd0b492bfcdd37b9b3d20d9744f874c0b860bf021cec81`.

### One additional full original native function

The original disabled-prediction helper at client VA
`0x24195720` is a **118-byte x86 method** ending in
`RET`, with whole-method SHA256
`88a6354f6a65291b7036c5305c67e3a307ce828fb1196172af78b31fccbe9f8f`.
It has no explicit stack parameters. The original source
execution path is not a simple return and is not the same
thing as a generic reset.

The original `client.dll` source does the following:

1. Fetch the original client entity-list pointer at
   `0x242F7EC8`, invoke its count virtual at vtable
   byte offset `+0x04` **once**, then compute
   `lastIndex=count-1`.
2. **Iterate BACKWARDS**, from lastIndex to zero
   (inclusive), invoking the entity-list `Get(index)`
   virtual at byte offset `+0x00` for every index,
   and skip null returned entity pointers.
3. For each non-null entity, call the original
   `C_BaseEntity` single-byte `IsPredictable` getter
   at `0x240C5500`, previously SHA-attested in
   Phase1665 (reads entity+`0x2E8`).
4. **Predictable entity:** invoke authentic native direct
   callback `0x240CA9F0` and increment the predictable
   shutdown counter.
5. **Nonpredictable/client-created entity:** access
   the **embedded subobject at entity+0x08**, read
   its own vptr, call vtable byte offset `+0x04`
   (slot1) with ECX=entity+8, and increment the
   client-created shutdown counter.
6. **After the loop only**, if either counter is positive,
   call the original imported formatting/printing function
   at IAT `0x242761EC`, using the exact on-disk string:
   `Shutdown %i predictable entities and %i client-created entities`
   (terminated by newline), with the **predictable count
   first**, client-created count second. With no
   qualifying non-null entities, no summary is printed.
7. Restore saved registers and return, **RET0**.

The original message string resides in client .rdata at
`0x242A3560`. It is authenticated exactly, together with the
complete native method SHA, its INT3-padded endpoint, and
the original client-list/data/format/import PE32 HIGHLOW
relocation sites at method-relative offsets
`+0x02`, `+0x22`, `+0x65`, `+0x6B`.

The direct call from the original Pre method at VA
`0x24195FC4` points to this exact helper.
No class identity or full behavior is invented for the
opaque original `0x240CA9F0` predictable cleanup,
or the embedded client-created object's virtual method.

## Original-free implementation

`pc/independent_slot0/native_original_prediction_disabled_shutdown.h`
defines a tiny host-owned callback model, **not an
original game-class implementation**. It uses the
source-matched descending traversal, the same count/get and
predictability classification, two independent callbacks
and two separate shutdown counters, and the same conditional
summary order. The original method does not validate
external pointers or suspicious counts; the synthetic
facade **refuses missing callbacks, negative counts and
more than 4096 synthetic entries** for safety.

`test_original_prediction_disabled_shutdown.cpp` is a
real Win32 MSVC x86 executable. It uses invented entities,
an invented client entity-list Count/Get, and a
**two-virtual-slot compiler-generated embedded controller
at entity+0x08** to make the nonpredictable branch's
`__thiscall vptr+0x04` ABI concrete, including ECX/ESP
stack invariants. Each host-created entity has an
independent predictable byte at the original `+0x2E8`
layout offset.

The test covers descending indices `4,3,2,1,0`,
two predictable entities, two client-created entities,
one null slot, precise event ordering, the summary
`(predictable=2,clientCreated=2)`, normal zero/all-null
lists with no log, negative/oversized count refusal,
missing-backend fail-closed behavior, and original-free
object lifetime.

It also attaches the shutdown model to the previously
source-authenticated native CPrediction vtable
`slot4 PreEntityPacketReceived(uint32_t,uint32_t)`,
then calls its actual function pointer with
**ECX=this and RET8** under `cl_predict=0`. The same
prediction instance's slot5 Post noarg virtual is
called with **RET0** and must NOT run the disabled
fallback. Enabling `cl_predict` without a local player
also must not run this cleanup.

## Audit and workflow

`research/tools/audit_sin_original_prediction_disabled_fallback.py`
authenticates both original whole-DLL SHA gates, the original
whole 118-byte method SHA, original client entity-list globals,
authentic direct calls and embedded-vptr sequence, exact summary
string/count order, PE32 relocation sites, native method
boundary, and original Pre callsite. Public `--self-test`
constructs invented **non-executable sparse x86 byte windows**
and checks every covered opcode/operand mutation and
forged-original rejection, without copying a complete
commercial method body or loading game code.

The original decomp synthetic workflow runs the new auditor
on invented data. CMake registers one source-audit Python
CTest and one native Win32 x86 executable CTest, raising
the stacked target from **278** to **280 CTests**.

## Limitations

This closes the original **disabled-prediction cleanup
dispatch boundary**. It does NOT implement the opaque
per-entity shutdown routines, original network prediction
logic, normal retail Windows startup, client/server engine
ABI parity, Android runtime, or Quest 3 VR gameplay.

The replacement client still does not register its
incomplete `VClientPrediction001` factory. No physics,
surfaces, models, props, materials or rendering work
was added, per the user's active project override.

The PR intentionally remains open and stacked until
exact-commit original-free CI has completed.
