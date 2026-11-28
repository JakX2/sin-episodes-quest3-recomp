# Phase 1669: original C_BaseEntity PostNetwork result and ring-contract boundary

## Why this follows Phase 1668

Phase1668 PR #1677 authenticated the original 2006 PE32
`CPrediction::PostNetworkDataReceived` virtual slot6 at
`0x241960E0`, 1,039 bytes. It established that each
predictable entity is passed one frame argument to
an original direct native callback at `0x240C6900`.
The original caller examines that callee's return
byte AL and sets prediction `this+0x30` to one
if ANY eligible entity returns nonzero.

This phase authenticates **that original per-entity
callee** and creates an original-free native x86
contract for its verified outer conditions.
It does NOT invent the full underlying difference
comparison helper code.

The unchanged 2006 original full-file SHA gates
are inherited from parent Phase1658:

- `client.dll` complete SHA256
  `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.
- `engine.dll` complete SHA256
  `f3add5b41300e6d709fd0b492bfcdd37b9b3d20d9744f874c0b860bf021cec81`.

No retail game DLL is bundled or invoked in public CI.

## Source-authenticated complete 192-byte method

Original C_BaseEntity direct native callback:
`0x240C6900`. Its **entire** method extends
through `0x240C69BF`, length **192 bytes**,
whole SHA256:

`c16da945561c4d57e3bfcfc148bdfe913879116d2e144ee315ff3cbb1f8b4fc1`

This exact length matters: the final `RET 4`
occupies three bytes, `0x240C69BD..0x240C69BF`.
A different native function begins immediately
at `0x240C69C0`, with instruction prefix
`81 C1 6C 01 00 00 E9`. There is **no INT3
padding** between these two original methods.
The audit explicitly authenticates this
exception rather than assuming generic
compiler padding.

The complete method SHA and all **three**
internal PE32 HIGHLOW relocation sites
`0x240C6912`, `0x240C6921` and
`0x240C6997` match the untouched owner
original file. The original source includes
profile marker literal
`PostNetworkDataReceived` at
`0x24288FA8`, and direct caller
`0x241961C3` inside the authenticated
Phase1668 PostNetwork method.

## Original control-flow / ABI contracts

The method accepts **one 32-bit stack DWORD**
(the frame/index originally passed by prediction),
with `ECX=C_BaseEntity`, and returns with
`__thiscall RET 4`. The result is a Boolean
in AL:

1. Call original opaque native helper
   `0x240C5960` with the original profile name.
   This occurs even if the frame is nonpositive.
2. Load an original ConVar parent pointer from
   `0x24374618` and read its numeric value
   at `parent+0x2C`. The original code has
   additional negative-debug and embedded
   object virtual conditional paths that are
   **NOT** reconstructed here.
3. Check whether the input frame is **signed
   greater than zero**. With frame zero or
   a negative signed value, bypass the expensive
   comparison path, returning AL zero.
4. For a positive frame, inspect original
   entity `this+0x454` history-availability
   field. If zero, use a zero history word.
   Otherwise compute ring index
   **`(frame-1) % 90`** (the original uses
   x86 `IDIV` with divisor hexadecimal
   `0x5A`, 90 decimal) and read the DWORD
   at original entity
   **`this+0x2EC+index*4`**.
   The 90 DWORD ring spans +`0x2EC` through
   +`0x450`, ending before +`0x454`.
5. Call original native helper `0x241978A0`,
   with source-attested stack setup including
   the history value. Its complete behavior
   and exact high-level meaning are **opaque**.
6. Invoke original C_BaseEntity **primary
   virtual byte offset +0x40**, **slot16**,
   without stack args; it returns a pointer
   in EAX. This pointer is forwarded with
   other original operands to the second
   opaque native helper `0x24199BA0`.
7. The result of that helper is compared as
   **signed EAX > 0**. Return **AL=1** only
   if the helper's count/result is strictly
   positive. If zero or negative, return
   **AL=0**. The success and failure branches
   both clean the original one-argument stack
   with `RET 4`.

This is the precise Boolean dependency that
Phase1668's source-backed loop accumulates
into prediction `this+0x30`. It is not evidence
that the deeper comparison body is complete.

## Original-free source audit and nested native x86 tests

`research/tools/audit_sin_original_postnetwork_entity_compare.py`
authenticates dual original whole-DLL SHA,
both caller and callee whole-method SHA,
original direct rel32 callsite, the whole
192-byte native callee, 14 disjoint x86
opcode/operand witness checks, three genuine
HIGHLOW sites, original profile string,
signed-frame branch, ring-IDIV path,
two opaque helper calls, real primary
vptr+`0x40` call and positive-count return.
It explicitly verifies the next adjacent
native function's first seven opcode bytes
instead of erroneously demanding INT3 padding.

Its public `--self-test` constructs invented
**non-executing sparse x86 byte islands** and
mutates every covered witness byte. It also
refuses wrong method extents and forged
original complete-image SHA.

`pc/independent_slot0/native_original_postnetwork_entity_compare.h`
is a **strictly host-owned diagnostic model**,
with two separate opaque comparison helper
callbacks and one actual virtual slot16
getter callback. It reproduces only the
source-proven conditions and history index
and maps the opaque difference helper's
signed count to the original result AL.
Unknown original debug and embedded-object
paths are not filled with invented gameplay.

`test_original_postnetwork_entity_compare_chain.cpp`
is a genuine original-free MSVC Win32 x86
executable. It creates a **17-virtual-slot**
fake C_BaseEntity, with the original
source-attested memory layout at
`+0x2E8` (predictable byte), `+0x2EC`
(90-element ring), and `+0x454`
(history available). Synthetic primary
vtable[16] is called through the real
compiler-generated function pointer, with
ECX=this and before/after ESP invariant.

It tests frame ==0, signed negative frame,
frame1 -> ring index0, frame90 -> index89,
frame91 -> index0 wrapping, inactive
history using zero, missing opaque backend,
and signed difference count negative/zero/
positive producing exact AL0/AL1.

The test then composes that host-owned
callee with the previously source-pinned
23-slot CPrediction vtable[6], executing an
actual x86 **one-argument RET4** call.
Two predictable entities, one nonpredictable
and one null entity exercise the Count/Get
loop, two separate real entity vtable[16]
calls and an any-nonzero accumulation into
prediction `this+0x30`. A second call with
all result counts zero verifies that flag
remains zero. A missing local player skips
the entire entity comparison.

The CMake native suite adds one Python
synthetic SHA audit and one real MSVC x86
nested virtual-call regression, raising
the stacked target from **284 to 286 CTests**.
The original-free decomp workflow also
runs the new Python auditor self-test.

## Explicit non-goals and remaining dependencies

This phase is a source-authentic bounded
condition/history/return-value contract
for an original 192-byte native method,
NOT its fully reconstructed comparison
algorithm. Its opaque originals
`0x240C5960`, `0x241978A0` and
`0x24199BA0`, the negative-debug ConVar
and embedded subobject vtable callback,
and Phase1668's many later correction
branches still require independent analysis.

No original retail DLL is committed or
executed in CI. No game-ready production
VClientPrediction001, Windows original
retail gameplay startup, Android executable
or Quest 3 VR gameplay is claimed.
No physics, surface/material, prop, model
loading/rendering tasks were added,
per the user's active project override.

PR intentionally remains open/stacked
until 286-test exact-head CI is verified.
