# Phase 713: SiN enemy and additional weapon network classes

Both original owner DLLs SHA256 rechecked against existing pins. Fresh constructor disassembly and data pointer resolution, not inferred HL2 class inheritance. Extra subset retains actual client registration order (reverse prepend order); separate extension ABI preserves prior aggregate ABI.

| Class | Direct slots | Server table constructor call | Client call | Parent |
|---|---:|---|---|---|
|CAI_BaseNPC|7|2203efe5|240b1ee3|core.inventory.combat_base|
|CBaseHLBludgeonWeapon|1|222a38c5|242160b9|core.weapons.tables[BaseHL]|
|CNPC_EliteGrunt|1|22355405|2421c069|tables[ExGrunt]|
|CNPC_GruntBase|3|2235bfb8|2421cc1f|tables[ExAI]|
|CNPC_Hologram|3|223612da|2421cecc|tables[ExAI]|
|CWeaponSMG|1|2231d135|24228449|core.weapons.tables[SelectFire]|
|CQuadShockwave|2|2231867e|24229f76|core.inventory.combat_base|
|CWeaponGrenade|1|223159e5|2422adf9|core.weapons.tables[BaseHL]|
|CWeaponChaingun|1|22315505|2422aec9|core.weapons.tables[SelectFire]|
|CWeaponNPCPistol|1|22317d25|2422b099|core.weapons.tables[BaseHL]|
|CWeaponMelee|1|223175e5|2422b169|tables[ExBludgeon]|

Direct non-parent fields (hex offsets):

| Table | Field | Server | Client | Encoding |
|---|---|---|---|---|
| AI_BaseNPC | m_lifeState | 8c | 94 | 3 bits unsigned DWORD |
| AI_BaseNPC | m_bPerformAvoidance | c10 | d48 | 1 bit byte |
| AI_BaseNPC | m_bIsMoving | c11 | d49 | 1 bit byte |
| AI_BaseNPC | m_bFadeCorpse | c12 | d4a | 1 bit byte |
| AI_BaseNPC | m_iDeathPose | 890 | d4c | signed 10 |
| AI_BaseNPC | m_iDeathFrame | 894 | d50 | signed 5 |
| NPC_GruntBase | m_receivedDecapitatingBlow | c44 | d74 | 1 bit byte |
| NPC_GruntBase | m_decapitateVelocity | c48 | d78 | vector NOSCALE (4) |
| NPC_Hologram | m_state | 1014 | d7c | signed 32 |
| NPC_Hologram | m_timeEffectStops | 1018 | d80 | NOSCALE float |
| QuadShockwave | m_vecTargetPos | 798 | d64 | COORD vector (2) |

Server integer factory 2225ebf0 expands -1 bits to size*8. The hologram time wrapper 2227ebd0 calls 2225e8c0 with flags4. Grunt vector explicitly 32 bits + NOSCALE and quad target -1 bits + COORD, both normalize unused fields. Parent pointers confirmed in original initialized data. Core BaseCombatCharacter and weapons reused, no HL2 AI or physics reimplementation.

Separate owned extension builds 11 tables and role-specific class records, pins aggregate against premature destruction, checks callbacks and engine-owned state on teardown. Tests actual paired callback transfers of all 11 scalar/vector fields and hierarchy/list ordering. With core: 46 tables and 26 class records, two distinct lists, not published as retail complete. Client constructors remain borrowed. No gameplay or bitstream claim.
