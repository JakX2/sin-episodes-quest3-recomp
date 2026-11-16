# Phase 251: original client RecvProxy and RecvProp/RecvTable factories

16 November 2026. Stacked draft on Phase250 PR #306. This phase is grounded directly in the owner-supplied original **SiN Episodes: Emergence (2006)** retail PE32 Windows DLLs, not a surrogate SDK. No commercial original DLLs, original whole-native-code bodies, disassembly dumps or game assets are committed.

## Owner-original whole-file provenance

| Module | Entire original bytes | Complete SHA256 |
| --- | ---: | --- |
| client.dll | 3,592,192 | 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d |
| server.dll | 6,049,792 | 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650 |

Both complete retail DLL digests gate new original method credit. In addition, the auditor checks 12 whole original x86 native function SHA256 digests, 61 sparse x86 opcode witnesses, 12 actual native return paths, 94 trailing original INT3 alignment bytes, six direct E8 rel32 CALL edges, one native E9 tail jump, and eight absolute function-pointer DWORDs with matching original PE32 HIGHLOW relocations. Synthetic fixtures earn zero original method credit.

## Twelve newly authenticated complete original client functions

| Native entry VA | Original bytes | Whole native code SHA256 | Recovered operation |
| --- | ---: | --- | --- |
| 0x24126A20 | 1 | ae3f4619b0413d70d3004b9131c3752153074e45725be13b9a148978895e359e | RecvTable trivial destructor |
| 0x24126A30 | 34 | 6f9e7546053378e2146df443c8244c8250fa214ff339c5404fefd8c291a05555 | RecvTable registration: pointer/count/name |
| 0x24126C40 | 14 | 4c10fc0974c78557d955abfeaa3ce9ffdcec438069cd51b103595d7cdc569d81 | Float receive proxy raw DWORD copy |
| 0x24126C50 | 26 | da6843b9ae645f72bcba8cd99e4ddd566ad840ddc6856e598fcd5b1b1e763768 | Vector3 receive proxy three-DWORD copy |
| 0x24126C70 | 14 | a72672d7f606259b258d1d90e5d914ddcd6375fca84ae77714c2df93a4b8be91 | Byte receive proxy |
| 0x24126C80 | 16 | 6ab922c6b5f7991f06f8ddf29061ee9e88fe1704531243b5939be6ae3079ff88 | Word receive proxy |
| 0x24126C90 | 14 | 4c10fc0974c78557d955abfeaa3ce9ffdcec438069cd51b103595d7cdc569d81 | Integer DWORD proxy, byte-identical to float proxy |
| 0x24126D00 | 37 | 782c9edae7e1e020d98681c5286b604b3b2289c2a5383629ce45e428946ce0fa | Five-element proxy dispatch constructor |
| 0x24126D30 | 22 | 2af9482667ace8889976660d8d59a14dc9d46b8fc6a26f464bcbd303af8bcefa | RecvTable default constructor |
| 0x24126D50 | 36 | c6ebab665525b706f20c7c67603ec975f835084052ad0e3a0883029f54305697 | RecvTable argument-taking constructor |
| 0x24126D80 | 106 | bd1fa7c7ccdc0a278ee0ad889437803cc195d10aa2bd53040b5b66f5f8b97257 | Float RecvProp descriptor factory |
| 0x24126DF0 | 146 | c292491b77e181d88ab5c65d6c49667f99cbd9fcf82a6be39d62f1af23e7c4b5 | Integer/placeholder RecvProp descriptor factory |

**New original entire-code bytes: 466. Reviewed original method catalog: 391 → 403 (server 147, client 244 → 256).** This is an original-method review inventory, not a recovered-source percentage or proof of game boot.

## Original receive proxy ABI and dispatch table

The original proxy source object contains a pointer at DWORD+0 and its actual value begins at byte +4. Methods accept an independent output pointer as their third stack argument. The five recovered proxies copy raw values from input+4 to output without numeric conversion:

| Method | Bytes copied | Native meaning |
| --- | ---: | --- |
| 0x24126C70 | 1 | Byte |
| 0x24126C80 | 2 | Word |
| 0x24126C90 | 4 | INT32 |
| 0x24126C40 | 4 | FLOAT |
| 0x24126C50 | 12 | Vector3 from three consecutive DWORDs |

The FLOAT and INT32 bodies have **identical whole-code SHA256**. Float handling is byte-for-byte, including NaN payloads and negative zero. Host tests prove the first metadata DWORD is not copied.

Native 0x24126D00 populates a contiguous five-pointer table in the exact order byte, word, int32, float, vector3. These absolute DWORD instructions and genuine HIGHLOW relocation sites are:

| Original relocation operand VA | Native target |
| --- | --- |
| 0x24126D04 | 0x24126C70 |
| 0x24126D0B | 0x24126C80 |
| 0x24126D12 | 0x24126C90 |
| 0x24126D19 | 0x24126C40 |
| 0x24126D20 | 0x24126C50 |
| 0x24126E34 | 0x24126C80 (integer width 2) |
| 0x24126E3E | 0x24126C90 (integer width 4) |
| 0x24126E45 | 0x24126C70 (integer fallback) |

