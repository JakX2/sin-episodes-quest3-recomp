# Phase 663: Opt-in static skill levels replacing SiN adaptive combat tuning

## Decision

The owner wants a **Half-Life-style selectable Easy / Normal / Hard**
difficulty instead of SiN Episodes' adaptive *Personal Challenge
System*. This phase implements an independent, reusable **fixed
difficulty gameplay policy**. It does **not** attempt to remove
original gameplay and entity interfaces, nor does it prove native
Windows/Quest game runtime loading.

This work is stacked after Phase662, but it deliberately changes
course from generic Source soundscapes/physics/props into missing
gameplay compatibility and user-visible design choices.

## What is original binary evidence and what is new port policy?

The original 2006 `server.dll` SHA256:
`568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`.

Phase78 reconstructed **14 original named difficulty metrics** at
`server.dll` `0x22523478`, seven float32 fields per record.
All 98 initializer words and the original case-sensitive names were
independently authenticated against the owner-retail binary.
Phase79 recovered several actual combat consumers, and Phase80 tied
manager initialization to the original `dd_challenge` and
`dd_forgiveness` ConVars.

**The fixed skill values below are NEW balancing choices, not
recovered original Easy/Normal/Hard presets.** They reuse actual
original f32 values/endpoints for a conservative first playable
prototype. No in-game balance or original-engine runtime test has
yet been performed.

## Why not just freeze `dd_challenge` to three values?

The original 14-variable blend computes
`(1-t)*record.field1+t*record.field2`, which produces mixed gameplay
directions across different metrics:

- Original `DAMAGE_TAKEN_SCALE`: `0.75` when `t=0`;
  `1.4` when `t=1`. Increasing `t` means more incoming damage.
- Original `PERCENT_NPC_ACCURATE`: `1.0` when `t=0`;
  `0.5` when `t=1`. Increasing the same `t` means a lower
  NPC accuracy metric.

Therefore a scalar `dd_challenge` skill slider cannot guarantee a
consistent Easy→Hard progression. A separate static metric set per
difficulty is necessary, while retaining all 14 original gameplay
metric **names and index identities**.

## Proposed fixed profiles

The selector maps the Source-style `skill` number to a static
profile: `1=Easy`, `2=Normal`, `3=Hard`. The original game's
registered `skill` default was observed as `0` in the earlier
retail research; this new port should **explicitly choose Normal (2)
for a new game** rather than interpreting the original zero as a
difficulty setting. Invalid skill values are rejected instead of
silently changing the active profile.

| Original game metric | Easy (new) | Normal | Hard (new) |
|---|---:|---:|---:|
| `DAMAGE_TAKEN_SCALE` | **0.75** | **1.075** | **1.4** |
| `DAMAGE_DONE_SCALE` | 1 | 1 | 1 |
| `AMMO_QUANTITY_SCALE` | 1 | 1 | 1 |
| `NUM_FORCED_HEALTH_KITS` | 0 | 0 | 0 |
| `PERCENT_HEALTH_KITS_DROP` | **0.5** | **0.4** | **0.3** |
| `PERCENT_NPC_2ND_FIRE` | **0** | **0.25** | **0.5** |
| `PERCENT_NPC_MAKE_DIFFERENT` | 0.15 | 0.15 | 0.15 |
| `PERCENT_NPC_CHANGE_SPAWN_NUM` | 0.15 | 0.15 | 0.15 |
| `PERCENT_NPC_WEARS_HELMET` | **0.1** | **0.35000002** | **0.6** |
| `PERCENT_NPC_HELMET_SHOT_OFF` | 0.6 | 0.6 | 0.6 |
| `PERCENT_NPC_TAKES_COVER` | 0.2 | 0.2 | 0.2 |
| `RANDOM_NPC_WAIT_BEFORE_SHOOT` | **+0.1** | **0** | **−0.1** |
| `PERCENT_NPC_ACCURATE` | **0.5** | **0.75** | **1.0** |
| `PERCENT_NPC_MOBILE` | **0.3** | **0.5** | **0.7** |

All **Normal** entries preserve original `record.fields[0]`
**exactly**, including `PERCENT_NPC_WEARS_HELMET`'s original
float32 `0x3EB33334`, not a rounded `0.35f`.

**Only seven named metrics vary by skill.** The other seven remain
at their authenticated current original values until their true
gameplay/engine consumers are verified. In particular, we do not
pretend to understand how negative spawn variation fields should be
interpreted as probabilities, or how helmet-shot-off behavior maps
to player difficulty. Fixed metrics also do not make random enemy
rolls or scripted spawn placement completely deterministic.

## What we preserve from the original game

1. **Gameplay-variable interface and index order.**
   Original code can still resolve named `GamePlayVarValue` metrics;
   `metric_index` remains exact-case and returns the original slot
   0 through 13. The fixed service does not rename these keys.

2. **Original combat consumers.**
   We reuse Phase79's `apply_damage_taken_metric`,
   `scaled_damage_done`, `ammo_quantity_multiplier`, and
   `npc_random_wait_before_shoot`. The original damage bypass
   mask `0x80024021` still applies; some damage types are not
   scaled even on Hard.

