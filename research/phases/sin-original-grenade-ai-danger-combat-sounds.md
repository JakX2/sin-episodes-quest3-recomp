# Original SiN Episodes 2006: Assault Rifle grenade AI sound events

**Verified primary source:** user-held, unmodified Windows x86 2006 \`server.dll\`, SHA-256 \`568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650\`. Client string cross-check uses original \`client.dll\`, SHA-256 \`6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d\`. This report is about actual server machine code, not maps, APKs, reconstructed source, or a playable ARM64 module.

## New finding: two AI hearing events, not one client-side explosion sound

The original grenade has **two distinct AI-sound mechanisms**:

1. \`CGrenadeAR\` flight/think callback **0x22314360** calls original **0x221B8FA0** at **0x22314559**, supplying exactly **0x08** as its sound classification, a sound origin derived from \`grenade origin + velocity*0.5\`, a changing integer radius, **0.2-second** lifetime, grenade entity as owner and sound channel 1. An original-era Valve \`soundent.h\` identifies \`0x08 = SOUND_DANGER\`, and \`soundent.cpp\` independently documents \`CSoundEnt::InsertSound(int, const Vector&, int, float, CBaseEntity*, int, ...)\`. The compiled routine at 0x221B8FA0 accesses the global sound-event manager at 0x2256E844, allocates/reuses sound-list entries and records origin/owner/type/radius/expiry, supporting this identification from actual original SiN behavior.

2. The shared original server \`RadiusDamage\`-style wrapper **0x222A5E10**, invoked by the grenade's actual detonation at 0x223148FB, first dispatches its game-rules damage virtual call at **0x222A5E39**. It then tests damage type bit **0x40 (DMG_BLAST)** at **0x222A5E3F**. Only with that bit set does it compute an AI event radius using **max(128.0, explosion radius * 1.5)** and call the *same* original sound-event routine at **0x222A5EC4** with combined type **0x02000001** and duration **1.0 second**. Original-era \`soundent.h\` identifies the bitmask as \`SOUND_COMBAT | SOUND_CONTEXT_EXPLOSION\`. It also passes the damage inflictor as event owner, where available.

Source corroboration, independent of these original machine-code findings:
- Original-era \`soundent.h\` enums and \`CSoundEnt::InsertSound\` signature: https://workshop.perforce.com/files/guest/knut_wikstrom/ValveSDKCodedlls/soundent.h
- Original-era \`soundent.cpp\` implementation: https://workshop.perforce.com/files/guest/knut_wikstrom/ValveSDKCodedlls/soundent.cpp
- Valve-era \`basecombatcharacter.cpp\` RadiusDamage wrapper illustrates the same max(128, radius*1.5) combat-event calculation: https://workshop.perforce.com/files/guest/knut_wikstrom/ValveSDKCodedlls/basecombatcharacter.cpp

**SiN-specific difference:** The historical Source SDK example sends the explosion combat AI sound with a **0.25-second** lifetime. The user's original SiN \`server.dll\` unambiguously pushes float **1.0**, instruction address **0x222A5EA4**. Never silently copy the SDK's 0.25-second value into the SiN port.

## CGrenadeAR state-field recovery

Previous PR #68 and PR #69 left original object float field **+0x7C4** unclassified; it was initialized to **100.0**, compared with **300.0** and increased in **15.0** increments. This research pins its purpose:

- At original **0x22314CED**, \`CGrenadeAR\` initializes \`+0x7C4 = 100.0f\`.
- At **0x2231444A/0x22314450**, the think routine compares that member to **300.0f**; **0x22314463** adds original double **15.0** under the branch condition.
- At **0x22314537** this same member is converted to integer and passed as the **iVolume / AI listening radius** argument to the common sound event routine.
- The callback schedules its next tick at current game time + original float **0.05 seconds** (0x2231436B / 0x223D9CB0).
- It uses the projectile origin plus half its velocity as the AI warning position (original velocity scale float **0.5**, 0x2231448A / 0x223D9778).

**Important:** This is an AI danger sound radius, **not the 250.0 starting radius used by grenade blast damage** (\`+0x798\`) and not an audible wav/voice sound. The comparison to 300 is not necessarily a hard mathematical clamp: the member may temporarily pass 300 by one increment before the next comparison.

## Original value table

| Behavior / parameter | Original compiled value |
|---|---:|
| Flight event type | \`SOUND_DANGER = 0x8\` |
| Flight AI-sound radius initial | 100 |
| Flight radius increment | +15 per gated think |
| Flight radius comparator | 300 |
| Flight think cadence | 0.05 seconds |
| Flight event lifetime | 0.2 seconds |
| Flight origin advance | \`velocity × 0.5\` |
| Flight sound channel argument | 1 |
| Blast damage type bit | \`DMG_BLAST = 0x40\` |
| Blast event type | \`SOUND_COMBAT \| SOUND_CONTEXT_EXPLOSION = 0x02000001\` |
| Blast AI event radius | \`max(128, damage_radius × 1.5)\` |
| Blast AI event duration | **1.0 seconds** |
| Blast sound channel argument | 0 |

If the grenade's separately confirmed initial blast radius is 250, the initial radius expression yields **375 source distance units**. That is a derived consequence of the formula, **not an additional hardcoded constant**; damage radius changes would change the AI sound radius accordingly.

## Network/client boundary

The original \`client.dll\` contains the literal \`DT_BaseGrenade\`; neither DLL contains the literal \`DT_GrenadeAR\`. This is **string-level evidence only**: it does not identify the inherited send table actually used for this projectile, specific receive properties, the recipient filter of explosion temporary effects, or a VR rendering path.

**These two CSoundEnt events are exclusively server-side AI stimuli.** They are NOT themselves the audible explosion or the visual explosion messages that a Quest client renders. Separate original-server-to-client networking and sound/effect tracing is still needed.

## Repeat the exact original evidence checks

    py research/tools/audit_sin_grenade_ai_sounds.py --server "C:\SiN\bin\server.dll" --client "C:\SiN\bin\client.dll" --output "grenade-ai-sounds.json"

This read-only SHA-pinned stdlib tool verifies **20 exact original x86 instruction sites, both real x86 CALL rel32 edges, 3 original float constant sites and 3 double constant sites**, and reports unresolved protocol uncertainties. Synthetic original-free PE32 regression tests exercise modified opcodes, call targets, bitmasks, radius constants and the SiN-specific one-second duration.

## ARM64 port implementation implications

1. In the future real SourceVR-compatible server game module, schedule and publish AI **SOUND_DANGER** events during active grenade flight with the original time/position/radius behavior.
2. When applying \`DMG_BLAST\` radius damage, also insert the separate **SOUND_COMBAT | SOUND_CONTEXT_EXPLOSION** event with source-specific **1.0-second** duration; use the actual server-side radius calculation and owner.
3. Do **not** couple either AI event to client audible sound volume, explosion particle visuals or VR camera shaking. These are different systems.
4. Map original \`CGrenadeAR\` physics/touch and detonation guards, real damage rules and skill modifiers; separately identify emitted client effects from the original binary's effect dispatch.
5. Retain the original user's proprietary DLLs/scripts privately; no actual SiN 2006 binary bytes or source are committed to GitHub.

**State of project:** script + original DLL research is progressing, but a complete SourceVR Android ARM64 \`client.so\` and \`server.so\` implementing SiN gameplay still does not exist.
