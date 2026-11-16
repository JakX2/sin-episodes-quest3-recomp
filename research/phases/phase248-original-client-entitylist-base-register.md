# Phase 248: original client entity-list constructor and registration

16 November 2026. Stacked draft on Phase247 PR #303, using the owner-original **SiN Episodes: Emergence (2006)** Windows PE32 x86 client and server DLLs. Neither commercial DLL nor full proprietary original native method bytes nor disassembly dumps are committed.

## Whole owner-original provenance

| Original file | Size | Full original SHA256 |
| --- | ---: | --- |
| client.dll | 3,592,192 | 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d |
| server.dll | 6,049,792 | 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650 |

The original-only auditor requires BOTH entire DLL SHA256s and then separately authenticates all original whole-code bodies, instruction witnesses and call relationships. Synthetic fixture tests never receive original method credit.

## Five NEW complete original x86 methods

| Original client VA | Function | Full original code bytes | Whole original native code SHA256 | Original exit and padding |
| --- | --- | ---: | --- | --- |
| **0x24127E80** | 4,096-slot base entity-list constructor | 200 | 3bd028ccda4ac019dbf491fbc834c2fc7713baeae2e4253e84702131e8882d3e | RET at +0xC7; 8 INT3 |
| **0x24127F50** | Explicit entity slot registration with optional serial override | 106 | 9fc4b5d7f510991579d995c6de2ff6f3e4a5685909e99fb59f295ff05874d3e8 | RET16 at +0x67; 6 INT3 |
| **0x241280A0** | Explicit registration forwarding wrapper returning output pointer | 32 | d94b9d0d4d8e4bc3c2f0b807c25f9e2219cc0c28f279b1e08f2169c2ab05a266 | RET16 at +0x1D; no alignment bytes |
| **0x241280C0** | Non-networked entity registration from high free-list head | 181 | e762b3bfac73a8566578848cfc8d7e7fcb8a285b1558789ae29b05aa24870583 | RET8 at +0xB2; 11 INT3 |
| **0x241A8B90** | Imported vstdlib RandomInt(0,32767) wrapper | 17 | 13be98bd3c171a6acbe7e5e895939b352b95778c23860569cb51ea9c328739d6 | RET at +0x10; 15 INT3 |

**Five new entire-original-native code bodies, 536 x86 bytes. Catalog 373 -> 378: server 147, client 226 -> 231.** This is a cryptographically authenticated native-method inventory, not an estimated percentage of independently decompiled gameplay source.

The original-code auditor pins all five whole-method SHA256s, **56 sparse x86 opcode witnesses**, each original return encoding, **40 trailing INT3 alignment bytes**, **12 exact E8 rel32 original call edges**, and two original Windows IAT call instruction encodings.

## Constructor findings

The original base constructor at **0x24127E80** writes original base vtable **0x24298EB4** and zeroes the four parent list head/tail pointer fields at +0x10004, +0x10008, +0x1000C and +0x10010.

It initializes exactly **4096 primary slots**, each of stride **16** from parent+4: entity pointer DWORD+0 cleared, serial DWORD+4 assigned from the original imported serial wrapper then ANDed with 0x7FFF, and slot pointers DWORD+8 and +0xC self-linked.

The first loop calls native **0x241A8B90** once per slot. This short original function calls **vstdlib.dll!RandomInt(0,32767)** through original IAT DWORD **0x24276278**. It is an imported runtime dependency, not an in-DLL RNG implementation. The host reference injects a serial source rather than inventing the original runtime's pseudo-random generator.

The constructor second loop adds exactly **2049 through 4095**, inclusive, to its high-index free chain: **2047 initially free slots**. Index **2048 is not initially on that chain**, despite the original Phase247 removal path allowing a freed index 2048 to join it. The reference test verifies all 4096 slots and exactly these initial chain boundaries.

## Native explicit and non-networked binding semantics

