# Phase 249: original client entity-list intrusive-list methods and vtable ABI

16 November 2026. Stacked draft on Phase248 PR #304. Evidence comes only from the owner-supplied original SiN Episodes: Emergence (2006) Windows PE32 client.dll and server.dll. No commercial original DLL, native whole-method bytes or original disassembly are committed.

## Whole-retail-DLL provenance

| Original file | Complete size | Full retail SHA256 |
| --- | ---: | --- |
| client.dll | 3,592,192 | 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d |
| server.dll | 6,049,792 | 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650 |

The native auditor requires BOTH full original module digests, nine individual complete original function-body SHA256s, section/return/padding boundaries, original instruction witnesses, and relocated virtual-function pointer evidence.

## Nine newly authenticated complete original x86 methods

| Original client VA | Full original code bytes | Whole code SHA256 | Original semantic / return |
| --- | ---: | --- | --- |
| 0x24127C80 | 7 | 5a7018f52a7e9b612ef6070e55e66c84d87c73491d4f5ab7a72318192f149305 | slot self-link previous/next DWORD+8/+C, RET |
| 0x24127C90 | 16 | 9d20fb9737545d0eeb198c79dc5d325beabd6d570a4058d3f1e7f4a887df0403 | list head/tail zero DWORD+0/+4, RET |
| 0x24127CA0 | 17 | 1e2f1affbccb04c25e27dafd454fe1ac8c0c0935f714a6dfd7d22ff0c203b67b | linked predicate from previous field only, RET4 |
| 0x24127CC0 | 3 | 1d10894625976bca9d8906f3ecf1e766b94e9219ec3bf1c30e02feea5e18e45a | base vtable slot0 no-op RET8 |
| 0x24127CD0 | 3 | 1d10894625976bca9d8906f3ecf1e766b94e9219ec3bf1c30e02feea5e18e45a | base vtable slot1 no-op RET8 |
| 0x24127CE0 | 3 | 19d200194d49b34530548575f37a1bb4203ee60f18d42f0084e86b7a8c899faa | return node entity pointer, RET |
| 0x24127CF0 | 67 | 74eccc13557d320aae9d1ea67f0f030c3180423ae617a9582ee385c3402cbe44 | remove linked node, maintain head/tail and neighbors, two RET4 |
| 0x24127D40 | 106 | e9faf33f4d1d0dd86a256414e97a9947d7a2f6b1625a70091695833038951359 | insert BEFORE node, nullptr position appends TAIL, two RET8 |
| 0x24127DB0 | 105 | 880abd3b6e2202c6f80b688ffc1583ca7e5f0f974e6635b9001f6accc5b6788a | insert AFTER node, nullptr position prepends HEAD, two RET8 |

**Nine authentic whole SHA-reviewed functions, +327 original native code bytes. Original ledger 378 to 387: server 147, client 231 to 240.** This is not a recovered source-code percentage or independently booted gameplay module. The original-only verifier checks 29 sparse original opcode witnesses, 12 native return locations and 89 trailing INT3 bytes.

## Original native linked-list layouts

An original intrusive list stores head DWORD+0, tail DWORD+4. Each original 16-byte entity slot stores entity DWORD+0, serial DWORD+4, previous pointer DWORD+8 and next pointer DWORD+0xC. A detached node has BOTH link fields equal to its own pointer; null represents list endpoints.

Original 0x24127C80 self-links only, without clearing entity metadata or repairing a prior list. Original 0x24127C90 resets the head/tail but does not traverse nodes. Original 0x24127CA0 compares only PREVIOUS with self, ignoring next, even on a malformed node. Original 0x24127CE0 simply returns the entity pointer.

Original 0x24127CF0 checks PREVIOUS for self-linked state, then unlinks and repairs neighbor/manager pointers or fast-returns if unlinked. Original 0x24127D40 unlinks if already linked then inserts BEFORE a position, with null meaning APPEND at tail. Original 0x24127DB0 unlinks if linked then inserts AFTER a position, with null meaning PREPEND at head.

The independent host C++17 reference replaces original process pointers with typed node indices, preserves source-grounded transitions and adds explicit bounds/corruption guards. Those host exceptions are not claims about original native error handling.

## Original base vs derived virtual call ABI

| Exact original vtable pointer DWORD VA | Native target VA | Meaning |
| --- | --- | --- |
| **0x24298EB4** | **0x24127CC0** | base CBaseEntityList virtual slot0: no-op RET8 |
| **0x24298EB8** | **0x24127CD0** | base CBaseEntityList virtual slot1: no-op RET8 |
| **0x242973B8** | **0x241089D0** | derived client parent slot0: already reviewed registration callback |
| **0x242973BC** | **0x24108A70** | derived client parent slot1: already reviewed removal callback |
| **0x24297390** | **0x241056F0** | embedded entity-list virtual slot0: already reviewed accessor |
| **0x24297394** | **0x24105BB0** | embedded entity-list virtual slot1: already reviewed EHandle accessor |

All six actual original .rdata function pointer DWORDs have verified IMAGE_REL_BASED_HIGHLOW relocation entries at their precise addresses, proving their intended relocation under the Windows PE loader. Do not substitute derived registration/removal for the two base no-op virtuals. The original base constructor writes vtable 0x24298EB4, derived parent initializes 0x242973B8, and base destructor restores 0x24298EB4.

The nine method bodies have **no direct E8 rel32 callers** found in the complete retail .text scan. Their connection is authenticated through original vtables and direct inline structural behavior. No callsites are invented.

## Deliverables and evidence tests

- research/tools/audit_sin_original_client_entitylist_intrusive_list_vtable.py: dual full-DLL SHA, nine complete original method SHA256s, 29 sparse native witnesses, 12 exits, 89 INT3, six vtable DWORDs and genuine relocations.
- research/tests/test_sin_original_client_entitylist_intrusive_list_vtable.py: synthetic-only mutation suite for sparse witnesses, returns, padding, PE image base, .text/.rdata sections and exact vtable pointers. Synthetic evidence grants zero authentic original code credit.
- include/sin_compat/original_client_entitylist_intrusive_list_vtable.hpp: independent C++17 self-link, list reset, previous-only predicate, base no-op virtual callbacks, entity token getter, unlink, before/after insertion with correct null-position semantics.
- tests/test_original_client_entitylist_intrusive_list_vtable.cpp: native metadata contracts, both no-op virtuals, multi-node list inserts/reorders, head/tail unlink, error-guard cases, and composition with Phase247 masked handle removal and base destructor.
- Root Linux C++17 and genuine native MSVC Win32 x86 CTest registrations; reviewed-method JSON catalog updated to 387.

Owner-original audit command:

    python3 research/tools/audit_sin_original_client_entitylist_intrusive_list_vtable.py --archive /private/bin.zip

Synthetic-only test command:

    python3 -m unittest discover -s research/tests -p test_sin_original_client_entitylist_intrusive_list_vtable.py -v

## Remaining original engine dependencies

No independently rebuilt full gameplay client.dll/server.dll has yet been demonstrated loading in the original Windows retail game. Original engine-owned 32-bit pointers, native vtable object lifetime, factory integration, entity class virtual bodies and memory ownership remain unresolved. A host model or proxy that forwards calls to the retail DLLs is NOT complete game recompilation.

Platform order: Windows original game first; Android ARM64 second; Quest 3 VR third.
