# Phase 1683: original 110-byte CPrediction reinit on 0→1 prediction transition

## Retail original PE32 evidence

Phase1682 authenticated the `CPrediction::PostNetworkDataReceived` slot6 58-byte tail. When cached `CPrediction+0x10` is zero but the current parent `cl_predict` numeric value is nonzero, original slot6 calls native `0x24195C20` at callsite `0x24196485` before clearing prediction fields.

Direct read-only inspection of the owner-provided unchanged original 2006 PE32 client.dll confirms the **complete 110-byte callee** at `0x24195C20` (whole-original method SHA256 `5ac6aa884651974b909ac16a8a3fbc921314e723a7d8622c00482158de0894a5`). It is a nonvirtual no-argument **`__thiscall` ECX=this, RET0**, ending in a single-byte `RET` at `0x24195C8D` and followed by two `INT3` padding bytes before `0x24195C90`. Its complete method has exactly six genuine PE32 HIGHLOW relocations, source-verified at `0x24195C22`, `0x24195C42`, `0x24195C5E`, `0x24195C73`, `0x24195C7E` and `0x24195C84`.

The method performs the following original operations in order:

1. From global pointer `0x242FA738`, address an object at original `+0x10014`, then make its **vtable+0x18** call to retrieve the **maximum index**. This is an upper bound, not a population count.
2. If maximum index is signed negative, skip the per-index loop. Otherwise iterate **indices 0 through maximum inclusive**. Each iteration calls original `0x24105B50` with that index and the global entity registry.
3. For each nonnull entity, invoke original `IsPredictable` at `0x240C5500`. **If AL is nonzero, skip the next call. If AL is zero**, invoke an engine-dependent original `0x240CC2D0` member with the literal `ReinitPredictables` at `0x242A36EC`. Despite the routine's logging name, this is the directly observed branch behavior. The body of `0x240CC2D0` remains opaque and is **not** translated here.
4. After the indexed walk, read a **different** client entity-list global at `0x242F7EC8`, call its **vtable+0x04 Count**, then invoke imported `tier0.dll!Msg` through IAT `0x242761EC` with the exact source literal `Reinitialized %i predictable entities\n` at `0x242A36C4` and the Count result. This printed value is **NOT the number of `0x240CC2D0` invocations**, nor necessarily `maximum+1`.

The original code has no null/sparse-budget checks for arbitrary game engine pointers. No original DLL, native function bytes, private game asset or proprietary machine code is committed to public GitHub.

## Implementation and regression testing

New `pc/independent_slot0/native_original_prediction_reinit_predictables.h` supplies a real compiler-generated MSVC Win32 x86 **no-stack-argument `__thiscall RET0`** original-free fixture. The original engine globals and unknown functions are represented by explicitly injected, separately owned callbacks for maximum indexed entity, original index resolver, IsPredictable, unknown reinit entity action, a separate entity-list Count, and source-format Msg. Host-only safety refuses missing callbacks and an oversized maximum index. No virtual class layout is fabricated for the missing retail entities; only the observable callback argument flow, order and gates are modeled.

New `pc/independent_slot0/test_original_prediction_reinit_predictables.cpp` confirms actual native ESP preservation around helper RET0 and also around the **existing Phase1682 compiler-generated virtual `CPrediction::PostNetworkDataReceived` slot6 RET4**. Its invented registry has sparse index2, predictable index1 and nonpredictable indices0 and3: two original-opaque reinitialization callbacks execute, while a **separate Count 11** is logged through the source literal. It covers maximum index 0 inclusive, negative maximum with still-executed Msg, unsafe maximum host refusal, missing opaque backend refusal, and the complete selected slot6 **0→1 enable → native reinit RET0 → source +0x24/+0x28/+0x2C reset**. The stable enable and 1→0 transitions never reinitialize. A second 0→1 transition with an empty registry still logs the separate global Count. All objects, callbacks and native code are original-free.

New `research/tools/audit_sin_original_prediction_reinit_predictables.py` requires **both** whole-SHA-authenticated original retail client+engine, verifies the full native 110B method SHA256, exact Phase1682 original caller, original source children/calls, registry globals, original two literal strings, named `tier0.dll!Msg` IAT, six HIGHLOW relocation sites, two INT3 pads and sparse instruction witnesses. Public `--self-test` constructs only **invented non-executable sparse x86 islands**, mutates covered bytes, tests native method bounds and rejects forged whole-original hashes.

## Test status and constraints

Verified parent Phase1681 [native run](https://github.com/JakX2/sin-episodes-quest3-recomp/actions/runs/46104398199): **308/308** original-free Win32 MSVC CTests passed. Phase1682 adds two tests (310 target); Phase1683 adds another two tests (**312 target**). Neither child target should be claimed until each exact-head Windows CI run reports success.

This establishes a deeper original prediction reenable call sequence and reduces the number of opaque engine-dependent boundaries. The actual engine-owned registry, entity reinit helper `0x240CC2D0`, original locale tables, remaining slot6 diagnostics, playable Windows client.dll and Android/Quest 3 runtime remain unresolved. Physics, surfaces, props/models and rendering are excluded under the project's active priority override.
