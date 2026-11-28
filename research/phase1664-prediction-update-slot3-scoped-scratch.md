# Phase 1664: original CPrediction::Update slot3, scoped work buffer and inner slot22 ABI

## Original provenance and scope

This phase is a read-only static decompilation of exactly **164 original
2006 x86 function bytes**, combined with original-free synthetic native
MSVC Win32 x86 tests. It is stacked on Phase1663 PR #1672.

Original owner-supplied unchanged `client.dll` complete SHA256:
`6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.
Original `engine.dll` complete SHA256:
`f3add5b41300e6d709fd0b492bfcdd37b9b3d20d9744f874c0b860bf021cec81`.
Both are authenticated in the Phase1658 parent audit before original
code bytes are examined. No original DLLs or executable method bodies
are committed.

The original 23-slot `CPrediction` primary vtable is at
`0x242A363C`. Vtable **slot3** points to
`0x24197210`, a whole method of **164 bytes**. Its complete
SHA256 is
`0636e37880c589bef9f874d6d259763a61aa1eceb4fe1b7cba58dbbe68d463e6`.
The code ends at native `RET 0x10` and is followed immediately
by compiler INT3 padding.

The **actual original function name** is authenticated by its native
profiling literal, at original RVA `0x2A39A8` / VA
`0x242A39A8`: `CPrediction::Update`. Its profiling scope uses
original literal `Prediction` at VA `0x242A39BC` and tier0's
`CVProfile::EnterScope` / `ExitScope` imports.

This method has four stack DWORD arguments and returns with
`__thiscall RET 16`. It is not a request for physics/material
reimplementation. Its internal virtual callee slot22 at native
`0x241977B0` remains **opaque** in this phase.

## Exact identified Update behavior

Original assembly, checked by a complete function SHA and sparse
opcode witnesses, performs the following:

1. Enter the original `CPrediction::Update` profiling scope.
2. Load input argument1 into EAX and compare it with prediction
   object `this+0x24` (the last frame/index field already independently
   observed in Phase1658's reset). It initializes a local byte flag
   to one. When this frame/index equals argument1 **and** two original
   ConVar-parent numeric DWORDs are nonzero, the local byte is
   overwritten with zero. The two ConVar parent pointer cells are
   VA `0x24374780` and `0x24374540`, with numeric field
   at `parent+0x2C`. This local flag's downstream uses are not
   inferred from the native method without evidence.
3. Store argument1 in `this+0x24`.
4. Read the **global pointer cell at VA `0x242FA458`**. This
   cell's initial on-disk pointer value is `0x242FA464`,
   a backed client `.data` location. It is *not* the scratch
   DWORD array itself. The original source pointed to by this
   cell is copied to 11 local DWORDs using
   `mov ecx,0x0B` / `rep movsd`, saving **44 bytes**.
5. Call **virtual CPrediction slot22** at vptr byte offset
   `+0x58`. The four stack args supplied by the actual
   compiled callsite are **the first saved work DWORD**,
   argument2, argument3, and argument4. Original argument1
   has instead been stored at `this+0x24`. This distinction
   is tested explicitly; no invented fourth-argument mapping.
6. Read the global pointer cell at VA `0x242FA458` **AGAIN
   after slot22 returns**. Copy back the previously saved
   11 DWORDs into the newly read target via another
   `rep movsd`. Thus an opaque callback that changes the
   global pointer would affect the **restore destination**.
   The original has no concurrency / null-pointer guard.
7. Exit the profiling scope, restore registers and return,
   cleaning four stack DWORDs with `RET 0x10`.

The method does not itself implement slot22's gameplay prediction
body. Nor is the 44-byte work-buffer type or its original
full meaning inferred from the arbitrary DWORD contents.

The original PE32 HIGHLOW relocation blocks authenticate the
primary slot3 vtable cell, pointer-cell initializer and ten
original native addresses (profile, strings, ConVar pointers and
both work-pointer loads).

## Original-free C++ fixture and tests

The existing 23-slot native `Prediction23Fixture` now gives its
previously inert slot3 the original four-stack-DWORD signature,
`UpdateFrame(frame,arg2,arg3,arg4)`. The last-frame field at
`this+0x24` stays in the same native offset, and the synthetic
ConVar flag calculation is exposed only for regression.

A new **host-owned** `UpdateFrameContext` contains a pointer to a
11-DWORD `std::array<uint32_t,11>`, two synthetic ConVar values
and three strictly optional callbacks for scope enter, inner
slot22 and scope exit. It is separate from original game memory.
The fixture's slot22 is now a **four-argument opaque callback**
with the correct native stack pop contract, rather than a falsely
invented implementation of retail `0x241977B0`.

`test_original_client_prediction_update_slot3_scratch.cpp`
is a genuine original-free MSVC x86 runtime test. It executes
compiler-generated 23-slot virtual calls with ECX=this,
four actual x86 stack DWORDs and before/after ESP invariant
checks. It verifies:

- original slot3/slot22 distinct vtable entries, four-arg `RET16`;
- native `this+0x24` field offset and changed-frame state;
- identical frame with both ConVars enabled versus either disabled;
- profiling enter, inner virtual callback, restore, exit ordering;
- exact 11-DWORD saved source, **saved DWORD[0]** plus input
  argument2–4 forwarding to the opaque virtual;
- a callback that writes all 11 synthetic DWORDs and then
  returns, with each word restored from the saved copy;
- global-work-pointer retargeting **during** the opaque virtual:
  restore is to the *replacement* buffer and old buffer is
  intentionally left modified, matching the native reload;
- fail-closed synthetic guard if the original or replacement work
  pointer becomes null, and callback detachment/lifetime.

These last two original null-pointer situations would crash in
the original x86 routine; the test uses only owner-owned invented
memory and deliberately refuses unsafe accesses instead.

`research/tools/audit_sin_original_prediction_update_slot3_scratch.py`
authenticates the original dual-full-file SHA gate, whole 164-byte
method hash, original slot3/22 vtable identity, native profile
labels, 11-DWORD work-buffer pointer, both copies, stack
reorder, ConVar gating, profile imports and relocation witnesses.
Public `--self-test` constructs invented **nonexecuting sparse x86
buffers** and mutates every covered byte, including wrong length
and forged original retail-DLL SHA refusal. It never includes a
complete native original function.

One synthetic Python CTest and one native MSVC x86 CTest raise
the stacked suite from **274 to 276**. The original decomp
workflow also calls the new source auditor self-test.

## Explicit non-goals

This reconstructs one **wrapper/lifetime boundary**, not all
virtual prediction. Especially, the inner original
`CPrediction` slot22 body is still an opaque dependency.
Original Source engine lifetime, local-player construction,
gameplay prediction parity, Win32 original retail game boot,
Android native executable and Quest 3 VR startup are not
claimed.

The replacement client still does **not** register an incomplete
`VClientPrediction001` factory. No physics, surfaces, props,
models, their loading or rendering are implemented, under
the user's active project priority override.

All public GitHub CI remains original-free. PR deliberately
stays open and stacked until exact-commit CI is verified.
