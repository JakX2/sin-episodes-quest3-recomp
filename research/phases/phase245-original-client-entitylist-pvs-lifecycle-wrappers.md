# Phase 245: original client entity-list cleanup and PVS index frontends

16 November 2026. Stacked **DRAFT** above Phase244 PR #299, for the owner-supplied original **SiN Episodes: Emergence (2006)** Windows x86 retail DLLs. No complete original game DLL, full original native code bodies, disassembly dumps or game assets have been committed.

## Strict whole-retail-DLL provenance

| Owner original PE32 | Original size | Complete original SHA256 |
| --- | ---: | --- |
| client.dll | 3,592,192 bytes | 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d |
| server.dll | 6,049,792 bytes | 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650 |

**Seven newly authenticated complete original client native functions:**

| Native entry | Entire x86 code | Complete code SHA256 | Role |
| --- | ---: | --- | --- |
| **0x241061E0** | 209 | 371ab22a53c0c0927ca9098f22df381543b2beecb0c75347f2091009dd61b64d | Drain live primary entity handles, invoke engine virtual callbacks, unbind each handle, clear counters |
| **0x24107430** | 156 | ba31e5b972b0d2a3242e636970ad553c1df5e7e72ab49f3d0ebfd48aa99b8787 | Parent entity-list destructor; drain, release embedded PVS storage, tail-call base destructor |
| **0x241082C0** | 37 | 122e5732b38e8f7d785aa9bcb1c50954a08ebb2f1d6566f8d582cf323aa9454f | Eight-byte auxiliary list unlink/free-head push |
| **0x241082F0** | 40 | 8b3136ca2362a79f47968c2c3d648db881b2c50337bd5835493718d87f56027e | Separate 16-byte list unlink/free-head push |
| **0x24108320** | 54 | af3290321c9fbb9b378386b7106b59344e6d50c5b958295dc1df2e3e342cace4 | Eight-byte auxiliary index allocation, list insertion, payload DWORD copy |
| **0x24108360** | 34 | ce40641871fb98845468e99cebde1c872d57f1821150414248c74929ace84163 | Separate 16-byte list allocate and link |
| **0x24108390** | 42 | 2d2aad642b4d04a75ee602b01dd872dc26c005d768dbbb5012b6626b45c97392 | Red-black PVS map allocate, attach, live-count increment, no key copy |

**New complete original x86 code bytes: 572. Reviewed whole-original-SHA methods: 361 → 368 (server 147, client 214 → 221).** Complete authentic code hashes identify native method bodies; they do **not** establish byte-for-byte source-code equivalence, all original CRT object lifetimes or independent game DLL boot.

The original-only verifier pins both complete retail DLL hashes before new credit, every whole-method native SHA256, **68 independent sparse x86 opcode witnesses**, seven native exits (six returns and one terminal JMP), native INT3 boundary padding, **17 exact direct E8 CALL edges**, and a final E9 tail JMP. Synthetic fixtures cannot earn original code credit.

## Original entity-list drain 0x241061E0

The native client parent object contains the primary slot table in the first 0x10000 bytes, with an active primary entity pointer at **parent+0x10004**. The method loops while it remains nonnull, recomputes a current packed handle from a 16-byte table entry and serial (**serial << 12 | index**), and dispatches through the embedded interface at **parent+0x10014** and entity-owned vtables.

Depending on virtual lookup results, the original invokes different entity virtual methods, then calls **0x24128180** to unbind that handle and reexamines the current primary pointer. Once the list is drained it resets DWORD parent+**0x10018**, +**0x1001C**, and +**0x10020** to zero, and parent+**0x10024** to 0xFFFFFFFF. A host reference provides callbacks for entity virtual dispatch and removal; it does **not** invent the engine virtual method implementations. It bounds the host loop to avoid unsafe infinite iteration.

Original direct edges include **0x24106256 → 0x24128180** and **0x2410828F → 0x241061E0**, the latter inside the already-reviewed original entity-list constructor 0x24108190.

## Original parent destructor 0x24107430

This method restores the parent vtable **0x242973B8** and embedded interface vtable **0x24297390**, calls the drain, then cleans three **distinct** embedded storage managers:

