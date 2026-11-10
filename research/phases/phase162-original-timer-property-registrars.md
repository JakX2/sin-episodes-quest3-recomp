# Phase 162: original Breath and Poison Color/MutagenBar native HUD properties

**10 November 2026.** Continuation of the RTTI-corrected Phase161 original SiN Episodes: Emergence (2006) Windows PE32 client.dll decompilation. The owner-held DLLs were SHA256 verified:

- client.dll: 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d
- server.dll: 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650

This phase recovers the *original metadata and once-registration behavior* for all three CHudBreathTimer and four CHudPoisonTimer properties. No original DLL bytes, licensed game code or texture assets were committed.

## Seven real original 96-byte guarded registrar methods

Every entry below is an independently SHA-pinned original function body and points to real strings within the 2006 executable. Its code reads a unique global guard byte, returns immediately if it is already nonzero, sets the guard to 1 before invoking the original owner resolver at 0x24011DC0, builds a metadata descriptor, and calls the native descriptor-array insertion routine at 0x2400A300. Phase163 verifies that it appends a 24-byte six-DWORD record, not that it parses Color or string values. The complete native parser/registration engine is **not reconstructed**. Our C++ reference models only metadata and the exactly observed guard/callback order.

| Original function | Type byte | Property name | Native source member | Original default |
|---|---:|---|---|---|
| 0x24230DE0 | 0 | BreathTimerColor | m_breathTimerColor | 0 0 0 0 |
| 0x24230E50 | 1 | BreathBarForeground | m_breathBarForegroundTextureName | Breath_bar_foreground |
| 0x24230EC0 | 1 | BreathBarBackground | m_breathBarBackgroundTextureName | Breath_bar_background |
| 0x24236450 | 0 | PoisonTimerColor | m_poisonTimerColor | 255 255 255 0 |
| 0x242364C0 | 1 | MutagenBarForeground | m_mutagenBarForegroundTextureName | Mutagen_bar_foreground |
| 0x24236530 | 1 | MutagenBarBackground | m_mutagenBarBackgroundTextureName | Mutagen_bar_background |
| 0x242365A0 | 1 | MutagenBarInactive | m_mutagenBarInactiveTextureName | Mutagen_bar_inactive |

Actual type strings in .rdata are Color for byte-kind 0 and string for byte-kind 1. These are literal data from the shipped executable, NOT inferred assets or a modern Source SDK substitute. The Poison HUD uses the internal **MutagenBar** names for its three texture configuration properties, including a separate inactive texture that Breath lacks.

### Complete original code-region SHA256s

| Entry | Native bytes | Full original SHA256 |
|---|---:|---|
| 0x24230DE0 | 96 | efd0b2d7313e3cc2b6171ced8f771b8688205a6efe5bd4f4a1d903aa7caf64ef |
| 0x24230E50 | 96 | a677055002b590862fb1bae7f3b9803f269f6acee348c7420f9770da01be1419 |
| 0x24230EC0 | 96 | 7cce7da6f28c95bdb469aacb21d0543b14a273b2e0744b1342e63567860a07f4 |
| 0x24236450 | 96 | f580f5a30eb2da9e39f237ce4b82ab4893e6be1d1385a5b465b0685cc32af7a3 |
| 0x242364C0 | 96 | 391237abf11e211c1c64ef5da5eafc69bdc179e865b7c49926c130875ab0c7a6 |
| 0x24236530 | 96 | dfc9a2b9b13923550539a0b052199dbaf15e7486a8471f44646d846eaff3497c |
| 0x242365A0 | 96 | 75881836e013ae46e23c1cb8e375b64c49da116cdc9390441c4184af2581451f |

The original registration body pattern is confirmed through sparse original x86 byte witnesses:

- At original entry+0x03: CMP global guard byte with zero.
- At +0x0A: JNE skip registration.
- At +0x0C: PUSH the actual class string, either CHudBreathTimer or CHudPoisonTimer.
- At +0x11: MOV that unique guard byte to 1, before the native registry callbacks.
- At +0x18: CALL native owner lookup 0x24011DC0.
- At +0x2A/+0x32/+0x3A/+0x42/+0x4A: store pointers to external property name, original member name, type string, default text and member resolver function.
- At +0x52: set the original kind byte, 0 for Color or 1 for string.
- At +0x57: CALL original 24-byte property descriptor dynamic-array insertion helper 0x2400A300 (Phase163).
- At +0x5C: restore the native 24-byte temporary stack frame and return.

