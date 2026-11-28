# Phase 1697: original client RecvProp types and bounded network-table preflight

## Real retail-engine boundary

The last genuine original Windows engine test loaded our independent SiN client DLL, received all 21 source interfaces, completed the owner-tested native sound/prediction/entity-list/centerprint paths, and accepted `ClientLeafSystem001`. Its next call to `VClient011.slot5` deliberately stopped with diagnostic code `0x53494E45`. Original retail client and server were restored/retained intact.

Phase1696 recovered the **191 exact source ClientClass registrations**, their CRT head-prepend order, all **191 class-attached RecvTables**, and all **939 source property names** from the owner's SHA-pinned 2006 PE32 client.dll. Three whole original engine functions consuming this linked registry confirmed that the engine follows native node +0x10, gathers class RecvTables at +0x0C, and recursively traverses the actual RecvTable/RecvProp graph.

This phase **does not** bypass slot5 or substitute a guessed ClientClass list. It advances the source-backed contract and implements a safe portable preflight.

## Whole-original source classification of native RecvProp type DWORD +4

Original 2006 `client.dll` SHA256:
`6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.

`research/tools/audit_sin_original_client_recvprop_type_inventory.py` checks the **full original file SHA256**, exact full-function hashes of the original constructor functions and wrappers, native type assignment instructions and direct wrapper calls. Then it recovers all 939 class-attached original name/slot/source-constructor records through the prior Phase1696 audit. Its **original numeric property type** classification is:

| Native 2006 type DWORD at RecvProp +4 | Source representation | Class-attached slots |
|---:|---|---:|
| 0 | Integer-like / integer-wrapper | **339** |
| 1 | Float / float-wrapper | **238** |
| 2 | Three-component vector | **136** |
| 3 | String | **12** |
| 4 | Array descriptor | **12** |
| 5 | Nested RecvTable, including 11 generated array subtables | **202** |
| **Total** | **All native type tags authenticated** | **939** |

The 12 native type-four descriptors come through original `0x24126BD0` array helper. The remaining 11 array-copy routes enter the **351-byte original `0x24126E90`** method. Deeper whole-function disassembly verified that it allocates a variable-sized 60-byte descriptor array, constructs a separate 20-byte RecvTable, writes the **native type DWORD 5** into its outer temporary property descriptor and copies exactly 60 bytes into the destination. Therefore all eleven are **nested-table type 5**, not unknowns. Their nested table contents, element wiring, proxy callbacks and allocation-failure behavior still require further source recovery.

The unresolved class properties are `CPlayerResource`'s seven `m_iPing`, `m_iScore`, `m_iDeaths`, `m_bConnected`, `m_iTeam`, `m_bAlive`, `m_iHealth`; `CBreakableSurface.m_RawPanelBitVec`; `CBaseFlex.m_flexWeight`; `CBaseAnimating.m_flPoseParameter` and `CBaseAnimating.m_flEncodedController`.

Full source ordered `class|table|slot|name|numeric-tag|constructor|constructor-route` SHA256:
`149122c6e319169fe9763990d14049887da11e513f119599958776392309ecc8`.

These numeric native types are not a guarantee of matching modern Source SDK enum assignments, quantization rules, proxy semantics or platform field sizes.

### Proven original factory method anchors

The original type-2/3/4/5 factories at VAs `0x24126A60` (109 bytes), `0x24126AD0` (117 bytes), `0x24126BD0` (102 bytes), `0x24126B50` (113 bytes), **and the special `0x24126E90` (351-byte dynamic nested-table array constructor)** have individually SHA-verified complete original function bodies, original INT3 boundaries and exact native type-DWORD assignments. Earlier Phase251 already recovered type-zero integer factory `0x24126DF0` (146 bytes) and type-one float factory `0x24126D80` (106 bytes). Original wrappers at `0x241A8C60`, `0x241A8C90`, `0x241A8CC0`, `0x241A8D30` call the authenticated native integer or float constructors. Their complete function hashes and direct CALL edges are also checked by this phase.

Previously authenticated bytes are **not counted twice**. The five complete type-2/3/4/5/array-to-nested-table factory methods total **792 original bytes** (109+117+102+113+351), separately from the already authenticated Phase251 type-zero and type-one methods. This metadata milestone does not assert these bytes are novel across all older historical research without exhaustive duplicate checks.


### Exact original eleven dynamically constructed nested arrays

Whole-function original `0x24126E90` also exposes its generated **child** table layout. It computes `count * 60` bytes for element descriptors using the original integer arithmetic, calls original allocator `0x2416FCF0`, initializes each 60-byte element from the prepared template, writes the element's member offset at **descriptor+0x2C**, increments the offset by the **caller-supplied byte stride** after each element, and separately requests **0x14 (20) bytes** for the child RecvTable. The outer property is type **5**, pointing to that child table. The element template is first constructed with original type-0 integer factory `0x24126DF0` or type-1 float factory `0x24126D80` immediately before the dynamic array call.

The original 11 caller-side count/stride/first-offset operands, independently checked against the exact whole-DLL SHA256, are:

| Original class and property | Elements | Byte stride | First original x86 offset | Element native type |
| --- | ---: | ---: | ---: | ---: |
| `CPlayerResource.m_iPing` | 33 | 4 | 2212 | 0 |
| `CPlayerResource.m_iScore` | 33 | 4 | 2344 | 0 |
| `CPlayerResource.m_iDeaths` | 33 | 4 | 2476 | 0 |
| `CPlayerResource.m_bConnected` | 33 | 1 | 2608 | 0 |
| `CPlayerResource.m_iTeam` | 33 | 4 | 2644 | 0 |
| `CPlayerResource.m_bAlive` | 33 | 1 | 2776 | 0 |
| `CPlayerResource.m_iHealth` | 33 | 4 | 2812 | 0 |
| `CBreakableSurface.m_RawPanelBitVec` | 256 | 1 | 1236 | 0 |
| `CBaseFlex.m_flexWeight` | 64 | 4 | 2144 | 1 |
| `CBaseAnimating.m_flPoseParameter` | 24 | 4 | 1352 | 1 |
| `CBaseAnimating.m_flEncodedController` | 4 | 4 | 1644 | 1 |
| **Total** | **579** | | | |

### Native child-relative offsets versus original parent-object offsets

The original 351-byte array builder at `0x24126E90` explicitly executes **`xor ebp, ebp` at 0x24126F6B**, **`mov [edx+0x2C], ebp` at 0x24126F7D**, followed by **`add ebp, [esp+0x5C]` at 0x24126F87**. Consequently, the *actual* native RecvProp member-offset field at byte **+44** in each **generated child descriptor** is **0, stride, 2×stride, ...**, relative to the nested table, not a fully adjusted object address. The caller-supplied original 2006 PE32 offset (e.g. `m_iPing` at **2212**) belongs to the **outer type-5 parent property**, whose nested relative offsets start at zero. An effective entity-object byte position may be **derived** as outer base + child-local offset, but that derived sum must **not** be written to original child descriptor +44.

**Source-specific examples:** `CPlayerResource.m_iPing` has 33 child offsets `0,4,...,128`, with *derived* parent-object positions `2212,2216,...,2340`. `CBreakableSurface.m_RawPanelBitVec` has 256 child offsets `0,1,...,255`, while its derived parent-object positions are `1236...1491`. The updated portable builder stores the two fields separately (`originalChildRelativeMemberOffset`, `derivedParentObjectX86Offset`), and the native/Android tests assert both. Earlier source metadata exports that called the derived sum a native child member offset must be interpreted using this correction.

### All 579 generated element names source-authenticated

Original function `0x24126F80` loads each generated element name pointer from the real, position-indexed 256-entry pointer table at original PE32 `0x242FB110`, using `[table + elementIndex*4]`, then writes the resolved pointer to the 60-byte generated descriptor's name field at `0x24126F8B`. The entire **1,024-byte original table SHA256** is `7f419caa89c32a637e4f7b7b81f247e232fcc55d1673721e6a3c9dd46d040506`. Every one of its original entries dereferences to a verified NUL-terminated ASCII `000` through `255` in original `.rdata` (newline-delimited name SHA256 `e139eda07b5876a36e7be3471540c112ecbc58b7cfdd1e33f54c4c4c4fc958c4`). This is **actual original data**, not a naming assumption borrowed from a Source SDK.

Because each of the eleven arrays starts its element index at 0, the generated property names repeat `000` through `(count-1)` **within each distinct child table**. For example, `CPlayerResource.m_iPing` uses names `000` through `032`, while `CBreakableSurface.m_RawPanelBitVec` uses `000` through `255`. The private Python source audit verifies the entire table's SHA256, all 256 name pointers, and original indexed load/store opcode witnesses. The portable C++ builder retains the actual three-digit labels as inline owned character arrays and tests every one of the 579 names and NUL terminators on native platforms and Android.

These **579 generated inner elements** are additional to the 939 class-attached outer descriptors. This is source-backed structure and sizing, not a reconstruction of actual packet bit widths, proxy callback bodies, runtime allocation ownership or child-table network layout across systems.

`include/sin_compat/original_client_nested_array_table.hpp` implements a C++17 **host metadata** version of these exact shapes, with source name/count/width/offset/type identity checks, native offset-overflow checks and generated element-offset calculations. It cannot produce game-owned RecvProp pointers. Native and Android regression tests independently verify all 11 shapes, all 579 element offsets, mutation refusal and a graph walk of all 11 parent/child table pairs. The C++ tests never execute original proprietary code and do not claim a running Android/Quest game.


## Portable bounded RecvTable graph preflight

`include/sin_compat/original_client_recvtable_preflight.hpp` models the **original engine's actual traversal constraints**, with no assumed renderer or Windows DLL pointer ownership:

- Original engine collector stages up to **1,023 roots**, rejecting **1,024 or more** before risking its original 4,096-byte stack buffer if the original diagnostic callback were to return.
- Original source `ClientClass` node fields: RecvTable pointer byte **+0x0C**, next pointer **+0x10**.
- Original RecvTable state flag byte **+0x11**, read/marked by recursive source engine `0x20085630`.
- Original descriptor stride **0x3C (60 bytes)**, native type DWORD **+4**, nested-table type numeric **5**, nested RecvTable pointer byte **+0x28**.
- During validation, an independent visited-state vector marks each host table **before** descending. This matches the source recursion's cycle termination without mutating arbitrary live engine memory.
- Unknown original field type, missing nested table, wrong type, empty table name and out-of-range index **fail closed**. Revisited nested tables terminate safely.
- Host graph uses **stable table indices**, not original PE32 virtual addresses. `CanExposeToRetailEngine()` unconditionally returns **false**.

The C++ test loads the real source-authenticated **191 class names and linked-list order** from Phase1696, then uses explicitly labeled **synthetic** property distribution (939 across 191 tables) to stress safe traversal. It separately tests a cycle, a missing nested pointer, an unresolved type, invalid roots, mutation refusal and the 1,024-root bound. **The 939 synthetic test properties are not the original 939 field descriptors.**

### Reproduction

Public tests without commercial game binaries:

```sh
python research/tools/audit_sin_original_client_recvprop_type_inventory.py --self-test
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror -UNDEBUG -Iinclude tests/test_original_client_recvtable_preflight.cpp -o /tmp/sin-original-nettable-preflight
/tmp/sin-original-nettable-preflight research/manifests/original_client_class_catalog_phase1696.tsv
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror -UNDEBUG -Iinclude tests/test_original_client_nested_array_table.cpp -o /tmp/sin-original-nested-arrays
/tmp/sin-original-nested-arrays
```

Private, user-owned original file only:

```sh
python research/tools/audit_sin_original_client_recvprop_type_inventory.py --client /private/client.dll --full-json /private/sin-recvprop-types-metadata.json
python research/tools/audit_sin_original_client_nested_recvtable_targets.py --client /private/client.dll --full-json /private/sin-static-recvtable-links.json
```

The `--full-json` report lists source class names, all original property names, original x86 offsets where verified, native numeric type tags where authenticated and null for the remaining eleven. It contains no executable original game code and must not be used as a live engine network decoder. **All 939 native type tags are now classified; none should be silently mapped to newer Source SDK enum numbers.**


## All 191 static nested RecvTable pointers recovered from original callers

A complete original SHA-pinned `client.dll` analysis now resolves the **191 ordinary type-5 parent/child links** in all 191 class-attached source RecvTables. This is separate from the **11 dynamic array type-5 properties**, which allocate their own generated child tables through original `0x24126E90`.

The direct original x86 `push property_name; push descriptor; call 0x24126B50` callsite is authenticated per nested property. The **preceding argument sequence** shows the child table address with exact source proof:

| Native source pointer form | Original caller behavior | Number of links |
|---|---|---:|
| Indirect | `mov eax, [global]; push 0x24126CE0; push eax`, where the 4-byte global contains a source table pointer | **171** |
| Direct | Literal `push original RecvTable VA` and source offset argument, with or without a 32-bit offset PUSH | **20** |
| **Total** | Every original ordinary type-5 class property | **191** |

These 191 edges reference **46 unique** source static RecvTable objects. **28** are registered class-attached tables and **18** are independently constructed auxiliary subtables, all among the previously authenticated **211** original constructor-registered static RecvTable objects. For instance, original `CNPC_GruntBase.baseclass` refers to `DT_AI_BaseNPC`, `CBasePlayer.localdata` refers to `DT_LocalPlayerExclusive`, `CSE1Proxy.se1_gamerules_data` refers to `DT_SE1GameRules`, `CEntityParticleTrail.m_Info` refers to `DT_EntityParticleTrailInfo`, and `CEnvWind.m_EnvWindShared` refers to `DT_EnvWindShared`.

The recovered class-attached subgraph is acyclic with maximum source depth **9** table levels. This fact does not prove that the *remaining auxiliary subtable network* is acyclic, since their individual inner property edges still require analysis. The engine's visited flag and strict graph traversal must remain in place.

The ordered `class|table|slot|property|callsite|childName|childVA|directOrGlobal|operandSite|globalOrDash` source manifest fingerprint, independently checked against the exact whole retail binary SHA256, is:

`1ae2fc278381df74ab9d08179168169d87756f6e3f59dc86956c2ea6d2689242`

A new `research/tools/audit_sin_original_client_nested_recvtable_targets.py` reproduces this full original-source verification, checks 171 indirect and 20 direct native caller stack patterns, all 191 child table names and actual `.data` VAs, and refuses any mismatches or extra edges. Its public `--self-test` uses invented metadata only; the true original audit requires `--client /private/client.dll`. The user-private JSON export can include the complete 191-edge metadata list and the 579 dynamic child-element records; neither can be linked as a real native ClientClass structure until all needed owner-original callbacks are reconstructed.


## Critical remaining work

The **eleven copied property native type tags are now resolved as type 5**. Remaining work includes those dynamically constructed nested table **contents and element linkage**, per-field proxy functions, flags, other nested subtable target pointers, class creation call signatures, class IDs and actual engine networking initialization. Pass those independently compiled native ABI tests before allowing original engine `VClient011.slot5` to return a real ClientClass pointer.

The 2006 source's model/prop/rendering, surface property and HL2-equivalent physics systems remain explicitly deferred by the owner. None of the metadata graph work changes those systems or proves a new retail game boot, map, menu, Android APK or Quest3 runtime.
