# Phase 102: Sound-provider boundary QA and Bionic CI repair

This patch is stacked on the existing Phase 102 provider-epoch and original/native sound ABI candidate research.

## Real bug fixed

The Phase 101 token-to-name roundtrip used a zero-initialized output array. A mock provider that reported the correct required length while omitting the actual terminating NUL could pass validation because the NUL already existed in untouched storage. The output array is now prefilled with a nonzero sentinel, and `sin102_provider_lifecycle` specifically rejects a callback that omits the terminator. Valid roundtrip names still pass.

The Phase 102 GitHub Actions emulator script accidentally contained a dedented `grep` command outside the `script: |` YAML block, preventing a valid workflow. The command is removed from that position and replaced with explicit Phase 101 and Phase 102 log assertions in the post-emulator evidence step. The emulator script uses `set -euo pipefail`; the Phase 101 marker is captured into its uploaded log.

## Scope and verification

The fixes affect only synthetic/mock sound-provider callbacks and the integration test harness. The native SourceVR 0.5.5 `VSoundEmitter002` interface's candidate slots 7/10 are still ABI- and lifetime-unverified, and the real native provider remains blocked. The original SiN `VSoundEmitter001` methods are not blindly aliased.

Use CTest `sin102_provider_lifecycle` on a Linux build and inspect Android x86_64 Bionic evidence after GitHub Actions finishes. These tests do not establish real Quest 3 audio playback, a loaded SiN game client/server, or a playable VR port. No proprietary SiN DLLs or SourceVR ELF files are committed.