The original constructor CALLs establish actual class/registration order: Breath 0x24230FC2 -> Color, 0x24230FDF -> Foreground, 0x24230FE4 -> Background; Poison 0x242366A2 -> Color, 0x242366BF -> Foreground, 0x242366C4 -> Background, 0x242366C9 -> Inactive. All 7 constructor edges are exact x86 rel32 instructions.

## Critical connection to previous phases: the +0xA6 byte is part of Color

The original member pointer helper for each timer Color returns Panel-relative +0xA3 and its full-object offset helper returns +0xB7. The panel subobject starts at full-object +0x14. Both RTTI-verified constructors explicitly zero full-object +0xB7..+0xBA, corresponding to the four Color bytes at Panel +0xA3..+0xA6. The previously unknown *status byte* +0xA6 in Phase154 poison and Phase155 breath is the **fourth byte of these native Color fields**.

The native Color type and textual defaults prove ownership; the exact parser, byte-channel mapping, eventual alpha updates, class-specific transition callbacks and live on-screen appearance still need verification. The default values differ: Breath "0 0 0 0", Poison "255 255 255 0". This is an important fidelity detail, not a reason to hardcode a permanent zero alpha at runtime. Earlier reference comments have been corrected.

### Exact original property field offsets

| Native property | Panel offset | Complete-object offset | Original member-accessor VA |
|---|---:|---:|---|
| either Color | 0xA3 | 0xB7 | 0x242309F0 (breath), 0x24236030 (poison) |
| either Foreground | 0xA8 | 0xBC | 0x24230A10 (breath), 0x24236050 (poison) |
| either Background | 0xC9 | 0xDD | 0x24230A30 (breath), 0x24236070 (poison) |
| poison Inactive | 0xEA | 0xFE | 0x24236090 |

The background/inactive field resolver code has an unusual original fall-through behavior: when the supplied original address is zero, it reaches a second entrypoint returning the complete-object offset (0xDD or 0xFE), rather than the zero-pointer result used by earlier Color/Foreground helpers. We preserve this distinction in the reference and tests but do not count overlapping original code as an extra fully recovered function entry.

## Source / tests / limitations

- include/sin_compat/original_client_timer_property_registrars.hpp: independent portable C++17 table of all seven exact original metadata descriptors with per-property guard state and a host registration-observer callback. Guard=1 is written before callback to capture native one-time/reentrant semantics, but no actual engine property parser is invented.
- tests/test_original_client_timer_property_registrars.cpp: tests original names/defaults, type/kind byte, offsets and real registration order, independent guards, reentrant callback short-circuit, nonnull pointer wrap and original late-resource null fall-through, and integration with the corrected Phase161 constructor field-offset contract.
- research/tools/audit_sin_original_client_timer_property_registrars.py: chained exact private original DLL fingerprint and Phase161 RTTI verification; checks all seven full 96-byte registration function SHA256s, each native guard/order opcode and descriptor member pointer, class/type/default actual NUL-terminated strings, native lookup/dispatch CALLs and seven constructor CALL edges.
- research/tests/test_sin_original_client_timer_property_registrars.py: original-free synthetic byte/call/string mutations, verifies no sparse fixture satisfies a true original function-region hash.
- CMake/CTest and all platform compilation CI registration.
- Recovery atlas adds **seven original native registrar functions** as machine_code_verified with independently modeled metadata/guard semantics but incomplete native engine callback behavior. Total becomes **69 reviewed original methods** (25 server, 44 client). This count is a verified sampling of functions, not total reverse-engineering completion.

Owner-only exact executable audit:

    python3 research/tools/audit_sin_original_client_timer_property_registrars.py --client /private/client.dll --server /private/server.dll --output /private/phase162.json

No complete replacement Windows x86 Source-engine client.dll, live VGUI renderer, Android game binary or Quest 3 standalone gameplay has yet been demonstrated. Next target: native Color parser/renderer setter, visibility/alpha update callsites and more non-HUD player/weapon functionality in both original modules.
