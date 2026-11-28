# Phase 1673: original gated PostNetwork byte comparison for cases 8 and 9

## Source identity

Phase1672 PR #1681 independently authenticated the original
1,036-byte typed PostNetwork per-node dispatcher at
`0x241996F0` and its 27-type-to-13-case lookup.

This phase isolates one compact source-authentic descendant
called from **two distinct cases** of that native dispatcher:
the comparison gate at original client VA
**`0x24197A90`**, exactly **56 native x86 bytes**.

The unchanged 2006 PE32 original-image provenance is the
same dual whole-file SHA-gate established in Phase1658:

- Original `client.dll` complete SHA256
  `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.
- Original `engine.dll` complete SHA256
  `f3add5b41300e6d709fd0b492bfcdd37b9b3d20d9744f874c0b860bf021cec81`.

### Whole original native method

| Property | Exact original evidence |
|---|---|
| Original method VA | `0x24197A90` |
| Original native length | **56 bytes** |
| Complete original SHA256 | `0fb74121fd315127c05255395992aae9e6737a74cee4133f2d1c3685947f0276` |
| Original native calling convention | nonvirtual `__thiscall`, ECX=this |
| Original explicit arguments | three DWORDs: byte count, buffer1 pointer, buffer2 pointer |
| Original native stack cleanup | `RET 0x0C` |
| Original PE32 HIGHLOW site count | **zero** |
| Original method boundary | ends at `0x24197AC7`, followed by MSVC INT3 padding |
| Original case8 direct CALL | `0x24199A2E` |
| Original case9 direct CALL | `0x2419992E` |

Both original direct CALL rel32 targets are verified against
the whole-function-SHA-authenticated Phase1672 dispatcher.
Neither native caller is inferred from a newer Source SDK.

## Exact native control-flow and source semantics

The original 56-byte method is sufficiently small to recover
the complete outer comparison rule:

1. First read dispatcher BYTE `this+0x14`.
   If zero, immediately return **EAX=0** with
   `RET 0x0C`, without reading any record or buffer.
2. Otherwise read the dispatcher current-record pointer
   at `this+0x18`. Inspect BYTE `record+0x13`
   and test bit `0x04`.
   If set, immediately return **EAX=1** with
   `RET 0x0C`, without reading either input byte buffer.
   This byte is the HIGH BYTE of the original
   WORD flags at `record+0x12`, so bit `0x04`
   corresponds to WORD mask **`0x0400`**.
3. Otherwise load all three explicit native args:
   DWORD byte count, first buffer pointer, second buffer pointer.
   Use native **`REPZ CMPSB`**, the original x86
   byte-at-a-time equality comparison that stops on mismatch,
   then `SETE AL` to return **1 if all requested
   bytes matched**, else **0**. Its native
   `RET 0x0C` cleans the three caller DWORDs.

An active comparison with a **zero byte count**
returns 1: the source x86 initializes EDX with
`XOR EDX,EDX`, setting ZF=1, and
`REPZ CMPSB` with ECX=0 performs no comparison,
so `SETE AL` yields 1. This behavior is
explicitly regressed.

### Cases 8 and 9 use different byte counts

The parent native dispatcher source provides the byte count
through the sign-extended original WORD at
`record+0x10`, which becomes ESI in the common
dispatch entry:

- **Case8**, jump target `0x24199A29`:
  pushes ESI unchanged as the first argument
  to comparison VA `0x24197A90`. The length
  is **signed record WORD+0x10 in bytes**.
- **Case9**, jump target `0x24199921`:
  executes two native `ADD ESI,ESI` instructions
  at `0x24199922` and `0x24199924`,
  multiplying the signed source length by **four**
  before its comparison CALL to
  `0x24197A90`.

These lengths are not interchangeable. The same two
buffers can compare equal over case8's short prefix and
different over case9's four-times-longer prefix.

The two original case blocks contain **additional
formatting, comparison and conditional copy methods**.
This phase reconstructs the isolated 56-byte gate,
not the complete case8/case9 behavioral pipeline.

## Original-free implementation and tests

New
`pc/independent_slot0/native_original_postnetwork_case8_case9_bytecmp.h`
defines a synthetic `ComparatorFixture` inheriting
the existing Phase1672 dispatcher fixture
**without adding a vtable**. It therefore
shares the same original this+`0x14`
enable-byte and this+`0x18` record-pointer layout.

The fixture's compiler-generated, nonvirtual
`__declspec(noinline) __thiscall CompareBytes
(count,buffer1,buffer2)` uses the exact
native argument count and return convention.
It preserves the source's enable/record-flag
priority, uses a bytewise equivalent compare,
and returns strict 0/1.

The original would dereference potentially invalid memory;
the public original-free facade **refuses raw pointers
not matched to an explicitly authorized host-owned
synthetic record**, null positive-length buffers and
counts larger than 4096 bytes. It permits null buffer
pointers on zero-length calls because the native
method does not read them. These checks are explicit
safety divergences, not claims about the original.

New
`pc/independent_slot0/test_original_postnetwork_case8_case9_bytecmp.cpp`
is an actual MSVC **Win32 x86** runtime test.

It covers byte equality and mismatch, changed bytes
outside the compared prefix, enabled/disabled gating
(including enabled false overriding force-true flags),
the `record+0x13` bypass, zero-length truth,
signed negative count safety refusal,
null/unowned pointers, and compiler-generated
native `RET12` ESP restoration.

The test also calls the original-free Phase1672
typed dispatcher in its actual native
**four-argument RET16** convention with
case8 and case9 records. It confirms ESI-derived
byte-count selection **N** versus **4*N**.

Finally it invokes the actual Phase1671
two-argument `RET8` native finalizer,
which processes two linked entries containing
case8 and case9 records and calls the Phase1672
four-arg dispatcher. Each nested case then
calls the new three-arg byte helper.
All three compiler-generated calling conventions
are checked in one original-free test:
**RET8 -> RET16 -> RET12**, with unchanged
owner-owned pointers and correct result 1/0.

New
`research/tools/audit_sin_original_postnetwork_case8_case9_bytecmp.py`
authenticates both original whole-DLL SHA,
parent original dispatcher whole SHA, both
case direct rel32 CALLs, complete 56-byte
original method SHA, original INT3 padding,
eleven sparse x86 structural witnesses,
record flag bypass, REPZ CMPSB/SETE and
the two case-specific length paths. It
constructs invented non-executing sparse
opcode islands for public `--self-test`
and rejects covered byte mutations, wrong
method extents and forged-original SHA.

One synthetic Python CTest and one real
MSVC Win32 x86 CTest raise the stacked
target from Phase1672's **292 to 294**.
The original-free decomp synthetic workflow
also runs the new auditor.

## Explicit non-goals

The original 56-byte comparison gate has a
source-faithful original-free native equivalent.
That does NOT reconstruct the remaining
case8/case9 pipeline, the other nine
specialized type branches, entire
PostNetworkDataReceived recovery logic,
or real original game startup.

No proprietary DLL code or full executable
native body is committed to public GitHub
or executed in public CI. No production
`VClientPrediction001` factory is exposed.
Windows retail gameplay boot and Android
or Quest 3 VR gameplay remain unverified.

Physics, surfaces/materials, prop/model
loading/rendering tasks remain skipped
per the user's active project override.

The PR remains open/stacked on
Phase1672 PR #1681 until exact-head
native CI confirms the 294-test target.
