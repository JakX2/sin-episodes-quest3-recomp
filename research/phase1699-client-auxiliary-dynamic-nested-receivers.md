# Phase 1699: original client auxiliary dynamic network receivers and source-relative offsets

## Original retail state and scope

The real original Windows SiN Episodes engine already loaded the independently reconstructed PE32 client.dll, completed 21 original engine interface lookups, sound Connect continuation and diagnostic prediction, entity list, centerprint and leaf factories. The next original engine call `VClient011.slot5` intentionally emits `0x53494E45` because source-owned `ClientClass*` and `RecvTable*` objects, 2006 proxy callbacks and dynamic child allocation have not been reconstructed. Both owner retail originals were restored or retained. This phase does **not** make a fake slot5 return and does not claim another game boot.

Original file: user-supplied unchanged 2006 PE32 `client.dll`, SHA256 `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.

Phase1698 completed **211 static RecvTable objects and 1,110 typed static properties** (191 class-attached +20 auxiliary). Phase1697 added 11 dynamic class-owned nested array wrappers containing 579 generated child properties. This phase reconstructs **three additional AUXILIARY static-table properties whose constructor dynamically creates child RecvTables**, plus two special receiver paths that are not generic nested arrays.

## Original 351-byte dynamic array builder and three auxiliary callers

The exact full original `0x24126E90` dynamic child table factory is 351 native bytes, SHA256 `c6c8d8af7d77994a6ff4292d9a432172bad6e9a3d72d29b6ec8284c3a3eac0db`. It initializes a **native type-5 outer property**, allocates **count × 60-byte** child RecvProps and a separate **20-byte RecvTable**. The child RecvProp member-offset field at byte **+0x2C** is relative, explicitly set to **0, stride, 2×stride, ...** by original `0x24126F6B` / `0x24126F7D` / `0x24126F87`. The outer property contains the object-level base. New C++ code deliberately stores both native child-local offsets and separately *derived* parent-object offsets.

Three additional genuine original source callers:

| Original auxiliary table/property | Count | Stride | Original outer x86 base | Generated child-native offsets | Source caller |
| --- | ---: | ---: | ---: | --- | --- |
| `DT_BCCLocalPlayerExclusive.m_hMyWeapons` | 48 | 4 | `0xC80` (3200) | 0..188 | `0x240C37E3` |
| `DT_Local.m_chAreaBits` | 32 | 1 | `0x4` (4) | 0..31 | `0x240D3346` |
| `DT_LocalPlayerExclusive.m_iAmmo` | 32 | 4 | `0xC00` (3072) | 0..124 | `0x240D39B6` |
| **Total** | **112** | | | | |

All 112 generated child properties use source-backed **integer type 0** from the original native template factory or validated wrapper, with original indexed three-digit property names `000` through `047` where present. These **112 child descriptors** are additional to the 1,110 static properties, and additional to Phase1697's 579 child descriptors: there are now **691 known generic dynamic child descriptors across fourteen nested arrays**.

The independent original complete-code caller hashes:

- `0x240C37C4`, 43 bytes, SHA256 `b43812d7cb0a33498b14ac6ab4df7c2f6ba6cd13d71663d3f20d73868184436f`.
- `0x240D332A`, 40 bytes, SHA256 `e7e5b9dba03989a0142abf231cad509801b1d48357855925105f8988afe8b228`.
- `0x240D3997`, 43 bytes, SHA256 `1aace3b00eba9bba81a29f467f576d1be69a0f1eebb323e4e0ab474d30580ebc`.

The whole original DLL SHA is checked **before** accepting them. The original source literal count, width, parent base, `PUSH destination`, 15-DWORD `REP MOVSD`, and direct call to `0x24126E90` are authenticated. Source-relative offsets are not raw ARM64 pointers.

## Special native type-4 viewmodel and animation overlay remain distinct

`DT_LocalPlayerExclusive.m_hViewModel` is original **type 4**, not type 5. Its 21-byte original callsite at `0x240D3AFB` has SHA256 `8b9ac443d2e549508c2abf94f8f7ce87b1dd71929dc95fc15f594e0e1465ea04`, and it passes **count 2, width 4** to the whole-authenticated original type-4 helper `0x24126BD0` (102 bytes, SHA `dbfd5065ab7af1f331c11bd90a892adac57943b63a8af72b06e29da18400d8cd`). It does **not** allocate a nested RecvTable; treating it as another source type-5 receiver would be a native ABI error.

`DT_OverlayVars.m_AnimOverlay` uses an independent original **621-byte** specialized method at `0x241271B0`, full SHA256 `a41703fce2d6cd865d2457b21738fa927ee63cda2ca34b473188d213528df4c6`. Its complete **48-byte** original caller at `0x240C36B6`, SHA256 `fb22001d97755db34f3115158e6fc0b215ab7b002f6cca779223752e6c63657f`, supplies **requested overlay count 15**, component **32-byte stride**, and original outer object offset **0x7A4**. The source method executes `lea esi,[edi+1]` and allocates **16 × 60 = 960 bytes** of descriptor memory, plus helper-side objects. It installs two original specialized callback functions **`0x24127150`** (44 original bytes, SHA256 `d242eefc3be987bc2737d990d2570fcb1b8baee658423c52d5e9cb50d19863f5`) and **`0x24127180`** (46 bytes, SHA256 `10b210d9c6cec8dbd257b131df49f1c0631a9fbf50af8e4f439f536d8fbb948e`) into selected generated descriptors.

**Important:** Those two overlay callbacks are identified, not executable in the independent DLL. A 16-descriptor allocation does **not** establish sixteen independent final network fields or correct overlay proxy semantics. We deliberately do not fold the 16 into the authenticated 691 generic child-descriptor count.

## Implementation and tests

`research/tools/audit_sin_original_client_aux_dynamic_receivers.py` is an original read-only binary source auditor. It first authenticates all Phase1698 auxiliary table/property metadata, then checks the exact whole original native helper bodies, the three conventional array callsites, original count/stride/outer-base operand encodings, the specialized viewmodel and overlay callers, original overlay count+1 allocation arithmetic and the two source callback pointer stores. It supports owner-private `--full-json` metadata exports without original executable bytes.

`include/sin_compat/original_client_aux_dynamic_receivers.hpp` implements a portable C++17 safe metadata model of the three exact source arrays and separate immutable special type4/overlay facts. The three arrays generate **112** host-owned name/type/child-relative-offset records. The header refuses mutated count, stride, original outer offset, element type, invalid table name and integer overflow, and `CanExposeToRetailEngine()` ALWAYS returns false.

`tests/test_original_client_aux_dynamic_receivers.cpp` checks all 112 records, original names `000...`, exact child-local offsets, derived original x86 positions, mutation refusals and the native type5 nested-table cycle-safe graph. Special viewmodel/overlay identities and the fact they are **not generic nested arrays** remain separate. MSVC Win32 x86, strict Linux C++17, ASan/UBSan, Android ARM64 cross-build and Bionic x86_64 emulator metadata tests are included.

Public, original-free checks:

```sh
python research/tools/audit_sin_original_client_aux_dynamic_receivers.py --self-test
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror -UNDEBUG -Iinclude tests/test_original_client_aux_dynamic_receivers.cpp -o /tmp/sin-aux-dynamic
/tmp/sin-aux-dynamic
```

Original owner-private read-only source audit:

```sh
python research/tools/audit_sin_original_client_aux_dynamic_receivers.py --client /private/client.dll --full-json /private/sin-aux-dynamic-receivers-source-metadata.json
```

## Next runtime dependency

All original known table and descriptor names, original x86 type DWORDs and many nested links are now source-authenticated. Real `VClient011.slot5` still requires correct 2006 native descriptor contents, original proxy callbacks, object/class factories, lifecycle and the remaining auxiliary nested dynamic rules. A synthetic graph test passing is **not** a game boot. No work was done on the explicitly deferred physics, model/prop, surface-properties or renderer tasks.
