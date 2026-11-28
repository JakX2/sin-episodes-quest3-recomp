# Phase 1684: original entity prediction reinit guard and two complete encoded-state callees

## Original 2006 PE32 evidence

Direct read-only inspection of the owner-provided unchanged `SE1/bin.zip/client.dll` resolves the previously opaque Phase1683 per-entity helper at **`0x240CC2D0`**. The entire routine is exactly **126 native bytes**, whole-method SHA256:

`222233997800f9d90828afef3f4a73eb44f20fd12664945a7646f52a399794e4`

It ends at `0x240CC34D` with **`RET 4`**, not RET0. Its native ABI is **`__thiscall ECX=this` with one stack pointer argument**, consumed by callee. That pointer is the unchanged `"ReinitPredictables"` string passed at `0x24195C5D` by the 110-byte Phase1683 parent; the child routine **does not dereference this argument in its 126-byte body**, but still pops it on return. The parent CALL is at `0x24195C64`. Two `INT3` bytes follow the child method. The method contains exactly **two HIGHLOW relocation sites**, at `0x240CC2D1` (parent `cl_predict` ConVar global) and `0x240CC2FB` (a distinct engine-global mode provider). Both the client and engine complete retail SHA digests are authenticated by the private audit before inspecting method bytes.

### Source-controlled gates and exact ordering

1. Read global `0x24374540` (parent `cl_predict` ConVar) and compare its numeric DWORD at `+0x2C` with zero. When zero, exit immediately, leaving the entity unchanged.
2. When prediction is enabled, inspect **byte `this+0xC0`**. If it is nonzero, bypass the following two encoded-state checks and proceed directly to the virtual-method gates. A zero latch triggers two methods on the embedded state object at **`this+0xA4`**.
3. The first call is original **`0x241952A0`**, a complete **7-byte** method, SHA256 `de10d879f972c106f535bcedb18af6100950d793a358b52f7b2f146235ff216c`. It returns whether the DWORD stored at that embedded address is nonzero. On zero, the outer reinit returns without changing its latch.
4. When nonzero, read distinct global pointer `0x2433F684` and call its **vtable+0x30** method, obtaining EAX. Subtract one (original 32-bit modular arithmetic), then call second original **`0x241952D0`**, a complete **8-byte** method, SHA256 `a6e9e6573fcb22882747e8bc5c03cdd05af19899393d0fd0d5c5818e9163aef6`. That method returns **`(embedded_DWORD >> 1) & 0x1F`**. If `global_result-1` differs from the computed five-bit state index, exit; otherwise write **byte 1 at `this+0xC0`**.
5. The source then makes entity virtual call **vtable+0x1A0**, testing returned AL. A nonzero result exits. On zero, make entity virtual call **vtable+0x1BC**, which must return nonzero to continue. Those virtual implementations and their semantic game meanings remain separate, not invented from their offsets.
6. Next compare **DWORD `this+0x454`** against zero; nonzero exits.
7. Only then make the still-unresolved original direct call to `0x240CAF40` at source site `0x240CC344`, with ECX=entity pointer. Its side effects are **not implemented in this phase**.

Crucial behavioral detail: once `this+0xC0` is set, the source does not clear it when a later virtual predicate or `this+0x454` fails. Subsequent invocations with the latch still nonzero skip the embedded-state/global mode test altogether. This is relevant to repeated reinitialization attempts.

The original source method also contains two direct calls to the 7B/8B embedded helpers and has separate global/method virtual dependencies. Private audit authenticates those original E8 targets and all three whole method SHAs, source offsets/branch witnesses, exact two relocations and native RET4. No original DLL, complete disassembly or unlicensed original object code is committed.

## Original-free Windows native reconstruction

New `pc/independent_slot0/native_original_prediction_entity_reinit_guard.h` provides a true compiler-generated MSVC Win32 x86 **one-argument `__thiscall RET4`** `Fixture::ReinitEntity(reason)`. Its host-owned synthetic object keeps the directly attested **embedded state at +0xA4**, **latch byte at +0xC0** and **DWORD final gate at +0x454**. All host callback registries and counters are allocated *after* the known original +0x458 region and cannot overwrite source fields.

The complete original 7B/8B embedded-state calculations are implemented as simple C++ integer operations, not opaque callbacks. The external global mode provider, virtual predicates at +0x1A0 and +0x1BC, and final original helper remain **explicitly injected callback boundaries**. No virtual class layout is assumed for an unknown commercial entity type. Missing callbacks fail closed as a safety divergence, rather than pretending to perform engine behavior.

New `pc/independent_slot0/test_original_prediction_entity_reinit_guard.cpp` uses real MSVC x86 compiled code and ESP comparisons. It tests original cl_predict zero early return, zero embedded state, source encoded-index mismatch, first successful latch before failing the negative virtual predicate, already-latched bypass, second virtual predicate refusal, +0x454 nonzero refusal, final callback ordering, missing final backend, and 0xFFFFFFFF encoded-state masking with global index 32.

Its longer integrated test then composes the already source-authenticated chain:

`CPrediction::PostNetworkDataReceived slot6 RET4 → ReinitPredictables 110B __thiscall RET0 → per-entity reinit guard 126B __thiscall RET4 → opaque 0x240CAF40 delegate → slot6 state reset`.

An invented indexed registry has a null hole and a player with a true IsPredictable predicate. The 110B parent chooses only false-predicate nonnull entities and uses its **separate entity-list Count** for logging. The child guard uses the source-exact embedded field gates and returns through real MSVC x86 RET4. Stable enabled prediction and enabled-to-disabled transition do not enter the reinit chain. The test never loads the original PE32 DLL or calls actual engine-owned objects.

`research/tools/audit_sin_original_prediction_entity_reinit_guard.py` performs owner-private dual-original full-image authentication and checks the whole 126B child and whole 110B parent SHAs, original parent callsite, whole 7B/8B helper SHAs, native instruction witnesses/branch targets, exact 2 HIGHLOW relocation sites, source layout and RET4 with two trailing INT3 bytes. Public `--self-test` constructs only invented sparse non-executable opcode fragments, tests mutation/truncation and forged owner-original rejection.

## Verification scope

Fully verified parent Phase1683 commit `1b1de9eaaad7655c94e894d00d96aba4a7b4d802` passed **312/312 original-free Windows MSVC x86 CTests** and **all six** GitHub Actions workflows, including its Android Bionic library-loading smoke test.

Phase1684 adds **two** original-free tests, target **314**, pending its own exact-head GitHub Actions CI. This authentically reconstructs another **141 distinct native bytes** (126+7+8) and closes a previously opaque per-entity decision branch. It does *not* reconstruct the final entity `0x240CAF40`, engine-owned entity registry, player runtime, complete prediction reconciliation, production engine-loadable client.dll gameplay startup, Android game boot or Quest 3 rendering. HL2-equivalent physics, surfaces, props/models and materials/rendering remain excluded.
