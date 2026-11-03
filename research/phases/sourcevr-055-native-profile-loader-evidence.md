# Phase 83: SourceVR 0.5.5 native game profiles and loader path

**Scope:** read-only verification using the user's newly supplied split `source vr.z01/.z02/.zip` package. The final `source vr.zip` segment contains the central directory, all `.manifest`, all `.load` files, `version.properties`, `libsourcevrbootstrap.so` and `libhl2vrbridge.so`. Those entries were independently opened and inspected without the earlier volumes. Earlier blob payloads have **not** been extracted or rehashed.

## Version and bootstrap binaries

The embedded `source vr/assets/version.properties` records:
- `vrVersionName=0.5.5+d6c0a068`
- `vrSemver=0.5.5`
- `vrGitSha=d6c0a068426cdb9baed0106816e826e688bdd625`
- `vrUpstreamPin=ed8209cc35c61fbd8ddff8480962a01c981eef2f`

Both directly extracted small Android libraries are genuine ELF64 AArch64:
- `libsourcevrbootstrap.so` (7,136 bytes), SHA-256 `04fc065225478cb599a04e04da239ebe4d19c3aea3ce373f2adb908cb1bd9565`, exports **`ANativeActivity_onCreate`**. Its imported native symbols include `dlopen`, `dlsym` and `dlerror`, and its static strings mention `liblauncher.so` and `sourcevr/v1/native/blobs/`. That is evidence of an Android native activity bootstrap with dynamic loading, but not a complete decompilation of its loader control flow.
- `libhl2vrbridge.so` (36,288 bytes), SHA-256 `b3a8802d3b419d7480a5000549e27406ab1c213bdb3a1fd6e2743ef515d5dfbe`, exports **`JNI_OnLoad`**, device battery/thermal callbacks, bHaptics bridge methods, and crash diagnostic functions. The bridge is **not** the game `client.so` or `server.so`.

## Seven verified native profiles

Original manifest header format is `SOURCEVR_NATIVE_V1<TAB>game`. Each subsequent line is:

```text
zero-padded-index<TAB>sha256<TAB>source-soname<TAB>dest-soname<TAB>asset-path<TAB>comma-separated-dependencies-or-
```

Each `native/<game>.load` is a one-path-per-line list that matches the manifest's **original order exactly**.

| Game profile | Native modules |
|---|---:|
| `hl2` | 31 |
| `ep1` | 31 |
| `ep2` | 31 |
| `lostcoast` | 31 |
| `portal` | 31 |
| `entropyzero` | 31 |
| `portal2` | 37 |

Across all seven native profiles there are **67 distinct ELF module blob paths** listed in the ZIP directory. Every native manifest path exists in the archive index; every declared module dependency resolves to another member of that game's profile. This proves **declarative dependency closure**, not runtime `dlopen` success or valid engine API.

An additional seven `overlay/*.manifest` records cover **849 asset references** (189 unique SHA-256 blob names); seven `vr/*.manifest` records cover **1,044 asset references** (1,039 unique SHA-256 blob names). Every hash reference resolves to an entry in the ZIP index. **None of these blob contents has been audited in this phase.** No `sin` native profile exists.

## Per-game differences

Comparing the `hl2` profile's declared hashes:
- `ep1` and `ep2` differ from `hl2` in **`libclient.so`, `libengine.so`, `liblauncher.so`, `libserver.so`**.
- `lostcoast` differs in **`libengine.so` and `liblauncher.so`**.
- `portal` differs in **`libclient.so`, `libengine.so`, `liblauncher.so`, `libserver.so`, `libvphysics.so`, `libvrxr.so`**.
- `entropyzero` differs in those first four plus `libvphysics.so`.
- `portal2` adds six native multimedia modules and changes multiple engine/rendering modules.

## Version difference from HL2VR 0.989

Against the separately supplied `HL2VR-Standalone-0.989.zip` **standard** APK's actual `.so` SHA-256 inventory from PR #82: the new 0.5.5 manifest declares **0 identical hashes among 28 comparable module names** for each native game profile. The remaining modules have no same-name comparable entry in the old APK. This is a **revision/packaging difference**, not evidence of an invalid port. The manifest hashes here are declared values, while the 0.989 hashes were computed from payload bytes, so byte compatibility must not be assumed without extracting and verifying the new blobs.

## Practical consequences for SiN Episodes

This packaged SourceVR setup is **manifest-selected per game**, not a single universal pair of `libclient.so` and `libserver.so`. An eventual native SiN profile needs:
1. Correct ARM64 SiN-specific game modules and their `CreateInterface` versions / callback contracts, validated against this exact engine revision.
2. An engine/launcher pairing and dependency closure for its chosen Source branch; do not arbitrarily combine the 0.989 standalone APK's binaries with new 0.5.5 modules.
3. Explicit original-game data mounting, gameinfo/search paths, entity registration, map loading, proper message/wire and save/restore compatibility.
4. VR action/weapon/UI integration through native SourceVR/OpenXR, followed by device load smoke and in-game map tests.

The current `sin_compat` math/protocol implementations are reference code only. **No playable Quest APK, complete original Windows-to-ARM64 gameplay recompilation, proof of a usable Sin game module, or verified SourceVR loader plugin extension is claimed.**

## Deliverables and tests

- `research/tools/audit_sourcevr_native_profiles.py`: read-only final ZIP inspector for all seven native `manifest/load` pairs, dependency closure, content-addressed asset references, version metadata and SHA-pinned Android bootstrap/bridge binaries, with optional comparison against PR #82's 0.989 JSON inventory.
- `research/tests/test_sourcevr_native_profiles.py`: nine original-file-free tests for load order, missing dependencies, missing blobs, unsafe manifest paths, invalid hashes, bootstrap SHA protection and cross-revision differences.
- CI runs the tests through its existing `python3 -m unittest discover -s research/tests -p 'test_*.py' -v` workflow step. Local manual manifest checks on the supplied SourceVR package also passed.

**Next:** verify selected new native blob ELF build IDs/exports and the loader's exact library-resolution order from both split volumes, then determine whether a SiN-specific `libserver.so` can satisfy the SourceVR 0.5.5 `CreateInterface` contract. A real Quest 3 runtime smoke run would be required for device claims.
