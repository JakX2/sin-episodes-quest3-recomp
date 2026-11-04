# SiN Episodes: Emergence (2006) | Original client/server DLL static inventory

Binary-only, read-only evidence from the user-provided `bin.zip`. This document is an inventory of compiled RTTI and `CEntityFactory` type specializations, not recovered original C++ source and not proof that every class occurs in campaign gameplay.

## Binary identity

| File | Bytes | SHA-256 | Format |
|---|---:|---|---|
| `client.dll` | 3,592,192 | `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d` | PE32 / Windows x86 |
| `server.dll` | 6,049,792 | `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650` | PE32 / Windows x86 |

## RTTI overview

| Measure | client.dll | server.dll |
|---|---:|---:|
| Distinct decorated `.?AV` / `.?AU` RTTI names in scan | 799 | 1499 |
| Simple `.?AVClass@@` names | 669 | 938 |
| Simple `CEntityFactory<Class>` RTTI specializations | 0 | 470 |

Type names are observed in the binaries. Counts do not correspond one-to-one to entities, mechanics, or methods. Template type names are deliberately excluded from the `simple` count.

## 1. Server NPC catalog

41 distinct simple `CNPC_*` RTTI class names were located; 36 also have distinct `CEntityFactory` template RTTI specializations. The other five may be shared bases and do not imply missing/disabled entities.

| Server class | First ancestor in RTTI hierarchy | Factory-specialization RTTI | RTTI TypeDescriptor VA |
|---|---|:---:|---|
| `CNPC_APCDriver` | `CNPC_VehicleDriver` | yes | `0x2253238c` |
| `CNPC_AdBot` | `CNPC_CBot` | yes | `0x22532204` |
| `CNPC_AttackHelicopter` | `CBaseHelicopter` | yes | `0x22532880` |
| `CNPC_BaseMutant` | `CAI_BaseNPC` | no | `0x225372bc` |
| `CNPC_Bullseye` | `CAI_BaseNPC` | yes | `0x224dbeac` |
| `CNPC_CBot` | `CAI_BasePhysicsFlyingBot` | no | `0x225321ec` |
| `CNPC_CeilingTurret` | `CAI_BaseNPC` | yes | `0x22537e54` |
| `CNPC_Citizen` | `CNPC_PlayerCompanion` | yes | `0x22533ba8` |
| `CNPC_Count` | `CAI_BaseActor` | yes | `0x22533cb4` |
| `CNPC_CraneDriver` | `CNPC_VehicleDriver` | yes | `0x22534008` |
| `CNPC_Crow` | `CAI_BaseNPC` | yes | `0x2253445c` |
| `CNPC_DruglabJetpack` | `CNPC_JetpackGruntBase` | yes | `0x225344ec` |
| `CNPC_DruglabWorker` | `CNPC_Citizen` | yes | `0x22534650` |
| `CNPC_EliteGrunt` | `CNPC_GruntBase` | yes | `0x2253477c` |
| `CNPC_EliteJetpack` | `CNPC_JetpackGruntBase` | yes | `0x225347d0` |
| `CNPC_EnemyFinder` | `CAI_BaseNPC` | yes | `0x22534858` |
| `CNPC_FloorTurret` | `CAI_BaseNPC` | yes | `0x225383a0` |
| `CNPC_FriendlyDriver` | `CNPC_VehicleDriver` | yes | `0x22534b9c` |
| `CNPC_Furniture` | `CAI_BaseNPC` | yes | `0x224f9b04` |
| `CNPC_GenericActor` | `CAI_BaseActor` | yes | `0x22534d84` |
| `CNPC_GroundTurret` | `CAI_BaseNPC` | yes | `0x225392b0` |
| `CNPC_GruntBase` | `CAI_BaseHumanoid` | no | `0x2252d5b0` |
| `CNPC_HardcorpsHelicopter` | `CNPC_AttackHelicopter` | yes | `0x22535738` |
| `CNPC_Hologram` | `CNPC_GenericActor` | yes | `0x225358d4` |
| `CNPC_JC` | `CAI_BaseActor` | yes | `0x2253591c` |
| `CNPC_Jessica` | `CNPC_PlayerCompanion` | yes | `0x22535b0c` |
| `CNPC_JetpackGruntBase` | `CNPC_GruntBase` | no | `0x225344c8` |
| `CNPC_Launcher` | `CAI_BaseNPC` | yes | `0x22536118` |
| `CNPC_MobGrunt` | `CNPC_GruntBase` | yes | `0x2253681c` |
| `CNPC_Pigeon` | `CNPC_Crow` | yes | `0x22534474` |
| `CNPC_PlayerCompanion` | `CAI_PlayerAlly` | no | `0x22533b84` |
| `CNPC_Quadralex` | `?$CAI_BlendingHost@V?$CAI_BehaviorHost@VCAI_BlendedNPC@@@@` | yes | `0x2253726c` |
| `CNPC_RecombinantMajor` | `CNPC_BaseMutant` | yes | `0x22537720` |
| `CNPC_RecombinantMinor` | `CNPC_BaseMutant` | yes | `0x22537abc` |
| `CNPC_Seagull` | `CNPC_Crow` | yes | `0x225307e4` |
| `CNPC_SecurityBot` | `CNPC_CBot` | yes | `0x22537c74` |
| `CNPC_SinVehicleDriver` | `CNPC_VehicleDriver` | yes | `0x22537e14` |
| `CNPC_SintekGrunt` | `CNPC_GruntBase` | yes | `0x22537cc8` |
| `CNPC_SintekHelicopter` | `CNPC_AttackHelicopter` | yes | `0x22537d8c` |
| `CNPC_SintekJetpack` | `CNPC_JetpackGruntBase` | yes | `0x22537d28` |
| `CNPC_VehicleDriver` | `CAI_BaseNPC` | yes | `0x2251d598` |

