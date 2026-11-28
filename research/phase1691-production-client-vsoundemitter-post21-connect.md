# Phase 1691: authentic post-21-interface VSoundEmitter001::Connect inside production VClient011.Init

## The real startup gap

The previous source-authenticated phases advanced the engine-facing **independently built production** `client.dll` to an opt-in `VClientPrediction001`, `VClientEntityList003` and cold `VCENTERPRINT002`. Those factories are **not a complete retail client startup**: their diagnostic `VClient011::Init` currently returns 1 after the original first 21 required engine factory queries, whereas the original retail client carries out a substantial post-acquisition sequence before its authentic success return.

Phase538 already authenticated the **entire 1,220-byte original** `VClient011::Init` method `0x240FE0E0..0x240FE5A3` (whole SHA256 `14d81449866220aaef1f5f906b577179ea107df78a424a364c715261adbb35db`) against the complete original 2006 PE32 client SHA256 `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`. Its source has a **real first extra native callback gate immediately after the twenty-first required factory**:

- Original source `0x240FE302` queries `VSoundEmitter001`, stores at global `0x2433F6E4`, and branches to failure if the returned interface is missing.
- At `0x240FE31B`, original source loads the sound emitter's native vtable, pushes the same app-system factory, then invokes **virtual slot0 `Connect(Factory)`** with `ECX=this`. This is not a mere extra factory name or an SDK-inferred behavior.
- The original `test al,al` at `0x240FE322` and conditional jump at `0x240FE324` branch to exact original failure return `0x240FE59E` when `Connect` returns false.
- The actual original success path remains all the way at `0x240FE595` after many further post-acquisition registrations, engine/HUD lifecycle calls and possible failure gates. **A successful native sound Connect does NOT prove that full success path was executed**.

The repository already owns a source-authenticated MSVC Win32 x86 adapter `sound_emitter_connect_core.h`, `TryConnectSoundEmitter(void*,Factory)`, with no leading virtual destructor, typed native `bool (__thiscall *)(void*,Factory)`, and separately tested sound Connect true/false/invalid cases. Prior owner `-EnableClientSoundProbe` used that same adapter as an intentionally **terminal** diagnostic. It did **not** make the engine-facing prediction continuation verify sound startup.

## Production behavior

Phase1691 adds a separate user-controlled nonterminal flag `SIN_INDEPENDENT_CLIENT_SOUND_CONNECT_CONTINUE_PROBE=1`, requiring Phase1686's validated owner prediction factory trace/session gate. It runs only in the actual independently built production `client.dll`, following the source-verified 21 mandatory app-factory lookups and **before** the source-derived `cl_predict` parent numeric snapshot or any prediction/entity-list/centerprint factory registration.

- If original `VSoundEmitter001::Connect` returns **false**, `VClient011.Init` returns **0** and does not publish prediction/entity-list/centerprint factories, as the original source first post-acquisition failure branch requires. Diagnostics: `sound-connect-entered`, `sound-connect-rejected` and `client-sound-connect-continue-refused`.
- If it returns **true**, source `VClient011.Init` continues to the same existing optional prediction/ConVar/entity-list/centerprint owner-only boundary, recording `sound-connect-entered`, `sound-connect-succeeded` and `client-sound-connect-continued`. This does **not** invent any of the later GUI, material, physics, player or game-system lifecycle callbacks.
- A nonnull object without a usable native sound interface still follows the prior native adapter's **fatal unsupported-ABI guard** rather than guessed return values.
- Existing terminal `-EnableClientSoundProbe` behavior and default production factory refusal remain unchanged; the new continuation is deliberately incompatible with the old terminal sound probe in the owner launcher.

The additional flag is **not** automatically enabled merely by selecting the prediction or centerprint factory. It must be explicitly opted in by the owner after deciding to test this precise retail dependency.

## Actual independent client.dll regression

Rather than inflate instruction milestones or add another duplicate mock executable, Phase1691 strengthens the existing Phase1690 **actual PE32 production DLL** test `pc/independent_slot0/test_original_client_centerprint_factory_actual_dll.cpp`. Its fabricated `VSoundEmitter001` implements a genuine compiler-generated native **MSVC x86 virtual slot0 `Connect(Factory)`** (no virtual destructor ahead), retains the exact factory callback identity and counts invocations.

