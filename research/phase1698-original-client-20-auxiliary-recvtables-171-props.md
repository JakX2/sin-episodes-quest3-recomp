# Phase 1698: full 211 original static client receive tables and 1,110 property type tags

## Runtime reason and provenance

The owner-verified original SiN Episodes Windows engine loaded the independently reconstructed client.dll, completed all 21 source app-interface acquisitions and sound connection, and accepted prediction, entity list, centerprint and ClientLeafSystem001. The next live virtual callback was `VClient011.slot5`, which **intentionally terminated with diagnostic 0x53494E45** because no valid 2006-era native ClientClass / RecvTable network ABI has yet been reconstructed. Both original retail DLLs were verified restored or unchanged.

The exact original 2006 PE32 `client.dll` is read-only and SHA256-pinned to:

`6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.

Phase1696 previously recovered the source order of **191 ClientClass nodes** and all 939 class-attached original property names. Phase1697 extends that to source numeric type tags for all 939, the 11 dynamic nested-array builders with 579 named child elements, and original source targets for 191 ordinary type-5 nested links (171 global pointer loads +20 direct literal addresses). Phase1698 is a **distinct** original static receive-table inventory completion for the remaining **20 auxiliary tables**, not a duplicated method byte-count milestone.

## All original 211 statically constructed receive tables

Authentic native calls to original constructor `0x24126A30` describe **214 source callsites and 211 distinct static RecvTable objects**. Exactly 191 are directly attached to the ClientClass registry. The 20 remaining auxiliary objects are genuine native tables used by nested client networking data.

| Source aux table | Original 2006 RecvTable VA | Declared 60-byte properties |
| --- | --- | ---: |
| `DT_EnvWindShared` | `0x24323DDC` | 11 |
| `DT_LocalActiveWeaponData` | `0x2432B964` | 4 |
| `DT_LocalWeaponData` | `0x2432B97C` | 5 |
| `DT_BeamPredictableId` | `0x2432D124` | 2 |
| `DT_ServerAnimationData` | `0x24336278` | 1 |
| `DT_Animationlayer` | `0x24337464` | 5 |
| `DT_OverlayVars` | `0x2433747C` | 1 |
| `DT_BCCLocalPlayerExclusive` | `0x24337674` | 2 |
| `DT_AnimTimeMustBeFirst` | `0x24337944` | 1 |
| `DT_PredictableId` | `0x2433795C` | 2 |
| `DT_PlayerState` | `0x24338E5C` | 1 |
| `DT_Local` | `0x24338E74` | 49 |
| `DT_LocalPlayerExclusive` | `0x24338E8C` | 27 |
| `DT_ProxyToggle_ProxiedData` | `0x2433ECE0` | 1 |
| `DT_CollisionProperty` | `0x2435878C` | 8 |
| `DT_EffectData` | `0x24360F28` | 20 |
| `DT_EntityParticleTrailInfo` | `0x24361044` | 3 |
| `DT_EnvHeadcrabCanisterShared` | `0x24379860` | 15 |
| `DT_SE1GameRules` | `0x2437A398` | 0 |
| `DT_HL2Local` | `0x2437B7B4` | 13 |
| **Total** | **20 tables** | **171 slots** |

The 171 auxiliary properties plus the previously authenticated 939 class-attached properties account for **1,110 explicitly source-declared static RecvProp slots across all 211 static tables**. This does **not** count the 579 dynamically generated child array elements as static slots, nor does it mean those extra tables are ClientClasses.

### The 192nd source-proven static nested RecvTable edge

Phase1697 proved the original **191 ordinary type-5 nested links** originating from directly registered ClientClass tables. The last ordinary nested link is inside the auxiliary table **`DT_LocalPlayerExclusive`**: its property **`m_Local`** invokes original type-5 constructor `0x24126B50` at source CALL **`0x240D38FB`**, with the **actual child table** `DT_Local` at original client VA **`0x24338E74`** and original x86 parent-object member offset **`0x0D68`**. The parent table's original VA is **`0x24338E8C`**.

The original SHA-pinned source witness at `0x240D38E5` is a literal PUSH of `0x24338E74`, then zero and member offset `0x0D68`; at `0x240D38F1` the compiler pushes the property descriptor destination, name and a direct relative CALL to the original native nested-table helper. This was rechecked byte-for-byte against the unchanged source file, without importing proprietary original executable bytes into the repository.

There are now **192/192 source-accounted ordinary static nested-table edges** across the 211 statically constructed table objects: 191 from ClientClass-attached tables and **one from auxiliary RecvTable data**. This 192 total intentionally **excludes** the dynamic nested-array constructors (including the four auxiliary special constructors and the 11 class array constructors), which require their separately generated child table ownership rules.

## Exact source names, original x86 constructor paths and types

Of the 171 auxiliary properties, **166** use the canonical native `PUSH property name; PUSH 60-byte output descriptor; direct relative CALL to constructor` sequence. Their actual original constructor targets map to native type0/1/2 or type5. The previously unaudited `0x241A8D80` wrapper is **40 complete original executable bytes**, whole-method SHA256 `dc0a086f0f7bf32f2fe82a62e9dc92feb353d5374adc583b78cf6ef4d1a773a9`, and its real `0x241A8D9C` CALL to `0x24126DF0` proves native type0 rather than an invented new type.

The remaining **five original properties** are not ordinary factory calls:

| Original source auxiliary table and property | Native type | Actual constructor path |
| --- | ---: | --- |
| `DT_OverlayVars.m_AnimOverlay` | 5 | `0x241271B0`, original overlay-specific dynamic nested-table method |
| `DT_BCCLocalPlayerExclusive.m_hMyWeapons` | 5 | `0x24126E90`, dynamic nested-array constructor |
| `DT_Local.m_chAreaBits` | 5 | `0x24126E90`, dynamic nested-array constructor |
| `DT_LocalPlayerExclusive.m_iAmmo` | 5 | `0x24126E90`, dynamic nested-array constructor |
| `DT_LocalPlayerExclusive.m_hViewModel` | 4 | `0x24126BD0`, original native type-four array constructor |

Each special callsite is authenticated by the exact source destination descriptor, literal `.rdata` property name, direct relative CALL target and its actual stack shape (`REP MOVSD` plus call on the nested-array paths). The original `0x241271B0` overlay method is now authenticated as a **complete 657-byte native PE32 function**, SHA256 `b2e9bf21b248f751855489484f6ac04d4a8809473406e3287701a1ed4188c670`, followed by original INT3 padding. It sets native nested-table type **5** at source VA `0x241271E2`, requests a 28-byte auxiliary object at `0x24127231` and 20-byte child-table objects at `0x241272F1` / `0x241273B5`, calls source nested-table factory `0x24126B50` from `0x24127324`, and obtains generated element name pointers through the same original 256-entry numeric name table `0x242FB110` (source load/store witnesses `0x2412734C`, `0x24127386`). Its larger native side effects, lifetime and network proxy callbacks remain outside the host implementation; do not treat 657 source-verified bytes as a working animated overlay runtime.

Original type-DWORD totals for the **171 auxiliary properties**:

| Original 2006 type | Aux properties |
| ---: | ---: |
| 0, integer-like | **75** |
| 1, float | **62** |
| 2, vector3 | **28** |
| 3, string | **0** |
| 4, array | **1** |
| 5, nested table | **5** |
| **Total** | **171** |

Adding Phase1697's 939 class-attached type tags gives **1,110/1,110 source type-tagged statically declared properties**: type0 **414**, type1 **300**, type2 **164**, type3 **12**, type4 **13**, and type5 **207**. The 579 dynamically generated array element descriptors sit *inside* the source type5 nested tables and are separately counted.

The entire auxiliary `tableName|sourceTableVA|slot|name|type|factory|sourceCallsite|constructorRoute` sequence, sorted by actual original static table VA for a deterministic public metadata order (not a fabricated CRT linked order), has SHA256:

`0b0ad5e2abb9d0e9833acbf70ab166de70130a12b0a4de866eb3723e35dc4fa8`

## Code, reproducibility and boundaries

`research/tools/audit_sin_original_client_aux_recvtables.py` is a read-only source audit. Its `--client` mode authenticates the whole original client PE32, prior 191/939 metadata, all 211 original table constructor objects, 166 ordinary factory property names and callsites, five alternate source paths, 20 table counts and 171 type tags. It rejects changed original method bytes, source string literals, unknown helper targets, or table/name/field-order fingerprint drift. Public `--self-test` uses fabricated source names and SHA refusal, **not original retail binaries**.

`include/sin_compat/original_client_aux_recvtable_metadata.hpp` models only the 20 original table labels, source address *numbers* and declared counts as immutable C++17 metadata. The associated native test checks the source-defined cardinality, strict ordering and unique names/pointers, selected SiN-specific table identities and `CanExposeToRetailEngine()==false`. This header does **not** instantiate native 20-byte RecvTable structures, actual Win32/ARM network proxies or live original executable callbacks.

Linux strict C++17/ASan/UBSan, MSVC Win32 x86, Android ARM64 cross-compilation and real Bionic x86_64 emulator metadata tests are wired to the PR. As with all other phases, the connected original binaries are NEVER committed to public GitHub or injected into CI.

### Reproduce

Original-free checks:

```sh
python research/tools/audit_sin_original_client_aux_recvtables.py --self-test
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror -UNDEBUG -Iinclude tests/test_original_client_aux_recvtable_metadata.cpp -o /tmp/sin-aux-recvtables
/tmp/sin-aux-recvtables
```

Private owner-original full source audit:

```sh
python research/tools/audit_sin_original_client_aux_recvtables.py --client /private/client.dll --full-json /private/sin-all-20-auxiliary-tables-171-properties.json
```

**Next:** recover the five dynamic auxiliary child-table element layouts, *all* original per-property flags/proxy functions, and engine-approved native object/class factory ownership. Do not return fabricated ClientClass/RecvTable pointers from `VClient011.slot5` just to bypass startup. No retail menu/map boot, Android native game execution, or VR frame has been achieved by this metadata work. User-excluded HL2-equivalent physics, models, props, material surfaces and rendering remain deferred.
