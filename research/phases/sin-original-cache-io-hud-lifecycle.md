# Phase 92: Original SiN cache virtual I/O and HUD teardown body shapes

## Original code provenance

Read-only reverse engineering of the user's original **SiN Episodes: Emergence (2006)** Windows x86:
- \`client.dll\` SHA-256: \`6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d\`
- \`server.dll\` SHA-256: \`568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650\`

The source-free original-file audit chains **Phase91's class RTTI and vtable verifier** and pins **12 complete original function bodies** by individual SHA-256, eight from client and four from server. It checks **20 client virtual-dispatch instruction patterns**, **20 server original opcode anchors**, **12 direct original CALL targets**, and each function boundary.

## server.dll: real CSceneCache and CModelSoundCache virtual I/O

Two original classes were identified via RTTI and proved to have three real virtual methods in Phase91. Phase92 follows the first **two virtual slots** for each:

| Original RTTI | Slot 0 (writer) | Length | Slot 1 (reader) | Length |
|---|---|---:|---|---:|
| \`CSceneCache\` | \`0x221A7360\` | 88 | \`0x221AB0A0\` | 125 |
| \`CModelSoundCache\` | \`0x221437E0\` | 72 | \`0x221453E0\` | 119 |

### Writer call order, from original x86

- **Scene** writes a **32-bit floating-point value** read from object \`+0x04\` with helper \`0x221A73C0\` before writing a 16-bit count from \`+0x14\` via helper \`0x22143830\`.
- **Model sounds** immediately writes 16-bit count from \`+0x10\` using the same helper, without a float prefix.
- Both then loop over the exact count, enumerate entries from the original pointer array (scene \`+0x08\`, model \`+0x04\`), resolve each through global \`0x225721D0\` vtable byte offset **+0x18**, and forward each token/result to common writer helper \`0x2228DCE0\`.

### Reader call order, from original x86

- **Scene** first reads a 32-bit float with helper **\`0x221AB120\`** (x87), writes it to instance **+0x04**, then reads 16-bit count. **Model** reads the 16-bit count directly.
- Both invoke helper **\`0x22145460\`** for count; for each entry, call helper **\`0x2228D810\`** with temporary buffer capacity **0x200** bytes, then resolve that buffer via global \`0x225721D0\` vtable byte offset **+0x0C**.
- Resolutions equal to **0xFFFFFFFF** are not inserted; successful ones pass to helper **\`0x22097E10\`**, updating a per-instance table.

**Strict limitation:** This verifies *call sequence, bit width, conditional flow and temporary capacity*. It does **not** prove an independently decoded on-disk record payload schema, the real string encoding, or a production cache writer/reader. A 512-byte scratch allocation is not a 512-byte fixed disk record.

## client.dll: eight HUD shutdown methods, three distinct machine-code bodies

All eight original methods were linked to original panel RTTI in Phase91.

| Original RTTI | Original method | Length | Verified body behavior |
|---|---|---:|---|
| \`CDebugOverlayPanel\` | \`0x241ECA60\` | 38 | single |
| \`CNetGraphPanelInterface\` | \`0x241EF9F0\` | 38 | single |
| \`CFPS\` | \`0x241ED1A0\` | 38 | single |
| \`CMessageChars\` | \`0x241EEA30\` | 38 | single |
| \`CLoadingDisc\` | \`0x241EE480\` | 71 | **double** |
| \`CCenterPrint\` | \`0x241EC510\` | 43 | **single with pointer clearing** |
| \`CTextMessage\` | \`0x241F2320\` | 38 | single |
| \`CStatsPanelInterface\` | \`0x24207670\` | 43 | **single with pointer clearing** |

All five **single** functions have byte-for-byte **identical 38-byte original bodies**, SHA-256 \`f923966ea39aa30c84150b401978164d08bde5a0703c67333a66f0a44970e7ba\`. They:
1. Check nested pointer at **this+0x04**.
2. If nonnull, call the nested object's virtual method at byte offset **+0x98 with argument 0**.
3. **Re-read this+0x04**, then if still nonnull call its virtual method byte offset **+0x74 with argument 1**.

\`CLoadingDisc\` repeats this sequence for a second object pointer at **this+0x08** (71-byte complete hash \`a76c48a785305d420349c2fddf8b0162abcfefceacfafd92c7746e18c7cd705c\`).

\`CCenterPrint\` and \`CStatsPanelInterface\` have **identical 43-byte code** with both calls and an additional write clearing **this+0x04** afterward (SHA-256 \`f0d8ceffc860e87d455d2b6781db940593e046b7ad67f7339d5ee4e7857c2b0b\`).

**Therefore eight different original class methods reduce to just three original machine-code implementations.** This is useful for a clean-room reimplementation, but the names/signatures of the *nested engine object's* virtual methods are **not** yet proven and must not be guessed from a newer SDK.

## Files and checks

- \`include/sin_compat/original_cache_io_hud_lifecycle.hpp\`: original-order host-only C++17 mock callback contract; does not open caches or use original x86 pointers.
- \`research/tests/sin_original_cache_io_hud_lifecycle_reference.cpp\`: tests both cache call orderings, 16-bit counts, token skips, all three HUD release shapes, a pointer cleared between the two virtual calls and absent optional pointers.
- \`research/tools/audit_sin_cache_io_hud_lifecycle.py\`: SHA-pinned original dual-DLL x86 code and instruction verifier, chained to Phase91's full binary RTTI evidence.
- \`research/tests/test_sin_cache_io_hud_lifecycle.py\`: **12 original-free Python mutation suites**.
- \`CMakeLists.txt\`: CTest registration.

**Owner-held original audit:** PASS. **Local Python:** 12/12 PASS. **Original-free C++17 reference:** \`-Wall -Wextra -Wpedantic -Werror\`, CTest 1/1 PASS. GitHub CI tracked independently.

## What remains

No complete serialized cache-record parser, original class constructors, original DLL recompile, real SiN-specific ARM64 SourceVR \`libclient.so\`/\`libserver.so\`, Quest APK, headset gameplay or SourceVR engine ABI compatibility is claimed.

**Phase 93 candidate:** examine \`0x2228DCE0\`, \`0x225721D0\` and the record ID/token mapping; recover which original HUD constructors supply the nested +0x04/+0x08 pointers. Once verified, test native adapter against **the actual selected SourceVR revision**, not old interface names guessed equivalent to modern ones.
