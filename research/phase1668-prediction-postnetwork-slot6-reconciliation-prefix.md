# Phase 1668: original CPrediction PostNetworkDataReceived slot6
## Authentic original method, clearly bounded partial reconstruction

This phase examines the original 2006 PE32 SiN Episodes
`CPrediction::PostNetworkDataReceived` virtual function, then
reproduces only the **entry / entity-reconciliation prefix** using
original-free native MSVC x86 synthetic objects.

Original owner-supplied unchanged complete-file SHA256 gates:

- `client.dll` SHA256
  `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.
- `engine.dll` SHA256
  `f3add5b41300e6d709fd0b492bfcdd37b9b3d20d9744f874c0b860bf021cec81`.

Both are verified by the prior Phase1658 original-file audit
before original method analysis. No original DLL is shipped
or executed in public GitHub Actions.

### One whole method: 1,039 original x86 bytes

Original prediction primary vtable `0x242A363C`,
**slot6**, points to exact original VA `0x241960E0`.
The original native method ends immediately before
`0x241964EF` (INT3 padding), length **1,039 bytes**,
whole-function SHA256:

`968f8965dba88a881e3c752020ea9e5ec33a7930056ade4cc972c9d2120bf8a3`

Its original name is confirmed by the literal
`CPrediction::PostNetworkDataReceived` in client
`.rdata` at VA `0x242A380C`. The profiling group at
VA `0x242D393C` is actually **`Unaccounted`**,
NOT `Prediction`. The owner-original file was
inspected directly to settle this distinction.

The method takes exactly **one native x86 stack DWORD
argument**, has `ECX=this` and cleans its stack with
`RET 4`. The whole 1,039-byte method contains
**38 exact original PE32 HIGHLOW relocation sites**,
all pinned and verified, in addition to the relocated
prediction slot6 vtable cell. Its complete original
SHA gate protects the rest of the method even where
its business logic remains deliberately opaque.

### Bounded verified behavior: entry and entity reconciliation

The original disassembly establishes this sequence:

1. Enter original profiler `CPrediction::PostNetworkDataReceived`.
   An original direct call to `0x241535E0` occurs before
   the main prediction-object writes. Its full original
   semantics remain opaque in this phase.
2. Read caller's one DWORD argument and store it at
   prediction `this+0x2C`. Set prediction
   `this+0x30` to zero, **before** checking the
   current-player global.
3. Read original C_BasePlayer local pointer global
   `0x24338CF0` (already identified by Phase1659).
   If null, skip to the original profiler tail.
4. Read original `cl_predict` ConVar parent pointer
   at `0x24374540`, then numeric `parent+0x2C`.
   With prediction disabled, skip the predictable
   reconciliation loop, but original code STILL has
   additional follow-up branches after this gate.
5. The enabled, player-present path also reads the
   original optimization ConVar parent pointer at
   `0x243746A8`. This value influences **later
   original diagnostic/correction branches** and is
   recorded in the invented context, but no
   interpretation of those branches is claimed.
6. Read original global client entity list
   `0x242F7EC8`, query virtual `Count()` at its
   primary vtable byte offset `+0x04` **once**,
   and iterate ascending `Get(index)` virtual
   byte offset `+0x00` for indices `[0,count)`.
   A non-positive count bypasses the entity loop.
7. For non-null entity pointers, call original
   `C_BaseEntity` `IsPredictable()` at `0x240C5500`.
   Its seven-byte original body reads the byte at
   `entity+0x2E8` and returns it in AL.
8. For each eligible entity, forward the original
   **PostNetworkDataReceived input argument** to
   the original native entity method `0x240C6900`
   through an explicit one-DWORD stack argument.
   The original entity method's actual internals
   remain opaque here. If its result AL is nonzero,
   write **one** to prediction `this+0x30`.
   All other eligible entities are still visited;
   `this+0x30` tracks whether any callback returned
   nonzero, not an incremented count.
9. After this bounded loop, the **original function
   continues into extensive debug logging, error
   collection, prediction resets, conditional correction
   calls, display formatting, other globals and
   final state refresh**. Most of the remaining
   instructions are NOT implemented in this phase.

The source-only audit pins the original first-call
profile, local-player/cl_predict early guards,
client Count/Get and per-entity callsite, observed
member writes, native loop backedge and final
`RET 4`. It does NOT turn whole-function SHA
verification into a claim of whole-function C++
behavioral reconstruction.

### Original-free implementation and tests

The existing original-free 23-slot diagnostic
`Prediction23Fixture` now gives virtual **slot6**
one argument and native `__thiscall RET4`, rather
than an inert incorrectly signed placeholder.

An explicit 32-bit DWORD at original
`prediction this+0x30` was inserted **before**
synthetic host pointers. This matters: the
earlier fixture placed a diagnostic `view_host_`
pointer at that position, which the original
PostNetworkDataReceived method would overwrite.
All previously authenticated early prediction
offsets (`+0x08/0x09/0x0C/0x10/0x24/0x28/0x2C`)
remain unchanged.

`PostNetworkOps` is a new wholly host-owned callback
adapter: synthetic current-player presence and ConVar
values, Count/Get methods, predictable-byte checking,
opaque original per-entity reconciliation result,
opaque original entry and later-branch callbacks,
and scope enter/exit. No retail DLL code or real
engine entity object is loaded or invoked.
A missing backend, negative count, or more than 4096
invented entities causes an explicit safe refusal,
rather than following untrusted original pointers.

The method records that later corrections are
**unimplemented** with an explicit opaque-tail event.
Its synthetic return is NOT a complete implementation
of original client prediction or authoritative
gameplay state recovery.

`test_original_client_prediction_postnetwork_slot6.cpp`
is a genuine original-free MSVC Win32 x86 executable.
It calls actual compiler-generated vtable[6] with
ECX=this, one original frame argument and before/
after ESP invariant. Its invented entity array
includes the original +0x2E8 byte, null and
nonpredictable entries. Tests verify:

- actual vtable index6, one-stack-arg `RET4`;
- new authentic this+`0x2C` and this+`0x30` field
  offsets without overwriting former diagnostic host;
- current-player/cl_predict guards after initial
  member writes;
- one Count call, ascending Get(index), non-null
  and predictable-only reconcile callbacks;
- three eligible entities, raw argument passed to
  each, result AL !=0 sets the any-reconciled flag;
- if every callback returns zero the flag stays zero;
- zero/negative/oversized counts, missing backend,
  detached host callback and explicit opaque later
  branch boundary;
- no actual original-game or retail-entity method call.

The Python original-only auditor
`research/tools/audit_sin_original_prediction_postnetwork_slot6.py`
authenticates both original whole-DLL SHA gates,
the entire original **1,039-byte** method hash,
23-slot prediction target, correct original
profile/group string literals, all **38** native
HIGHLOW relocation sites, INT3 method endpoint
and 22 disjoint sparse opcode/operand witnesses.
Public `--self-test` constructs invented
**non-executing x86 byte islands**, mutates each
covered byte, rejects truncated/overlong methods
and fake original images.

A local independent original-file replay separately
confirmed the complete original SHA, all 22
disjoint x86 witness chunks, relocation count,
vtable index and method boundary. It also caught
and corrected the original `Unaccounted` profiler
group name before submitting public CI.

One synthetic Python CTest and one actual MSVC
x86 CTest raise the stacked suite from
Phase1667's **282 to 284**. The original-free
decomp synthetic workflow runs the new auditor.

## Explicit non-goals and next work

A full source-faithful original
`PostNetworkDataReceived` requires separately
authenticating the original `0x240C6900`
per-entity callback, `0x241535E0` original
entry helper, and later correction/debug
functions and ConVar branches. Those remain
opaque and are NOT claimed by this phase.

No production `VClientPrediction001` factory is
registered. No real original SiN Windows game
startup, gameplay parity, Android or Quest3
native/VR execution is proven. Original DLL
binaries are never committed. No physics,
surfaces/materials, prop, model-loading or
rendering work is added under the project's
active priority override.

PR is intentionally open and stacked on
Phase1667 PR #1676, with final-head 284-test
CI pending until actual run succeeds.
