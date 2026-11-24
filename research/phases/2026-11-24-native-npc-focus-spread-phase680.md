# Phase 680: native NPC attack-spread focus behavior

Authenticated original server.dll SHA256 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650.
RTTI-backed primary vtable analysis identifies slot243 (+0x3cc):
- CBaseCombatCharacter vtable 0x22459e5c -> 0x222a4b60, 39 bytes,
  SHA256 ee5c80c2d7171083a1f8fb796065116eba504c81c50e4ee2208569863e8c108a.
  Calls CSE1Rules slot61 (+0xf4) at 0x222a4b7b through game rules
  global 0x22572914; this is the original script spread lookup.
- CAI_BaseNPC vtable 0x223da7ac -> 0x22031120, 253 bytes,
  SHA256 63ae1a2473a601fb72b50629972caa9c2ce5698d51218b3c8608cc02b084fad7.
  Same method at CNPC_GruntBase 0x224954a4, CNPC_MobGrunt
  0x2249aaa4, CNPC_EliteGrunt 0x2248df04, CNPC_DruglabJetpack
  0x2248c514, CNPC_EliteJetpack 0x2248e83c, CNPC_Jessica 0x22497a14,
  CNPC_SintekGrunt 0x224a416c and CNPC_SintekJetpack 0x224a4a4c.
- CHL2_Player vtable 0x224aec3c -> 0x2239d560, 27 bytes,
  SHA256 6793c30b597ddaa29e6fc5e7266b480abad4b2d66c2d6661932a638148aa6e1f.
  Only delegates to base method without focus penalty.

The NPC override calls the base, then looks up target memory using
this+0x984 and 0x2206b1a0 with false danger-memory fallback. The owner
SDK2004 ai_memory.h and ai_basenpc.cpp independently identify the
observed memory +0x28 float as timeValidEnemy (not last-seen time).
Original gpGlobals+0xc gives curtime. ConVar parent globals 0x22547488
and 0x22547440 use cached float +0x28. Their original constructor
literals establish ai_spread_cone_focus_time default 0.6 and
ai_spread_defocused_cone_multiplier default 3.0 respectively.

Original finite branch: absent memory returns base; focus <=0 skips;
elapsed=max(0,curtime-timeValidEnemy); elapsed >=focus skips; cone<=1
skips; otherwise all three vector components are multiplied by
1 + (cone-1)*(1-elapsed/focus). No clamp to 1. This is applied after
the already difficulty-scaled rule vector. The port uses double
intermediates, f32 elapsed and output, and refuses nonfinite relevant
inputs or overflow; it does not claim arbitrary x87 exception parity.

The new portable native module API accepts live resolved class strings,
target-memory observations and tuning. Win32 engine adapter reads the
two original VEngineCvar002 cached values through checked parent roots,
only on branches that need them. No automatic memory reconstruction,
foreign object mutation, native NPC slot installation or retail firing
execution claimed. Regression checks original branch ordering, zero
and negative elapsed, halfway focus, exact end, missing memory,
difficulty changes, bypassed multiplier, aliases, wrong root, and
missing cvars. Existing engine skill lookup shares the verified
typed-root helper without changing its behavior.
