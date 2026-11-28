# Phase 1674: original case8/case9 PostNetwork conditional byte correction

## Source identity and what this phase actually adds

Phase1672 PR #1681 independently SHA-authenticated the
**1,036-byte** original 2006 PE32 typed PostNetwork
dispatcher at `0x241996F0`.
Phase1673 PR #1682 authenticated the **56-byte**
comparison helper at `0x24197A90` called from
cases 8 and 9 of that dispatcher.

This phase does **not** claim two new independent native
retail methods. It source-authenticates **two inline
code regions** within the *existing* 1,036-byte dispatcher
and models one missing side effect: gated copying of
corrected bytes from the right-hand comparison buffer
to the left-hand buffer.

Original owner-supplied unchanged whole-file SHA gate:

- `client.dll` complete SHA256
  `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.
- `engine.dll` complete SHA256
  `f3add5b41300e6d709fd0b492bfcdd37b9b3d20d9744f874c0b860bf021cec81`.

### Two original, separately bounded native code regions

| Source region inside typed dispatcher | VA | Bytes | Entire region SHA256 |
|---|---|---:|---|
| Case8 post-comparison correction | `0x24199A33` | **41** | `25a10a5e47d2ab9cd80bec67062f5e49d6d4366dd080a9471e236b8d9fceda75` |
| Case9 post-comparison correction | `0x24199933` | **43** | `efec6fd7569f9b5b36274b039b2f7063fb50eb8246c5870fca72bf7d37810083` |

Both exact spans are source-code blocks within the already
authenticated complete original dispatcher. Each region has
zero original PE32 HIGHLOW relocation sites. The parent
dispatcher SHA256 remains
`71406e900eef6ba6d8abd37ab73f8730c71c35d95a8700a8cb4ab7c9a2b6a7a6`,
and the actual comparison helper SHA256 remains
`0fb74121fd315127c05255395992aae9e6737a74cee4133f2d1c3685947f0276`.

## Exact source-authenticated correction rule

Immediately after each case calls the original
`0x24197A90` comparison helper and receives
its return DWORD in EAX, the native code does:

1. Test original dispatcher BYTE `this+0x2C`.
   If zero, skip the correction entirely.
2. Compare the returned EAX against **1**.
   If EAX is exactly 1, skip the correction.
3. Otherwise, copy the requested bytes using the
   actual native `REP MOVSD` / `REP MOVSB` sequence:
   - **destination LEFT** in EDI;
   - **source RIGHT** in EBX;
   - first `floor(N/4)` DWORDs via
     `REP MOVSD`, then `N & 3` bytes
     via `REP MOVSB`.
4. Restore the original case's saved pointer/length
   registers and continue to the remaining original
   diagnostic/formatting methods, which remain
   **opaque** in this phase.

The correction-enable byte at `this+0x2C`
is distinct from the original comparison-enabled
byte at `this+0x14`:

- If comparison is enabled and buffers equal,
  the helper returns 1, so no copy.
- If comparison is enabled but buffers differ,
  the helper returns 0, and a nonzero
  `this+0x2C` triggers the correction copy.
- If the record's force-true flag at
  `record+0x13` bit0x04 makes the helper
  return 1, copying is suppressed even
  with differing buffers.
- Importantly, if comparison itself is **disabled**
  at `this+0x14`, the helper returns **0**,
  so source code still performs the correction
  when `this+0x2C` is nonzero.
  This two-flag interaction is source-proven
  and tested, not inferred from generic logic.

The byte count is still **different per case**:

- Original case8 (type8), raw signed
  record WORD `+0x10` byte count `N`;
- Original case9 (type9), same signed
  word multiplied by **four** via
  two source `ADD ESI,ESI` instructions,
  then copied as `4*N` bytes.

The native branches restore registers differently
after copying but preserve the exact observable
source/right-to-left buffer direction and
length, including the `N%4` trailing bytes.

## Host-owned implementation and native Windows tests

The existing Phase1672 original-free typed dispatch
fixture reserved accurate source fields only through
`this+0x25`; its synthetic callback pointer table
started at `+0x28`, overlapping the original
retail correction-enable byte `+0x2C`.

For Phase1674 the synthetic callback fields are
moved BEYOND the observed retail prefix: the
fixture explicitly reserves DWORD `+0x28`
and the original correction BYTE `+0x2C`,
plus padding through `+0x2F`.
A native `offsetof` assertion verifies that
`correction_byte` is at exactly `this+0x2C`.
Every previously authenticated early source
offset (`+0x00/+0x04/+0x08/+0x0C/+0x10/
+0x14/+0x15/+0x18/+0x1C/+0x20/+0x24/
+0x25`) remains unchanged.

New host-only
`ComparatorFixture::MaybeApplyCase8Case9Correction`
implements only the verified correction gate
and byte copy. It is an **extracted inline
behavior helper**, not an original executable
retail function or falsely claimed new
`__thiscall` slot. It refuses unauthorized
buffer pointers, overlaps and oversized copy
counts; ordinary invented disjoint buffers
are copied using `memcpy` because the resulting
bytes match the original forward REP MOVSD/
REP MOVSB sequence for those safe inputs.

`test_original_postnetwork_case8_case9_copy_fixup.cpp`
is a genuine original-free MSVC Win32 x86
runtime test. It executes compiler-generated
native typed dispatcher calls, the prior
three-argument gated comparison helper,
and the previous original-free linked-list
finalizer.

It verifies that `this+0x2C` zero prevents
copying on mismatch; enabling it copies exactly
the mismatched case8 `N`-byte prefix in the
original RIGHT->LEFT direction; an equal
comparison and a force-true record byte
both suppress copying; a disabled
`this+0x14` comparison can still trigger
copying when correction is enabled; case9
copies `4*N` bytes while preserving later
suffix bytes; one-byte and four-byte edge cases
cover the native MOVSB remainder versus
one MOVSD word; negative/huge lengths and
invalid host memory grants are safely refused.

A final native test invokes a two-entry,
compiler-generated Phase1671 finalizer
(**RET8**), calls the actual Phase1672
typed dispatcher (**RET16**) twice, and
executes the Phase1673 three-arg comparison
(**RET12**) before applying source-backed
correction for both records. It verifies
that the initial case8 correction fixes
only five bytes, while the subsequent
case9 correction updates the twelve-byte
prefix, leaving all later bytes unchanged.
No original engine callback or retail game
code executes.

New
`research/tools/audit_sin_original_postnetwork_case8_case9_copy_fixup.py`
authenticates dual original retail whole-DLL
SHA, the parent dispatcher and comparison
whole-method SHA values, each entire 41/43
byte native inline region SHA, verified source
gates, pointer direction, REP MOVSD/MOVSB,
case lengths and lack of HIGHLOW sites.
The public `--self-test` constructs invented
**non-executing sparse x86 opcode islands**,
mutates each structural witness and refuses
invalid extents or forged owner images.

One synthetic Python CTest and one real
MSVC Win32 x86 CTest raise the stacked
target from Phase1673's **294 to 296**.
The original-free decomp synthetic workflow
also runs the new auditor.

## Explicit non-goals

This reconstructs the source-authenticated
inline **comparison/correction byte-copy gate**,
not the full original case8/case9 pipeline.
All later diagnostic/formatting routines,
other type cases, real prediction state
reconciliation and full client/server runtime
interface still need independent reconstruction.

No proprietary retail DLL or complete original
native executable method body is committed to
GitHub or executed in public CI. The project
does not yet register an incomplete production
`VClientPrediction001` factory or prove
original Windows retail game boot, gameplay
parity, Android executable or Quest 3 VR
startup. Physics, surfaces/materials, prop
and model loading/rendering remain skipped
per the user's active project override.

The PR remains open/stacked on
Phase1673 PR #1682 pending exact-head CI.
