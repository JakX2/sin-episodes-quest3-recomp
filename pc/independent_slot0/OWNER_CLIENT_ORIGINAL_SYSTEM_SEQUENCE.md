# Startup reconstruction queue: all 12 original direct systems

The verified original 2006 client Init registration path at
0x240FE3F6 through 0x240FE486 now has source-backed names,
registered subobject pointer identities, virtual slot-zero Init VAs,
and implementation status for each of its twelve objects. See
client_original_gamesystem_sequence.h.

The table is crosschecked against the twelve exact original CALL rel32
anchors in client_auto_gamesystem_init_core.h and the two SHA-attested
RTTI auditors (raw-PE static owners, and BSS-constructor-owned vptrs).

Full Init method behavior reconstructed for **7 of 12**:
CGameStringPool, CSoundEmitterSystem, CClientLeafSystem secondary
+4, CDetailObjectSystem, CViewportClientSystem,
CClientEffectPrecacheSystem and CClientThinkList.

The **next five original Init methods**, in exact startup order:
- CClientShadowMgr: 0x24118880
- C_StatsSystem: 0x241FCAA0
- C_DynamicDifficulty: 0x241FAC90
- C_SoundscapeSystem: 0x240F4160
- CPerfVisualBenchmark: 0x24182800

Actual game boot also requires faithful object construction,
related engine interfaces, HUD texture conversion and post-Init
follow-up calls. Virtual method identity is not equivalent to a
working implementation. Never substitute invented true-return stubs
for these five pending original methods to claim a completed client Init.
