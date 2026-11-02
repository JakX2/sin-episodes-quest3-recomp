# Build a separate SourceVR module-selection test APK (R1)

**Status:** Private local build workflow and synthetic regression tests. Not yet built or launched with a complete modern SourceVR APK. This is **not** a playable SiN port.

## What the builder does

The inspected SourceVR AndroidManifest.xml identifies package `com.sourcevrport.hl2vr». Our debug copy uses `com.sourcevrport.hl2vr.sinprobe» instead, preserving original fully qualified Java activity class names. The original APK and installed application are never modified.

The original Java environment sets `APP_LIB_PATH» to `ApplicationInfo.nativeLibraryDir». The earlier patcher adds `APP_MOD_LIB» with the same directory value to a **decoded copy**. The builder packages our separate, harmless ARM64 factory diagnostic under `lib/arm64-v8a/libclient.so» in that debug copy, keeping the ordinary SourceVR profile asset graph separate.

No custom module-selection behavior has been demonstrated on-device. In particular, the stock staged client and Android linker may still take precedence.

## Windows prerequisites

- The user's complete **original** SourceVR APK whose `classes.dex» SHA256 is `0dea2c61aa36557f49d125bd91607228e522fb25dbe023c1867b54d7ae7ec74a». The 29MB development subset is not an APK.
- The CI-built diagnostic ARM64 `libclient.so» from PR #60.
- Java/JDK and `keytool»; `apktool» v2.12.1 or compatible; Android SDK build-tools including `zipalign», `apksigner» and `aapt» on PATH.
- This GitHub branch's `tools» folder.

Start with the default **read-only** mode:

~~~powershell
py .\tools\sourcevr_isolated_apk_builder.py --apk "C:\SourceVR\original-sourcevr.apk" --probe "C:\SourceVR\libclient.so" --output "C:\SourceVR\sinprobe-output"
~~~

Explicitly build a private side-by-side APK (output directory **must not exist**):

~~~powershell
py .\tools\sourcevr_isolated_apk_builder.py --apk "C:\SourceVR\original-sourcevr.apk" --probe "C:\SourceVR\libclient.so" --output "C:\SourceVR\sinprobe-output" --build
~~~

The script validates the pinned original DEX and ARM64 diagnostic, decodes using apktool, changes decoded startup code and package identity, sets `android:extractNativeLibs=true», repacks and aligns the APK, signs it with a **new local disposable debug key**, and verifies its signature.

It additionally verifies the **compiled package ID** with `aapt dump badging» and checks that the **rebuilt DEX** contains both `APP_LIB_PATH» and `APP_MOD_LIB» in the patched startup method. The signed file will be `sin-sourcevr-loader-probe.apk».

**Do not publish the rebuilt APK or the debug key.** The keystore uses a known test password, not a production secret. The script does not install or run anything on Quest 3.

## What a later private install would establish

First independently check the signed package ID is different from the installed SourceVR app. On an isolated test installation the log tag `SiNFactoryProbe» could show `MODULE_LOADED» and `INTERFACE_REQUEST: VClient017», proving that the diagnostic client module was selected.

The diagnostic's `CreateInterface» returns NULL/IFACE_FAILED deliberately, so it **cannot launch gameplay**. Missing log entries would require investigating the module staging / preloaded SONAME path rather than changing game logic.

Package identity may change game-data access/import behavior; Android executable loading permissions, native library staging and engine interface ABI are all unresolved. These synthetic tests are not equivalent to a working side-by-side APK.

## Following milestone

After successful real module selection, construct a real `VClient017» and `ServerGameDLL010» client/server pair with verified C++ ABI, then incrementally connect the SiN gameplay implementation. Original 2006 Windows x86 DLLs cannot be loaded on Quest 3.
