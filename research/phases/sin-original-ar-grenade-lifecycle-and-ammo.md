# SiN Episodes 2006: Assault Rifle grenade entity lifecycle and ammo paths

**Grounding:** exact owner-supplied Windows 2006 SiN client.dll SHA256 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d and server.dll SHA256 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650. This is original x86 disassembly metadata, not a compiled Quest 3 module. User already loads SiN BSP maps in Android/VR HL2; no map or APK work is part of this milestone.

## Original entity class and virtual function evidence

The original server registers classname **grenade_ar** at x86 0x223C4FC0 with entity factory object at 0x2252D500. The implementation type descriptor identifies **CGrenadeAR** (0x2252D570), with direct inherited base **CBaseGrenade**. Their primary vtables each contain **260** virtual slots. The original CGrenadeAR overrides exactly 6 relative to that parent:

| Virtual index | CGrenadeAR target | CBaseGrenade target | Grounded role |
|---:|---|---|---|
| 0 | 0x22313BF0 | 0x222B1CF0 | Deleting destructor pattern |
| 13 | 0x22313900 | 0x222B1AC0 | Static class metadata return, exact name unresolved |
| 25 | 0x22314B10 | 0x220447C0 | Projectile spawn-style initialization |
| 26 | 0x22313910 | 0x222B1B50 | Model precache call |
| 58 | 0x22313920 | 0x222B1B80 | Delegates via vtable +0x3E8, exact purpose unresolved |
| **250** | **0x22314570** | **0x222B2600** | **Detonate**, tied to original named grenade exports |

**Detonate slot identification is independently anchored to original PE export names:** CGrenadeHomer::Detonate (x86 0x2230F4E0), CGrenadePathfollower::Detonate (0x22310EA0), CGrenadeSpitMutant::Detonate (0x223120B0), and CGrenadeSpitPoison::Detonate (0x22312B70) are named original exports. Each exactly matches virtual index 250 of that corresponding grenade subclass, and the CGrenadeAR virtual at the same index is 0x22314570. This is considerably stronger evidence than assigning a method name from code similarity.

## Separate original weapon-user paths

The Assault Rifle routine at x86 **0x22313C60** obtains its weapon owner and performs a virtual call at index **72** (vtable byte offset +0x120). The original CBasePlayer implementation at 0x22253C30 returns true; the original CBaseCombatCharacter implementation at 0x220017E0 returns false. The original routine also calls MSVC **__RTDynamicCast** at 0x223A5BAB, using CBaseCombatCharacter as source type and CNPC_GruntBase as target type. The cast result is stored in EBX and tested by the original branch at 0x22313DE7.

When the owner **is a grunt-base NPC** (successful cast), the branch selects the original **1000.0** float multiplier for a direction vector. In the ordinary player path, the branch selects **2000.0**. These are multipliers read from original .rdata at 0x223E97F0 and 0x224771F0, respectively. The *physical units*, exact origin vector semantics and all NPC-side differences remain to establish.

### Actual player-side secondary ammo

The original weapon's **m_iSecondaryAmmoType** index is read from weapon+0x4E8 at 0x22313CB1, consistent with the separately recovered server SendTable contract in PR #64.

The owner ammo-count routine at **0x222AA770** reads a valid indexed count from owner+0x64C+4*ammo_index, with special handling for invalid / infinite ammo indices. At original 0x22313F2E, the weapon pushes the secondary ammo index and **quantity 1**, then at 0x22313F39 calls **0x222AA670**. The latter subtracts that quantity and conditionally writes the indexed ammo count back. Thus **the normal player-fired alternate grenade uses one unit of secondary ammo** when the ammo mapping is finite/valid.

The grunt-base path does not take this exact player-side ammo-consumption branch. That does *not* establish unlimited NPC ammo in every possible AI situation.

The actual grenade entity creation was separately proven in PR #67: original 0x22313E2A calls 0x22146C40, which reaches CreateEntityByName at 0x22266B00 and the original entity registry getter at 0x2228B110. This is not the earlier PrecacheOther call.

## CGrenadeAR spawn, touch, think and detonation entrypoints

The original code at **0x22314B10** initializes the new projectile and configures the model using the original string at 0x22477208:

**models/weapons/grenades/ar_grenade.mdl**

It installs a touch callback pointing to **0x223142F0**, a recurring think callback **0x22314360**, and schedules the next think relative to the game time using original single-precision **0.1** from 0x224771F8.

Original object field writes also include **+0x7AC=100.0**, **+0x798=250.0**, and **+0x7C4=100.0** in the spawn-style initialization; the code reads these fields later in the slot-250 detonation path. These are *exact raw original field values and offsets*. Do **not** permanently label them damage/radius/restitution parameters until their accessor names and later uses are fully recovered.

The contact callback at 0x223142F0 checks a collision/trace record and conditionally invokes virtual slot 250 through vtable byte offset +0x3E8. The think callback at 0x22314360 updates game-time scheduling and conditionally invokes that same slot. Combined with the named export mapping, this is real evidence for more than one possible **detonation trigger**, although the precise conditions remain to be reconstructed.

The original detonation body **0x22314570** is a substantial physics/effect/damage-oriented routine. Its exact object dependencies, radius/damage fields and branching need a separate full CFG pass before gameplay-equivalent C++ can be written.

## Client network observation

The exact string **DT_GrenadeAR** is absent from both original DLL byte arrays in this build. That does **not** prove the projectile is unnetworked: its base class may provide the send/receive table used for this entity. The client DLL also lacks the literal class name CGrenadeAR. A client/server wire protocol and presentation comparison still needs separate work.

## Reproduction

With the exact original private DLLs, run:

    py research/tools/audit_sin_ar_grenade_lifecycle.py --server "C:\SiN\bin\server.dll" --client "C:\SiN\bin\client.dll" --output "sin-ar-grenade-lifecycle.json"

This SHA-pinned scanner verifies exact x86 byte patterns, compiled original factory/RTTI/vtables, PE export Detonate index identity, two owner-type paths, one-unit ammo decrease and literal model/timer/float evidence. Synthetic tests reject forged opcodes, type descriptors, field initialization and original hash mismatches. Commercial original DLL bytes remain private.

## Why this matters for the real Quest port

We now have a concrete original **entity factory + projectile lifecycle + player/NPC firing distinction + ammo consumption** contract, not just resource/weapon script names. A playable ARM64 SourceVR module still requires independently implementing CGrenadeAR and its host callbacks, damage/collision/physics state, weapon prediction, script bindings and the host engine ABI. Original vtable slot numbers and memory offsets cannot be transplanted into ARM64 as-is.
