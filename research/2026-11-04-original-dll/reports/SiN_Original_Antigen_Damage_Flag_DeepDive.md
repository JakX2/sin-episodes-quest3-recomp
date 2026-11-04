# SiN Episodes: Emergence (2006) | Antigen-associated damage flag `0x00400000`

**Scope:** Read-only reverse engineering of user-supplied, original 32-bit `server.dll` (SHA-256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`). All addresses are preferred-image VAs with PE image base `0x22000000`. The original executable is not modified, redistributed, or bundled. No Quest 3 or GitHub modifications.

**Reproduction:** `python sin_antigen_damage_flag_trace.py ORIGINAL_SERVER.DLL sin_antigen_damage_flag_evidence.json` using Python 3 standard library. The standalone script reports **67 passed binary anchors** on the SHA-pinned original DLL. The JSON records evidence addresses and interpretations, not binary contents.

## Main result

There is now **strong original-binary evidence** that damage bit `0x00400000` serves as an **antigen-associated gameplay channel**, rather than being merely a generic, unexplained flag. The critical confirmation is a complete chain inside `CNPC_Quadralex`:

```
CTakeDamageInfo+0x38 contains bit 0x00400000
    │
    ▼ 0x22374250: Quadralex class-specific living-damage virtual (+0x370)
       0x22374323: bit test
    │
    ▼ 0x2237436A: accumulate scaled damage at Quadralex+0xC68
       0x22374376: compare against original float 100.0
    │
    ▼ 0x223743B2: AI SetCondition(0x47)
       0x22375791–0x223757A1: condition 0x47 registered as
                         COND_QUADRALEX_ANTIGEN_DAMAGE
    │
    ▼ 0x2237499B: Quadralex virtual +0x434 schedule selector checks condition 0x47
       0x223749A8: returns schedule ID 0x51
    │
    ▼ 0x2237585C–0x22375891: schedule 0x51 is registered as
                         SCHED_QUADRALEX_ANTIGEN_DAMAGE
       Schedule body: TASK_STOP_MOVING → TASK_PLAY_SEQUENCE(ACT_QUAD_PAIN)
```

The link between **a concrete damage mask** and **both named antigen AI condition and pain schedule** is proven by instructions, primary vtable pointers, and original schedule strings. The literal original C++ damage-type enumerator, if there was one, is **not** recovered: do not silently relabel it `DMG_ANTIGEN` as though that were verified source.

## 1. Core class ownership proven by MSVC RTTI

| Class | Primary vtable | Relevant virtual | Actual handler |
|---|---:|---|---:|
| `CNPC_Quadralex` | `0x2249ED8C` | living damage `+0x370` | `0x22374250` |
| `CNPC_Quadralex` | `0x2249ED8C` | schedule selection `+0x434` | `0x22374970` |
| `CNPC_BaseMutant` | `0x2249F65C` | living damage `+0x370` | `0x22342CB0` |
| `CNPC_RecombinantMajor` | `0x224A0D8C` | living damage `+0x370` | `0x22379100` |
| `CNPC_RecombinantMinor` | `0x224A2974` | living damage `+0x370` | `0x2237D0C0` |
| `CHL2_Player` | `0x224AEC3C` | damage predicate `+0xD8` | `0x2239E430` |
| `CAntigenDispenser` | `0x2246AA6C` | its class-specific setup `+0x64` | `0x222F6FE0` |

Each has matching type-descriptor strings and a primary-vtable Complete Object Locator. This makes class identity much more reliable than assuming stripped-function proximity.

## 2. Quadralex reaction: actual constants, guards, and code

`0x22374250` first copies incoming damage information using `0x2202F480`. At `0x22374323`, its local damage type (`CTakeDamageInfo+0x38`) is tested against mask `0x00400000`.

On the special path:

1. It resolves the input attacker handle (damage info `+0x28`) and compares the resolved pointer to the single-player entity.
2. When the attacker is **not** that player, it multiplies the local damage-related scalar by **5.0**, from original float address `0x2249BDF4` (`fmul` at `0x2237435C`). When the attacker is that player, this extra multiplier is skipped.
3. It adds the scalar to Quadralex state `+0xC68`, preserving the result there (`0x2237436A–0x22374370`).
4. It compares the accumulator to **100.0**, stored at `0x2249BDE8` (`0x22374376`).
5. For ordinary finite values, **if the accumulator exceeds 100**, the code reaches `SetCondition(0x47)`. If it has not exceeded 100, it can still set that condition when the attacker resolves to the local player (`0x223743AE–0x223743B0`).
6. On setting condition 0x47, it resets the accumulator to zero and calls `0x22039760` (`0x223743B2–0x223743C0`).

A compact behavioral reconstruction (not Ritual source):

```cpp
// Locally reconstructed from original instructions; symbolic names invented here.
if (info.damageFlags & 0x00400000) {
    float stimulus = info.damage;
    if (attacker != localPlayer)
        stimulus *= 5.0f;

    antigenStimulusAccumulator += stimulus;
    if (antigenStimulusAccumulator > 100.0f || attacker == localPlayer) {
        antigenStimulusAccumulator = 0.0f;
        SetCondition(0x47);  // CONFIRMED original name: COND_QUADRALEX_ANTIGEN_DAMAGE
    }
    // Other NPC reaction / damage forwarding continues; not shown here.
}
```

**Subtlety:** This is an antigen *reaction accumulator*, not proof that Quadralex loses exactly that amount of HP. The function subsequently calls additional virtual/subobject processing and can reach `CAI_BaseNPC` living-damage code. An independent flag `0x00010000` also has a skip path; the meaning of that bit is not established here.

**Other class-specific condition:** At `0x22374310`, when the surrounding special actor checks and a comparison with **70.0** pass, it sets ID `0x45` via the same AI condition API. The original registration identifies `0x45` as `COND_QUADRALEX_CAR_DAMAGE`, **distinct from antigen ID `0x47`**. This prevents conflating two separate Quadralex reaction systems.

## 3. Proved reaction schedule

Registration at `0x22375791` binds the original text `COND_QUADRALEX_ANTIGEN_DAMAGE` at `0x2249DACC` to numeric condition **`0x47`**.

The class's schedule selection virtual function at `0x22374970` checks that same condition, through `0x220397C0` at `0x2237499F`. When present, the function returns **`0x51`** from `0x223749A8`.

The schedule registration at `0x2237585C` binds ID **`0x51`** to the original string `SCHED_QUADRALEX_ANTIGEN_DAMAGE` (`0x2249D830`), and the schedule body at `0x2249D7B0` contains exactly:

- `TASK_STOP_MOVING` (`0`)
- `TASK_PLAY_SEQUENCE` with `ACTIVITY:ACT_QUAD_PAIN`
- No listed schedule interrupts

This completes the direct **flag → accumulated stimulus → named condition → named schedule → pain animation** static chain.

## 4. The original antigen dispenser uses the identical bit for healing

Original RTTI identifies a separate `CAntigenDispenser`, with primary vtable `0x2246AA6C` and class setup function `0x222F6FE0`. Its code registers a periodic think/callback pointer `0x222F7100` (for example at `0x222F7FC1–0x222F7FC8`).

Within that callback:

| Address | Observation |
|---|---|
| `0x222F72B7` | Resolves an entity through a handle helper |
| `0x222F72C2` | Pushes integer `0x00400000` |
| `0x222F72C7` | Pushes a floating-point scalar already computed by the callback |
| `0x222F72CA` | Calls the resolved entity's virtual `+0xE4` |
| `0x222F72D0` | Checks the return value |

The `+0xE4` virtual is supported as a **health-gain routine** by the original base entity's `0x22140BE0`, which reads current HP `+0x9C` and maximum HP `+0x98`; the combat-character implementation `0x222A57A0` forwards into that routine. Thus the dispenser passes the special bit to `TakeHealth`-style semantics.

**Critical distinction:** Passing the flag as the second argument of a healing function is **not itself creating an incoming `CTakeDamageInfo` event**. We have therefore located a concrete original **antigen-system user** of the bit, but not yet the entity that originates the mutant-killing *damage* with this bit.

## 5. Player and Recombinant handlers corroborate the special meaning

**Mutants:** The shared `CNPC_BaseMutant` living-damage function tests `0x00400000` at `0x22342CD8`. When the single-player guard and player-attacker-handle comparison pass, it directly stores **`-5`** to entity HP `+0x9C` (`0x22342D4D`), before invoking normal NPC damage processing. The shared combat-character handler can then invoke the death callback on nonpositive HP. The full original control flow is documented in `SiN_Original_Mutant_Forced_Death_Damage_DeepDive.md`.

Additionally, its virtual `+0xD8` override at `0x22342BF0` returns true when the **entire** damage type equals exactly `0x00400000`; this differs from the bit-mask test in the living-damage handler.

**Player:** `CHL2_Player`'s virtual `+0xD8` at `0x2239E430` has an exact `0x00400000` comparison (`0x2239E4D7`). On a narrower, guarded path it passes the copied damage scalar and that special flag into virtual `+0xE4` (`0x2239E4EC–0x2239E4F4`), its inherited health-restoration path. The `CHL2_Player` health-gain method at `0x22246860` directly calls `0x222A57A0` at `0x222468A6`, then tests the result, which independently confirms the virtual-slot interpretation. This is not an assertion that *all* antigen damage heals the player, only that this guarded behavior exists in the executable.

**Ordinary base entity:** `CBaseEntity`'s `+0xD8` at `0x2213C880` explicitly singles out both `0x00400000` and `0x00010000` for special predicate outcomes. The meaning of the predicate is not assigned a definitive original function name in this report.

## 6. What remains unproven

- **Exact source-level enumerator.** The SiN-specific meaning is antigen-associated, but it is not yet safe to substitute a standard Source SDK constant name, or assert the original header contained `DMG_ANTIGEN`.
- **Damage-event producer.** No direct producer has been proven to create `CTakeDamageInfo` with the `0x00400000` bit. Potential sources include another game entity, map I/O, a compiled helper that passes the flag indirectly, and other modules. The dispenser's `TakeHealth` call is not such proof.
- **Gameplay reachability.** Compiled branches and registered schedules do not establish how often they occur in the shipped campaign without dynamic tracing.
- **Health versus reaction.** Quadralex's `+0xC68` accumulation measures a reaction stimulus. The remaining damage application may have additional checks, not reproduced in the compact pseudocode above.

### Suggested next reverse-engineering target

Work backward from the actual antigen canister/dispenser entity inputs and from constructors or key-value initialization of `CTakeDamageInfo` to prove **who sets `0x00400000` on a damage event**. Prioritize the original `CAntigenCanister` and `CAntigenDispenser` code, the player's antigen interaction, and map damage-type keyvalues. Do not infer the constructor solely from the presence of the antigen resource names or the dispenser's separate health-gain call.

No port integration or source reproduction was performed in this research round.