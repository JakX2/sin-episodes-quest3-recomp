# Phase 1682: native CPrediction PostNetwork slot6 source-tail enable/reset

## New original-source behavior from previously SHA-authenticated method

This phase does **not** claim a new original native method or additional whole function bytes. The parent Phase1668 original `CPrediction::PostNetworkDataReceived`, vtable slot6 at `0x241960E0`, is already **1039 bytes** and whole-method SHA256 `968f8965dba88a881e3c752020ea9e5ec33a7930056ade4cc972c9d2120bf8a3`. The user-provided original retail `client.dll` and `engine.dll` match the existing 2006 PE32 SHA evidence. Phase1682 models an additional **58-byte subsection** of that same method, `0x2419646B..0x241964A4`, which was previously treated as opaque.

After the source-authenticated prediction reconciliation loop, the original code proceeds through optional diagnostics, corrections and error reporting that remain **unimplemented** in this project. Immediately before its final client-state snapshot and scope exit, when the **local player exists**, the following control flow is visible directly in unchanged original x86 instructions:

1. At `0x2419646B`, original code loads global pointer `0x24374540` (parent `cl_predict` ConVar) and compares the parent numeric DWORD at `+0x2C` to zero, producing a Boolean.
2. It compares this value against cached **byte `CPrediction+0x10`**. When unchanged, it **does not reset** the prediction fields; the frame argument written earlier to `this+0x2C` remains intact.
3. When changed, if the **old cached byte is zero**, the source makes an original `__thiscall` call at `0x24196485` to the still-opaque helper `0x24195C20` **before** changing the state fields.
4. Either transition direction resets original `this+0x28=0`, `this+0x2C=0`, and `this+0x24=-1` in that order. The original slot7 reset virtual has analogous effects, but source slot6 writes these fields **inline**.
5. The code **reloads the ConVar** and sets `this+0x10` to `cl_predict!=0`. After this block the source calls an opaque `0x240C4940` helper to refresh the three DWORDs at `this+0x18/+0x1C/+0x20`, then follows still-unmodeled conditionals and exits the profiler. **Those later helpers and state updates are not reconstructed here.**
6. When the original local-player global is absent, source entry branches directly to the scope-exit epilogue at `0x241964DA`; the above late state-reset block is **not reached**.

No assertion is made that the entire 1039-byte function is now reconstructed.

## Implemented code and tests

The existing `pc/independent_slot0/native_client_prediction23_init_probe.h` **real compiler-generated MSVC Win32 x86 virtual slot6** remains the same ABI: one stack DWORD with RET4. Phase1682 adds an explicit **original-free opt-in** `PostNetworkOps::model_original_tail_transition` to the existing host-only callback structure. It is **off by default**, preserving the earlier Phase1668 tests' scope and avoiding behavioral changes in unrelated factory probes.

If enabled and a synthetic local player is present, the fixture follows the original enable-bit comparison, checks for a provided host `on_disabled_to_enabled` callback where original code called unknown `0x24195C20`, executes that callback before the authenticated three-field reset, and finally updates the cached enable byte. Missing callback is a **host-only refusal** that does not falsely claim the original side effect ran. Other postloop branches retain their old explicit **opaque remaining branches** callback.

New `test_original_prediction_postnetwork_tail_transition.cpp` uses actual MSVC PE32 vtable[6] calls with inline ESP readback. With authorized synthetic entity-list callbacks, it checks disabled→enabled call-before-reset, stable enabled no reset and retained frame, enabled→disabled reset with no re-enable helper, stable disabled retained frame, no-local-player early branch, missing opaque backend refusal and retry, and unchanged unknown-tail callback/lifetime behavior. All data/ConVar objects are invented and host-owned. The original DLL is never executed in CI.

The new `research/tools/audit_sin_original_prediction_postnetwork_tail_transition.py` pins the same **whole original 1039-byte SHA**, exact source x86 instructions at the 58-byte transition span, original two rel32 call targets (`0x24195C20`, `0x240C4940`), original ConVar absolute references and relocations, inline +0x28/+0x2C clears, +0x24 negative-one write, +0x10 Boolean cache, and unchanged epilogue. Its public synthetic test only builds invented sparse fragments, checks mutations/truncations, and rejects forged whole-original hashes.

## Verified parent and pending child status

Phase1681 adds two CTests and its exact native MSVC x86 run [46104398199](https://github.com/JakX2/sin-episodes-quest3-recomp/actions/runs/46104398199) verifies **308/308** tests. The Phase1682 branch adds two more tests, targeting **310 native CTests**. Its result must only be claimed after exact-head Windows CI completes.

This is a concrete extension to the existing native prediction **virtual** slot, not just another isolated scalar helper. It still is a safe original-free model, not an engine-loadable playable client.dll. Remaining work includes the original post-loop debug/correction chain, engine-owned state refresh and production class/factory registration. No HL2-equivalent physics, surface, model, prop or rendering features were developed.
