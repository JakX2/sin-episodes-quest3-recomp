# Phase 1688: original client-owned cl_predict registration inside the real independent DLL

## Source-backed gap addressed

Phase1686 made the **actual independently produced shipping `client.dll`** capable of advertising native `VClientPrediction001` to the real Source engine in a deliberately explicit, session-guarded startup probe. The previous behavior still required the engine to return an already-registered `cl_predict` ConVar through the authentic `VEngineCvar002` virtual slot3 `FindConstVar("cl_predict")`.

The original 2006 PE32 client.dll itself owns and statically constructs that ConVar. The entire original registration wrapper at `0x2426C550`, native `ConVar` constructor at `0x241216C0`, and base constructor at `0x241212E0`, including source constructor name `"cl_predict"`, default `"1"`, flags **`0x200`**, its source parent self-pointer at **ConVar+0x18**, and numeric at **ConVar+0x2C**, were authenticated from both complete original retail client+engine SHA256 in Phase1658. The original client ConVar registration accessor adds bit **0x08** (making `0x208`), detaches the command's linked-list pointer, then invokes `VEngineCvar002` **virtual slot0 RegisterConCommandBase**, as independently reconstructed in prior console ABI phases.

The production replacement DLL does **not** presently reproduce the original static global ConVar constructor list; therefore simply requesting `cl_predict` through engine ICvar may fail. This phase advances the missing registration dependency under a **separate opt-in**, while preserving the previous engine-owned existing-variable behavior.

## New production behavior and strict opt-in

New `pc/independent_slot0/native_client_prediction_convar_registration.h` owns one independently-authored, native MSVC x86 ABI-compatible **`console_abi::Variable`** with the correct 0x44-byte layout, ConVar+0x18 self-parent, default numeric 1, source default name/value/flag, and client flag+0x8 on registration. It is not a copy of the proprietary ConVar code.

The production `pc/independent_slot0/slot0.cpp` uses that owner **ONLY if**:

1. All Phase1686 requirements already hold: `VClient011` has acquired all 21 source-authenticated engine-app-system factories, `VEngineCvar002` was returned, `SIN_INDEPENDENT_CLIENT_PREDICTION_FACTORY_PROBE=1`, trace logging is on, and a valid 32-character lowercase hex trace session is present.
2. A second explicit session flag **`SIN_INDEPENDENT_CLIENT_PREDICTION_CVAR_REGISTER_PROBE=1`** is set. Without it, a missing `cl_predict` still results in the exact Phase1686 fail-closed refusal and the engine is not told that client initialization succeeded.
3. The original native ConVar `FindConstVar` returned missing. Existing `cl_predict` is reused, **never overwritten or registered twice**.
4. The acquired `VEngineCvar002` pointer and its relevant native virtual slots 0/3 are validated as readable using the existing source-bounded `VirtualQuery` before calling them.
5. The independently owned ConVar is created with the source-confirmed name/default/flags and source fields, **then** registered using the genuine native engine `VEngineCvar002::RegisterConCommandBase` vcall (slot0). After this mutation the owner validates that the engine now finds exactly that same object, its parent and string were not relinked unexpectedly and flags match `0x208`; otherwise it refuses. Since an engine may retain the pointer even after registration validation fails, it is allocated once and intentionally retained until process exit. Before registering, this opt-in path uses `GetModuleHandleExA(FROM_ADDRESS|PIN)` to **pin the native independent client.dll module until process exit**; therefore an ordinary `FreeLibrary` cannot unmap its ConVar virtual methods while the engine retains their address. **Failed registrations are never retried**.

If registration and the Phase1686 `FindConstVar` parent numeric snapshot then succeed, the engine can receive native `VClientPrediction001` and call source-authenticated slot1. This phase still does not implement gameplay slots3–6 or original entity ownership. It also **does not claim to duplicate the timing** of the original static C++ constructor; registration occurs at explicitly selected VClient011.Init instead.

## Actual DLL + launcher regression

The existing Phase1686 **actual production `client.dll` `LoadLibrary` integration test**, `test_client_prediction_factory_boundary.cpp`, now extends the source mock `VEngineCvar002` with a real native virtual Register slot0 that retains the independently authored CVar pointer. After verifying default missing-name refusal, linked-parent numeric behavior and shutdown revocation, it enables the **second** registration gate and exercises genuine `VClient011` virtual slot0 with no engine `cl_predict`. It asserts one native ICvar registration call, correct parent self-address at ConVar+0x18, default numeric 1 at +0x2C, original final flags `0x208`, real `VClientPrediction001` native slot1 caching enabled, stable singleton identity and no duplicate registration after repeated VClient011.Init. Disabling owner gates still refuses the factory.

The separate owner retail launcher `Invoke-OwnerIndependentEngineProbe.ps1` adds `-EnableClientPredictionCvarRegisterProbe`, which **requires** the already explicit `-EnableClientPredictionFactoryProbe`, and inherits its Client-only/no-other-terminal-probes restrictions. It sets the registration environment only for the reversible, SHA-authenticated disposable game copy, then restores every previous environment variable in `finally`. Existing original DLLs remain SHA-verified, backed up and restored by the launcher. New synthetic PowerShell tests check wrong-role/unguarded attempts cannot mutate the fake original DLLs. The offline startup classifier now distinguishes `prediction-convar-registered` and `prediction-convar-register-refused` from prediction factory binding, and synthetic trace replay requires both in one session without assigning any game-boot success verdict.

## Verification boundary

Verified Phase1685: 316/316 native Windows original-free CTests and all six workflows green. Phase1686 corrected native Windows run: **317/317 tests passed** via the actual replacement client.dll, though its C++/Android workflow was completing when written. Phase1687 launcher guardrail workflow had a synthetic replay double-cleanup bug, patched in the parent and inherited here.

Phase1688 adds substantial source-authenticated native ConVar registration behavior **inside the existing production DLL** and strengthens the **same test**. Its native test count remains **317**, not an invented larger target. Exact-head new CI must pass before this branch can be called verified.

This is still an owner-only diagnostic, not an actual Windows SiN Episodes game boot. It does not initialize client entities, finish PostNetwork prediction diagnostics, call the original retail ConVar constructor or load/copy protected game code. The priority-excluded physics/surface/prop/model/material/renderer subsystems remain untouched.