3. **Map scripting and campaign triggers.**
   The original `trigger_dynamic_difficulty` has keys
   `EstimatedTime`, `EstimatedDifficulty`, and `EndOfGame`.
   Reconstructed Source events, player-only touch behavior,
   deferred entity removal, map-relative names, and the special
   **low-byte** `EndOfGame` completion test remain separate
   responsibilities of `original_sin_trigger_entities`.
   The new `observe_dynamic_trigger` only records an observation
   and the original low-byte completion status, but
   **never alters skill or combat metrics**. It does not emit
   duplicate events or remove real engine entities itself.

4. **Original difficulty save archive shape.**
   Phases77–78 show a manager archive containing 14 mutable
   metric floats, 10 modifier pairs and six advisor objects.
   The new policy must not let restored adaptive metric values
   silently override a user-selected static skill. After the
   real engine's archive-reader runs, the host should call
   `reapply_after_archive(saved_skill)` and
   `publish_to_host(host)`. No new on-disk Source savegame
   serializer or original DD archive byte layout is invented here.

5. **Client protocol compatibility.**
   Original client callback registrations include
   `GamePlayVarValue`, `DDInfo`, `DDAdvisorScores`,
   `DDAdvisorChannels`, `DDDecisionMade`, and other messages.
   They should **not all be deleted blindly** just because
   adaptive scoring is disabled: their protocol framing and HUD
   side effects must be checked in a genuine Windows process.
   Hiding/de-emphasizing the now-redundant DD HUD can be a
   separate UI step.

## New cross-platform implementation

Files:

- `include/sin_compat/fixed_difficulty_phase663.hpp`
- `src/fixed_difficulty_phase663.cpp`
- `tests/test_fixed_difficulty_phase663.cpp`
- `.github/workflows/sin-phase663-fixed-difficulty.yml`

`FixedDifficultyRuntime` starts on **Normal** and has:

- `set_skill(1/2/3)`: immediately selects a static metric set.
  Invalid values preserve the existing profile.
- `lookup(name,out)` and `metric_at(index)`: query original
  14-name/index contract, with unknown case-sensitive names rejected.
- `observe_performance_sample(...)`: a deliberately non-adaptive
  no-op; no hidden player-statistics-to-difficulty feedback.
- `observe_dynamic_trigger(...)`: records observations and
  campaign completion using the original low-byte EndOfGame check;
  leaves the selected profile unchanged.
- `reapply_after_archive(skill)`: always rebuilds static metrics
  from the player's selected `skill`, even if skill is unchanged.
  This policy **ignores adaptive metric snapshots**; saved skill
  persistence itself remains the owner's game-host responsibility.
- `publish_to_host(IOriginalMetricSink&)`: publishes all 14
  selected float values in original index order through a typed
  engine-implemented callback. This is the **explicit future
  connection point** to the reconstructed original metric storage.
  A failed callback stops immediately and reports committed count,
  and never claims rollback of earlier writes. The engine must fail
  initialization / avoid gameplay on partial publication.

The model's current profile is not a global singleton, does not
write actual retail DLL memory, and is **not yet called by a live
`ServerGameDLL003::DLLInit` implementation**. The engine host
must ensure it publishes only on its game thread and recovers
the real original `0x222D3340` index-to-mutable-metric location
before mutating genuine process memory.

## Regression coverage and verification

- All 14 fields, exactly 7 unchanged metrics, and all seven
  independently adjusted Easy/Normal/Hard variables.
- Inconsistent original `dd_challenge` damage-vs-accuracy slope
  demonstrably avoided.
- Exact original metric key lookup/case sensitivity.
- Invalid and unchanged `skill` values and revision tracking.
- Performance samples and map triggers cannot secretly adapt
  the profile, including `EndOfGame=256` versus `257`.
- Real reconstructed damage and NPC delay math consuming fixed
  difficulty metrics; damage-type bypass mask preserved.
- Original 14-float archive fields filled with artificial
  adaptive values, then reapplication of static profiles.
- A typed mock host receiving all 14 published values in
  exact order, stopping at a rejected/throwing slot, preserving
  observable partial writes, and correct republish after restore.

Linux strict-warning C++17 / Win32 MSVC x86 CTest targets plus
an independent Clang AddressSanitizer/UndefinedBehaviorSanitizer
workflow exercise the same code.

## Required steps for a playable fixed-difficulty SiN port

1. Implement the host's **real `skill` ConVar/menu adapter**,
   initialized to 2 for new games (1/2/3 selectable). The
   existing original `dd_enabled` ConVar can be tested,
   but its specific effects when set to zero must not be
   assumed from its name alone.
2. Identify and bind the 14 real writable game-metric slots
   exposed by `0x222D3340` / `0x222D33E0` and associated
   actual gameplay consumers. Wire the above typed metric sink
   after engine initialization and after original archive restore.
3. Retain the original `trigger_dynamic_difficulty` map entity
   as an event/removal/completion adapter while disabling only
   the adaptive update step. Keep `trigger_secret` as-is.
4. Verify original NPC accuracy, spawn rolls, damage, pickup
   drops and weapon fire in a genuine Windows engine process.
   Iterate the **new** Easy/Hard values based on live results.
5. Only then decide which client DD HUD and advisor messages
   may be hidden or retired without breaking networking,
   menu behavior or save compatibility.

**No playable Windows game module, original retail process execution,
or Android/Quest 3 gameplay is claimed by this phase.**
