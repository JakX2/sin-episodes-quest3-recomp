# Phase 83: Exported Android Activity and Intent-to-APP_MOD_LIB path

**Evidence:** Offline inspection of the owner-provided exact HL2VR Standalone 0.989 APK, including Android binary XML and DEX method instructions. No game module, patched APK, or Quest device launched. Issues #12 and #20 remain open.

## Original build evidence

- APK SHA-256: 5ce5c4c16fd2aa934db45ed51de32878a561ed3dfe5405ee502ef199238a785c
- AndroidManifest.xml SHA-256: 0fb8c391729ec37666039b5131de84eed4cd5293a6cb5ca3cbda57c1c62f8c52
- classes.dex SHA-256: 7b4f4d909cb1858c30bd42c07c44962b604776653a24d623d48236e28cc42b95

### A real exported game Activity

The original manifest declares package **com.valvesoftware.source.vrtest**, target SDK **24**, and an Android Activity with:

| Field | Original declaration |
|---|---|
| Component | org.libsdl.app.SDLActivity |
| exported | true (explicit) |
| process | :game |
| launchMode | singleTask |
| Intent action | org.libsdl.app.SDLACTIVITY |
| Intent categories | android.intent.category.DEFAULT and OpenXR headset |
| Activity permission | Not specified |
| application debuggable | false |

This is an actual registered, exported Activity. It is different from the launcher's MAIN Activity. The static manifest does not prove external launching succeeds on an installed Quest.

## Instruction-level proof that the Intent reaches native setup

The classes.dex file contains **org.libsdl.app.SDLActivity**, and a separate helper **com.valvesoftware.ValveActivity2**. The latter is a static utility class, not an Activity. Exact DEX callsites from the pinned file:

1. At **0x1107e**, SDLActivity.onCreate invokes SDLActivity.init.
2. At **0x10978**, SDLActivity.init calls getIntent, and at **0x1097e** stores the returned Intent in register v7.
3. At **0x10980**, SDLActivity.init invokes ValveActivity2.initNatives with the Activity and the v7 Intent; the verifier checks the invoked method and argument register encoding.
4. At **0xd0ca / 0xd0ce**, initNatives obtains the string Intent extra **gamelibdir**.
5. If the extra is nonnull and nonempty, **0xd218 / 0xd21c** sets **APP_MOD_LIB** to its value (Phase 82).
6. Phase 80 separately verified that the native filesystem module loader consults APP_MOD_LIB before its default APP_LIB_PATH location.

**Interpretation:** the original APK supplies a code path from an externally declared Activity and its Intent to a native library search-root override. This is not proof that arbitrary AArch64 gamecode loads or that a reconstructed SiN module is ABI compatible.

## Reproduction

Run against the separately held, original APK:

    python3 scripts/audit_hl2vr_activity_entry.py /private/HL2VR-Standalone-0.989.apk
    python3 -m unittest discover -s research/tests -p 'test_*.py'

The independently authored auditor validates the exact APK, Manifest and DEX hashes, parses Android binary XML, checks the exported component and action, and verifies the DEX callsite/argument chain. The tool emits metadata only. No original manifest, DEX, APK, native library or proprietary game assets are checked into GitHub.

**Fourteen original-free synthetic tests passed locally**, including UTF-16/UTF-8 XML, malformed resource bounds, missing or unexported Activity, wrong action, and unpinned input rejection. GitHub CI can run the synthetic tests but not the original APK audit unless the original is privately supplied.

## Safe developer investigation procedure

**A. First, inspect only.** On a Quest where the owner has installed HL2VR and enabled authorized ADB access, these commands inspect package state without launching it:

    adb devices
    adb shell pm path com.valvesoftware.source.vrtest
    adb shell dumpsys package com.valvesoftware.source.vrtest

**B. Explicit Activity invocation is an experiment, not an established working flow.** Do not use a random /sdcard folder or guess an APK-private directory. The following template would start the original app, and can fail or crash, so use only on an authorized development device once a legitimate executable module location has been established:

    adb shell am start -n com.valvesoftware.source.vrtest/org.libsdl.app.SDLActivity -a org.libsdl.app.SDLACTIVITY --es gamelibdir /PATH/TO/AUTHORIZED/NATIVE/LIB/DIR

An exported Activity does not defeat Android native linker namespace, file execute/mount restrictions, game-module ABI mismatches or application lifecycle requirements. Neither this command nor the owner's headset has been run by this audit.

**C. Before any SiN replacement:** obtain an authorized buildable native SourceVR/game module development environment, load a tiny independently authored ARM64 test module under documented Android permissions, validate real Source CreateInterface callbacks, then integrate gameplay and test actual SiN campaign levels. Do not claim the R1 integration blocker resolved just from the Intent chain.

**Conclusion:** We have confirmed a genuine Activity-to-environment search-root path and narrowed the next live compatibility experiment, but **no SiN gamecode or Quest runtime has loaded**.
