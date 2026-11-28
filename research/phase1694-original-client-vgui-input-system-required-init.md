# Phase 1694: Original `VClient011::Init` VGUI input/system gate

## Exact provenance and goal

This is a **private-original PE32 decompilation milestone**, not an implementation of Source rendering, not a loaded replacement-client callback, and not a playable boot. The owner supplied `SE1/bin.zip` from their connected Drive, containing SHA-matched retail `client.dll` (`6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`) and `server.dll` (`568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`). The investigation uses original code privately and commits **hashes, source-derived control-flow metadata and independently written pseudocode only**.

The previously authenticated 1,220-byte `VClient011::Init` at `0x240FE0E0` SHA256 `14d81449866220aaef1f5f906b577179ea107df78a424a364c715261adbb35db` invokes `0x241EDFB0` at source `0x240FE3C7`, with the app factory from its local register. It checks the helper's byte return and immediately fails at common `0x240FE59E` when false. This gate follows prior mandatory interface acquisition, sound connection, linked-interface resolution, and a separate particle-manager/material-dependent gate. **Those prior operations cannot be skipped to claim full source startup success.**

## Three newly whole-method authenticated originals

| Original client VA | Size | Full SHA256 | Native role |
| --- | ---: | --- | --- |
| `0x241EDFB0` | 123 bytes | `64cbfa079f093b1e577a59721396d73742eb4543b836e7fb0cc55d724783417d` | Required `CLIENT`-scoped resolver and VGUI factory setup |
| `0x2422ECD0` | 1 byte | `ae3f4619b0413d70d3004b9131c3752153074e45725be13b9a148978895e359e` | Source empty return, no side effects |
| `0x2422ECF0` | 14 bytes | `d1f9d41fb9e6c8628fa934c2ac6a848a9aae39e604b44f7b4f1ddab5ceed29b3` | Query original `VEngineVGui001` virtual +0x04 with numeric argument 2 |

Each length ends immediately before original INT3 padding and has been SHA-checked directly against the exact user-provided original client, for **138 additional distinct complete method bytes**. This does not re-count the previously authenticated parent Init.

## Original observed behavior and exact gating

The first native helper call supplies the string `CLIENT`, the address of an on-stack factory slot, and count `1` to original helper `0x240010D0`, returning early on false. The precise deeper semantics of `0x240010D0` remain to be decompiled. The `0x241EDFB0` helper then invokes the original passed-in factory twice, in this order:

| Order | Required interface name | Original returned-pointer global | On null |
| ---: | --- | --- | --- |
| 1 | `VGUI_InputInternal001` | `0x24378AB8` | Branches to false return `0x241EDFC8` |
| 2 | `VGUI_System010` | `0x24378ABC` | Branches to same false return |

The original stores each pointer to its destination global **before** its null check takes the error branch; no cleanup/rollback on the early failure is demonstrated by this method.

Only when both factory acquisitions succeed does the helper test original one-time byte flag `0x24378B28`. If zero, it sets the flag to `1` and invokes `0x24009340` with original descriptor pointer `0x2431DDCC` and literal name `CHudTextureHandle` (`0x242AC4F8`). The deeper effects of this registration callback are not inferred here, and the original source has no immediate result gate on the registration call.

Then it invokes the one-byte empty function `0x2422ECD0` followed by `0x2422ECF0`. The latter loads the previously acquired `VEngineVGui001` global at `0x2433F6B0`, calls its original virtual entry `+0x04` with integer `2`, and returns the raw result. `0x241EDFB0` converts that result to its own Boolean (nonzero/zero); **the Boolean is mandatory for the parent `VClient011::Init` to continue**.

## Source-order readable reconstruction (not executable original code)

```cpp
bool OriginalClientVGuiSetup(Factory engineFactory) {
    if (!OriginalClientListResolver_240010D0("CLIENT", &engineFactory, 1))
        return false;
    inputInternal = engineFactory("VGUI_InputInternal001", nullptr);
    if (!inputInternal) return false;
    vguiSystem = engineFactory("VGUI_System010", nullptr);
    if (!vguiSystem) return false;
    if (!originalOnceFlag_24378B28) {
        originalOnceFlag_24378B28 = true;
        OriginalRegister_24009340("CHudTextureHandle", descriptor_2431DDCC);
    }
    OriginalEmptyCallback_2422ECD0();
    return OriginalVEngineVGuiSlot1Query_2422ECF0(2) != nullptr;
}
```

The function-name labels are **analytical placeholders**. The parent Init does not return true merely because these operations succeed: several further mandatory subsystem/game-system and secondary-factory checks remain.

## Reproduction and tests

Run `python research/tools/audit_sin_original_client_vgui_input_system.py --self-test` without any original game. It validates invented relative CALL and short-conditional-branch witnesses by mutation and refuses wrong whole-original SHA values. To verify the exact user's original files privately, run `python research/tools/audit_sin_original_client_vgui_input_system.py --bin-zip /private/SE1/bin.zip`; only original method hashes, CALL sites, interface literals and control-flow witnesses are read. The original methods and their relevant source call/branch edges were checked locally against the owner's exact ZIP.

An independent MSVC Win32 CTest registers the same original-free self-test. A quick GitHub Actions synthetic job also tests it without the original PE32 files. **Do not use a synthetic green result as a claim of original retail engine execution.**

## Remaining work

The missing `CLIENT`-scoped resolver `0x240010D0`, handle registrar `0x24009340`, and first 12 system/gameplay Init callbacks need further semantic recovery. Do not equate this nonrendering input/VGUI dependency inventory with graphics, prop or material implementation. No original DLL bytes are checked in, no original files altered, and no new game/Quest runtime execution claimed.
