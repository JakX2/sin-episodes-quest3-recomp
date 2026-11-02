# Original SiN Episodes 2006 RTTI, inheritance and vtable recovery

**Scope:** Original user-owned Windows PE32 client.dll and server.dll. This is machine-code metadata recovery, NOT original C++ source or a working ARM64 SourceVR module.

Original SHA-256:
- client.dll: 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d
- server.dll: 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650

## Verified original MSVC RTTI / vtable structures

The scanner recovers and validates TypeDescriptor metadata in .data; CompleteObjectLocators (COL) in .rdata; associated ClassHierarchyDescriptors and BaseClassDescriptors; per-base PMD displacements; and actual vftables immediately following COL pointers. Each slot is a pointer into the original executable .text section. No DLL bytes need to be uploaded to GitHub.

| Recovered evidence | Original client | Original server |
|---|---:|---:|
| RTTI TypeDescriptors | 799 | 1,499 |
| CompleteObjectLocators | 1,428 | 1,750 |
| Classes with valid hierarchies | 797 | 1,485 |
| Vftables | 1,428 | 1,750 |
| Secondary/multiple-inheritance vftables | 631 | 265 |
| Virtual dispatch slots, including inherited pointers | 61,653 | 123,162 |
| Distinct original executable virtual targets | 5,919 | 8,664 |

Distinct targets do NOT account for nonvirtual code, and the sum of slots includes multiple references to inherited virtual functions. These counts are not a count of complete method implementations.

## Primary-vtable comparisons within the SAME 2006 DLL

| Original DLL | Child | Comparison base | Changed shared virtual slots |
|---|---|---|---:|
| server | CNPC_SintekGrunt | CNPC_GruntBase | 1 of 532 |
| server | CNPC_SintekJetpack | CNPC_JetpackGruntBase | 4 of 532 |
| server | CNPC_GruntBase | CAI_BaseHumanoid | 71 of 482 (50 extra trailing derived slots) |
| server | CWeaponMagnum | CBaseHLCombatWeapon | 15 of 316 |
| server | CWeaponAssaultRifle | CHLSelectFireMachineGun | 23 of 319 |
| server | CWeaponScatterGun | CBaseHLCombatWeapon | 18 of 316 |
| server | CPropPhone | CBaseAnimating | 9 of 184 |
| server | CNPC_Quadralex | CAI_BaseNPC | 85 of 475 |
| client | C_WeaponMagnum | C_BaseHLCombatWeapon | 4 of 301 |
| client | C_WeaponAssaultRifle | C_BaseHLCombatWeapon | 4 of 301 |
| client | C_WeaponScatterGun | C_BaseHLCombatWeapon | 4 of 301 |

In the ordinary SinTEK Grunt subclass, the sole difference is virtual slot zero. The original routine at x86 0x22380670 calls 0x223542F0 and conditionally frees the object, consistent with a compiler-generated deleting destructor. **The inherited CNPC_GruntBase code is a higher-value AI reverse-engineering target than the almost empty direct subclass.** This cannot rule out nonvirtual subclass methods.

Jetpack-specific changed primary slots: 0, 25, 26, 52 with corresponding replacement targets 0x223807E0, 0x223806E0, 0x22380690, 0x223807D0. CPropPhone differs at slots 0, 10, 11, 12, 13, 25, 26, 44, 135. The three original client weapon primary vtables differ from their parent only at slots 0, 14, 15, 16. **Method names are intentionally NOT inferred solely from slot numbers.**

## Reproduce with original private files

    py research/tools/audit_sin_rtti_vtables.py --client C:\private\SiN\client.dll --server C:\private\SiN\server.dll --output C:\private\sin-rtti-vftables.json

Generated JSON includes complete vtable slot addresses, class hierarchy arrays with displacement metadata and selected parent/derived pointer differences. The tool refuses unexpected original SHA fingerprints; tests use synthetic PE32 fixtures.

## Remaining reverse-engineering workload

1. Integrate PR #63 original InterfaceReg factories and server entity registrations with RTTI/vtables.
2. Disassemble and name the SiN-specific changed virtual targets; recover machine-code control flow, direct and indirect calls, constants, member offsets, and script/data references. Include nonvirtual methods separately, which vtable analysis misses.
3. Reconstruct network SendProp/RecvProp proxies and original prediction logic from PR #64, not just their string names.
4. Correlate weapon/combat/phone/vehicle script consumers from PR #65, weapon firing and SiN AI schedules/dynamic difficulty against original server code.
5. Independently implement equivalent gameplay modules in compatible ARM64 C++ once real SourceVR game ABI and lifecycle APIs have been validated.

User confirms SiN maps already load in Android/VR HL2. Do not divert the investigation into BSP/map loading, APK packaging or stand-alone synthetic gameplay in place of original client/server reconstruction.

**Limit:** neither the complete original implementation nor a playable port has been reconstructed. The goal is systematic, code-address-grounded progress without implying otherwise.