### Particularly informative verified relationships

- `CNPC_RecombinantMajor` and `CNPC_RecombinantMinor` both inherit from `CNPC_BaseMutant`.
- `CNPC_AdBot` and `CNPC_SecurityBot` both inherit from `CNPC_CBot`, a class whose next ancestor is `CAI_BasePhysicsFlyingBot`.
- `CNPC_Jessica` inherits from `CNPC_PlayerCompanion`, whereas `CNPC_JC` inherits from `CAI_BaseActor`.
- `CNPC_DruglabJetpack`, `CNPC_EliteJetpack` and `CNPC_SintekJetpack` share `CNPC_JetpackGruntBase`.
- `CNPC_Quadralex` follows a templated `CAI_BlendingHost<CAI_BehaviorHost<CAI_BlendedNPC>>` ancestry, separate from the `CNPC_BaseMutant` branch.

The `First ancestor` column is the first base TypeDescriptor referenced by the MSVC class-hierarchy array after the class itself. It describes RTTI ancestry, not necessarily a complete multiple-inheritance graph or runtime decisions.

## 2. Selected other server classes

| Class | First RTTI ancestor | Factory RTTI | TypeDescriptor VA |
|---|---|:---:|---|
| `CSE1Rules` | `CSingleplayRules` | no | `0x225236a8` |
| `CDynamicDifficulty` | `unresolved` | no | `0x22521d48` |
| `CTriggerDynamicDifficulty` | `CBaseTrigger` | yes | `0x2250d3fc` |
| `CInfoChallenge` | `CBaseEntity` | yes | `0x22527e64` |
| `CAntigenDispenser` | `CBaseAnimating` | yes | `0x225285e0` |
| `CAntigenCanister` | `CPropContainer` | yes | `0x2252a234` |
| `CPropPhone` | `CBaseAnimating` | yes | `0x2252a7d4` |
| `CPropSinVehicle` | `CPropVehicleDriveable` | yes | `0x22531544` |
| `CWeaponAssaultRifle` | `CHLSelectFireMachineGun` | yes | `0x2252d58c` |
| `CWeaponMagnum` | `CBaseHLCombatWeapon` | yes | `0x2252ddf4` |
| `CWeaponScatterGun` | `CBaseHLCombatWeapon` | yes | `0x2252e8a8` |
| `CMutagenBarrel` | `CPropContainer` | yes | `0x2252a2d4` |
| `CExplodingBarrel` | `CPropContainer` | yes | `0x2252a2b4` |
| `CPoisonGas` | `CBaseEntity` | yes | `0x2252a01c` |

`CDynamicDifficulty` has a TypeDescriptor name, but this bounded hierarchy scan did not resolve a complete class hierarchy record for it. Do not infer it has no parents.

## 3. Client-side classes

The client binary includes 5 `C_NPC_*` simple class records, 28 `C_Weapon*` class records and 33 `CHud*` class records. These include many inherited/retained Source classes and **are not** confirmed campaign-used classes.

