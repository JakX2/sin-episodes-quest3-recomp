# SourceVR Android native game-module launch gate

**Status: verified read-only DEX/native binary analysis; no modified APK or SiN gameplay module is running.**

## Java startup control flow

The owner's SourceVR APK classes.dex has SHA-256:

`0dea2c61aa36557f49d125bd91607228e522fb25dbe023c1867b54d7ae7ec74a»

The normal `MainActivity.onCreate()» invokes `setupEngineEnvironment()» at DEX code-unit PC 171, then the superclass `NativeActivity.onCreate()» at PC 200. Thus Java sets the environment **before** native activity initialization on this normal startup path.

Within `MainActivity.setupEngineEnvironment()» (code item offset `0x1ebe70»):

| PC | Instruction | Resolved symbol |
|---|---|---|
| 40–41 | `iget-object» | `Landroid/content/pm/ApplicationInfo;->nativeLibraryDir» |
| 42–43 | `const-string» | `APP_LIB_PATH» |
| 44–46 | `invoke-static» | `MainActivity.setenv(...)» |

The `MainActivity.setenv» wrapper (code item `0x1ebe14») invokes `android.system.Os.setenv» at PC 1. This genuinely sets the process environment rather than merely logging it.

The DEX has no literal `APP_MOD_LIB» string. Its `LauncherActivity.supportedProfiles()» diagnostic embeds the seven allowed names: `hl2,portal,portal2,ep1,ep2,lostcoast,entropyzero». Profile recognition is not derived automatically from an arbitrary SiN `gameinfo.txt».

## Native loader combined with this evidence

- `liblauncher.so» defaults both `APP_LIB_PATH» and `APP_MOD_LIB» to its own library directory **only if APP_LIB_PATH is absent**.
- Java explicitly sets `APP_LIB_PATH» in the standard activity path.
- `libengine.so» offers a separate candidate file check/open route for `APP_MOD_LIB», but that variable is not set anywhere via a literal name in the examined DEX.
- The native `.load» manifest stages stock `libclient.so» and `libserver.so» before running the launcher.

Therefore the unmodified standard app cannot be assumed to activate the engine's mod-library override. Another setup route could still exist; we have **not** observed the actual process environment.

## Shortest plausible private-use route

1. Keep HL2 as the temporary **test profile**. We do not need to add a `sin» identifier before proving that a custom module can be selected.
2. Prepare a **separate isolated debug variant** that explicitly supplies and logs `APP_MOD_LIB» before `NativeActivity.onCreate()». Its value must be a genuine Android-linker-permitted path. Do not modify the user's existing installation.
3. Supply a harmless ARM64 module with a `CreateInterface» factory that only records which interface strings were requested, then returns a failure. It is **not** a usable replacement for Source game interfaces; this probe is purely for module selection.
4. Only if the correct candidate is selected, build a genuinely compatible game interface against verified versioned headers and then connect the existing SiN gameplay work.

An arbitrary app-writable directory is not guaranteed to be permitted for loading native executable code. Android target SDK restrictions, file labeling and linker namespace configuration must be tested rather than guessed.

## Reproduce with the owner's local DEX

    python tools/sourcevr_java_env_audit.py /private/apk/classes.dex --output startup_env.json
    python -m unittest discover -s tools/tests -v

The Python 3 standard-library tool pins the exact DEX SHA-256, resolves bytecode fields/methods and refuses unrecognized revisions. It writes a report but does not alter the APK or contain proprietary bytes.

**R1 is still incomplete.** No custom module selection or actual Source game interface has been demonstrated yet.