1. **PVS red-black map at parent+0x18060**: if growth policy at parent+0x1806C is not 0xFFFFFFFF, free nonzero backing pointer at parent+0x18064 via original 0x2416F940, then set capacity at parent+0x18068 to zero.
2. **16-byte list at parent+0x18044**: first reset via original **0x24106360**, then (unless fixed policy is -1) free nonzero backing pointer at parent+0x18044 and clear capacity at +0x18048.
3. **8-byte auxiliary list at parent+0x18028**: first reset via newly recovered **0x24105DD0**, then (unless policy at parent+0x18030 is -1) free nonzero backing pointer at parent+0x18028 and clear capacity at +0x1802C.

Finally, the parent destructor **tail-JMPs to original base destructor 0x24128070**, rather than returning directly. The previously SHA-reviewed scalar-deleting wrapper **0x241082A0** invokes this destructor at native callsite **0x241082A3** and optionally frees the containing object.

Native original allocator/free and base destructor bodies are not replaced here; host callbacks record original order and do not dereference original process addresses. Fixed-policy containers preserve their backing pointer and capacity. Real C++ object destruction, engine ownership and virtual destructor effects remain outside this phase.

## Five new compact PVS list/map manager wrappers

- 0x241082C0: call original eight-byte list unlink 0x24106C20, write old free head to recycled record WORD+6, then set manager free head WORD+0x10 to recycled index. Native wrapper does not branch on unlink result.
- 0x241082F0: same behavior for the separate **16-byte list** using original unlink 0x24106CB0 and node next WORD+0x0E. It is not the eight-byte list or the red-black map.
- 0x24108320: call original eight-byte allocator 0x24106570 with zero argument, link before requested index using original 0x241075E0, copy a single caller payload DWORD into new record+0, return new WORD index (RET8).
- 0x24108360: call original sixteen-byte list allocator 0x24106610 with zero argument, link via original 0x24107660, return new WORD index (RET4). It does not copy a payload.
- 0x24108390: allocate PVS red-black map index via earlier-recovered 0x24106700, attach and balance via earlier-recovered 0x241076E0, increment map live-count WORD+0x12, return WORD index (RET8). **There is no payload copy here.** This differs from original Phase236 map-insertion frontend 0x24108750, which separately copies its full eight-byte key.

The C++17 reference reuses typed host structures for the 8-byte list, 16-byte list and red-black map and injects the still-opaque list link/unlink callbacks. It composes map allocation with the recovered real original-style red-black tree attachment model.

## Evidence and tests

- research/tools/audit_sin_original_client_entitylist_pvs_lifecycle_wrappers.py: dual complete original-retail-DLL SHA gates, all seven full original method SHA256 hashes, 68 sparse byte witnesses, native exits/INT3, 17 authentic E8 sites and the base-destructor E9 tail jump.
- research/tests/test_sin_original_client_entitylist_pvs_lifecycle_wrappers.py: synthetic-only mutation tests for every native witness, each exit byte, all padding, 17 direct CALLs, the terminal JMP, PE32 base and section boundaries. Synthetic fixtures count as zero original reviewed methods.
- include/sin_compat/original_client_entitylist_pvs_lifecycle_wrappers.hpp: independent C++17 host reference for entity-handle drain, destructor callback sequence, fixed-policy native allocation/free gates, both distinct auxiliary recycle and link wrappers, and the map index allocation/attach/frontend count.
- tests/test_original_client_entitylist_pvs_lifecycle_wrappers.cpp: synthetic handle dispatch/removal order, no-entity and stuck-callback safety, destructor reset/free/tail order under fixed and nonfixed policies, distinct 8/16-byte free-head chains, allocation callback order and payload copy, red-black map root/left/right attachment without copying key payload, malformed-index refusals.
- Root Linux portable CTest and genuine native MSVC Windows x86 CTest registration.
- research/catalog/original_dll_verified_methods.json: seven new client original-code entries, reviewed ledger 368.

Owner-private original audit:

    python3 research/tools/audit_sin_original_client_entitylist_pvs_lifecycle_wrappers.py --archive /private/bin.zip

Synthetic-only tests:

    python3 -m unittest discover -s research/tests -p test_sin_original_client_entitylist_pvs_lifecycle_wrappers.py -v

## Outstanding original engine dependencies

This does not reconstruct the original virtual entity methods invoked by the entity-release dispatcher, base destructor 0x24128070, native heap/free 0x2416F940, full parent object actual Windows RTTI/CRT lifetime, or build and load fully independent gameplay client.dll/server.dll into the retail engine. The existing proxy forwarding to the original game DLLs is not a complete decompilation.

Target order unchanged: **original Windows PC retail game-engine independent DLLs first; Android ARM64 second; Quest 3 VR third**.
