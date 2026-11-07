# SiN Episodes 2006: original Source NPC condition names, numeric IDs, and assault call sites

**November 7, 2026 | Original 2006 Windows x86 server.dll**
SHA-256: `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`

## What changed

Earlier PRs #114, #115, #121, #122 and #124 recovered several **numeric**
NPC condition IDs, but deliberately left their original C++ `COND_*`
symbol names unresolved. The owner's **actual original server.dll** contains
a per-condition native registration routine at `0x22073760`.
These registrations explicitly pass **both the condition name and numeric ID**
through x86 push instructions before invoking the registry helper.

The mappings below are therefore **proven by the 2006 binary**, not inferred
from neighboring Source SDK revisions or generic Half-Life 2 NPC behavior.

| Original 2006 ID | Original registered symbol | Relevance |
|---|---|---|
| `0x04` | `COND_NO_PRIMARY_AMMO` | Original assault approach interrupt |
| `0x0A` | `COND_SEE_ENEMY` | Original assault point clear-on-contact check |
| `0x14` | `COND_CAN_RANGE_ATTACK1` | Original assault combat schedule interrupt |
| `0x18` | `COND_PROVOKED` | **BeginAssault** and diversion condition signaling |
| `0x19` | `COND_NEW_ENEMY` | Original diversion schedule cleanup |
| `0x30` | `COND_HEAR_DANGER` | Original assault navigation interrupt |
| `0x33` | `COND_HEAR_COMBAT` | **Gunfire** assault cue predicate |
| `0x36` | `COND_HEAR_BULLET_IMPACT` | Original assault hold/clear interrupts |
| `0x3B` | `COND_RECEIVED_ORDERS` | Original diversion state check |

### Direct native evidence, not only a string lookup

Each row has an original name literal address, a short x86 `PUSH`
of the numeric condition ID, a `PUSH` of the same original
symbol address, the exact registry receiver, and a relative
`CALL` to **`0x22073760`**. For example:

- `COND_PROVOKED` string at `0x223E0960`; original
  `PUSH 0x18` at `0x2205AA4D`, name pointer at
  `0x2205AA4F`, registry call at `0x2205AA59`.
- `COND_HEAR_COMBAT` string at `0x223E0714`;
  `PUSH 0x33` at `0x2205AD0B`, name pointer at
  `0x2205AD0D`, call at `0x2205AD17`.
- `COND_RECEIVED_ORDERS` string at `0x223E0654`;
  `PUSH 0x3B` at `0x2205ADFE`, name pointer at
  `0x2205AE00`, call at `0x2205AE0A`.

**Seven independent original executable call sites** were checked
against these registrations, including the original condition-query,
-set and -clear routines.

### Important corrections to the earlier investigations

- **Original `ai_goal_assault::InputBeginAssault`:** at
  `0x220CFFAA` pushes original condition `0x18` and
  calls the NPC condition setter `0x22039760` from
  `0x220CFFAC`. We can now state definitively this is
  **`COND_PROVOKED`**, not `COND_RECEIVED_ORDERS`.
- **Original assault cue mode 2 (“Gunfire”):** pushes
  `0x33` from `0x220CEAB3` and calls the
  NPC condition query `0x220397C0` from
  `0x220CEAB5`. The condition is **`COND_HEAR_COMBAT`**.
  Mode 2 is *not* equivalent to `COND_HEAR_DANGER`
  or `COND_HEAR_BULLET_IMPACT`.
- **Original diversion condition:** query at
  `0x220CEB05` / call `0x220CEB07` uses
  **`COND_RECEIVED_ORDERS`** (`0x3B`).
  A distinct diversion branch sets
  **`COND_PROVOKED`** (`0x18`) at
  `0x220CEBCC` / call `0x220CEBCE`.
- **Original assault clear-on-contact query:** at
  `0x220CF970` / call `0x220CF972` uses
  **`COND_SEE_ENEMY`** (`0x0A`).
- **Original schedule cleanup:** original
  `0x220CED99` / `0x220CED9B` clears
  **`COND_NEW_ENEMY`** (`0x19`), and
  `0x220CEDA3` / `0x220CEDA5` clears
  **`COND_SEE_ENEMY`** (`0x0A`).

