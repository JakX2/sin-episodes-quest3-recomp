# Phase 91: Original SiN server cache handler RTTI and client HUD shutdown classes

**Evidence:** Direct read-only analysis of the user's original 2006 x86 PE32 \`client.dll\` and \`server.dll\`, never committed or included in release ZIPs. Exact original SHA256:

- \`client.dll\`: \`6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d\`.
- \`server.dll\`: \`568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650\`.

The verifier **chains Phase90** and proves each class by traversing the original data/vtable/MSVC RTTI layout: client **global pointer cell -> singleton object -> vptr -> vtable[-1] RTTI CompleteObjectLocator -> type descriptor -> decorated class name**. It independently SHA-256-pins all pointers in each complete contiguous method table. Server object vptr and complete type descriptor are likewise grounded in the allocation code recovered in Phase90.

## server.dll: the cache classes are real

| Class from original RTTI | Original vtable VA | Virtual functions slots 0/1/2 | Auxiliary allocation |
|---|---|---|---:|
| **\`CSceneCache\`** | \`0x2242DCA0\` | \`0x221A7360\`, \`0x221AB0A0\`, \`0x221AD750\` | \`0x1C\` (28 bytes) |
| **\`CModelSoundCache\`** | \`0x2240ABC8\` | \`0x221437E0\`, \`0x221453E0\`, \`0x221454B0\` | \`0x18\` (24 bytes) |

Actual MSVC decorated type names are **\`.?AVCSceneCache@@\`** and **\`.?AVCModelSoundCache@@\`**. These came from original RTTI, not guessed names or the Half-Life 2 SDK.

- \`CSceneCache\` vtable pointer words (12 bytes) SHA256: \`d0e88dc9b98044232979296247f0aa6916d6a3a9fa82817566d78f1a41b22e6e\`. RTTI locator \`0x224C1820\`.
- \`CModelSoundCache\` SHA256: \`694e23672e8a79cfd4f40b492de8f77193699fd0dd9e3e7d5572e4f437a64d70\`. RTTI locator \`0x224BBB94\`.

Original record find/insert path \`0x221ACFD0\` (scene) writes the \`CSceneCache\` vtable at the allocated object's first word; model sound \`0x22149770\` does the same for \`CModelSoundCache\`.

**Further method evidence:** \`CSceneCache\` vtable slot 0 reads the 16-bit count at instance **+0x14**; the analogous \`CModelSoundCache\` slot 0 uses **+0x10**. The slot-1 methods at \`0x221AB0A0\` and \`0x221453E0\` use a **0x200-byte temporary data buffer**. The slot-2 methods are different parsing/setup paths: scene \`0x221AD750\`, model sound \`0x221454B0\`. **Eleven exact original method-opcode anchors** independently confirm these facts, while Phase90's full SHA256 and original calls anchor the record-insertion path.

The method names and parameter signatures (beyond original vtable indices and shown field accesses) are still **unrecovered**. This is not proof of a complete \`scene.cache\` or \`modelsounds.cache\` file decoder.

## client.dll: eight shutdown classes identified

The original client shutdown dispatcher at \`0x241EE0C0\` was recovered in Phase90. Its eight previously unnamed object-method invocations now have complete original RTTI links:

| Shutdown order | Confirmed class from original RTTI | Original global cell | Original vtable | Original invoked method |
|---:|---|---|---|---|
| 1 | \`CDebugOverlayPanel\` | \`0x2431DC90\` | \`0x242ABF0C\` | \`0x241ECA60\` (slot +4) |
| 2 | \`CNetGraphPanelInterface\` | \`0x2431DEA4\` | \`0x242ACC34\` | \`0x241EF9F0\` (+4) |
| 3 | \`CFPS\` | \`0x2431DD00\` | \`0x242AC1F0\` | \`0x241ED1A0\` (+4) |
| 4 | \`CMessageChars\` | \`0x2431DE3C\` | \`0x242ACB38\` | \`0x241EEA30\` (+4) |
| 5 | \`CLoadingDisc\` | \`0x2431DDD0\` | \`0x242AC8B0\` | \`0x241EE480\` (+4) |
| 6 | \`CCenterPrint\` | \`0x2431DBEC\` | \`0x242ABC4C\` | \`0x241EC510\` (**+0x1C**) |
| 7 | \`CTextMessage\` | \`0x2431DF40\` | \`0x242AD484\` | \`0x241F2320\` (+4) |
| 8 | \`CStatsPanelInterface\` | \`0x2431EEBC\` | \`0x242AF988\` | \`0x24207670\` (+4) |

All ten classes' actual RTTI locator pointers, descriptors, type-name strings, vtable method arrays and original linked singleton pointers are validated. **40 consecutive executable method entries** belong to the eight client virtual tables, and **six** belong to the two cache virtual tables. For all ten, the following pointer word is verified *not* to extend into another executable method entry.

The **exceptional +0x1C** shutdown byte offset on \`CCenterPrint\` is important: mapping all HUD objects to vtable slot 1 would be incorrect. Likewise these original x86 addresses **cannot** be invoked as native Android ARM64 pointers.

## Delivered

1. \`include/sin_compat/original_rtti_handler_contract.hpp\`: standalone C++17 metadata descriptions (8 client, 2 server), original method pointers treated as **numbers only**, and testable callback-based iteration.
2. \`research/tests/sin_original_rtti_handler_reference.cpp\`: class order, original offset exception, cache method and auxiliary-allocation contracts.
3. \`research/tools/audit_sin_rtti_cache_client_hud.py\`: chains Phase90 original binary SHA256 evidence; validates 10 RTTI locators, 10 type names, exact original vtables with all **46** method words and 11 server code anchors, then emits non-proprietary metadata JSON.
4. \`research/tests/test_sin_rtti_cache_client_hud.py\`: **12 original-free mutation test groups** checking every original singleton pointer, vptr, RTTI class, vtable hash, server opcode site and the unsupported extra-slot case.
5. \`CMakeLists.txt\`: registers C++17 independent CTest target.

**Local verification:** Both exact owner-held original DLL fingerprints and all chained Phase90 audits passed. All **12 Python mutation tests** passed and the C++17 standalone reference compiled with \`-Wall -Wextra -Wpedantic -Werror\` and passed CTest 1/1. GitHub CI status tracked separately.

## Limits and Phase 92

This establishes **true original class identities**, method pointers and part of the cache serializer/control flow. It does **not** supply a full 2006 SiN decompilation, Android SourceVR engine-matched \`CreateInterface\` ABI, binary-compatible \`libclient.so\`/\`libserver.so\`, full cache serializer or a game playable on Quest 3.

**Next:** analyze each original server cache vtable slot's arguments, actual per-record buffer/data handling and caller responsibilities; simultaneously trace original HUD object constructors/destructors and client initialization ties. Then evaluate native Android module interfaces against the **correct** SourceVR revision before claiming loader compatibility.
