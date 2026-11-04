# SiN Episodes: Emergence — Jetpack Grunts & Security Bots (Deep Dive 15)

**Research date:** 2026-11-04. **Scope:** original 2006 x86 SiN Episodes `server.dll`, `client.dll`, and five user-captured console logs from SiN maps running under HL2. **Status:** SHA-pinned static reverse engineering, not original-game runtime testing and not Quest 3 gameplay integration.

Original server SHA-256: `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`.
Original client SHA-256: `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.

## 1. The missing classes are genuinely different types

- `npc_druglab_jetpack` has a compiled factory registration at `0x223C68B7` for `CNPC_DruglabJetpack`. RTTI and primary-vtable comparisons place it below `CNPC_JetpackGruntBase`, itself below `CNPC_GruntBase`.
- `npc_securitybot` has a compiled factory registration at `0x223C7707` for `CNPC_SecurityBot`, derived from `CNPC_CBot`. The parent is associated with physics-flying-bot AI; the precise complete parent hierarchy deserves separate verification.
- In the original primary vtables, `CNPC_DruglabJetpack` differs from `CNPC_JetpackGruntBase` in **5** slots; `CNPC_JetpackGruntBase` differs from `CNPC_GruntBase` in **31**; `CNPC_SecurityBot` differs from `CNPC_CBot` in **16**. These are pointer-difference counts, not counts of entirely novel functions.

In the five logs, `npc_druglab_jetpack` fails **29** entity creations and `npc_securitybot` fails **16**. Repeat map loads may repeat the same entity.

## 2. Jetpack spawn and companion entity

The derived jetpack spawn method `0x22353D60` invokes shared base spawn `0x22367F20` at call `0x22353D8A`. The base routine references `models/combine_soldier.mdl` as one fallback/default model and `models/weapons/jetpack.mdl` as a jetpack-specific model. Neither filename establishes the final model chosen by every map placement.

Inside base spawn, `0x22367FF4` passes the compiled `prop_dynamic` classname to the original entity creation helper (`0x22367FF9` → `0x22144DA0`). The returned entity handle is stored at `CNPC_JetpackGruntBase+0x1028` (`0x22368011`); failure writes `-1` (`0x22368019`). Later code validates and uses that handle. This supports an attached jetpack/prop sub-entity lifecycle, not a mere bodygroup texture.

The base datamap declares **26** records but only one is fully readable without replaying its static constructor: `m_pEngineSound` at `+0x1008`. Do not invent the remaining 25 symbolic field names. Other frequently accessed slots include `+0x1024` and `+0x102C`, but their complete semantics require continued tracing.

## 3. Security bot's model, attachment, and animation event paths

The security bot precache method at `0x2237E940` references:
- `models/characters/SecurityDrone_02/securitydrone_02.mdl`
- `models/characters/SecurityDrone_02/gibs_01.mdl`
- `models/characters/SecurityDrone_02/gibs_02.mdl`

Its original datamap declares and resolves exactly two fields, `m_fAlertTime` at `+0xD14` and `m_fWarningTime` at `+0xD24`. In spawn (`0x2237E990`), both are initialized to the current game time from the global timing object.

Method `0x2237EAE0` resolves model attachment IDs for `gun_right` and `gun_left`, stores them at `+0xD18` and `+0xD1C`, and clears the selector at `+0xD20`. In animation-event dispatch `0x2237F280`, event **`0x43`** increments the selector and wraps it after 1, then indexes those two attachment IDs. This establishes alternating attachment selection, **not** an independently measured alternating firing cadence.

Event **`0x95`** enters a different branch, generating a random float bounded by **5.0 and 6.0**, adding game time, and storing the resulting deadline at `m_fWarningTime` (`0x2237F2D6`). It also invokes a sound helper with the compiled `NPC_CScanner.PreShoot` name. The symbolic original names of numeric events and precise game-facing alarm behavior remain unproven.

The schedule selector at `0x2237E9F0` checks condition ID `20`, then compares current time to `+0xD24` and returns numeric schedules `0x5A` or `0x5B` on its special branches. Assigning original symbolic schedule names to those IDs requires additional decoding.

## 4. Reproduction boundaries and next questions

The local read-only verifier `sin_jetpack_securitybot_audit_15.py` passed **44 address-, hash-, and log-anchored checks** before publication. The original binary and log data stay outside this repository. No game code, Quest integration, or enemy behavior was implemented.

**Open questions:** decode all statically constructed jetpack datamap fields; identify sub-entity parent/attachment semantics; trace jetpack flight schedules and attack control; recover security-bot gun projectiles/damage and condition-to-schedule names; compare with level-specific map entity keyvalues.

Do not treat original DLL factory existence as proof that a stock HL2 runtime can instantiate these entities. This chapter is an evidence dossier, not complete clean-room C++ replacement code.
