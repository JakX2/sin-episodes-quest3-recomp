# Phase 681: original NPC spread bias and composed firing parameters

Original server.dll SHA256 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650.
CBaseCombatCharacter slot244 (+0x3d0) is 0x222a4b90 (9 bytes,
SHA256 010239550f56b03d633cc0c02d383b4b70be7c750b88d949d1ed58c2624979fe)
and returns the literal 1.0f at 0x223d22f8.
CAI_BaseNPC and inherited SiN NPC vtables identified in Phase680
have slot244 0x22031060, 192 bytes, SHA256
e979af212ae53377eddd54e3c8e85a9674149771117c2149688935f59be30def.
CHL2_Player uses base slot244.

The original override calls the base, queries enemy memory as in
Phase680, reads ai_shot_bias parent 0x225473f8 cached float +0x28,
and uses that value when different from 1.0. With memory, it reads
ai_spread_pattern_focus_time parent 0x225474d0 cached float +0x28.
Original constructors 0x223b65b0 and 0x223b6640 authenticate
default strings 1.0 and 0.8 respectively.

For finite normal inputs with positive focus time, original
valid-elapsed = max(0,curtime-memory+0x28). Reacquired elapsed is
curtime-memory+0x24, stored f32 and NOT clamped. If valid-elapsed
< focus, scale bias by valid-elapsed/focus. Otherwise if reacquired
elapsed < focus, scale by reacquired/focus (even if negative).
Otherwise leave bias unchanged. SDK2004 AI_EnemyInfo_t names
the two fields timeValidEnemy and timeLastReacquired. No claim
that all x87 rounding or exceptional values are reproduced.

Original shot call sites 0x22036b2e/0x22036b53 and
0x220371af/0x220371d1 invoke slot244 bias first, then slot243
spread, and pass both to 0x22033280. New loaded-module NpcAimApi
and Win32 SiN_QueryNpcAimFromEngine compose the two recovered
stages in that order, using the single port-owned campaign and
script spread store. The host provides a stable live resolved
memory observation. Win32 reads only the required ConVar roots.
No raw entity reads, original vtable replacement or projectile
generation; actual engine host still must consume the two outputs.

Portable and Windows loaded-module tests check bias defaults,
overrides, new-target/reacquire priority, exact threshold, negative
reacquire, disabled focus, missing memory, invalid values, and
ordered native lookup (bias, pattern focus, cone focus, multiplier).
Phase680 Windows near-macro fixture correction is included in base.
