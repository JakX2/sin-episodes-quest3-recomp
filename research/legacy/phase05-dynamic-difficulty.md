# Phase 5: game rules and dynamic difficulty (historical conversation excerpt)

**Evidence level:** From an earlier saved assistant research report. We do not have the full original Phase 5 ZIP or its evidence files in this repository. Addresses are historical build-specific VAs and are **not re-verified in this publication**.

Earlier work reported `CGameRules -> CSingleplayRules -> CSE1Rules`, with a 114-entry vtable and difficulty-related virtual slots:

| Vtable slot | Address | Earlier interpretation |
|---:|---|---|
| 62 | `0x222D47F0` | Damage dealt modifier |
| 63 | `0x222D5420` | Conditional damage taken modifier |
| 77 | `0x222D4830` | Ammunition quantity modifier |
| 97 | `0x222D4940` | Health-kit decision |
| 98 | `0x222D4980` | NPC secondary-fire probability |
| 101 | `0x222D4A80` | NPC helmet probability |
| 102 | `0x222D4AD0` | Helmet removal probability |
| 104 | `0x222D4B70` | NPC mobility decision |
| 107 | `0x222D4CF0` | NPC firing delay adjustment |

## Spawn-count adjustment

The earlier report associated address `0x222D4A00` with `PERCENT_NPC_CHANGE_SPAWN_NUM`, adding or subtracting **one** NPC probabilistically according to the sign and magnitude of a modifier. This pseudocode was an interpretation, not original source:

```cpp
int adjustSpawnCount(int count, float modifier, float rand01) {
    if (modifier != 0.0f && rand01 < std::abs(modifier))
        return count + (modifier > 0.0f ? 1 : -1);
    return count;
}
```

Historical configuration examples from that report:

| Challenge | Reported modifier |
|---:|---:|
| 0.0 | −0.10 |
| 0.2 | 0.00 |
| 0.5 | +0.15 |
| 0.8 | +0.30 |
| 1.0 | +0.40 |

The earlier analysis also described a selective damage-type bitmask `0x80024021` in the damage-taken path at `0x222D5420`; exact semantics for individual Source damage flags were unresolved.

## Open work

Verify call sites from spawners and damage hooks; identify exception masks against the chosen SDK build; test with scenarios and AI. Neither the earlier branch pseudocode nor the sample challenge table should be treated as a validated in-game integration.
