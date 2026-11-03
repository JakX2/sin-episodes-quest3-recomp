# Phase 101: native sound-provider handoff between two actual shared libraries

Phase101 provides libsin_source_provider_abi.so, an independently loadable shared library with a versioned **12-function C ABI**. It dynamically loads the existing Phase100 libsin_legacy_bridge_abi.so, validates its original 11 function exports and creates isolated old/modern crosswalk state.

## Safe callback boundary

sin101_provider_v1 is size/version checked and exposes three C callback operations: name to candidate token; candidate validity; candidate token back to name. In MOCK mode only, the new bridge checks all three, requires valid native token 0..65534 and exactly matching round-trip names, then binds the mapping through Phase100. A legacy ID 70000 can map to a separate native ID 77.

**SIN101_PROVIDER_SOURCEVR_NATIVE is rejected** with SIN101_NATIVE_ABI_NOT_VERIFIED: this is no engine-level SourceVR adapter. Real selected VSoundEmitter002 native slots 7/10 are only candidate methods, with signatures and lifetimes not yet verified. Direct aliases to original 001 slots 3/6 are known unsafe.

External callbacks execute outside the context mutex; an epoch increments on provider change/reset, preventing callbacks that raced with a reset from installing stale results. Callers must coordinate destruction against in-flight operations. Returned strings are copied to caller-owned buffers.

## Files

- include/sin_compat/source_provider_c_api.h: versioned provider callback table and twelve exported function declarations
- src/source_provider_c_api.cpp: actual module, dynamic Phase100 loader, callback validation, epoch protection, error returns
- src/source_provider_exports.map: frozen twelve-symbol ELF allowlist
- tests/sin101_two_library_smoke.cpp: actual Phase100 dynamic loading and unavailable-provider checks
- tests/sin101_mock_provider_roundtrip.cpp: mock mapping 70000 -> 77 and explicit native-provider refusal
- CMakeLists.txt, Android ARM64 and emulator build scripts, GitHub Actions CI: new shared-library and Bionic integration tests

An independent local ZIP also contains a larger C11 two-library dlopen test covering missing DLL/version, conflicts, invalid tokens, name mismatches, and resetting the provider during callback execution.

## Limits

Phase101 is native loadable compatibility plumbing. It does NOT run original client.dll/server.dll in SourceVR, recreate a playable Quest 3 map, implement the full C++ sound engine ABI, produce an APK, or prove headset compatibility. Original game binaries, SourceVR ELF files and privately reconstructed 1024-byte keymap are not committed.

Next: verify actual selected SourceVR 0.5.5 VSoundEmitter002 native slots 7/10 ABI/lifetime, then implement a separately reviewed provider. Keep the VGUI InputInternal slot56 mismatch blocked.