# Phase 1670: original PostNetwork comparison argument initializer and count wrapper

## Purpose and source authority

This phase continues the 2006 PE32 client DLL reconstruction under
Phase1669 PR #1678 (the original `C_BaseEntity`
`PostNetworkDataReceived` callback at `0x240C6900`).
Its two previously opaque child functions were independently checked
byte-for-byte in the unchanged owner-provided original client DLL.

The parent original audit requires BOTH complete SHA256 image gates
before inspecting any original machine code:

- Original `client.dll` complete SHA256
  `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.
- Original `engine.dll` complete SHA256
  `f3add5b41300e6d709fd0b492bfcdd37b9b3d20d9744f874c0b860bf021cec81`.

No retail DLL or complete original native instruction body is
committed to the repository or executed in public CI.

## Two complete original native method extents: 175 bytes

| Original method | VA | Bytes | Entire original SHA256 |
|---|---|---:|---|
| Comparison context initializer | `0x241978A0` | **104** | `13bac9785ff2c2f21cd983d5d911f542538a449f4990bd21ab825a96d24f2c82` |
| Comparison wrapper / sequence count | `0x24199BA0` | **71** | `9dfd73ea32a62b437e7492038a3b7f01706d6ba62966d162e8b776c897cfeab3` |

The exact source owner DLL confirms both are native `.text` routines
ending immediately before MSVC INT3 padding, unlike the Phase1669
192-byte adjacent-to-thunk exception. The original
`C_BaseEntity::PostNetworkDataReceived` calls them at
`0x240C6987` and `0x240C699F` respectively. Both
original direct CALL rel32 targets are verified.

### Original 104-byte argument-object initializer

The 104-byte nonvirtual original x86 function uses ECX as
an in-place writable context pointer and accepts **ten native DWORD
stack arguments**, returning with `__thiscall RET 0x28`.
It initializes a partially written **0x34-byte** argument structure.
No global address operands or PE32 HIGHLOW relocations occur within
this original routine.

With stack argument DWORDs `a1..a10` in original call order,
the actual observed stores to destination offsets are:

| Destination field | Exact original store |
|---|---|
| `+0x00`, `+0x04` | full DWORD `a1`, `a2` |
| `+0x08` | full DWORD `a4` |
| `+0x0C` | DWORD Boolean `(uint8_t(a3) != 0)` |
| `+0x10` | DWORD Boolean `(uint8_t(a5) != 0)` |
| `+0x14`, `+0x15`, `+0x16` | low BYTE of `a6`, `a7`, `a9` |
| `+0x18`, `+0x1C`, `+0x20`, `+0x28` | full DWORD zero |
| `+0x24`, `+0x25` | BYTE zero |
| `+0x2C` | low BYTE of `a8` |
| `+0x30` | full DWORD `a10` |

**The two Boolean tests examine ONLY the low byte.**
For example, original input DWORD `a3=0x100` normalizes to
zero, not one. Some target padding remains uninitialized/untouched;
the synthetic implementation deliberately does not clear the whole
0x34-byte object.

This is a nonvirtual initializer, not an original C++ class recovered
from trustworthy RTTI. The model names the structure `PreparePayload`
for diagnostic convenience, without claiming exact Source type names.

### Original 71-byte result/counter wrapper

The second native x86 function at `0x24199BA0` accepts
**three stack DWORDs**, with ECX as the comparison object, and returns
`RET 0x0C`. Its actual original behavior is:

1. First increment DWORD global at client VA `0x242FDB50`
   using x86 `add dword ptr [global],1`. The original counter
   is a plain DWORD with normal 32-bit wrapping arithmetic.
2. Load third stack argument as a pointer to a comparison context.
   If BYTE at `thirdArg+0x10` is zero, call original native
   `0x24197F50` with that pointer, then clean its caller stack
   argument with `add esp,4`.
3. Invoke original `0x24199100` with ECX=wrapper object
   and exactly three stack values: original first argument,
   original second argument, and third context pointer, in
   the source-authenticated push order.
4. Reload the updated global sequence DWORD from
   `0x242FDB50`. Invoke original `0x24199B50` with ECX=wrapper,
   updated global count and third context pointer.
5. Read full DWORD from original wrapper `this+0x28`
   **AFTER** both opaque child methods and return it in EAX,
   cleaning its own three stack arguments with `RET 0x0C`.

There are exactly **two PE32 HIGHLOW sites** inside this native
wrapper, at `0x24199BA2` and `0x24199BD2`, both
targeting the same global sequence counter. The direct relative
calls to original `0x24197F50`, `0x24199100` and
`0x24199B50` are authenticated. All three are still
independent opaque original functions. This phase does NOT guess
their comparison, allocation or logging effects.

The wrapper's return DWORD feeds Phase1669's signed
greater-than-zero check: the outer entity result becomes AL=1
only if the returned DWORD, interpreted as signed, is positive.
No extra hard-coded difference/accuracy claim is introduced.

## Original-free implementation and tests

New
`pc/independent_slot0/native_original_postnetwork_compare_helpers.h`
models only the authenticated writes and call order. It contains:

- a host-owned **nonvirtual** 52-byte target with a
  `__declspec(noinline) __thiscall Initialize(a1..a10)`
  method, exact field stores, low-byte Boolean rules,
  and preserved unwritten padding;
- an independent `WrapperFixture` with a source-faithful
  result DWORD at native `this+0x28` and a real
  three-argument `__thiscall CompareAndCount` member;
- an injected host-owned DWORD counter plus **three distinct
  opaque callbacks** for preflight, core and finalization;
- original count increment / conditional preflight / core /
  finalization / late result read ordering.

Unlike unguarded original x86 dereferences, the original-free
model refuses missing callbacks or null fake input **before**
mutating synthetic counter state. That divergence is explicitly
for test safety, not a purported original-game contract.

New
`pc/independent_slot0/test_original_postnetwork_compare_helpers.cpp`
is an actual MSVC **Win32 x86** executable. It calls both
compiler-generated `__thiscall` nonvirtual methods, checking
before/after ESP stack invariants for the original ten-argument
RET40 and three-argument RET12 call shapes.

The test checks byte-for-byte destination field values,
untouched padding and surrounding canary DWORDs, the critical
`0x100`/`0x101` low-byte Boolean distinction, repeated
initialization, the exact source input byte mapping, and
all 52 output bytes. It checks counter increments BEFORE
each opaque callback, preflight behavior with input+0x10
zero/nonzero, argument order, final callback overwriting
the returned `this+0x28` word, unsigned counter overflow,
null-input/missing-backend safety refusal, and teardown.

Finally, it composes the wrapper with the Phase1669
original-free per-entity `PostNetworkDataReceived`
Boolean contract. A synthetic history-ring frame invokes
the helper through the injected difference-count callback;
a final wrapper result of +5 becomes entity AL=1, while
a zero result becomes AL=0.

New
`research/tools/audit_sin_original_postnetwork_compare_helpers.py`
pins both complete owner original file SHAs, complete method
SHA256 values, source-attested direct CALL edges, both native
function boundaries, 14 disjoint initializer opcode witnesses
and 11 wrapper witnesses, and exact original HIGHLOW relocation
counts. Its public `--self-test` constructs invented
**non-executable sparse x86 opcode islands** and mutates every
covered byte, rejecting incorrectly sized methods and forged
original-DLL SHA.

One Python CTest and one real MSVC Win32 x86 CTest raise the
stacked total from **286 to 288 CTests**. Original-free decomp
GitHub Actions also executes the new auditor.

## Explicit non-goals

This is concrete source-faithful helper argument and orchestration
reconstruction, NOT an implementation of original
`0x24197F50`, `0x24199100`, or `0x24199B50`.
Actual game difference detection, diagnostics, prediction correction,
complete `CPrediction` slot6, and original engine
initialization still require independent analysis.

The replacement client still does NOT register its incomplete
production `VClientPrediction001` factory. Retail Windows
gameplay startup, Android executable and Quest 3 VR gameplay
are not verified, and no commercial original code executes
in public CI. No physics, surfaces/materials, prop/model
loading/rendering tasks were added per the priority override.

PR remains open and stacked on Phase1669 PR #1678 until
exact-commit 288-test CI evidence is available.
