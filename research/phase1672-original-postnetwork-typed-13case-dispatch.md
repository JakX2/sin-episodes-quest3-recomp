# Phase 1672: original typed PostNetwork per-node comparison dispatcher

## Original provenance and scope

This phase continues the authenticated 2006 PE32 SiN Episodes
PostNetwork reconciliation chain through the large per-node method
called by the Phase1671 finalizer.

The unchanged owner-provided original images are SHA256-gated through
the parent original-file auditor before native bytes are examined:

- Original `client.dll` complete SHA256
  `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.
- Original `engine.dll` complete SHA256
  `f3add5b41300e6d709fd0b492bfcdd37b9b3d20d9744f874c0b860bf021cec81`.

No retail DLL is included in GitHub or executed in the original-free
MSVC Win32 x86 tests. The original C++ class identity beyond the
observed calling convention, fields, and method names is not invented.

## Original 1,036-byte native method and separately bounded tables

| Original file-backed span | VA | Bytes | Whole-span SHA256 |
|---|---|---:|---|
| Per-node x86 typed dispatcher executable | `0x241996F0` | **1,036** | `71406e900eef6ba6d8abd37ab73f8730c71c35d95a8700a8cb4ab7c9a2b6a7a6` |
| Absolute 13-target MSVC switch table | `0x24199AFC` | **52** | `ff8cdf1f255d6ad46c274954e0fd52391b16f301127095b83e0c3372c0593bc1` |
| Type-index to case-index lookup | `0x24199B30` | **27** | `481a950d38ce95aa847fe32951b237c29b5471f4f910ec35b36167cdb650ffb3` |

The executable method ends precisely at `0x24199AFB`,
with original **`RET 0x10`**. The 52-byte jump table begins
at the *very next byte*, `0x24199AFC`. Those table bytes
are not executable x86 method instructions and must NOT
be appended to its whole native-function hash.

The 27-byte type lookup ends at `0x24199B4A`,
followed by original INT3 padding before the next
original native function at `0x24199B50`.

The parent original Phase1671 linked-list finalizer at
`0x24199B50` calls this exact dispatcher twice
at disjoint native callsites `0x24199B68` and
`0x24199B7F`. Both original direct E8 rel32
CALL targets and the parent finalizer's own whole SHA
are independently checked.

The dispatcher method itself has **four** genuine PE32
HIGHLOW relocation sites: two absolute type/switch table
lookups and two error text/import operands. The 52-byte
switch table contains **13 additional relocated DWORD
pointers**, one for each case entry. The 27-byte type
lookup has **no relocation sites**.

## Source-authenticated x86 entry and record traversal

The original method accepts exactly **four native x86
stack DWORD arguments**, with `ECX=this`,
and returns with `__thiscall RET 0x10`.

The Phase1671 original parent finalizer's callsites prove
the order to be:

1. First argument: the parent sequence-count DWORD.
2. Second argument: current linked-list entry pointer.
3. Third argument: record array pointer loaded from entry `+0x00`.
4. Fourth argument: **signed** record count loaded from entry `+0x04`.

The source native instructions establish:

- At entry, save the second argument at dispatcher
  `this+0x20`. If cached DWORD at dispatcher
  `this+0x1C` is zero, initialize it from current entry
  `entry+0x08`. The source epilogue clears
  dispatcher `this+0x1C` to zero even on zero/
  negative signed record count.
- A nonpositive signed count bypasses the record loop.
  For a positive count, begin at third argument's
  record array base, iterate exactly that many records
  in ascending order, advancing **52 bytes / 0x34**
  per record.
- Each record has a type DWORD at `record+0x00`,
  original state flags WORD at `record+0x12`,
  an optional pointer DWORD at `record+0x28`,
  and a last-sequence DWORD at `record+0x2C`.
  Before ordinary type dispatch, source writes
  `this+0x18` to the current record pointer.
- If the pointer at `record+0x28` is nonnull,
  the original writes the current sequence count
  to DWORD `[thatPointer+0x2C]` BEFORE checking
  `record+0x2C`. This is a true pointer-following
  write, not necessarily a write to the current
  record. A self-alias can therefore cause the
  current record to be skipped, while an alias to
  another object need not.
- If `record+0x2C` equals the sequence, skip
  all further work for this record.
- For type other than **10**, skip when either
  (a) flags `& 0x0200` is set, (b) this+0x00
  mode is 1 and flags `& 0x0100` is set, or
  (c) mode is 2 and flags `& 0x0100` is
  clear. **Type 10 bypasses these three gates.**
- For remaining records, read two base DWORDs
  at `record+0x08 + 4*(this+0x0C)` and
  `record+0x08 + 4*(this+0x10)`,
  respectively; add dispatcher DWORD biases
  at `this+0x04` and `this+0x08`.
  The original also sign-extends WORD
  `record+0x10` and stores a copy of BYTE
  `this+0x15` to `this+0x24`; it sets
  byte `this+0x25` to 1.
- The source switches on the unsigned original
  type code. Codes `0..26` are mapped to
  one of 13 native case pointers by the
  authenticated byte lookup. A type beyond
  26 directly invokes the original error
  import at `0x242761E0` with literal at
  `0x242A41E0`.
- After each case/skip, advance by `0x34`
  bytes and decrement the remaining count.
  At exit, clear `this+0x1C` and return RET16.

These are ABI and control-flow facts, not a declaration
that every comparison case is now equivalent to retail.

## Exact 27 type-code to 13 case-index mapping

The original on-disk lookup contains exactly these
case indices, one byte per type code `0..26`:

`[0,1,2,3,4,5,6,7,8,9,10,0,0,11,0,0,0,0,0,0,12,0,12,12,12,12,0]`

- **Case 0** points to the common next-record path
  `0x24199AD5`. In particular, type codes
  `0,11,12,14..19,21,26` map to that no-op.
- **Cases 1..11** jump to distinct blocks whose
  original comparison pipelines call different
  native helper routines. Case **10** at
  `0x241997CE` performs a recursive call back
  into this SAME dispatcher at `0x24199825`,
  with a transformed context. It is not replaced
  with generic byte comparison.
- **Case 12** points to original diagnostic
  `0x24199AC7`, which invokes the error
  import. Type codes `20,22,23,24,25` map
  to case12. Type codes outside `0..26`
  also go directly to the error path.

The original 13 native jump addresses in order:

`[0x24199AD5, 0x24199847, 0x2419987F,
  0x241998AD, 0x241998E7, 0x241999B7,
  0x2419997D, 0x241999EF, 0x24199A29,
  0x24199921, 0x241997CE, 0x24199A8E,
  0x24199AC7]`

These are **authenticated original entry points**,
not an inferred SDK enum or invented case ordering.

## Original-free source audit and native tests

New
`research/tools/audit_sin_original_postnetwork_type_dispatch_13case.py`
authenticates both original whole-DLL SHA gates,
the entire original **1,036-byte method SHA**,
both parent direct CALL edges, 18 disjoint sparse
native x86 opcode/operand witnesses, all four
within-method HIGHLOW pointer addresses,
all 13 jump-table entries and their relocations,
the 27-byte lookup values and whole separate
span hashes, and the function-versus-table
boundary. Public `--self-test` constructs invented
**non-executing sparse x86 opcode islands** and
mutates every covered byte, every jump target
and every type alias, plus method-length and
forged-original checks.

New
`pc/independent_slot0/native_original_postnetwork_type_switch.h`
provides a strictly host-owned synthetic
`Fixture::Dispatch(seq,node,records,count)`
with the correct compiler-generated
**four-DWORD __thiscall RET16**.
It reserves source-authenticated member offsets,
uses exactly 52-byte fake record buffers, retains
the original sequence/alias/flag/offset gating,
and selects cases with the exact native
27-to-13 constant table.

**No case-specific comparison is invented.**
Case0 is an inert skip, case12/out-of-range
uses an opaque diagnostic callback, while
cases1–11 each call a separately observable
*opaque case-index* callback with the original
source pointer arithmetic. Case10 is
explicitly marked as having a still-unknown
recursive inner call. The original raw
alias pointer is resolved only by a callback
to host-owned synthetic memory; the facade
never dereferences an arbitrary integer
address. Negative/oversized counts, null
record arrays and out-of-range source
indices are guarded as **safety divergences**.

New
`pc/independent_slot0/test_original_postnetwork_type_switch.cpp`
is an actual MSVC **Win32 x86** test. It executes
compiler-generated four-argument nonvirtual
dispatcher calls and checks before/after
ESP invariants.

The test exercises **all 27 source type codes
and one out-of-range code**, verifies exact
case selection, case0/no-op and case12/error,
case10 opaque-recursion marker, flag modes
0/1/2, aliases writing to self versus another
owned record before the last-sequence check,
signed zero/negative count, exact +0x34
array stride, original this+0x18/+0x1C/
+0x20/+0x24/+0x25, option bytes and
pointer biases. It checks safe refusal of
oversized counts/missing buffers/invalid
record indices.

Finally, it nests an actual Phase1671
**two-argument RET8 native linked-list
finalizer** into the four-argument RET16
dispatcher: first and second linked nodes
forward sequence, node pointer, node's
record-array pointer, and signed record
count correctly. This verifies the actual
original per-entry callsite argument
relationship, not real game prediction
or the unknown comparison routines.

One Python and one real Win32 x86 CTest
raise the native suite from **290 to 292**.
Original-free synthetic decomp CI also
executes the new SHA source auditor.

## Explicit non-goals

The dispatcher *outer shell* is now source-
authenticated. The actual scalar/vector,
string, array, special-pointer and recursive
comparison case implementations are NOT
complete. In particular, the function's
full 1,036-byte SHA proves method identity,
**not C++ gameplay parity**.

No production VClientPrediction001,
original engine startup, retail Windows
gameplay, Android executable or Quest3
VR runtime are claimed. No proprietary
game DLL or full original method body is
committed or run in public CI.

Physics, surface/material, prop, model
loading/rendering tasks remain skipped
under the user's active project override.

This PR remains open/stacked on
Phase1671 PR #1680 until exact-head
native CI and synthetic audits are green.
