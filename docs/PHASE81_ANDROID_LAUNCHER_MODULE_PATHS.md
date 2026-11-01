# Phase 81: Android launcher sets SourceVR native library search roots

**Evidence:** Private read-only inspection of the user's HL2VR Standalone 0.989 and PortalVR 1.12 APK bytecode and binary Android manifests, November 1, 2026. **No modified APK, device execution, custom SiN module or loader success claim.**

## Exact pinned DEX provenance

| Binary | DEX | SHA-256 |
|---|---|---|
| HL2VR 0.989 | classes.dex | 7b4f4d909cb1858c30bd42c07c44962b604776653a24d623d48236e28cc42b95 |
| HL2VR 0.989 | classes2.dex | 18c5944efa31751e90ee01fe131dab0f91183781545eefea8b5575660ea9b48c |
| PortalVR APK version 1.12 | classes.dex | c876e067cfa06c84efe32747141c2994b66c9150b6d2303956b8491fa991944b |

The original HL2VR 0.989 APK was separately verified against the official release digest in Phase 78. No proprietary DEX, game assets or libraries are committed.

## The optional game-module override is a real Android Intent hook

In **both** APKs, org.libsdl.app.SDLActivity.init invokes com.valvesoftware.ValveActivity2.initNatives(Context, Intent). That initializer reads an Android Intent **String extra named gamelibdir** using getStringExtra. If the value is nonnull and nonempty, it calls its native setenv method to set **APP_MOD_LIB**.

| Bytecode evidence (16-bit method-code-unit offsets, not ELF VA) | HL2VR | PortalVR |
|---|---|---|
| SDLActivity.init calls ValveActivity2.initNatives | 0x128 | 0x103 |
| initNatives: gamelibdir string | 0x39 | 0x36 |
| initNatives: getStringExtra | 0x3b | 0x38 |
| initNatives: null check / String.isEmpty / conditional skip | 0xd8 / 0xda / 0xde | 0xc3 / 0xc5 / 0xc9 |
| initNatives: APP_MOD_LIB string | 0xe0 | 0xcb |
| initNatives: setenv for APP_MOD_LIB | 0xe2 | 0xcd |
| initNatives: APP_LIB_PATH string | 0x159 | 0x140 |
| initNatives: ApplicationInfo.nativeLibraryDir field | 0x15b | 0x142 |
| initNatives: setenv for APP_LIB_PATH | 0x161 | 0x144 |

**The stock launcher is not proven to supply gamelibdir.** Within the bundled DEX bodies, no other direct literal references to that string were found. It may be legacy/external Intent support, or a path used by launch flows not established here. No Java UI exposed for arbitrary compiled gamecode has been demonstrated. Lack of other literal references does not exclude dynamically constructed keys.

## The renderers have different APP_LIB_PATH implementations

- **PortalVR 1.12:** initNatives supplies ApplicationInfo.nativeLibraryDir directly to APP_LIB_PATH.
- **HL2VR 0.989:** initNatives supplies nativeLibraryDir as fallback into VRLauncherTabs.selectedNativeLibraryDirectory(context, fallback), at method offset 0x15d. That delegates to RendererRuntime.directory(context, fallback), defined in classes2.dex. The normal renderer keeps the supplied fallback. Selected Turbo prepares a private directory under Context.getFilesDir(), named with a native-turbo- prefix and tied to the bundled hl2quest_turbo/manifest.tsv. RendererRuntime.load also sets APP_LIB_PATH via Android Os.setenv and loads extracted libraries using System.load.

**None of these are fixed universal on-device path values**. They depend on installation details, renderer selection, and application-private storage. The code proves source expressions, not what Quest 3 returns at runtime.

## Manifest exposure and security boundary

In the HL2VR Android manifest, org.libsdl.app.SDLActivity is marked exported=true with process=:game. PortalVR also exports SDLActivity. This supports tracing how launch Intents reach the SDL game process but **does not prove acceptance of user-installed native code**.

Phase 80 already confirmed the independently observed AArch64 filesystem loader: APP_MOD_LIB provides an optional preferred candidate location and APP_LIB_PATH the normal fallback. Native candidates use .so names and lib prefixes and reach dlopen.

**APP_MOD_LIB is therefore a real cross-layer Java-to-native configuration seam.** It remains unknown whether:
- a custom path is used by the default launcher;
- a harmless external developer-built .so is accessible from an eligible app-owned location;
- Android linker namespaces, SELinux or file permissions permit its loading;
- engine factories and C++ ABI are compatible with an independently rebuilt SiN module.

The existing srceng/mods feature distributes VPKs and other *content*. It should not be presented as a working native gamecode mod installation path.

## Reproducible verification

On the Phase 81 branch, using privately owned original APK files:

    python3 scripts/verify_sourcevr_launcher_environment.py --profile hl2vr-0989 --apk /private/HL2VR-Standalone-0.989.apk
    python3 scripts/verify_sourcevr_launcher_environment.py --profile portalvr-112 --apk /private/PortalVR_1.1.apk
    python3 -m unittest discover -s research/tests -p 'test_*.py'

The Python verifier is independently authored, uses the standard library, pins full DEX SHA-256 hashes, and validates DEX class/method tables, initNatives callsites, guarded APP_MOD_LIB initialization, ApplicationInfo.nativeLibraryDir field reference and Turbo directory delegation. All three actual DEX members passed privately. Eleven original-free synthetic DEX/ZIP regression tests passed locally; CI must rerun those independently.

The verifier does not claim a native module has loaded. Its JSON explicitly records app_mod_lib_set_by_stock_launcher_verified=false, android_linker_namespace_tested=false, external_custom_module_loaded=false and quest_runtime_tested=false.

## Phase 82 acceptance target

A **maintainer-authorized, documented Android dev environment** for independently authored native test modules, with permission to install a test .so into an executable app-private directory and observe both environment variables **inside the actual :game process**, would be the first valid proof-of-loading experiment. The experiment must use legally usable exact engine/module headers to verify factory callbacks. No copying original SiN Windows DLLs into Android is supported.

**R1 #12 and actual campaign integration #20 remain open. The APKs and proprietary content stay private.**
