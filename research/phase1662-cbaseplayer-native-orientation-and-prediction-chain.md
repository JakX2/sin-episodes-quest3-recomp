# Phase 1662: original C_BasePlayer player-angle native virtual method contracts

## Source and trust boundary

The owner-provided, unchanged 2006 SiN Episodes `client.dll` complete
SHA256 is
`6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.
The original `engine.dll` complete SHA256 is
`f3add5b41300e6d709fd0b492bfcdd37b9b3d20d9744f874c0b860bf021cec81`.
Both are authenticated by the existing Phase1658 read-only parent
before inspecting any original virtual target in Phase1662.
No binary or copied original function implementation is shipped.

Phase1659 established that the current player global
`0x24338CF0` refers to original `C_BasePlayer` by its original
local-player writer, conditional destructor clear and MSVC RTTI.
Phase1661 established CPrediction slots 9–12 dispatch to four
C_BasePlayer primary virtual offsets `+0xE4`, `+0xE8`,
`+0xEC` and `+0xF0`. This phase resolves those original
player vtable pointers to their actual implementations.

## Four original primary-vtable methods authenticated

The original `C_BasePlayer` primary vtable is at client
VA `0x2428AAE4`. Four additional whole-method native SHA
windows were authenticated, 216 bytes total:

| Player slot / vtable offset | Original function | Bytes | Entire original SHA256 |
|---|---|---:|---|
| 57 / `+0xE4` | `0x240C5190` | 7 | `58beed9442fa84e2898a6468a1096bece7c950612ede928be4c9c05d280b198e` |
| 58 / `+0xE8` | `0x240C51B0` | 101 | `3f4bb4faf68496fd5f46ccde0790bcb116909152c50e74c7b124dd3b451fd5b3` |
| 59 / `+0xEC` | `0x240C5260` | 7 | `6a8d91e1da08718432a2b232c0bd73c760a9437e0182a6a3a39529cbdb21f11b` |
| 60 / `+0xF0` | `0x240C5280` | 101 | `a0de64a8e7a4a04f9f85f423e922323163a13f73b36c010eacfe1a4942db953e` |

Each pinned primary vtable cell is a genuine PE32 HIGHLOW
relocation. The original methods reside in .text and end
before the original compiler INT3 padding. This provenance is
additional to Phase1661's four CPrediction wrapper methods.

### Original getters: player slots 57 and 59

Both methods are only seven bytes and take no stack args,
returning a pointer in EAX, not the actual three-DWORD value:

- slot 57 returns `this+0x220`, plain `RET`.
- slot 59 returns `this+0x25C`, plain `RET`.

This is why original CPrediction getters slot9 and slot11
receive a pointer from those virtual calls and then copy
exactly three DWORDs into their own caller's output.

### Original setters: player slots 58 and 60

Both are 101 native bytes and accept exactly one 32-bit pointer
argument, returning via `RET 4`. Their behavior is NOT equivalent
to unconditional bitwise copying.

Each compares the existing three **IEEE-754 binary32 floats**
against the three incoming floats, one component at a time,
using original x87 `FUCOMPP` / `FNSTSW AX` followed by
`TEST AH,0x44` and the specific parity branches.

- When **all three values compare numerically equal**, return
  without invoking the dirty helper or writing any new data.
  Positive and negative floating zero compare equal, even though
  their raw DWORD representations differ.
- If any value compares **unequal or unordered**, go down the
  changed path. Every NaN comparison is unordered, even when
  both NaN DWORDs are bitwise identical, so an identical
  three-word NaN source still triggers the change path.
- On change, first call authentic helper `0x24096290`,
  with flag **1** (slot58) or **2** (slot60), then copy the
  caller's three raw DWORDs to the corresponding player fields.
  Slot58 writes `+0x220`/`+0x224`/`+0x228`;
  slot60 writes `+0x25C`/`+0x260`/`+0x264`.
  The original helper runs **before** those three stores.
- The complete semantics of original dirty helper
  `0x24096290` are NOT claimed by this phase. Its original
  callbacks and engine-side dependencies are represented
  only by a synthetic notification hook.

The original setters do not guard a null source pointer.
The original-free fixture refuses null rather than triggering
an access violation.

## Source-free audit and Windows regression

`research/tools/audit_sin_original_cbaseplayer_orientation_native_methods.py`
authenticates both original whole-DLL SHA gates, all four whole
original method SHA digests, the exact four primary-vtable
targets, relocation sites, native boundaries and sparse x86
instruction witnesses. It checks the original dirty-helper
direct relative-call target, flag 1/2, float-compare parity
branches, three field DWORD store offsets, and stack cleanup.

Its public `--self-test` creates invented non-executable sparse
x86 fragments and mutates covered opcode/operand witnesses,
rejects incorrect helper targets, flags, function extents and
forged original image SHA. The source contains NO 101-byte
original method body.

`pc/independent_slot0/native_client_player_orientation_61_probe.h`
implements an entirely new MSVC x86 fixture with **61 ordered
virtual slots**, the first 57 deliberately inert. Slots 57,
58, 59, 60 have the exact observed pointer/no-pointer native
signatures. Synthetic player fields at `+0x220`/`+0x25C`,
notifiers at `+0x22C`/`+0x268`, mirrors at
`+0x2C8`/`+0x2D4`, and a synthetic helper callback
model original offsets without using real engine memory.

Its deterministic `OriginalFloatEqualBits` routine models
the observed x87 *numeric* equality rules without dependence
on host SSE FTZ/DAZ modes. A whole-vector comparison triggers
one helper callback before three word copies when necessary.

`test_original_client_player_orientation_61_chain.cpp`
is an actual MSVC Win32 x86 native regression, NOT just a
portable model. It calls both **compiler-generated vtables**
through their real function-pointer slots, with ECX=this,
stack argument layout and before/after ESP checks:

- original-shaped 61-slot player GetA/SetA/GetB/SetB;
- original-shaped 23-slot prediction GetA/SetA/GetB/SetB;
- full prediction virtual slot10 -> player slot58 ->
  dirty flag1 -> mirror `+0x2C8` -> notifier
  `+0x22C` virtual slot3;
- full prediction virtual slot12 -> player slot60 ->
  dirty flag2 -> mirror `+0x2D4` -> notifier
  `+0x268` virtual slot3;
- direct player signed-zero equality (no dirty/writes),
  raw-bit-identical NaN unordered change (dirty again),
  unrelated B change and all-three-equal no-op;
- **outer CPrediction still mirrors and notifies even when the
  inner C_BasePlayer setter found no change**;
- normal no-player fallback, null-input safety refusal,
  detachment and no original game method calls.

One public original-free Python CTest and one native x86
CTest are added, raising the stacked target from Phase1661's
270 to **272 CTests**. The original-free decomp synthetic
workflow also calls the new audit.

## Explicit non-goals

This is concrete decompiled behavior for 216 additional
retail C_BasePlayer method bytes, and a fully synthetic native
x86 ABI/ordering test. It is not complete C_BasePlayer,
not production prediction, not engine scheduling, not
retail Windows boot, and not Android/Quest 3 startup.

No original commercial machine code or DLL images are
committed to GitHub. Under the user's priority override,
this phase does not implement physics, surfaces, props,
models, or their rendering/loading code.
