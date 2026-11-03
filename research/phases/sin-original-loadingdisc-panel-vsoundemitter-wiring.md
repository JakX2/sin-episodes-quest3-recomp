# Phase 95: Original SiN CLoadingDiscPanel and VSoundEmitter001 mapping

## Provenance

Inspected the user's original SiN Episodes: Emergence (2006) Windows PE32 game DLLs. Both full SHA-256 fingerprints match the prior Phase 84-94 original read-only audits. No original executable code, proprietary game resources or DLL bytes are committed.

| Module | Verified original x86 region | Start VA | Bytes | SHA-256 |
|---|---|---|---:|---|
| client.dll | complete CLoadingDiscPanel constructor | 0x241EE350 | 222 | 2c5e8fea3c7332248b021c97705ce2aa2b69ef35a402047b4bca1735a5eaf283 |
| client.dll | vgui::Label constructor PREFIX only | 0x2401BAC0 | 80 | 233bece8e700106b6f5c03f93112a040910d551396d65510ed6b1681254b6196 |
| server.dll | VSoundEmitter001 required factory sequence | 0x221E7792 | 25 | d94db11b145522c33f27fecc5beef3d659f04ee5df72d9e459100040b18674d9 |

The 80-byte vgui::Label region is only the constructor prefix, not its full function. The original auditor independently verifies 14 client direct CALL edges, 11 client opcode anchors, 15 server opcode anchors, four original strings and both MSVC RTTI locators.

## client.dll: real CLoadingDiscPanel and vgui::Label

Phase94 showed that CLoadingDisc lazily creates two 0xDC-byte objects through original methods 0x241EE4D0 and 0x241EE530, which call 0x241EE350. Phase95 follows that exact constructor:

- The panel virtual table is **0x242AC5C4**, with MSVC RTTI class **CLoadingDiscPanel**. Original RTTI locator **0x242EADD4** and decorated name .?AVCLoadingDiscPanel@@ are validated.
- The constructor assigns its vtable at **0x241EE36A**, runs additional base/panel setup helpers, and allocates **0xF0 (240) bytes** for a nested label at **0x241EE39C**.
- The label constructor **0x2401BAC0** writes virtual table **0x242D4D64**, whose original RTTI identifies **vgui::Label** (decorated .?AVLabel@vgui@@).
- Four verified literal strings: **CLoadingDiscPanel**, **LoadingLabel**, **ClientScheme**, and **resource/LoadingDiscPanel.res**.
- The label pointer is stored at original panel **this+0xD8** at 0x241EE3CC. The panel calls the nested label's virtual byte offset **+0x100** with argument zero at 0x241EE3D8 and sends the resource-path string to helper **0x2400DB60** at 0x241EE3E9.
- Original methods also perform other panel geometry and setup calls not included in the simplified host model; it is not a full VGUI implementation.

## server.dll: recovered true sound service interface

The formerly anonymous global pointer at **0x225721D0** is demonstrably the engine interface requested under exact name **VSoundEmitter001**:

1. ServerGameDLL003 Init pushes original string VSoundEmitter001 (at 0x2244293C), queries it through the first engine factory at **0x221E7799**.
2. Result saved to **0x225721D0** at 0x221E77A0; a null result branches to original Init failure at 0x221E77A5.
3. Virtual byte-offset **+0x00** on the resulting interface must return true at **0x221E77E6**.
4. CModelSoundCache and CSceneCache resolve **name-like values to integer tokens via virtual +0x0C** in original callsites 0x221456EE, 0x221AB0F4, 0x2214542E.
5. Both cache writers resolve **integer tokens to name-like results via virtual +0x18** at 0x221A739F and 0x2214380F. The previously documented invalid-token sentinel is 0xFFFFFFFF.

These directions are verified through original x86 callers. The *complete method signatures* and the ABI for actual SourceVR 0.5.5 ARM64 libraries are **not** established. No newer Source SDK method names are silently assigned.

## Reconstructed code, tests and limitations

- include/sin_compat/original_loadingdisc_panel_sound_emitter.hpp: host-only C++17 callback model with original class/resource names, two allocation sizes, nested label call order, and bidirectional sound token mapping.
- research/tests/sin_original_loading_panel_sound_emitter_reference.cpp: mock GUI creation, failure handling, factory initialization requirement and name/token tests.
- research/tools/audit_sin_loading_panel_emitter.py: original SHA/RTTI/x86 instruction/call verifier; metadata JSON only.
- research/tests/test_sin_loading_panel_emitter.py: 12 synthetic original-file-free regression groups, including tampering with every original opcode and CALL anchor.
- CMakeLists.txt: independent CTest entry.

**Local verification:** original dual-DLL audit PASS, Python 12/12 PASS, strict C++17 CTest 1/1 PASS.

This is not a complete native SiN-specific libclient.so or libserver.so, not a Quest 3 APK and not a playable game. The host reference deliberately returns false safely on null allocation, where the original x86 may dereference null. Never treat original 32-bit virtual addresses as ARM64 pointers.

**Phase96 focus:** recover more of CLoadingDiscPanel nested Label behavior; then validate the named VSoundEmitter001 original contract against the *actual selected* SourceVR 0.5.5 runtime, rather than inventing compatibility aliases.
