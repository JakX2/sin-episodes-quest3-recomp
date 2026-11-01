# Phase 75: Engine-source eligibility and pinned provenance

**Status:** Reference-only research and a synthetic probe. **R1 remains blocked.** No engine module loader, real SiN map or Quest runtime was compiled or tested in this phase.

## Why

Phase 74 supplies independently authored gameplay entity output scheduling and standalone Android native ABI. It is not a SourceVR game module. An engine host implementing `IWeaponSessionHost` or `IEntityOutputHost` cannot be built honestly against arbitrary guessed Source headers.

The publicly visible [tinsarfal/SourceVR](https://github.com/tinsarfal/SourceVR) repository at `0cf2ab64458abda88006c689f921503666c09b67` is not a public SDK checkout with C++ engine/game module headers. Its documented public release does not provide an established third-party game-DLL loader. **Do not infer a loader from the existence of an APK.**

The official [Valve Source SDK 2013](https://github.com/ValveSoftware/source-sdk-2013) provides genuine Source game interfaces. It is **reference-only here**: different engine vintage and license, with no validated SourceVR/SiN 2006/Android ABI compatibility. Its SOURCE 1 SDK LICENSE is not the MIT license of this project's own code.

## Pinned candidate

File: [`research/engine-candidates/valve-sdk-2013-reference.json`](../research/engine-candidates/valve-sdk-2013-reference.json)

- Repository: `https://github.com/ValveSoftware/source-sdk-2013`
- Commit: `be57e3d01ac7d7aa6a2cacbfef187c0b46616db0`
- Included exact Git *blob* SHA-1 checks: license, engine interface, server interface, gameplay logic, combat weapon shared interface.
- Original SDK files are **not** copied into this repository.

Run with a separately acquired checkout:

```sh
git clone https://github.com/ValveSoftware/source-sdk-2013.git /tmp/sin-sdk-reference
git -C /tmp/sin-sdk-reference checkout --detach be57e3d01ac7d7aa6a2cacbfef187c0b46616db0
python3 scripts/probe_engine_source.py --manifest research/engine-candidates/valve-sdk-2013-reference.json --checkout /tmp/sin-sdk-reference
```

The probe checks origin, exact HEAD commit, tracked license/header/source paths and their current content-addressed Git blobs. A successful probe **only** proves checked-out source provenance. Review the actual upstream license before development. A source checkout might contain other changes or have local metadata changes beyond the selected hash set.

## Verification and limits

`research/tests/test_engine_source_probe.py` supplies **12 synthetic regression cases** for a temporary fake Git repository: valid pinned checkout, CLI JSON/exit contract, wrong commit/repo, mutated/missing file, traversal/symlink escape, license requirement, unsupported role and upstream URL, and CLI error handling.

Reproduce with:

```sh
python3 -m unittest discover -s research/tests -p 'test_*.py' -v
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

**Not demonstrated:** Source SDK game adapter compiling for Quest, loader accepting custom gameplay libraries, runtime entity input semantics, original SiN DLL ABI compatibility, playable BSP, physical HMD. Local tests and CI are not replacements for those proof requirements.

## Next milestone / external dependency

To close [R1 issue #12](https://github.com/JakX2/sin-episodes-quest3-recomp/issues/12), obtain a legally usable, **accessible** Quest-capable engine or public module SDK **with a loader path for game code**, pin its revision and license, then compile a minimal real adapter against its actual headers before calling the milestone done. Until such an engine is available, keep portable R0 integration work clearly separate from engine execution.