The test first invokes real DLL `VClient011.Init` with the explicit new gate and native sound return `false`. It must observe exactly one authentic source slot0 call, correct `appFactory` parameter, original 21 engine factory requests in order, native Init return 0 and absence of `VClientPrediction001`, `VClientEntityList003` and `VCENTERPRINT002` factory registrations. Then it switches the **same test-owned** native sound object to `true`, reenters the **same real DLL** Init with the same original 21 queries and requires native return1, exactly one Connect call and correct callback identity, and successful re-binding of all three already-authenticated interfaces. Source centerprint cold slots0–5,7 and fatal slot6 remain separately verified in the same Windows native test.

The final Phase1691 head adds a conservative Win32 page-boundary audit before the actual source virtual call: it requires the returned engine object to occupy readable committed memory, its native x86 vtable pointer to be readable, and virtual entry0 to lie in committed executable memory, without PAGE_GUARD or PAGE_NOACCESS. A nonnull but forged interface does NOT pass this check. Unlike original retail code's possible crash on an invalid engine pointer, the independent diagnostic returns ordinary Init failure, and it does not pretend VirtualQuery proves true C++ object identity. The SAME production client.dll integration CTest feeds pointer `0x1`, a writable-data method pointer, and a null-vtable object through the 21st app factory, and requires return0, no sound Connect invocation and no downstream factory registration. It then verifies a normal genuine x86 virtual Connect(true) still succeeds.\n\nExisting Windows source-only audit CTests remain unchanged; the previous Phase1690 target was **321/321 native Windows tests**, and Phase1691 intentionally still targets **321**, with a stricter original-source native runtime boundary. That count is not boot progress by itself, and final exact-head CI must pass before any green claim.

## Owner Windows disposable-copy probe

`pc/independent_slot0/Invoke-OwnerIndependentEngineProbe.ps1` adds `-EnableClientSoundConnectContinueProbe`. It requires the already explicit `-EnableClientPredictionFactoryProbe`, which requires Client-only mode and refuses simultaneous old `-EnableClientSoundProbe`. The launcher keeps existing complete-SHA original DLL verification, disposable-copy proof, backups and automatic restores; it sets the new environment flag to 0 for ordinary sessions or 1 only for this owner-selected process, and restores the inherited value in `finally`, even if the child game exits or crashes.

The offline startup verdict adds `OWNER_CLIENT_SOUND_CONNECT_CONTINUATION` for `client-sound-connect-continued` / `client-sound-connect-continue-refused` and source-defined `VSoundEmitter001` version only. The companion PowerShell synthetic test first replays a real-shaped `Connect(false)` trace leading to normal incomplete Init (NOT the old sound-only terminal probe), then a real-shaped `Connect(true)` before prediction/entity/centerprint factory requests and cold slot4/5 invocations, with the original unsupported GUI constructor terminal. **Neither path is classified as game boot, functional sound playback or a rendered frame.**

This is a non-render/non-physics engine service required by the original client, not one of the user-excluded Source/HL2-equivalent physics, materials, model/prop reading/loading/rendering projects. The next actual retail owner trace will identify whether the engine rejects the sound Connect callback or continues to a later authentic startup callback.

## Verification limits and follow-up

Prior fully verified Phase1689: exact-head 319/319 Windows native CTests and five applicable green workflows. Phase1690 PR #1699 initially passed 321/321 native Windows x86 tests; a final child branch with first-use centerprint source call diagnostics was still awaiting exact-head CI when Phase1691 began. **Phase1691 tests are not claimed verified until actual head workflows pass.** All public CI uses synthetic engine inputs to the independently authored DLL, not original commercial runtime code or its maps.

The real original `VClient011::Init` after sound connection also performs `-textmode` acquisition, an optional `VMaterialSystemStub001` request, derived hardware/GUI interface acquisitions, and many source game-system initializers with additional genuine failure branches. This phase deliberately leaves those user-deferred rendering/physics and unidentified lifecycle services untouched; a complete native original-client Init success and gameplay boot are not yet demonstrated.
