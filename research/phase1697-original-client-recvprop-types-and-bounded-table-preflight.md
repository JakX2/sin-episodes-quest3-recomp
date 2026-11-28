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
```

Private, user-owned original file only:

```sh
python research/tools/audit_sin_original_client_recvprop_type_inventory.py --client /private/client.dll --full-json /private/sin-recvprop-types-metadata.json
```

The `--full-json` report lists source class names, all original property names, original x86 offsets where verified, native numeric type tags where authenticated and null for the remaining eleven. It contains no executable original game code and must not be used as a live engine network decoder. **All 939 native type tags are now classified; none should be silently mapped to newer Source SDK enum numbers.**

## Critical remaining work

The **eleven copied property native type tags are now resolved as type 5**. Remaining work includes those dynamically constructed nested table **contents and element linkage**, per-field proxy functions, flags, other nested subtable target pointers, class creation call signatures, class IDs and actual engine networking initialization. Pass those independently compiled native ABI tests before allowing original engine `VClient011.slot5` to return a real ClientClass pointer.

The 2006 source's model/prop/rendering, surface property and HL2-equivalent physics systems remain explicitly deferred by the owner. None of the metadata graph work changes those systems or proves a new retail game boot, map, menu, Android APK or Quest3 runtime.
