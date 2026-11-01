# Phase 82: HL2VR launcher native-directory selection and optional module override

**Evidence:** Offline analysis of the exact owner-provided HL2VR Standalone 0.989 APK. This concerns Java/Dalvik launcher setup, **not** a supported custom game-module SDK or demonstrated external library loading. No APK, DEX, Valve library, original map source or copyrighted assets are in git.

## Exact original DEX provenance

| Original APK entry | SHA-256 |
|---|---|
| classes.dex | 7b4f4d909cb1858c30bd42c07c44962b604776653a24d623d48236e28cc42b95 |
| classes2.dex | 18c5944efa31751e90ee01fe131dab0f91183781545eefea8b5575660ea9b48c |

The owner-supplied APK itself has SHA-256 5ce5c4c16fd2aa934db45ed51de32878a561ed3dfe5405ee502ef199238a785c, matching the published 0.989 release APK (Phase 78).

## Discovery: APP_MOD_LIB comes from optional Android Intent extra

In classes.dex, **ValveActivity2.initNatives**:

- Offset 0xd0ca: loads the literal **gamelibdir**.
- Offset 0xd0ce: calls Android **Intent.getStringExtra**.
- Offset 0xd0d4: stores the result into register v4.
- Offsets 0xd208 and 0xd20c–0xd214: skip the subsequent assignment if v4 is null or the string is empty.
- Offsets 0xd218–0xd21c: only for a nonempty string, invokes **setenv("APP_MOD_LIB", v4)**.

Thus APP_MOD_LIB is a conditional *caller-supplied gamelibdir Intent extra*. It is **not** automatically the ordinary user content-mod directory. Phase 80 established that the native filesystem loader consults APP_MOD_LIB, making this an interesting possible override. It does not establish that the Android Activity is externally accessible, that arbitrary directories are executable, or that the engine accepts a rebuilt SiN game module.

## Normal vs Turbo APP_LIB_PATH

The same initNatives code reads Android ApplicationInfo.nativeLibraryDir (offset 0xd30e), passes the fallback to VRLauncherTabs.selectedNativeLibraryDirectory (0xd312), then sets APP_LIB_PATH (0xd31a).

The second DEX implements **VRLauncherTabs.selectedNativeLibraryDirectory** as a call into **RendererRuntime.directory** at 0xe4c8. That method, at 0x65f8–0x6628:

1. Calls **RendererRuntime.selected**.
2. If Turbo is **not** selected, returns the input fallback string, which was the application's installed nativeLibraryDir.
3. If Turbo **is** selected, calls **RendererRuntime.prepare**, obtains the File's absolute path, and returns it.
4. Throws an error if the Turbo runtime preparation fails.

**RendererRuntime.selected** (0x5f2c–0x5fc6) checks the campaign directory, then excludes Portal and Portal 2 from Turbo selection. For the other campaigns it reads the **launcher_release_legacy_gles_backend** shared preference; an optional **legacy_gles** launch Intent flag can override that preference.

## Turbo native library preparation and initialization

**RendererRuntime.prepare** (starts 0x5fd8) opens the packaged **hl2quest_turbo/manifest.tsv** asset, computes its digest and creates an app-private directory under **getFilesDir()**, with a **native-turbo-** prefix plus the manifest-derived hash. It unpacks and validates packaged native libraries there, and records the prepared directory.

**RendererRuntime.load** (starts 0x5d80) is gated on Turbo selection. It calls prepare, sets **APP_LIB_PATH** to the prepared directory with Android Os.setenv, configures **HL2QUEST_GLES_LEGACY=1** and **HL2QUEST_DXVK_RENDERER=0**, and loads bundled native dependencies using Java System.load (plus the SDL bootstrap).

The original code does not demonstrate installation of a user-compiled SiN gameplay library. Neither a successful path check nor a native factory export proves Android linker access or C++ engine compatibility.

## What this changes for the recompilation

| Confirmed static fact | Project implication |
|---|---|
| Normal mode uses APK-installed nativeLibraryDir | No evidence that /sdcard/srceng contains a valid native plugin directory |
| Turbo uses an app-private, asset-extracted, digest-named native folder | Backend selection changes the module search roots and dependencies |
| APP_MOD_LIB comes from a nonempty gamelibdir Intent extra | Candidate for a **controlled developer investigation**, not a proven plug-in interface |
| Portal and Portal 2 are excluded from Turbo selection in this release | Do not assume all bundled clients share a selectable runtime |
| Phase 79 requires VClient017 / ServerGameDLL010 negotiation | Identifying a native path does not establish ABI compatibility |

The ordinary HL2VR user-mod UI still describes VPK/config/content mounting, **not** arbitrary native gamecode support.

## Independent reproduction

With a separately held original 0.989 APK:

    python3 scripts/verify_hl2vr_renderer_contract.py /private/HL2VR-Standalone-0.989.apk
    python3 -m unittest discover -s research/tests -p 'test_*.py' -v

The independent verifier pins both complete DEX digests, resolves named methods from class-data tables, checks selected opcodes/branches inside their method bounds, and prints a short metadata-only report. **Twelve synthetic original-free tests passed locally**, including a handmade miniature DEX, altered references, malformed tables, truncated code and unpinned binary rejection. CI will independently run synthetic tests, not the owner-private APK audit.

## R1 remains blocked

Next we need (1) a supported/authorized Android developer launch workflow to supply gamelibdir and **observe** the runtime environment; (2) a synthetic independently authored .so loading test under actual linker permissions; and (3) usable licensed SourceVR C++ interface headers before building a compatible SiN client/server module.

**No patched APK, custom .so load, working SiN game module, Quest device test or campaign playback demonstrated.** Issues #12 and #20 remain open.