Original **0x24127F50** takes output DWORD pointer, entity pointer, raw index, and serial override. It writes the entity at slot+0 and, if the override is other than 0xFFFFFFFF, writes that override directly as the slot serial DWORD+4 **without a new 0x7FFF mask**. It appends to the active chain with native **0x24127E20** before invoking two still-opaque callbacks: entity vtable slot1 with a pointer to local packed handle and parent vtable slot0 with entity/handle. It finally writes the (potentially modified) local handle to caller output and returns with RET16. The native 32-bit handle is (serial << 12) | index. Host callbacks are explicit interfaces and safety checks against null/occupied indexes are **not** attributed to retail native code.

The original **0x241280A0** passes four args to native **0x24127F50** and returns the caller-provided result pointer. It has a real original gameplay caller at **0x240C9002**.

Original **0x241280C0** reads the high-free-list head. When no slot is free, it invokes **tier0.dll!Error** via IAT **0x242761FC** with the native message **CBaseEntityList::AddNonNetworkableEntity: no free slots!**. Host throws safely instead of entering that original fatal error path. Otherwise, it unlinks the high-free head, saves entity pointer, appends the node to the active list through **0x24127E20**, invokes entity vtable slot1 and parent slot0, and writes the resulting packed handle. The low twelve bits of the handle identify the selected original slot.

## Twelve authentic original E8 call edges

| Site | Target | Meaning |
| --- | --- | --- |
| 0x24127EB6 | 0x241A8B90 | base constructor serial helper |
| 0x24127F7C | 0x24127E20 | explicit register active-list tail |
| 0x241280B5 | 0x24127F50 | explicit forwarding wrapper |
| 0x24128136 | 0x24127E20 | non-networked active-list tail |
| 0x24108193 | 0x24127E80 | previously recovered parent entity-list constructor |
| 0x240C9002 | 0x241280A0 | independent gameplay explicit-register caller |
| 0x240CAD84 | 0x241280C0 | independent non-networked register caller |
| 0x240CB0AC | 0x241280C0 | independent non-networked register caller |
| 0x240DBD05 | 0x241280C0 | independent non-networked register caller |
| 0x240DC2D0 | 0x241280C0 | independent non-networked register caller |
| 0x240E4940 | 0x241280C0 | independent non-networked register caller |
| 0x240E5C7A | 0x241280C0 | independent non-networked register caller |

Two additional original IAT callsites are independently pinned: **0x241A8B97** executes indirect call via DWORD 0x24276278 (vstdlib RandomInt); **0x241280D5** calls indirect DWORD 0x242761FC (tier0 Error). Original import table inspection establishes those names.

## New independently testable host source

- research/tools/audit_sin_original_client_entitylist_base_register.py authenticates both original whole DLLs, five full original method SHAs, witnesses, native returns/padding, all 12 E8 edges and both IAT instructions.
- research/tests/test_sin_original_client_entitylist_base_register.py mutates every opcode witness, RET byte, INT3 pad, E8/IAT call and PE image bound. Synthetic tests cannot earn original SHA code credit.
- include/sin_compat/original_client_entitylist_base_register.hpp implements the C++17 host constructor, injected vstdlib random source, raw/keep serial override, explicit registration forwarding and high-free registration with injected opaque entity/parent vtable callbacks. Uses Phase247 original-style slot/free chain model.
- tests/test_original_client_entitylist_base_register.cpp verifies exactly 4096 serial draws, slots 2049..4095 initial free, excluded 2048, serial masking, explicit serial preservation, callback order and mutable output handles, non-networked bind, Phase247 removal and reuse, and host safety failure modes.
- Linux C++17 CTest and native MSVC Win32 x86 CTest registrations added, and original reviewed method ledger incremented to 378.

Private owner-original audit: python3 research/tools/audit_sin_original_client_entitylist_base_register.py --archive /private/bin.zip

Synthetic-only tests: python3 -m unittest discover -s research/tests -p test_sin_original_client_entitylist_base_register.py -v

## Remaining scope

The original entity/parent virtual methods, real runtime allocator, full original object ABI, native factory exports, and independently booting reconstructed Windows client.dll/server.dll remain unresolved. These host references and SHA audits are not complete original gameplay code equivalence. Continue Windows PC-first DLL reconstruction, then Android ARM64, then Quest 3 VR.
