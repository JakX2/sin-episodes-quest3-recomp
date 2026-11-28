# Phase 1667: original predictable C_BaseEntity shutdown, 61-byte method

## Original provenance and scoped behavior

The unchanged owner-supplied 2006 PE32 SiN Episodes files are
authenticated by the Phase1658 parent audit before original code
is interpreted:

- `client.dll` complete SHA256
  `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.
- `engine.dll` complete SHA256
  `f3add5b41300e6d709fd0b492bfcdd37b9b3d20d9744f874c0b860bf021cec81`.

Phase1666 established the direct original native
`CPrediction` disabled fallback `0x24195720` and
its reverse entity enumeration. For each predictable
entity, the original code directly calls
`0x240CA9F0`. This phase reconstructs **the original
61-byte C_BaseEntity cleanup method** to that direct
call boundary, without inventing its child engine methods.

The complete original function at
`0x240CA9F0` (61 native bytes) has SHA256:

`f8bc7dad78fd2752442ef4e7544464b618a377bfd3ba267c36c1ac4a742ec4d0`

The native method ends in a plain `RET`, immediately
before source-visible INT3 padding.

## Source-authenticated native operation sequence

Original disassembly, confirmed by its whole native
SHA and sparse ISA witnesses, has the following exact
order:

1. Preserve the entity's pointer and load a DWORD at
   `C_BaseEntity+0xB4`. The precise semantic identity
   of this handle is not invented; it is passed to the
   first opaque registry operation.
2. Use original global service object at
   `0x242F7F7C`, and call original native direct helper
   `0x240C98B0`. That helper's full engine/global
   consequences remain opaque in this phase.
3. Call another original native direct helper
   `0x240C5710` using the entity pointer.
4. **Before any primary entity virtual dispatch**, write
   byte value **zero** to the exact original predictable
   flag at `entity+0x2E8`. This is the same source-attested
   byte whose getter `0x240C5500` controlled selection
   into this predictable branch.
5. Call original entity PRIMARY vptr byte offset
   `+0x44`, which is **virtual slot17** (18th vptr
   entry), with ECX=entity and **zero explicit stack args**.
   The native callback's return value is in EAX.
6. Push the exact returned EAX word as the input for a
   final original direct helper `0x240C8F00`,
   using ECX=entity. Return via plain `RET`.

The **observable side-effect order matters**: the
entity is no longer marked predictable by the time
the slot17 virtual call executes. The final helper
receives the original virtual method's return DWORD.
None of the three direct helper bodies is assumed
equivalent to their counterparts in newer Source
engine versions.

The only original method-level PE32 HIGHLOW relocation
is the immediate reference to global service
`0x242F7F7C` at original method VA `0x240CAA00`.
All three direct relative-call target addresses,
the byte-flag store, virtual byte offset and full
method hash are authenticated. Source audit also
checks that the previous Phase1666 original fallback
directly calls this method from `0x2419575E`.

## Original-free implementation and tests

`pc/independent_slot0/native_original_predictable_entity_teardown.h`
defines a host-owned callback contract that independently
models each of the five ordered side-effect stages:
unregister -> primary cleanup -> clear flag ->
entity virtual17 -> final cleanup. It reads a
synthetic handle first and sends the return DWORD
of the vptr callback to the final callback. This is
**not** a production C_BaseEntity or original engine
registry implementation. Missing callbacks or entity
memory fail before any side effects.

`test_original_predictable_entity_teardown.cpp`
is an actual MSVC 32-bit Windows executable. It
creates a synthetic C_BaseEntity with **18 compiler
virtual slots**, the first 17 inert and the 18th
returning an invented unique token. The C++ object
places a DWORD at native `+0xB4` and the predictable
flag byte at native `+0x2E8`; layout is checked by
runtime pointer distances. The test invokes the
real generated vtable[17] via ECX=this and checks
before/after ESP (native plain RET0).

It verifies event sequence
**handle -> unregister -> primary -> flag clear ->
virtual17 -> final**, observes the flag zero during
the actual native virtual callback, checks the
return token reaches final helper, and tests null
entity/missing callback refusal without state changes.

The test then composes the prior Phase1666 backward
shutdown model with this original-free entity model,
including triggering it from a synthetic CPrediction
vtable[4] Pre packet **with cl_predict disabled**.
Reverse traversal across two predictable entities and
one nonpredictable/client-created entity must cause
two complete predictable teardowns, one separate
client-created callback, and one authentic summary
with predictable count first.

The source-only audit
`research/tools/audit_sin_original_predictable_entity_teardown.py`
authenticates dual complete original image SHAs,
whole 61-byte method SHA, original helper calls,
global HIGHLOW relocation, full source method boundary,
exact +`0xB4`/+`0x2E8`/vptr+`0x44` operands and direct
reference from the previous fallback. Public
`--self-test` mutates invented non-executable sparse
ISA windows; no original executable native body or
DLL files are included in Git.

One new Python and one real Win32 x86 CTest raise
the stacked target from Phase1666's **280 to 282**.
The original-free decomp synthetic workflow also
executes the new auditor.

## Limitations

This phase recovers 61 additional original native
method bytes and authentic dispatch order, but the
three original callees and actual engine entity
registry/state still require decompilation. The
class is only an original-free diagnostic fixture,
not a playable C_BaseEntity implementation.

This does not register production
`VClientPrediction001` or prove original retail
Windows gameplay, Android native startup, Quest3 VR,
or game prediction state recovery. No physics,
surfaces/materials, props, models or rendering are
implemented under the active project priority.

PR remains open and stacked on Phase1666 PR #1675,
with 282-test CI pending until exact-head results.
