# SourceVR ARM64: APP_MOD_LIB dispatch path (R1)

**Status:** Offline, read-only reconstruction of the native loader's control flow, verified against the owner's private SourceVR HL2 and Entropy: Zero ARM64 engines and launchers. It is not a live module-override demonstration.

## Concrete result

The SourceVR engine reads two native-library search configuration variables:

- **APP_LIB_PATH**, the ordinary application native-library path.
- **APP_MOD_LIB**, a separate candidate location for mod-native libraries.

The engine's module-loader routine around ELF-relative address 0x53f2c0 evaluates APP_MOD_LIB first. At 0x53f3f8 it tests a constructed module pathname; at 0x53f408 it attempts to open the candidate through its dlopen wrapper. If a candidate exists but loading fails, the path at 0x53f41c raises a fatal error.

The saved mod-library handle can be returned at 0x53f454–0x53f45c after a subsequent APP_LIB_PATH candidate check. **Crucial nuance:** if the ordinary APP_LIB_PATH candidate doesn't resolve, another SONAME fallback path runs. Therefore static disassembly establishes a mod-library *candidate route*, not an unconditional override.

The naming helper around 0x550b80 normalizes request extensions for Android. Engine requests for client.dll and server.so can correspond to the native candidates libclient.so and libserver.so. This does not make the older SiN Windows DLLs compatible.

## Launcher behavior

At ELF-relative 0x37308–0x37390, the native launcher checks APP_LIB_PATH. If it is **unset**, it determines its own directory and assigns **both APP_LIB_PATH and APP_MOD_LIB** to that directory. If APP_LIB_PATH is already set, this default block is skipped.

The provided APK classes.dex includes a literal APP_LIB_PATH string but **does not include the literal APP_MOD_LIB**. That is only string evidence, not a complete analysis of Java data flow or an assurance the app cannot configure the latter indirectly.

## Corrected staging order

The two verified native .load files each have **31 staged native libraries**:

| Name | HL2 stage index | Entropy: Zero stage index |
|---|---:|---:|
| libclient.so | 13 | 13 |
| libengine.so | 14 | 14 |
| libserver.so | 18 | 18 |
| liblauncher.so | 30 | 30 |

Both game libraries are staged before the launcher, **but libserver.so is staged after libengine.so**. Earlier generalized language that both preceded the engine was incorrect. The client and server ELF files also advertise matching DT_SONAME values, increasing the importance of verifying how the Android linker resolves already staged module names. Manifest position alone cannot prove the actual linker handle used.

## Reproduce with your private files

Run the independently authored Python 3 script on local extracted data:

    python tools/sourcevr_mod_dispatch_audit.py --profile hl2 --engine /path/to/hl2/libengine.so --launcher /path/to/hl2/liblauncher.so --load /path/to/metadata/native/hl2.load --dex /path/to/apk/classes.dex --output hl2_dispatch.json

To run the synthetic regression suite:

    python -m unittest discover -s tools/tests -v

The tool checks actual ARM64 ADRP/ADD references and control-flow branches, pins SHA256 hashes for the two examined versions, reads the native load graph and fails closed for unknown binaries. Outputs contain **no proprietary executable bytes**.

## Next engine gate

We need **an observable native module-selection trace**, preferably using a disposable test launcher before touching the Quest installation, to answer four questions:

1. Does the real Android app process set APP_MOD_LIB or allow an alternative directory?
2. Which physical libclient.so / libserver.so gets opened with both default and mod-native path candidates?
3. Does preloading a stock library's SONAME override a module requested by absolute path?
4. Can a correctly built game-module factory satisfy VClient017 and ServerGameDLL010 with verified method layouts?

Until then, do not rename the existing sin_weapon_bridge as a game DLL, overwrite working APK libraries, or claim SourceVR runs custom gameplay. No original game data or engine binaries are committed.
