# Phase 98: original 2006 SiN engine DLLs vs SourceVR 0.5.5 ARM64 method ABI

## New primary evidence: the real original implementations

Alongside the original client.dll and server.dll, the user's original `bin 1.zip` includes **`bin/SoundEmitterSystem.dll`** and **`bin/vgui2.dll`** from 2006. These can be compared directly with actual SourceVR 0.5.5 ARM64 `libsoundemittersystem.so` and `libvgui2.so`, rather than guessing legacy C++ signatures from modern SDK headers.

All six original/native binaries were inspected privately. The read-only auditor chains all **Phase97 original client/server and selected ARM64 ELF full-SHA256 checks**, then pins both newly recovered original engine DLLs:

| Original file | Exact full SHA-256 | `CreateInterface` |
|---|---|---|
| `bin/SoundEmitterSystem.dll` | `8c92cdc58f366cde07a1e14b8d95bfc36abdb552242508993189479faa79dba5` | `0x2C001230` |
| `bin/vgui2.dll` | `b6a6a16f41e3dac8f5ffee1803b71d97bc4be376f0ee0e9d3fe68228001dd897` | `0x1000A520` |

No proprietary DLL, native ELF or original disassembly bytes are committed.

## Actual legacy sound emitter: 36 methods, not just an interface string

Original sound factory registration code at `0x2C0183D0` installs **`VSoundEmitter001`** using callback `0x2C004BF0`, which returns singleton `0x2C01DA58`. Original constructor `0x2C0096D0` installs vtable `0x2C019C88`.

Its **36 full contiguous method pointers** have SHA256 `a774e77a808e744d441a4095b7c3bab8d0f3a2611aea834b7b99382268139c92`. Two original semantics are now verified from the implementing DLL:

- **Old virtual slot 3** at `0x2C00A830`, 23-byte x86 method: accepts name-like string, calls `0x2C00A5E0` on a field at `this+0x5C`, returns 32-bit token or `0xFFFFFFFF`.
- **Old virtual slot 6** at `0x2C008FC0`, 71-byte x86 method: accepts full **signed 32-bit token**, verifies it via the original object's slot 4 (`+0x10`) and returns a string pointer or empty fallback `0x2C01963C`. The original slot-4 validator checks nonnegative index against the actual record count at `this+0x70`.

### Selected native VSoundEmitter002: two candidates, NOT a verified bridge

Phase97 proved that **native 002 slot 3 and slot 6 are entirely different operations**. This phase traces alternative native vtable functions:

| Operation needed by SiN | Old 001 method | SourceVR 002 candidate | Observed native behavior |
|---|---|---|---|
| name → token | slot 3, `0x2C00A830` | **slot 7**, `0x133A0` | accepts a string-like pointer, computes name hash/lookup, returns **16-bit table index or -1** |
| token → name | slot 6, `0x2C008FC0` | **slot 10**, `0x13570` | checks token using native slot 8, **masks token to 16 bits**, retrieves char-like pointer from table or empty fallback |

Native index 7 is 336 SHA-pinned bytes, SHA256 `f2fd0908b3a15993aa23cda25464009aaedd3a250d2a156e73657e13a86aa07d`; index 10 is 88 SHA-pinned bytes, SHA256 `4ab11e12272f034525b4f4ea1b4f5603d0700263c141385a8bc62cf02a8ed55e`.

**Important:** Both are *functionally plausible* but **not ABI-approved**. In particular, full signed-32 original tokens and the native index-10 **16-bit mask** are not interchangeable. Numbering/lookup namespace, invalid-value semantics, pointer lifetime, and exact method signatures still require proof. The new host reference rejects `0xFFFF`/out-of-range candidate indexes and **enables no native virtual calls**.

## The real old vgui2.dll disproves two direct VGUI slot aliases

Original `VGUI_InputInternal001` is registered through callback `0x100064E0`, singleton `0x10053748` and original vtable `0x100472AC` (**71 methods**, SHA256 `03c7e3969272653459ad21d296aa4771643dcdfa62a01fd5bb8269549701b365`).

Original `VGUI_System010` is registered through callback `0x1001BDB0`, with original vtable `0x1004B95C` (**33 methods**, SHA256 `f49170def3240bbe74c11d9246d1d5c6f123a9cec49de9399c03e454cd93d30e`).

- **InputInternal virtual slot 56**: Original `client.dll` passes integers `0` and `1` at `0x240F97C9` and `0x240F97E2`. The original 2006 implementation at `0x10007D90` reads that small integer and writes two per-context byte flags (862-byte SHA-pinned function). SourceVR ARM64 method **at the same slot 56** is `0x25CA0`: it accepts a pointer-like argument, searches a pointer array, calls `memmove` to remove a matching entry, decrements an array count. **Semantically incompatible with the old calls.**
- **System virtual slot 24**: Original `client.dll` passes an integer key at `0x240F9870`. The original `vgui2.dll` implementation at `0x1001CB60` (35 bytes) bounds checks **key 0..255** and indexes a translation table at `0x10053270`. The native ARM64 method at **same slot 24** is `0x3DE30`, a 20-byte global-function-pointer tail dispatch that does not perform the old integer lookup. **Also semantically incompatible.**

Native InputInternal has **89 methods**, native System **40 methods** (both verified in Phase97). The two proven wrong-slot cases are distinct from the remaining original-used VGUI slots, whose native method ABI is *still unknown*.

## Exact-code checks, clean-room tests and repository scope

- **9 whole-function or explicitly bounded original/native code-region SHA-256 hashes**: 3 original sound, 2 original VGUI, 2 native sound, 2 native VGUI.
- Three **full original PE32 vtable hashes** covering **36 + 71 + 33 = 140** method pointers, chained to three native vtable hashes covering **47 + 89 + 40 = 176** method pointers from Phase97.
- **22 original x86 opcode anchors** and **25 native AArch64 opcode anchors**, plus the full Phase97 ABI instruction/factory audits.
- `include/sin_compat/original_engine_sound_vgui_abi.hpp`: host-only evidence, 16-bit candidate index guard and explicit no-live-call status.
- `research/tools/audit_original_engine_sound_vgui_abi.py`: original game+engine/native **six-binary** SHA verifier and JSON compatibility matrix.
- `research/tests/test_original_engine_sound_vgui_abi.py`: 12 original-file-free mutation tests; every new opcode/critical table index has a corruption regression.
- `research/tests/sin_original_engine_sound_vgui_reference.cpp`: strict C++17 reference safety tests.
- `CMakeLists.txt`: CTest integration.

**Locally tested on all six exact owner-held files:** complete binary verification PASS, 12 new Python tests PASS; standalone reference archive totals 24 Python regression suites and CTest 2/2 PASS with `-Werror`. GitHub CI evaluated after PR creation.

**Not claimed:** source-level reconstruction of complete sound/VGUI DLLs, ABI-compatible native `VSoundEmitter001` wrapper, full `libclient.so`/`libserver.so` running in SourceVR, Quest APK or gameplay. Native VGUI slot 56/24 are **blocked**, and the 7/10 sound candidates are **not approved**.

**Phase99:** verify exact native slot 7/10 signatures, number-space translation, null/error/ownership semantics; search verified native replacements for old VGUI Input 56 and System 24. Only then implement live adapters, and test them against the selected full SourceVR runtime.