| Client class | First RTTI ancestor | TypeDescriptor VA |
|---|---|---|
| `CHudAmmo` | `CHudNumericDisplay` | `0x24320ce4` |
| `CHudAnimationInfo` | `CHudElement` | `0x242fbdd4` |
| `CHudArena` | `CHudNumericDisplay` | `0x24320d1c` |
| `CHudBaseTimer` | `CHudNumericDisplay` | `0x242fbe9c` |
| `CHudBitmapNumericDisplay` | `Panel@vgui` | `0x242fbeb8` |
| `CHudBreathTimer` | `CHudElement` | `0x24320d34` |
| `CHudCamera` | `Panel@vgui` | `0x242fbee0` |
| `CHudChat` | `CBaseHudChat` | `0x242fbefc` |
| `CHudCloseCaption` | `CHudElement` | `0x242fbf14` |
| `CHudCredits` | `CHudElement` | `0x24320d54` |
| `CHudCrosshair` | `CHudElement` | `0x24320d70` |
| `CHudDamageIndicator` | `CHudElement` | `0x24320e30` |
| `CHudElement` | `IGameEventListener2` | `0x242fbd14` |
| `CHudGrenades` | `CHudElement` | `0x24320e54` |
| `CHudHealth` | `CHudElement` | `0x24320e8c` |
| `CHudHintDisplay` | `Panel@vgui` | `0x242fbf34` |
| `CHudHistoryResource` | `CHudElement` | `0x242fbd30` |
| `CHudLabel` | `Label@vgui` | `0x2431db9c` |
| `CHudMenu` | `CHudElement` | `0x242fc6dc` |
| `CHudMessage` | `CHudElement` | `0x242fc70c` |
| `CHudNumericDisplay` | `Label@vgui` | `0x242fbe78` |
| `CHudPoisonTimer` | `CHudElement` | `0x24320ea8` |
| `CHudPoliceRadio` | `CHudElement` | `0x24320ec8` |
| `CHudSecondaryAmmo` | `CHudNumericDisplay` | `0x24320cfc` |
| `CHudStatTitle` | `CHudElement` | `0x24320e70` |
| `CHudTextMessage` | `IHudTextMessage` | `0x24300ff8` |
| `CHudTextureHandleProperty` | `IPanelAnimationPropertyConverter@vgui` | `0x2431dda4` |
| `CHudTrain` | `CHudElement` | `0x24301090` |
| `CHudVehicle` | `CHudElement` | `0x242fbf70` |
| `CHudViewport` | `CBaseViewport` | `0x24320ad4` |
| `CHudWeapon` | `CHudElement` | `0x242fbf8c` |
| `CHudWeaponSelection` | `CBaseHudWeaponSelection` | `0x24320ee8` |
| `CHudZoom` | `Panel@vgui` | `0x24320f0c` |
| `C_DynamicDifficulty` | `CBaseGameSystem` | `0x2431e270` |
| `C_NPC_EliteGrunt` | `C_NPC_GruntBase` | `0x2431fcb8` |
| `C_NPC_GruntBase` | `C_AI_BaseNPC` | `0x2431fc98` |
| `C_NPC_Hologram` | `C_AI_BaseNPC` | `0x2431fd40` |
| `C_NPC_Hydra` | `C_AI_BaseNPC` | `0x2431fd7c` |
| `C_NPC_Manhack` | `C_AI_BaseNPC` | `0x2431fdb4` |
| `C_PropPhone` | `C_BaseAnimating` | `0x2431ff18` |
| `C_PropSinVehicle` | `C_PropVehicleDriveable` | `0x243202cc` |
| `C_SE1Rules` | `C_SingleplayRules` | `0x2431f5e0` |

### Caution about client-only records

`C_NPC_Hydra` occurs as RTTI in the client scan, but there is no corresponding `CNPC_Hydra` simple RTTI name in the server scan. That discrepancy is **not proof** Hydra is playable or present in the SiN Episodes campaign. It may reflect retained Source code or an absent/stripped class; further executable references and registration evidence would be required.

## 4. Evidence technique

1. Verified both PE32 images by file size, SHA-256 and PE section metadata.
2. Located null-terminated MSVC decorated RTTI type-name strings (`.?AV...@@`, `.?AU...@@`).
3. For each selected simple class, resolved the TypeDescriptor address (class-name address minus 8), then matched the `RTTICompleteObjectLocator` (signature, offset, cdOffset, type pointer, class-hierarchy pointer).
4. Followed `RTTIClassHierarchyDescriptor` to the ordered base-class TypeDescriptors. Every server `CNPC_*` record in the table has a resolved ancestry array.
5. Separately scanned for `CEntityFactory<Class>` RTTI specializations. This demonstrates compiled factory type information, not a verified map classname nor a successful game spawn.

## 5. Recommended next read-only binary investigations

- **Enemy decisions:** identify complete vtables and disassemble `CNPC_BaseMutant`, `CNPC_RecombinantMajor/Minor`, `CNPC_Quadralex` schedules, condition tests and per-frame AI functions.
- **Companions:** inspect `CNPC_Jessica` and `CNPC_JC` input handling, state and voice/script hooks.
- **Gameplay state:** reconstruct `CSE1Rules`, `CDynamicDifficulty` and `CInfoChallenge` call relationships, with address-anchored evidence.
- **Client UI:** trace `CHudArena`, `CHudPoliceRadio`, `CHudPoisonTimer`, `CHudGrenades` and their animation/message dependencies.
- **Network contracts:** match genuine server SendTables to client RecvTables; distinguish fields supported by the original code from dead inherited declarations.

This inventory intentionally does not involve SourceVR, Quest 3, ARM64 ports or any bridge implementation. It contains no proprietary DLL bytes. The companion JSON provides the collected TypeDescriptor and locator addresses plus full observed ancestry arrays.