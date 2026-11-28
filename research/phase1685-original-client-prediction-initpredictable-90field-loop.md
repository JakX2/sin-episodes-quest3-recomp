# Phase 1685: original 90-byte InitPredictable entry and exact 90-field update loop

## Original whole-method PE32 authentication

The Phase1684 original 126-byte per-entity reinitialization gate at `0x240CC2D0` calls previously opaque `0x240CAF40` when `cl_predict`, the embedded encoded-state latch, two virtual predicates, and DWORD `this+0x454` all allow it. Direct read-only inspection of the owner-provided unchanged 2006 PE32 `client.dll` proves that this callee has exactly **90 native bytes**, whole-method SHA256:

`78c3167df98218dd8298d9e7f7c194fac9bc224a0d3c773d65e023e22ff2ed2e`

It is a nonvirtual **`__thiscall ECX=this`, zero stack arguments, `RET0`**. The first instruction is at `0x240CAF40`; the final `RET` is at `0x240CAF99` and six `INT3` padding bytes follow through `0x240CAF9F`. The two exact HIGHLOW relocation sites within this method are `0x240CAF6B` (absolute global object) and `0x240CAF84` (absolute source label). Its authentic parent CALL is at `0x240CC344`, in the unchanged whole-SHA-authenticated 126-byte Phase1684 parent method.

### Source-observed original action sequence

1. **Write BYTE 1 to entity+0x2E8** *before* any child method is invoked. This value is an original prediction-related per-entity flag; what all downstream logic uses it for is not asserted here.
2. Call entity virtual **vtable+0x44** and pass the returned DWORD as the sole stack argument to still-separate original `0x240C8F00` with ECX=entity pointer.
3. Invoke still-separate `0x240C8720` with ECX=entity pointer and no C++ stack arguments.
4. Copy original **entity DWORD+0xB4** to a temporary stack word and invoke `0x240CAC00` using global object ECX=`0x242F7F78`. The original game meaning of that handle or global is not invented.
5. Call original `0x240C6900` with ECX=entity pointer and one literal **zero** stack DWORD.
6. Initialize an index to zero and perform **exactly 90 iterations**, values **0 through 89 inclusive**. Each iteration calls original `0x240C5960` with ECX=entity and three stack arguments: literal `"InitPredictable"` (address `0x242893DC`), the current index, and a final literal zero. It increments the index and repeats while the *signed* index is less than `0x5A`. These 90 iterations do not themselves describe or reimplement the internal effects of `0x240C5960`.
7. Restore the preserved registers and return to the 126-byte Phase1684 guard.

Whole-method original evidence authenticates the exact five original direct E8 call sites, the entity vcall+0x44, original `"InitPredictable"` null-terminated string, source offsets, signed loop bound, two absolute relocation sites and six trailing INT3 bytes. The entire original `client.dll` and `engine.dll` SHA256 values are required for private owner audits and no proprietary executable bytes are committed.

## Reconstructed native original-free behavior

`pc/independent_slot0/native_original_prediction_initpredictable.h` adds a native MSVC x86 **`__thiscall RET0`** method `Fixture::InitializePredictable()` on a synthetic C++ owner that **inherits the existing Phase1684 owner memory**. The actual original source fields `+0xB4` (raw DWORD) and `+0x2E8` (prediction flag byte) reside in the already-reserved owner source memory, *not in invented parallel objects*.

The source-ordered runtime method writes the flag, then passes control to six explicit, host-owned callback boundaries representing original virtual+0x44 and downstream unknown helper methods. The core itself executes the original exact 90-index loop, preserving the original string and final zero parameter, while making no assertion about the engine-dependent callback implementation. Unknown callbacks are refused and never silently replaced. The source's unconditional early flag write remains visible even if a callback is unavailable, unlike an invented atomic transaction.

The native test `pc/independent_slot0/test_original_prediction_initpredictable.cpp` uses real compiler-generated MSVC x86 calling conventions and ESP snapshots. It verifies source flag timing, field/handle offsets, original helper callback sequence and parameter values, **90 exact 0..89** indexed callbacks and source literal, missing-virtual host refusal, and the same-owner integration into Phase1684's full entity gate.

Then it follows the actual reconstructed nested call chain, with invented host-owned entity registry and no retail engine calls:

`CPrediction slot6 RET4 → Phase1683 batch ReinitPredictables RET0 → Phase1684 per-entity guard RET4 → Phase1685 InitPredictable RET0 → 90 per-field callbacks`.

The synthetic registry has two eligible entities and two ineligible indexes (one null, one already-predictable). Each eligible entity processes exactly 90 field callbacks per initialization, and Phase1683 separately logs the Count returned by its independent synthetic entity-list callback. No initialization occurs on stable enabled prediction or the enabled→disabled transition.

`research/tools/audit_sin_original_prediction_initpredictable.py` adds a dual-whole-retail-SHA private audit and a public original-free `--self-test` over invented sparse x86 instruction islands, individual opcode mutations, method extent, parent call edge, original string, exact full-function SHA, relocations and padding.

## Validation boundary and future native priorities

Phase1683 exact head `1b1de9eaaad7655c94e894d00d96aba4a7b4d802` passed **312/312 original-free Windows MSVC x86 tests and all six workflows**.

Phase1684 is a separately stacked parent PR with **314 tests targeted but not yet verified** at writing; it is undergoing a native regression fix. This Phase1685 change adds two new CTests, targeting **316** on its own exact-head Windows native run, not counted as passed until CI verifies it. The original source-engine callbacks `0x240C8F00`, `0x240C8720`, `0x240CAC00`, `0x240C6900` and `0x240C5960` remain unresolved; the 90-loop control flow alone does not make the game playable.

This strengthens an *actual original prediction/gameplay initialization chain* rather than expanding arbitrary instruction-count milestones. Actual Windows game startup, native Android/Quest 3 gameplay, renderer/frame, and source engine ownership/factory integration remain separate unresolved tasks. The user-excluded physics, surfaces, props/models and materials/rendering remain out of scope.
