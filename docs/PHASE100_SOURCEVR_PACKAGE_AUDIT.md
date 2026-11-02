# Phase 100: SourceVR Android package interface reconnaissance

**Scope:** original-free, read-only tooling. This phase does **not** implement or demonstrate a real SourceVR engine adapter, SiN game-module support, OpenXR or headset gameplay.

## Evidence from the owner-provided file listing

The owner provided a text listing of an extracted SourceVR APK, not its libraries' contents. It includes 1,436 paths, with:

- 1,228 opaque content-addressed entries in \`assets/sourcevr/v1/blobs/sha256/\`.
- 67 native-library variants under \`assets/sourcevr/v1/native/blobs/<SHA256>/lib*.so\`, including names \`libengine.so\`, \`libclient.so\`, \`libserver.so\`, \`liblauncher.so\`, \`libvrxr.so\`, \`libopenxr_loader.so\`.
- Android \`lib/arm64-v8a/\` libraries \`libhl2vrbridge.so\`, \`libsourcevrbootstrap.so\`, \`libc++_shared.so\`, \`libzstd-jni-1.5.7-6.so\`.
- Native \`.manifest\` and \`.load\` pairs for \`hl2\`, \`ep1\`, \`ep2\`, \`lostcoast\`, \`portal\`, \`portal2\`, \`entropyzero\`, along with overlay and VR manifests.
- No explicit SiN native/VR manifest entry in the listing.

These observations establish **only file presence/naming**. They do not establish ELF architecture of content-addressed modules, matching engine versions, symbol exports, which variant loads for a title, or support for custom SiN code.

SourceVR's published documentation describes support for *content-only* modifications and notes restrictions on mods with their own \`client.dll\`/\`server.dll\`; verify the actual installed SourceVR build/version and permitted integration route before any runtime adapter. See https://github.com/tinsarfal/SourceVR .

## Offline original-free audit

\`\`\`sh
python3 scripts/sourcevr_package_audit.py /path/to/SourceVR_FileList.txt -o sourcevr-audit.json
# Or directly inspect the APK without extracting it:
python3 scripts/sourcevr_package_audit.py /path/to/sourcevr.apk -o sourcevr-audit.json
# Or inspect an already extracted SourceVR directory:
python3 scripts/sourcevr_package_audit.py /path/to/extracted_apk -o sourcevr-audit.json
python3 -m unittest discover -s research/tests -p 'test_sourcevr_package_audit.py'
\`\`\`

No network, external dependencies, binary execution or archive extraction. The tool reads the ZIP central directory, **at most 512 bytes** per relevant native header/manifest, and does not copy proprietary module bytes into the report. It distinguishes listing-only evidence from read ELF-header architecture (AArch64/x86/x86_64). It previews only small, plain-text metadata and encodes binary metadata only as a 32-byte hex prefix. RAR and 7z archives are not parsed: point the tool at their extracted directory instead.

**Privacy/IP:** Keep \`*.apk\`, \`*.so\`, original manifests, proprietary gameplay files and local audit JSON outside Git and outside published CI artifacts. The JSON may contain a short metadata preview and should be reviewed before sharing.

## Engine integration gate

1. Owner locally runs the auditor against the extracted APK (or APK directly), and privately inspects the \`hl2.load\`, \`hl2.manifest\`, \`portal.manifest\`, \`hl2vr.manifest\`, and \`portalvr.manifest\` formats. Our earlier archive-reading environment failed; contents have **not** been verified here.
2. Map native manifest -> selected \`libengine.so\`/\`libclient.so\`/\`libserver.so\` and compatible VR library variant. Determine whether the intended SiN target uses one of the already present Source versions.
3. Locate an appropriately licensed engine/SDK with real Android ARM64 headers; verify supported gameplay module loading before implementing host callbacks or vtable bridges.
4. Demonstrate a real host/module lifecycle against that engine and one genuine Source entity/weapon before claiming R1 complete.

Existing \`sin_compat_host_abi\` is independently authored and **not** a SourceVR client/server ABI. Phase100 is instrumentation to choose the actual ABI without guessing. Keep all owner-held game and APK content private.
