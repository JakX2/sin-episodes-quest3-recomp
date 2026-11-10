# Phase 160: original 2006 client CHudElement HUD visibility

**10 November 2026.** This phase reconstructs the original SiN Episodes: Emergence (2006) Windows x86 client.dll CHudElement base class, its vtable, field helpers and the shared 125-byte HUD suppression predicate. It builds on the HUD drawing research of Phases 153–159, without inventing Source Engine API names or implementing actual VGUI rendering.

## Exact owner-held original binaries

Client DLL SHA256: 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d

Server DLL SHA256: 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650

Original MSVC RTTI TypeDescriptor at **0x242FBD14** contains the class token CHudElement. Original CompleteObjectLocator **0x242E72E8** precedes a verified **13-slot primary vtable at 0x2429B6EC**. Every slot is checked against the owner-held original DLL.

## Eight complete original function SHA256s

| Original client VA | Bytes | SHA256 | Actual behavior |
|---|---:|---|---|
| 0x24142220 | 32 | 1b26c2de954f86fcc47c355833261a30cd32d9976ae2b9c1a71a89f187c56a14 | Constructor |
| 0x24142240 | 1 | ae3f4619b0413d70d3004b9131c3752153074e45725be13b9a148978895e359e | Virtual slot2 no-op |
| 0x24142250 | 125 | d81fb46c0b4c26ad6364969d6f8d99b0e9a479ff3ef181861e2680a5eaf7b3dd | Shared global HUD mask gate |
| 0x24142A60 | 22 | e21dd7b8dfcbbb81a780740afe04a24e02841e2ec2380cafe930f75ec591f3d9 | CHudElement virtual slot9, logical NOT of gate |
| 0x24140770 | 4 | 2e339cdc5de837ec151dbde90057caeabba18ac3ea32e7c2cffc8f95df41a6ff | Virtual slot8 returns DWORD +0x0C |
| 0x24140780 | 4 | e0eb8cffec07b89f095aa1e60d89eed1f3338b76216e649ad3aeb3ba92dc7d04 | Virtual slot10 returns byte +0x04 |
| 0x24141830 | 10 | bd85059e1d7267ad70adf88bb4cea9f4c8413c98ecc8813abe2560acb18983c9 | Virtual slot11 sets byte +0x04 |
| 0x24141850 | 10 | b35caac991eb0d06657c3fbba49c85de5f83e8cf4de0b07b70fa4b90cb5770fd | Virtual slot12 sets DWORD +0x08 |

All eight entire function-region hashes matched the private original 2006 client.dll. The audit additionally validates 33 sparse original opcode anchors, all 13 vtable cells, RTTI/COL, and four exact native CALL edges. It never publishes the original machine-code bodies.

## Constructor and virtual base class

The exact native CHudElement constructor at 0x24142220 sets vtable 0x2429B6EC at this+0x00, clears byte +0x04, clears DWORD +0x08, copies the first caller 32-bit argument into DWORD +0x0C, clears bytes +0x10 and +0x11, and returns with RET 4. The model has the exact base field effects, but is not a binary-compatible original C++ class.

Verified virtual entries: slot2 returns without action, slot8 reads DWORD +0x0C, slot10 reads byte +0x04, slot11 writes byte +0x04, slot12 assigns DWORD +0x08. No unverified source-level member names are inferred.

## Global HUD visibility predicate at 0x24142250

The code accepts a 32-bit mask and follows this order:

1. Calls the engine-global object at 0x2433F684, original virtual byte offset +0x68. If its result byte is zero, returns true (suppression).
2. Loads the local-player global pointer from 0x24338CF0. Null player returns true.
3. Reads the optional override DWORD at (*(0x243623A0))+0x2C. If nonzero, this REPLACES player flags at +0xD8C; otherwise uses the player flags.
4. If effective flags contain bit 0x04, returns true regardless of incoming element mask.
5. If the incoming mask has bit 0x10, calls the local player virtual function at +0x190. Signed return <=0 suppresses. No call is made when the bit is clear or an earlier branch exits.
6. If incoming mask has bit 0x20 and player byte at +0xE15 is zero, suppresses.
7. Otherwise the result is boolean (effective_flags & incoming_mask) != 0. Original x86 uses AND / NEG / SBB / NEG to form the zero-or-one result.

Native CHudElement virtual slot9 at 0x24142A60 loads the mask from this+0x08, invokes the gate on the original HUD manager at 0x243623D0 and returns logical NOT of that gate's AL result via NEG / SBB / ADD 1. A true slot9 result means the suppression conditions did not fire.

## Four original x86 callsite cross-references

- 0x24142A69 -> 0x24142250: CHudElement slot9 delegates to global gate.
- 0x2422FFA6 -> 0x24142A60: original HUD caller uses the slot9 result.
- 0x24235E89 -> 0x24142220: derived HUD constructor calls CHudElement base constructor.
- 0x24235F3D -> 0x24141850: derived HUD constructor initializes the element's visibility mask.

These establish real linkage between original constructors, element masks and rendering eligibility. They do not prove which individual HUD is on screen at runtime.

## New independently authored code and tests

- include/sin_compat/original_client_hud_element_visibility.hpp: portable C++17 constructor, virtual accessors, no-op, exact suppression decision and negating slot9. Player virtual +0x190 is a lazy host callback, never a fabricated original object pointer.
- tests/test_original_client_hud_element_visibility.cpp: field isolation, virtual get/set, engine/global/player early exits, override precedence, hard bit4, signed <=0 player callback, bit0x20 status test, and a comprehensive mask/state regression matrix.
- research/tools/audit_sin_original_client_hud_element_visibility.py: owner-only exact original DLL fingerprints, 8 complete method hashes, RTTI and 13 virtual slots, sparse opcode and call witnesses.
- research/tests/test_sin_original_client_hud_element_visibility.py: synthetic original-free mutation regression and independent x86 control-flow flag interpretation over **8,064 combinations**.
- Root CMake/CTest registration.
- Phase152 reviewed recovery ledger adds eight complete small original client methods, moving from 43 to **51 reviewed entries: 25 server and 26 client**. This is NOT total game-function coverage or a decompilation percentage.

Owner-only audit command:

    python3 research/tools/audit_sin_original_client_hud_element_visibility.py --client /private/client.dll --server /private/server.dll --output /private/phase160.json

Original-free test commands:

    python3 -m unittest discover -s research/tests -p test_sin_original_client_hud_element_visibility.py -v
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
    cmake --build build --target sin_original_client_hud_element_visibility_tests
    ctest --test-dir build -R sin_original_client_hud_element_visibility_tests --output-on-failure

## Remaining boundaries

Native engine-global ownership, original player virtual method +0x190 implementation, live engine callback integration, actual VGUI drawing and the separately researched HUD +0xA6 color/status byte writer remain unresolved. The CHudElement +0x04 byte is NOT asserted to be the timer HUD +0xA6. Original Windows PC replacement DLL load testing comes before Android game runtime and Quest standalone work. A 51-function evidence ledger does not imply a playable game.
