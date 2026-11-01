# Phase 81: Android DEX native module environment setup

**Scope:** Offline analysis of the owner's lawfully supplied HL2VR Standalone 0.989 APK. This phase narrows SourceVR's actual native library search setup without claiming a supported external SiN native module. **No APK, DEX, original code, game assets, or native libraries have been committed.**

## Direct evidence, not assumptions

The extracted APK `classes.dex` has SHA-256:

`7b4f4d909cb1858c30bd42c07c44962b604776653a24d623d48236e28cc42b95`

The Android DEX's `initNatives` method contains the following verified environment setup string references and calls:

| Variable | DEX const-string file offset | Evidence |
|---|---|---|
| `APP_MOD_LIB` | `0xD218` | Environment-setting call on a conditional code path. The concrete value and condition require further analysis/runtime verification. |
| `APP_DATA_PATH` | `0xD2FC` | Nearby field fetch and `setenv` call |
| `APP_LIB_PATH` | `0xD30A` | Field fetch, `selectedNativeLibraryDirectory()` invocation, then `setenv` |

The source of `APP_LIB_PATH` in this Java code is the application's `selectedNativeLibraryDirectory()` result, not a hardcoded `/sdcard/srceng` string. **That is a codepath observation, not the concrete runtime pathname.** It may be affected by renderer selection or application installation layout.

`APP_MOD_LIB` is also assigned, but its precise source and the condition that guards the call have not been fully determined. There is not enough evidence to claim that it points to a user-writable directory or that Android will allow external native code to load from it.

## Link to Phases 79 and 80

- Phase 79 verified the game's choice of client/server module names and passing `GAMEBIN` to the filesystem module-loading API.
- Phase 80 verified the native filesystem's `APP_MOD_LIB` / `APP_LIB_PATH` search, candidate filename formatting, and Android `dlopen` call.
- Phase 81 identifies **where HL2VR's Java-side Android code populates these environment variables**. It does not prove a successful custom-module load or C++ vtable compatibility.

**Do not mistake `srceng/mods` asset mounting for a plugin SDK.** The native libraries shipped with the APK can be loaded in their supported application environment, but independently compiled SiN gameplay binaries are not yet proven supported.

## Reproduction

Using the original privately held owner APK:

```sh
python3 scripts/audit_hl2vr_android_env_dex.py /private/HL2VR-Standalone-0.989.apk
python3 -m unittest discover -s research/tests -p 'test_*.py' -v
```

The auditor enforces the exact `classes.dex` digest before checking bytecode signatures and produces a short, non-proprietary JSON report. Six independently authored synthetic Python tests cover DEX header/string bounds, ULEB parsing and provenance rejection. It is not a general purpose DEX disassembler; bytecode interpretation was additionally inspected manually. PortalVR contains the same environment-key names but differs in bytecode; the Phase 81 verifier is **only for pinned HL2VR 0.989**.

## Real next step

1. Trace `selectedNativeLibraryDirectory()` and the branch that sets `APP_MOD_LIB`, distinguishing the Vulkan and Turbo runtime configurations.
2. Observe native environment values through an authorized Android developer build or appropriate diagnostic tooling.
3. Prove the linker can load an **independently authored, non-game, synthetic** `CreateInterface` library from an authorized application-controlled location.
4. Only then use accessible compatible headers and engine source to build/load SiN gameplay. A working engine API, correct save/restore, network prediction and HMD behavior remain unverified.

**Issue #12 remains open.** Phase 81 is loader-provenance evidence, not R1 completion or a playable Quest 3 campaign.