## Critically incomplete native factory descriptor initialization

Original 0x24126D80 and 0x24126DF0 create a 60-byte temporary stack RecvProp descriptor, set *some* fields, and then copy 15 DWORDs to the output. They do **not** explicitly initialize all 60 bytes. The clean-room C++ reference tracks only the 49 known written bytes and marks 11 bytes as unknown rather than manufacturing zeros.

| Offset in original 60-byte descriptor | Verified factory operation |
| --- | --- |
| +0x00 DWORD | Property name pointer |
| +0x04 DWORD | Type: 0 integer or 1 float |
| +0x08 DWORD | Extra original argument |
| +0x0C DWORD | **Unspecified by these methods** |
| +0x10 low BYTE | Zero; bytes +0x11..+0x13 unspecified |
| +0x14,+0x18,+0x1C DWORD | Zero |
| +0x20 DWORD | Receive proxy pointer |
| +0x24,+0x28 DWORD | Zero |
| +0x2C DWORD | Entity member offset |
| +0x30 DWORD | 0xFFFFFFFF sentinel |
| +0x34 DWORD | 1 |
| +0x38 DWORD | **Unspecified by these methods** |

The original float factory at 0x24126D80 stores its explicit proxy and type=1. Its fourth stack argument (4-byte width at the Phase250 callsites) is **not read by the entire factory function** and therefore is not invented as a native field write.

The integer factory at 0x24126DF0 uses type=0 and allows an explicitly specified proxy pointer. If absent, it chooses a byte proxy for width 1 or any unrecognized width, a word proxy for width 2, or an integer DWORD proxy for width 4. Phase250's internal placeholder passes width 4 with no explicit proxy; its default therefore is the integer DWORD proxy.

## Original RecvTable factory methods

- 0x24126D30 default constructor zeros DWORD fields +0,+4,+8,+0xC and bytes +0x10,+0x11.
- 0x24126D50 constructor with first descriptor pointer, count and table name initializes those fields, resets +8, and clears +0x10,+0x11. Native stack return is RET12.
- 0x24126A30 registration writes that same three-argument table state and clears the same flags, also RET12.
- 0x24126A20 destructor is a single RET and performs no field writes.

The clean-room host models only the first **18 proven bytes** of the table rather than assuming that is its full object size.

## Original verified call relationships from Phase250

| Original instruction | Original callee |
| --- | --- |
| 0x241281F2 E8 | 0x24126DF0 internal placeholder constructor |
| 0x2412820C E8 | 0x24126D80 lifetime float constructor |
| 0x24128226 E8 | 0x24126D80 start-size float constructor |
| 0x24128243 E8 | 0x24126D80 end-size float constructor |
| 0x2412825C E8 | 0x24126A30 RecvTable registration |
| 0x24268965 E8 | 0x24126D30 CRT table constructor |
| 0x242736B5 E9 | 0x24126A20 CRT cleanup tail jump |

The new **ParticleTrailRuntimeAdapter** composes these recovered factories and actual bitwise receive proxies with Phase250's EntityParticleTrailInfo initialization and CRT lifetime model, replacing previously opaque callback behavior in the host-side reference. Unknown native engine lifetime, actual pointer allocation, and other proprietary Source runtime methods remain explicit boundaries.

## Changed source and tests

- research/tools/audit_sin_original_client_recvprop_proxies_factories.py: dual entire-original-DLL hashes, twelve full original method SHA256s, 61 opcode witnesses, all RET/INT3, six E8, one E9 and eight relocated pointer checks.
- research/tests/test_sin_original_client_recvprop_proxies_factories.py: synthetic mutation tests for all witnesses, exit bytes, INT3, E8/E9, all eight pointers and section/PE32-base boundaries. Synthetic fixtures never award authentic original-method credit.
- include/sin_compat/original_client_recvprop_proxies_factories.hpp: C++17 bitwise proxies, exact proxy dispatch, known-byte-only RecvProp factories, RecvTable constructors/register/dtor, and Phase250 integration adapter.
- tests/test_original_client_recvprop_proxies_factories.cpp: original addresses and dispatch table, NaN/negative-zero bit preservation, byte/word/int/vector raw copy sizes, width/explicit-proxy selection, undefined field guards, original table layout and full Phase250 particle-trail CRT integration.
- Root Linux and genuine native MSVC Windows x86 CTest registration, original reviewed-method ledger now 403.

Private authenticated original retail binary audit:

    python3 research/tools/audit_sin_original_client_recvprop_proxies_factories.py --archive /private/bin.zip

Synthetic-only regression suite:

    python3 -m unittest discover -s research/tests -p test_sin_original_client_recvprop_proxies_factories.py -v

## Not yet an independent retail game boot

The original networking object owners, 32-bit heap/pointer lifetimes, complete Source factory interface and independent Windows gameplay client.dll/server.dll replacement are still not proven. The current PC forwarding proxy and Android Bionic smoke tests do not establish full gameplay. Platform priority remains **retail Windows PC first → Android ARM64 second → Quest 3 VR third**.
