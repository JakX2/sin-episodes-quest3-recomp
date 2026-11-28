# Phase 1671: original PostNetwork diagnostic core and linked-list finalizer

## Source identity and scope

This phase continues the authenticated 2006 PE32 SiN Episodes
PostNetwork comparison chain through the two direct descendants
of the Phase1670 comparison wrapper (`0x24199BA0`).

The original unchanged owner-provided images are authenticated
by the Phase1658 parent audit before original instructions are read:

- `client.dll` complete SHA256
  `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.
- `engine.dll` complete SHA256
  `f3add5b41300e6d709fd0b492bfcdd37b9b3d20d9744f874c0b860bf021cec81`.

No original DLL executable bytes or complete original function body
are committed to public GitHub or run in original-free CI.

## Two complete newly authenticated functions: 204 native bytes

| Original routine | Original VA | Full size | Complete original SHA256 |
|---|---|---:|---|
| Diagnostic core / gate | `0x24199100` | **139 bytes** | `2d9113fbab33660ac6bf3c8af365f522270be8194682c51fc810d0ccf9f9be71` |
| Linked-list finalizer | `0x24199B50` | **65 bytes** | `45490bbfdb993b0b40c255aaf9612d1f1e116d10de5b3c55d220f10a524e5005` |

Both original native method extents end immediately before original
compiler INT3 padding. They are the authentic original direct CALL
targets at `0x24199BCB` (core) and `0x24199BDA`
(finalizer) from the Phase1670 71-byte wrapper, whose own original
complete SHA has also been independently verified.

### Original 139-byte diagnostic gate

The original core accepts **three 32-bit stack args** and uses
ECX as its comparison-context object, returning `RET 0x0C`.
It is an optional diagnostic builder, NOT a direct gameplay
difference calculation:

1. Always write DWORD **zero** to context `this+0x34`
   and store the first argument DWORD pointer at
   context `this+0x38` before checking the later gates.
2. Require the first argument pointer to be non-null
   and the first byte it points at to be nonzero.
3. Load original ConVar parent pointer at global
   `0x243747C8`, then its signed numeric DWORD at
   `parent+0x2C`. If negative, or if it does not
   equal the SECOND caller argument, skip formatting.
4. Read original global debug bitfield at
   `0x2437480C` and test bit `0x1000`.
   - If set, use the original hard-coded label pointer at
     `0x24298188`.
   - If clear, read an original global object at
     `0x24374810` and its pointer at `object+0x20`.
     Require a non-null and non-empty first byte;
     later code also has a fallback pointer to original
     label at `0x242D330E`.
5. Call original opaque formatter `0x24197ED0`,
   passing the third caller argument together with
   the chosen label pointer. Its behavior and output
   allocation remain unknown in this phase.
6. Store the formatter result DWORD at `this+0x34`
   and return; the early branches leave that field zero.

Exactly **five** original HIGHLOW relocation sites within the
core method have been verified:
`0x2419911B`, `0x2419912E`,
`0x2419913A`, `0x24199155`,
`0x24199172`. The two native relative CALLs
to `0x24197ED0` are separately verified.
No additional original global service or class semantics are
inferred from newer Source SDK implementations.

### Original 65-byte linked-list finalizer

The finalizer accepts **two 32-bit stack args**,
with ECX as its comparison-context object and
returns `RET 0x08`. Its first argument is the
original global sequence-count DWORD forwarded
by the Phase1670 wrapper; its second argument is
a pointer to the first linked entry.

The original method does **not** check the first
entry pointer for null: it immediately reads DWORDs
at `entry+0x00` and `entry+0x04` and invokes
native `0x241996F0` with four original x86
stack args, representing the sequence count,
the entry pointer, and both leading DWORDs.
The full effects of `0x241996F0` remain OPAQUE.

**Critical ordering:** only AFTER the opaque helper
returns, the original reads the next pointer at
`entry+0x0C`. If non-null, it processes the
next entry and repeats until the link is null.
The link is reloaded after EACH callback; it is
not cached before invoking the helper. Thus,
a callback that mutates the current entry's
next pointer affects which entry the original
finalizer will visit next. This is explicitly
modeled and tested.

The function has **zero original HIGHLOW
relocation sites**. Both direct relative CALL
instructions to `0x241996F0` are authenticated,
as are original native loop-back branch and
the final `RET 0x08`. The source does NOT
authenticate the entire linked structure's
class name or deeper ownership semantics.

## Original-free implementation and tests

New
`pc/independent_slot0/native_original_postnetwork_diagnostic_core_finalizer.h`
defines two separately testable invented/native-interface facades:

- `DiagnosticCoreFixture` reserves exact
  original `this+0x34` result and
  `this+0x38` input-pointer fields;
  a real nonvirtual three-argument
  `__thiscall BuildDiagnostic` preserves original
  guard order and label-selection paths, calling
  an injected opaque formatter and storing its
  raw returned word;
- `ListFinalizerFixture` provides a real
  two-argument `__thiscall Finalize`,
  host-owned nodes with DWORDs at +`0x00`/+`0x04`
  and next pointer at +`0x0C`, and a distinct
  callback standing in for unknown original
  `0x241996F0`. It reloads each link AFTER
  the callback and processes the first node
  before checking for another.

Unlike the original routine, the diagnostic finalizer
refuses a null first entry or absent callback and
caps traversal at 4096 invented nodes to avoid
unbounded loops. These are **safety divergences**,
not claims about the retail original null-pointer
or cyclic-list behavior.

`test_original_postnetwork_diagnostic_core_finalizer.cpp`
is a genuine original-free MSVC Win32 x86 executable.
It directly executes compiler-generated
`__thiscall` member functions, with a precise
three-DWORD `RET 0x0C` core and two-DWORD
`RET 0x08` finalizer, checking before/after
ESP invariants.

It checks original result/input pointer member
offsets; null/empty first input, signed negative/
mismatched ConVar gate, empty configured text,
debug-bit-`0x1000` hardcoded label branch,
opaque formatter result; then a three-node
linked-list that forwards first and second word
values with the original sequence count.
A specific test mutates `node1.next` during
the first opaque callback and verifies that
the actual original-free traversal skips
node2 and visits node3, proving a **post-callback
link reload**, not a cached traversal.

Finally, it composes this bounded finalizer
with the prior Phase1670 original-free result
wrapper: global counter increment -> original
opaque preflight/core -> finalizer callbacks ->
late `this+0x28` wrapper result read. This
is a synthetic orchestration check, not an
original game-world or engine regression.

New
`research/tools/audit_sin_original_postnetwork_diagnostic_core_finalizer.py`
authenticates dual original whole-image SHA,
both entire original native method SHA digests,
authentic direct CALL targets from the Phase1670
wrapper, five/zero original HIGHLOW sites,
all source-attested member offsets, ConVar gates,
opaque formatter/final per-node helper calls,
loop order and native RET12/RET8 boundaries.
Its public `--self-test` constructs invented
**non-executable sparse x86 instruction islands**,
mutates each covered byte, and rejects incorrect
lengths or forged retail-image SHA.

One Python CTest and one real MSVC Win32 x86
CTest raise the stacked total from **288 to 290**.
The original-free decomp synthetic workflow also
executes the new auditor.

## Limitations and remaining work

This authenticates 204 more original native
function bytes, but does NOT reconstruct
the opaque format builder `0x24197ED0`,
the per-node processing helper
`0x241996F0`, original comparison allocations,
all diagnostic state, or the complex
PostNetwork correction paths.

The replacement client does NOT register
a production `VClientPrediction001` factory.
Retail original Windows game boot, accurate
prediction gameplay, Android native startup
and Quest 3 VR gameplay remain unverified.

No original commercial method body is committed
or executed in public CI. No physics, surfaces,
materials, props, model loading or rendering
are implemented, per the user's priority override.
PR stays open/stacked pending exact-head CI.
