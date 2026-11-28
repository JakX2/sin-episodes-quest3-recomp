# Phase 1675: original PostNetwork binary and signed-short diagnostic gates

## Original provenance and scope

This phase recovers two COMPLETE adjacent original 2006 PE32
native diagnostic methods used by the source-attested
PostNetwork typed dispatcher, not the underlying retail
formatting engine or actual gameplay comparison algorithm.

The unchanged owner-provided original client.dll SHA256 is
`6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`
and engine.dll SHA256 is
`f3add5b41300e6d709fd0b492bfcdd37b9b3d20d9744f874c0b860bf021cec81`.
Both complete images are SHA-gated by the parent original audit.

Two complete original functions totaling **63 native bytes**:

| Real retail method | Original VA | Native bytes | Entire original SHA256 |
|---|---|---:|---|
| Binary-count diagnostic gate | `0x24198360` | **30** | `c654d2ca175b5fad2fd98a18efcbc4709f8feb15190c9ee733e578d63cf6279d` |
| Signed-short diagnostic gate | `0x24198380` | **33** | `80ed762c28807f23273f2449855fd6063b3b3d37ee4068a007b77e3b2783cb2f` |

Both original methods are native **nonvirtual MSVC x86
`__thiscall`** with **FOUR 32-bit stack arguments** and
`RET 0x10`. Each ends before compiler INT3 padding.
Each carries exactly ONE HIGHLOW relocation for its
literal string pointer, and each forwards to the
same still-opaque native formatter at `0x24197FE0`
via a direct, original E8 relative CALL.

## Exact source gating, argument flow, and real retail text

Both routines first compare a DWORD in the dispatcher
object at **`this+0x34`** against the current
record pointer DWORD at **`this+0x18`**.

If these words differ, each method returns RET16
without formatting or dereferencing any input pointer.
If the words are equal:

- The **binary** method at `0x24198360`
  loads its **SECOND original stack argument**
  as a full 32-bit DWORD and pushes it unchanged,
  then pushes on-disk literal pointer
  `0x242A3FF0`, original text
  **`binary (%i bytes)`**, and the dispatcher
  object pointer as the third formatter arg.
  It calls opaque `0x24197FE0` and
  cleans the formatter's three 32-bit args.
- The **short** method at `0x24198380`
  instead interprets SECOND original argument
  as a POINTER TO 16-BIT SIGNED INTEGER.
  It performs actual native
  `MOVSX EAX, WORD PTR [EDX]`, pushing the
  sign-extended value. It then pushes on-disk
  literal pointer `0x242A4004` with exact
  original text **`short (%i)`**, plus the
  dispatcher pointer, and calls the SAME
  opaque formatter `0x24197FE0`.

The two methods thus share the selected-record
gate and native 4-DWORD RET16 stack shape
but have different original second-argument
interpretations. The short routine can read
`-32768` and pass it as the signed DWORD
`0xFFFF8000`; the binary routine preserves
all 32 argument bits without dereference.

The complete original Phase1672 dispatcher
at `0x241996F0` contains two authentic
CALLs to binary helper `0x24198360`,
at `0x24199973` and `0x24199A87`,
and one authentic direct CALL to short
helper `0x24198380` at `0x24199A1F`.
The caller's whole executable original SHA
is verified together with the relative-call
targets. This does not imply the complete
caller type branches are reconstructed.

The actual original file was independently
rechecked read-only for both whole-method
SHAs and both exact text literals. This
research did not execute commercial game code.

## Original-free ABI and native tests

Phase1674 reserved original correction-enable
byte at dispatcher `this+0x2C` before fake
callback pointers. This phase also reserves
the real source `this+0x34` selected-record
DWORD (and adjacent `+0x30` DWORD),
shifting synthetic callback storage AFTER
all observed retail source fields. Native
`offsetof` checks confirm all original
`this+0x18/+0x2C/+0x34` offsets coexist
without overlap or pointer clobbering.

New
`pc/independent_slot0/native_original_postnetwork_scalar_formatters.h`
adds a host-owned synthetic fixture exposing
two genuine compiler-generated
`__declspec(noinline) __thiscall` methods,
each with four native stack DWORD arguments.
It compares the two original source DWORD
fields and dispatches to an explicitly
injected original-free downstream formatter
callback, never to original game code.
The short method requires an authorized
host-owned signed-short pointer before
dereference. Missing backend or revoked
pointer is refused, which is an intentional
safety divergence from unguarded original x86.

`test_original_postnetwork_scalar_formatters.cpp`
is a real Win32 MSVC x86 executable. It
measures ESP before/after each native
four-arg member call (RET16 ABI),
checks original field offsets and
selected-record mismatch suppression,
exact original literal strings, unchanged
binary argument2 including `0xFFFFFFFE`,
signed short values -32768, 32767 and -1,
null/missing/revoked callback refusal, and
that a mismatched record guards BEFORE any
short pointer dereference.

It then attaches the two safe native gates
as opaque downstream callbacks to the
existing Phase1672 genuine typed dispatcher:
one selected type8 binary case, one
selected type9 binary case, and a type7
signed-short diagnostic case. In each call,
the source-selected `this+0x34` pointer
matches exactly one of the source-record
`this+0x18` values during native case
processing. Only the matching case emits
a diagnostic callback. The type specialist
branches' other effects remain opaque.

`research/tools/audit_sin_original_postnetwork_scalar_formatters.py`
pins both complete owner original-DLL
SHA digests, complete 30B and 33B
native function SHA values, entire
typed parent source SHA, three original
direct rel32 caller sites, original binary
and short string literals, exact source
MOVSX short input, one HIGHLOW literal
relocation per method and original INT3
boundaries. Public `--self-test`
constructs invented non-executing x86
opcode islands and rejects byte mutations,
wrong method extents and forged original
whole-image SHA. It ships no original
commercial method body or executable DLL.

CMake adds one Python SHA audit CTest
and one actual native MSVC x86 test,
raising the stacked target from Phase1674's
**296 to 298**. The original-free decomp
workflow includes the auditor self-test.

## Explicit non-goals

Neither opaque original
`0x24197FE0` formatter behavior
nor the full original type7/8/9
diagnostic and correction pipelines
are reconstructed. The whole original
PostNetworkDataReceived recovery logic,
production VClientPrediction001 factory,
retail Windows game engine boot,
Android executable and Quest3 VR startup
remain unverified. No physics, surfaces,
materials, props, model loading/rendering
work was added under the user's override.
Public GitHub tests use only synthetic
original-free objects. PR intentionally
open and stacked until exact-head
298-test CI evidence is available.