These are verified *identities and uses*. Further reverse
engineering is still required to reproduce when the original
game's sensors set/clear every condition, especially combat
sounds and line-of-sight.

## Code and reproducibility

`research/tools/audit_sin_assault_condition_registry.py`
is a SHA256-pinned, read-only verifier. It fails closed unless
all nine selected original condition registrations, their strings,
the surrounding native receiver and registration calls,
and the seven separate assault-use call sites match.
The actual original DLL and all source code/assets remain
outside this repository.

`research/tests/test_audit_sin_assault_condition_registry.py`
tests synthetic original-assembly witnesses, corruption rejection
and a cross-language comparison ensuring the C++ reference does
not silently drift from the original DLL audit.

`include/sin_compat/original_assault_condition_ids.hpp`
is a portable C++17 semantic enum with a distinct
`LegacyAssaultConditionId` type, separate from the already
recovered `LegacyAssaultTaskId` and
`LegacyAssaultScheduleId` types. The accompanying
`tests/test_original_assault_condition_ids.cpp` verifies
all ID mappings and a simulated goal input / gunfire cue.
The existing goal and cue code now references the
source-proven condition names.

Run the original binary audit without changing the game:

```powershell
py research\tools\audit_sin_assault_condition_registry.py --server "D:\SiN\bin\server.dll" --output "D:\SiN_Test_Only\original_condition_ids.json"
```

Adjust the actual server.dll path. The output is written
only as a new JSON file, and no original DLL bytes are
copied to the repository.

## Additional original behavior: an integer received-cue setter

Original function **`0x220CE520`** is a direct, independent
write path for `CAI_AssaultBehavior.m_ReceivedAssaultCue`:

- It checks the assault behavior's owning NPC pointer at
  behavior `+0x04` and, when present, zeros the owner's
  fields `+0x864` and `+0x85C`.
- It reads a caller-supplied **integer argument** from
  original x86 stack `+0x04` (`0x220CE535`) and stores
  that integer into behavior `m_ReceivedAssaultCue`
  at `+0x20` (`0x220CE539`).
- It tail-jumps from **`0x220CE547`** into the native
  `SetCondition` method **`0x22039760`** after
  replacing its argument with `0x18`, now independently
  proven to mean **`COND_PROVOKED`**.

The verifier now checks seven original instruction groups
covering those effects **and the exact native tail-jump
destination**. Its synthetic tests reject a changed cue
storage field, a changed condition number, or a changed
tail-jump destination.

This is a verified *parameterized* signal helper. It
demonstrates how **any supplied integer cue value** can
reach the original behavior's received-cue field. It does
**not** establish that any actual code or map input invokes
the helper with the value 4. The caller graph remains a
separate reverse-engineering question.

## Important limit: received cue override 4

The earlier `CAI_AssaultBehavior` cue helper at `0x220CEA90`
verifiably returns success if `m_ReceivedAssaultCue == 4`.
However, **the original writer or condition that sets this
particular behavior member to 4 is still unproven**.

The original DLL contains other unrelated objects with fields
at local `+0x20`, including a `MOV [ECX+0x20],4` at
`0x222C4D2B`. Matching an offset and literal by themselves
**does not prove** that this instruction writes the assault
behavior's `m_ReceivedAssaultCue`. We intentionally do not
attribute that unrelated write to this class.

## Next original decomp targets

1. Trace exact original producers/consumers of
   `COND_RECEIVED_ORDERS` and `COND_PROVOKED`,
   and map the sensor/event effects into the portable
   AssaultBehavior host.
2. Resolve the remaining `m_ReceivedAssaultCue==4`
   override source via real callgraph and typed object provenance.
3. Connect the authentic condition semantics to
   the existing schedule-runner callbacks. The current
   runtime is still a **simulation**, not a Source entity.
4. Solve the native SourceVR ARM64 game-module interface,
   original NPC factories, pathfinding, combat and scripting.

The working desktop one-grunt HL2 prototype is unchanged;
no original assets or binaries were modified.
