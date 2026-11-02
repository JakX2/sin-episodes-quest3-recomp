# Original SiN Episodes client/server network table and property audit

**Evidence:** owner-provided original 2006 Windows PE32 x86 `client.dll» (SHA-256 `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d») and `server.dll» (SHA-256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650»). Both are available from the owner's private Google Drive `sin files/bin.zip» and correspond to the previously pinned game binary set. This report records reverse-engineered metadata; original executable bytes have not been committed.

## Grounded original runtime metadata

The original client implements a network data-table construction routine at **`0x24126A30»**. Scanning .text for genuine calls to it with the required preceding `push DT_name; push property_count; push property_array; mov ecx,RecvTable_object» sequence finds **213 calls representing 210 distinct client receive-table names**. Three names have more than one registration call. This distinguishes true original `RecvTable» construction from a loose string search.

The original client `RecvProp» slots are **0x3C bytes apart** in the recognized arrays. Each call that initializes a slot is checked against an actual NUL-terminated property string in .rdata, a slot within its table's declared array and a valid helper in .text. **1,079 of 1,107 declared slots** had their property name/constructor sites recovered by this narrow pattern. The other **28** remain explicitly unresolved, including array or special proxy encodings. The recovered field offsets are the immediate values passed into the original property constructors, not an inferred ARM64 object layout.

The original server's corresponding original send-table construction routine is **`0x2225E850»**. Server code generation often reads a name pointer from a global (`mov eax,[global]; push eax») rather than directly pushing the name address. Scanning that verified sequence finds **40 complete send-table constructor calls with unique names**. Their original `SendProp» records are spaced **0x4C bytes apart**. Among their 275 declared slots, **270** have readable name and property initializer call sites with the recognized pattern; five remain unresolved.

**Important limitation:** the original server contains **183 unique `DT_*» strings**, but our server constructor scanner recognizes *only this one compiler idiom*. The other 143 are not proven absent and must **not** be counted as unsupported tables or missing game features.

## Exact source-to-client weapon state (original x86)

| Table | Property | Client offset | Server offset |
|---|---|---:|---:|
| DT_LocalWeaponData | m_iClip1 | 0x7E0 | 0x4EC |
| DT_LocalWeaponData | m_iClip2 | 0x7E4 | 0x4F0 |
| DT_LocalWeaponData | m_iPrimaryAmmoType | 0x7D8 | 0x4E4 |
| DT_LocalWeaponData | m_iSecondaryAmmoType | 0x7DC | 0x4E8 |
| DT_LocalWeaponData | m_nViewModelIndex | 0x78C | 0x498 |
| DT_BaseViewModel | m_nViewModelIndex | 0x788 | 0x494 |
| DT_BaseViewModel | m_viewmodelOriginOffset | 0x7A8 | 0x4B0 |
| DT_BaseViewModel | m_hWeapon | 0x7C0 | 0x4C8 |
| DT_BaseAnimating | m_flPlaybackRate | 0x4C8 | 0x3C0 |

The client additionally registers `DT_LocalActiveWeaponData» with four directly recovered properties:

- `m_flNextPrimaryAttack» at client +0x790.
- `m_flNextSecondaryAttack» at client +0x794.
- `m_nNextThinkTick» at client +0x84.
- `m_flTimeWeaponIdle» at client +0x798.

The server's matching sendtable for `DT_LocalActiveWeaponData» was not captured by the current 40-table compiler-pattern scanner, so the present report does **not** assign a server offset from guessed symmetry. For these fields, previously confirmed server offsets may exist elsewhere in the phase archive, but this network scanner does not assert them without a matching constructor site.

**218 original client/server property slots** have a matching table name, slot index and literal field name within the specifically recovered patterns. Matching names do **not** establish that the two sides use identical quantization, proxy functions, wire encoding, or prediction behavior. Most numeric offsets differ because the client/server class layouts differ.

## Player, enemy, vehicle contract examples

The client `DT_HL2_Player» has six registered properties including `m_HL2Local» at +0x11C8, `m_fIsSprinting» at +0x1224 and `m_flFlashTime» at +0x123C.

The original `DT_NPC_GruntBase» receive table includes `m_receivedDecapitatingBlow» (+0xD74) and `m_decapitateVelocity» (+0xD78). This is direct network field evidence for SiN's grunt decapitation reaction, **not** reconstructed NPC AI.

`DT_PropSinVehicle» exposes `m_bHeadlightIsOn» at original client +0x854. This is a networked vehicle property, not proof the car physics have been ported.

Source SDK 2013 documents the table/property roles and typed encodings, but its modern layout is **not assumed equivalent** to the original SiN x86 binaries. The recovered stride and offsets above come from disassembled 2006 code, not from importing modern SDK C++ structs.

## Relative table-name comparison

Client constructor names: 210 unique. Server `DT_*» **string names**: 183 unique. Literal intersection: **182**. The client has 28 names absent from the server strings, largely old HL2 weapons/entities (including `DT_WeaponRPG» and `DT_WeaponSniperRifle»). The server string set contains `DT_WORLD» whereas the client registers `DT_World». This is string-level evidence only, **not** a proof that a game protocol connection would accept the server/client table configuration.

## Reproducible private analysis

~~~powershell
py research\tools\audit_sin_recv_tables.py --client "C:\SiN\bin\client.dll" --server "C:\SiN\bin\server.dll" --output "sin-recv-contract.json"
py research\tools\audit_sin_send_tables.py --client "C:\SiN\bin\client.dll" --server "C:\SiN\bin\server.dll" --output "sin-send-recv-offset-pairs.json"
~~~

Both analyzers use only the Python standard library and previous Phase 73's `PEImage» parser, reject binary SHA mismatches by default, expose constructor instruction addresses, and record unresolved slots instead of inventing field definitions. Synthetic PE mutation tests exercise constructor targets, property pointers and original client/server offset divergence without distributing game binaries.

## What needs to be implemented next for a real Quest 3 build

1. Expand server SendTable recovery to the other compiler registration patterns (143 names not covered here); only then compare property **types, flags, bit widths, range quantization and proxy callbacks** rather than names/offsets alone.
2. Recover actual SiN player and weapon network/prediction datamaps, including intermediate base tables, weapon state and animation sequences, and map these to C++ classes supported by a legally usable game SDK.
3. Implement a proper game code bridge exporting the SourceVR-compatible **versioned client/server C++ interfaces**, with live constructor, entity lifecycle, predictable state and control callbacks. The old x86 vtable ABI does not automatically match SourceVR ARM64.
4. Integrate original SiN weapon, enemy and scene behavior into the supported host and validate against real game assets/maps. **No playable SiN Quest port exists yet.**

**Evidence boundary:** exact original binary interface/table constructor patterns and property offsets = *binary-verified*; synthetically tested extraction tools = *test-verified for those patterns*; full SiN gameplay/SourceVR ABI compatibility = *not demonstrated*.
