## Resume a previously decoded build after the setupEngineEnvironment signature error

If Apktool fully decoded the private source, but the builder failed with "expected one setupEngineEnvironment()V method", **do not rerun Apktool decode**. This error came from a bad assumption in the original patcher: the verified SourceVR DEX declares:

~~~text
setupEngineEnvironment(Ljava/util/List;Ljava/util/List;Lcom/sourcevrport/hl2vr/ProfileAssetStage$NativeGraph;)V
~~~

The patcher now matches that exact method descriptor and verifies its APP_LIB_PATH setter instructions.

On Windows, with the previous failed directory still at Desktop\sinprobe-build, run these commands from the **updated repository checkout**:

~~~powershell
powershell -NoProfile -ExecutionPolicy Bypass -File ".\resume_sourcevr_probe.ps1"
powershell -NoProfile -ExecutionPolicy Bypass -File ".\resume_sourcevr_probe.ps1" -Build
~~~

The first command is read-only and checks the previous decoded MainActivity.smali, its original input archive, stage completeness and existing output conflicts. The second is opt-in and continues to modifying the *decoded copy*, rebuilding, aligning, signing and inspecting the separate debug APK.

The resume option rejects unknown original DEX fingerprints, source tree changes affecting the reconstructed archive member list, previously patched code, and any build stage that may be partially completed. It **does not overwrite** an installed APK or original SourceVR game data.

If the files have moved, pass -SourceDir, -Probe, -BuildOutput or -ToolRoot. The resume mechanism only handles the specific failure point **before** any APK build/signing stages. It cannot safely resume arbitrary later build errors.

# R1: SourceVR diagnostic APK from original extracted folder

**Result:** A complete modern SourceVR APK upload is **no longer required** to prepare the private launcher experiment, if the owner's original extracted SourceVR directory is intact.

The original directory observed in the user's file listing contains 1,436 entries including:
- `AndroidManifest.xml», `resources.arsc»
- `classes.dex», `classes2.dex», `classes3.dex», `classes4.dex», `classes5.dex»
- ARM64 bootstrap and VR bridge shared libraries
- Native and overlay assets, including all 31 entries referenced by both the HL2 and Entropy: Zero native `.load» manifests.

These findings are from the earlier file listing; they do not prove every file is currently readable or unchanged on the user's Windows PC. The build script independently checks each relevant file when run.

## What has changed

The builder from PR #61 now also accepts `--extracted-dir» instead of `--apk». It first checks:
- The exact known `classes.dex» SHA-256 fingerprint.
- Binary AndroidManifest and compiled resources table.
- Five contiguous DEX files.
- Required ARM64 bootstrap/bridge binaries.
- Both HL2 and Entropy: Zero 31-module native load graphs, including presence of every referenced private blob.
- No symlinks or duplicate case-insensitive entry names.
- Output directory does not already exist and is not nested inside the original source.

It streams every file into `original-extracted-unsigned.apk» inside the **new output directory**, excluding only stale original `META-INF» signing files. The result is **not an installable APK**; it is an unsigned transport ZIP for apktool to decode. The existing isolated builder then modifies the decoded copy, packages the diagnostic ARM64 `libclient.so», rebuilds and signs a separate app identity.

No proprietary binaries, original game files or reconstructed APKs are committed to GitHub.

## Windows PowerShell workflow

The **first check requires only Python 3**, no Android SDK and no diagnostic library:

~~~powershell
py .\tools\sourcevr_extracted_apk.py "C:\Users\Dav\Desktop\source vr"
~~~

Expected: a JSON report reporting **five DEX files**, **31 HL2 native modules**, and **31 Entropy: Zero native modules**. If any files were lost or changed during extraction, the tool fails and names the missing entry. This check is read-only and does not create or sign any APK.


In a clone of branch `feat/sourcevr-extracted-folder-build», with `apktool», `zipalign», `aapt», `apksigner» and JDK `keytool» installed:

First get the ARM64 diagnostic `libclient.so» from the passing GitHub Actions artifact for the `ARM64 Source factory diagnostic» workflow on this branch.

Dry-run without modifying anything:

~~~powershell
py .\tools\sourcevr_isolated_apk_builder.py --extracted-dir "C:\Users\Dav\Desktop\source vr" --probe "C:\SourceVR\probe\libclient.so" --output "C:\SourceVR\sinprobe-output"
~~~

To create the isolated, locally signed debug APK **only after the dry-run passes**:

~~~powershell
py .\tools\sourcevr_isolated_apk_builder.py --extracted-dir "C:\Users\Dav\Desktop\source vr" --probe "C:\SourceVR\probe\libclient.so" --output "C:\SourceVR\sinprobe-output" --build
~~~

The builder will produce `sin-sourcevr-loader-probe.apk», `build-report.json», source/decoded work directories, and a local disposable signing key. It **does not install or launch** the APK and does not modify SourceVR on the user's device.

If the original source folder is at a different location, change only `--extracted-dir». The output path must be outside the original folder and not exist before a build.

## Limitations and next decisive step

- CI currently exercises this construction with **synthetic fixtures**. It has not completed a real 600 MB SourceVR rebuild, installation or Quest run.
- Repacking with apktool may expose resource or compression incompatibilities that synthetic tests cannot predict. The user's full local extracted directory is needed for the first genuine build.
- Even if the separate APK builds and launches, its staged native libraries and Android SONAME handling may still select the original HL2 client. Logcat must establish that `SiNFactoryProbe» logged `INTERFACE_REQUEST: VClient017» from the custom module.
- The diagnostic always returns interface failure; a playable SiN build requires real versioned client/server interfaces and game implementation.

**No more uploading gigantic APK parts is necessary for this particular build attempt.** The original extracted folder on Windows is the intended input.
