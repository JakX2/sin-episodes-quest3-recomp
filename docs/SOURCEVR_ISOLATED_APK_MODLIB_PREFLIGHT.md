# R1: isolated SourceVR native game-module selection preflight

**Status:** Implemented and CI-tested development tooling, not a working SourceVR or SiN game module. No APK has been rebuilt or loaded on Quest 3 as part of this PR.

## What this tests

The exact examined SourceVR APK's `MainActivity.setupEngineEnvironment()» already obtains `ApplicationInfo.nativeLibraryDir» and assigns it to `APP_LIB_PATH». It does not set `APP_MOD_LIB» in the observed Java bytecode.

This patch adds a second call **immediately after** the original assignment, passing the *same verified nativeLibraryDir* value as `APP_MOD_LIB». It does **not** change `APP_LIB_PATH», profile selection, or any actual game code.

Our already-disassembled native engine checks `APP_MOD_LIB» for a matching `libclient.so» candidate. The PR also compiles a harmless ARM64 diagnostic factory with output name `libclient.so» and ELF SONAME `libclient.so». Its `CreateInterface» logs requested names, returns nullptr and `IFACE_FAILED» unconditionally. It is **not a valid game client**.

If a later safely isolated app launch prints `MODULE_LOADED» and `INTERFACE_REQUEST: VClient017» under the `SiNFactoryProbe» log tag, that provides evidence of custom module selection, before implementing the real SDK interfaces.

## Tool preflight, default read-only

The user's original APK and a decoded **copy** of it are required. The APK's SHA-pinned `classes.dex» is checked; unknown versions are refused.

With a locally installed `apktool», decode the personal copy (the command does not install or sign it):

~~~powershell
apktool d -f ".\sourcevr-original.apk" -o ".\sourcevr-isolated-src"
py .\tools\sourcevr_isolated_modlib_patch.py ".\sourcevr-isolated-src" --original-apk ".\sourcevr-original.apk"
~~~

For the **opt-in change to that decoded copy only**:

~~~powershell
py .\tools\sourcevr_isolated_modlib_patch.py ".\sourcevr-isolated-src" --original-apk ".\sourcevr-original.apk" --apply
~~~

The patcher:
- Requires the inspected `classes.dex» fingerprint `0dea2c61aa36557f49d125bd91607228e522fb25dbe023c1867b54d7ae7ec74a».
- Requires exactly one expected `MainActivity.setupEngineEnvironment()V» method in the decoded tree.
- Verifies the key/value registers and nearby `ApplicationInfo.nativeLibraryDir» load.
- Refuses prepatched methods and unknown instruction sequences.
- Saves the previous smali outside the smali sources as `sourcevr_original_MainActivity.smali.backup», then replaces the decoded copy atomically.

**No APK is produced by the patcher.** The original APK, installed application and game assets are left unchanged.

## Diagnostic ARM64 client build

GitHub Actions builds both `libsin_factory_probe.so» (standalone diagnostic) and `libclient.so» (same diagnostic with Source game client's real Android library *name*). It checks AArch64 ELF headers, `CreateInterface» export, and `libclient.so» SONAME.

`libclient.so» only logs calls. It always returns an interface failure and would intentionally prevent a game session from starting if selected. **Never overwrite an installed working SourceVR library with this diagnostic.**

For a future test, it would be included only in an isolated **side-by-side private debug APK** at the Android native library location. Do not push this to an installed app or select it in a production/gameplay profile.

## Still needed for an actual launch experiment

1. Obtain/assemble a **complete original SourceVR APK** locally. The Drive connector's 256 MB download limit currently prevents fetching the split 600 MB distribution into this environment; the extracted `classes.dex» and required native engine subset are available.
2. Implement/verify a separate debug application identity, signature, and native binary packaging, without uninstalling or overwriting the user's existing SourceVR installation. A new Android manifest package can break authority or lookup behavior, so it must be validated rather than assumed safe.
3. Validate real Android library loading with logs. The native loader's preloaded SONAME fallback may still select a stock library even if `APP_MOD_LIB» is supplied.
4. Only then build correct `VClient017» and `ServerGameDLL010» game modules, with genuine method layouts and lifecycle.

This is a controlled attempt to resolve **native module selection**. It is not evidence that the SiN game is executable on Android.
