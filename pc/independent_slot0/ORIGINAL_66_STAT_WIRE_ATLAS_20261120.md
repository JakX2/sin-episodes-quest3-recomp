# SiN Episodes: Emergence, original client Stats 66-ID wire-format atlas

**Source of truth:** SHA256-authenticated original Windows 2006 `client.dll`, documented in `OWNER_RETAIL_STATS_HOOKS_AND_BITBUF_20261120.md`. Independently authored interpretation from three checked public metadata files: `client_stats_original_id_map.h`, `client_stats_original_type_registry.h` and `client_stats_value_factory_contract.h`, plus `client_stats_wire_value_decoder.h`.

**Original retail evidence:** 66 ordered stat names; immutable compiled ID→type records at 0x2431E660, stride 12, 792 total bytes, SHA256 `63c1ee30f9b2a021115150b8b98a298530f4f2e0ebd621fca3ff7132d7b80ffb`; 16-case value factory at 0x24205BC0; original live CStatReader vtable at 0x242AE06C. All nine selected reader methods and their 32-bit primitive counts were audited against the SHA-pinned original binary.

**Interpretation:** `float32` means a 32-bit Source bit-buffer field represented by its raw IEEE754 bits; `int32→float32` means a 32-bit raw integer field whose value object uses x87 signed-integer-to-float conversion. The table does NOT claim a completed game-stat mutation. Type codes are factory discriminants, not stat IDs. Metadata columns `M1/M2` are preserved raw 32-bit original registry values, not guessed flags. Each `DynamicStat` message additionally has a bounded, NUL-terminated string field **before** its numeric fields. Each `StaticStat` message starts with the 16-bit stat ID followed by the numeric fields.

| Original stat ID | Original stat name | Message | Factory type | Value fields after header/string | Raw M1/M2 |
|---:|---|---|---:|---|---:|
| 0 | `STAT_TOTAL_TIME` | StaticStat (27) | 7 | 1 × float32 | 1 / 1 |
| 1 | `STAT_ABSOLUTE_TIME` | StaticStat (27) | 7 | 1 × float32 | 1 / 2 |
| 2 | `STAT_EXPECTED_TIME` | StaticStat (27) | 1 | 1 × float32 | 1 / 1 |
| 3 | `STAT_TOTAL_TIME_IN_COMBAT` | StaticStat (27) | 7 | 1 × float32 | 1 / 1 |
| 4 | `STAT_AVG_TIME_LENGTH_IN_COMBAT` | StaticStat (27) | 6 | 1 × float32 | 1 / 1 |
| 5 | `STAT_TOTAL_SHOTS` | StaticStat (27) | 0 | 1 × int32→float32 | 2 / 1 |
| 6 | `STAT_TOTAL_SHOTS_IN_COMBAT` | StaticStat (27) | 0 | 1 × int32→float32 | 2 / 1 |
| 7 | `STAT_TOTAL_PRIMARY_SHOTS` | StaticStat (27) | 0 | 1 × int32→float32 | 2 / 1 |
| 8 | `STAT_TOTAL_SECONDARY_SHOTS` | StaticStat (27) | 0 | 1 × int32→float32 | 2 / 1 |
| 9 | `STAT_TOTAL_WEAPON_ACCURACY` | StaticStat (27) | 5 | 1 × float32 | 3 / 1 |
| 10 | `STAT_TOTAL_WEAPON_ACCURACY_IN_COMBAT` | StaticStat (27) | 5 | 1 × float32 | 3 / 1 |
| 11 | `STAT_NUM_GRENADES_THROWN` | StaticStat (27) | 0 | 1 × int32→float32 | 2 / 1 |
| 12 | `STAT_NUM_MELEES` | StaticStat (27) | 0 | 1 × int32→float32 | 2 / 1 |
| 13 | `STAT_TOTAL_RELOADS` | StaticStat (27) | 0 | 1 × int32→float32 | 2 / 1 |
| 14 | `STAT_TOTAL_HEADSHOTS` | StaticStat (27) | 0 | 1 × int32→float32 | 2 / 1 |
| 15 | `STAT_TOTAL_HEADSHOTS_IN_COMBAT` | StaticStat (27) | 0 | 1 × int32→float32 | 2 / 1 |
| 16 | `STAT_TOTAL_DAMAGE_TAKEN` | StaticStat (27) | 0 | 1 × int32→float32 | 2 / 1 |
| 17 | `STAT_TOTAL_HEALTH_GAINED` | StaticStat (27) | 0 | 1 × int32→float32 | 2 / 1 |
| 18 | `STAT_AVG_HEALTH` | StaticStat (27) | 11 | 1 × float32 | 3 / 1 |
| 19 | `STAT_AVG_HEALTH_IN_COMBAT` | StaticStat (27) | 11 | 1 × float32 | 3 / 1 |
| 20 | `STAT_TOTAL_DEATHS_PER_MINUTE` | StaticStat (27) | 9 | 1 × float32 | 0 / 2 |
| 21 | `STAT_TOTAL_DEATHS_PER_HOUR` | StaticStat (27) | 10 | 1 × float32 | 0 / 2 |
| 22 | `STAT_TOTAL_DEATHS` | StaticStat (27) | 0 | 1 × int32→float32 | 2 / 2 |
| 23 | `STAT_TOTAL_REBIRTHS` | StaticStat (27) | 0 | 1 × int32→float32 | 2 / 2 |
| 24 | `STAT_TOTAL_TIME_AT_FULL_HEALTH` | StaticStat (27) | 7 | 1 × float32 | 1 / 1 |
| 25 | `STAT_TOTAL_TIME_BELOW_25_HEALTH` | StaticStat (27) | 7 | 1 × float32 | 1 / 1 |
| 26 | `STAT_TOTAL_NPC_KILLS` | StaticStat (27) | 0 | 1 × int32→float32 | 2 / 1 |
| 27 | `STAT_TOTAL_KILLS_PER_MINUTE` | StaticStat (27) | 9 | 1 × float32 | 0 / 1 |
| 28 | `STAT_TOTAL_KILLS_PER_MINUTE_IN_COMBAT` | StaticStat (27) | 9 | 1 × float32 | 0 / 1 |
| 29 | `STAT_TOTAL_DPS` | StaticStat (27) | 8 | 1 × float32 | 0 / 1 |
| 30 | `STAT_TOTAL_DPS_IN_COMBAT` | StaticStat (27) | 8 | 1 × float32 | 0 / 1 |
| 31 | `STAT_DPS_TAKEN_IN_COMBAT` | StaticStat (27) | 8 | 1 × float32 | 0 / 1 |
| 32 | `STAT_TOTAL_JUMPS` | StaticStat (27) | 0 | 1 × int32→float32 | 2 / 1 |
| 33 | `STAT_TOTAL_JUMPS_IN_COMBAT` | StaticStat (27) | 0 | 1 × int32→float32 | 2 / 1 |
| 34 | `STAT_TOTAL_DUCKS` | StaticStat (27) | 0 | 1 × int32→float32 | 2 / 1 |
| 35 | `STAT_TOTAL_DUCKS_IN_COMBAT` | StaticStat (27) | 0 | 1 × int32→float32 | 2 / 1 |
| 36 | `STAT_TOTAL_SAVES` | StaticStat (27) | 0 | 1 × int32→float32 | 2 / 1 |
| 37 | `STAT_TOTAL_LOADS` | StaticStat (27) | 0 | 1 × int32→float32 | 2 / 1 |
| 38 | `STAT_NUM_FOOT_STEPS` | StaticStat (27) | 0 | 1 × int32→float32 | 0 / 1 |
| 39 | `STAT_NUM_PAUSES` | StaticStat (27) | 0 | 1 × int32→float32 | 2 / 1 |
| 40 | `STAT_NUM_ITEMS_USED` | StaticStat (27) | 0 | 1 × int32→float32 | 2 / 1 |
| 41 | `STAT_NUM_NPCS_CAUGHT_ON_FIRE` | StaticStat (27) | 0 | 1 × int32→float32 | 2 / 1 |
| 42 | `STAT_NUM_NPC_HELMETS_SHOT_OFF` | StaticStat (27) | 0 | 1 × int32→float32 | 2 / 1 |
| 43 | `STAT_NUM_NPCS_DECAPITATED` | StaticStat (27) | 0 | 1 × int32→float32 | 2 / 1 |
| 44 | `STAT_NUM_CONTAINERS_EXPLODED` | StaticStat (27) | 0 | 1 × int32→float32 | 2 / 1 |
| 45 | `STAT_MOST_KILLS_BEFORE_DEATH` | StaticStat (27) | 14 | 1 × float32 | 2 / 2 |
| 46 | `STAT_NUM_SECRETS` | StaticStat (27) | 2 | 2 × int32→float32 | 3 / 1 |
| 47 | `STAT_NUM_ITEM_CRATES_BROKEN` | StaticStat (27) | 2 | 2 × int32→float32 | 2 / 1 |
| 48 | `STAT_CHALLENGE_RATING` | StaticStat (27) | 1 | 1 × float32 | 3 / 1 |
| 49 | `STAT_INSTANT_CHALLENGE_RATING` | StaticStat (27) | 1 | 1 × float32 | 3 / 1 |
| 50 | `STAT_MOST_KILLS_BEFORE_DAMAGE` | StaticStat (27) | 14 | 1 × float32 | 2 / 1 |
| 51 | `STAT_NUM_DD_TRIGGERS` | StaticStat (27) | 0 | 1 × int32→float32 | 3 / 1 |
| 52 | `STAT_NUM_ITEMS_PICKED_UP` | StaticStat (27) | 0 | 1 × int32→float32 | 2 / 1 |
| 53 | `STAT_NUM_ITEMS_USED_GENERIC` | StaticStat (27) | 0 | 1 × int32→float32 | 2 / 1 |
| 54 | `STAT_NUM_FLAILS` | StaticStat (27) | 0 | 1 × int32→float32 | 2 / 2 |
| 55 | `STAT_WEAPON_ACCURACY` | DynamicStat (28) | 5 | 1 × float32 | 3 / 1 |
| 56 | `STAT_WEAPON_HEADSHOTS` | DynamicStat (28) | 0 | 1 × int32→float32 | 2 / 1 |
| 57 | `STAT_WEAPON_PRIMARY_SHOTS` | DynamicStat (28) | 0 | 1 × int32→float32 | 2 / 1 |
| 58 | `STAT_WEAPON_SECONDARY_SHOTS` | DynamicStat (28) | 0 | 1 × int32→float32 | 2 / 1 |
| 59 | `STAT_WEAPON_ACCURACY_IN_COMBAT` | DynamicStat (28) | 5 | 1 × float32 | 3 / 1 |
| 60 | `STAT_NPC_KILLS` | DynamicStat (28) | 0 | 1 × int32→float32 | 2 / 1 |
| 61 | `STAT_WEAPON_NPC_KILLS` | DynamicStat (28) | 0 | 1 × int32→float32 | 2 / 1 |
| 62 | `STAT_LEVEL_KILLS` | DynamicStat (28) | 0 | 1 × int32→float32 | 2 / 1 |
| 63 | `STAT_LEVEL_DEATHS` | DynamicStat (28) | 0 | 1 × int32→float32 | 2 / 2 |
| 64 | `STAT_LEVEL_SECRETS` | DynamicStat (28) | 2 | 2 × int32→float32 | 3 / 1 |
| 65 | `STAT_GAME_PLAY_VAR_AVERAGES` | DynamicStat (28) | 11 | 1 × float32 | 0 / 1 |

## Type distribution

- Original factory type 0: 38 stat IDs
- Original factory type 1: 3 stat IDs
- Original factory type 2: 3 stat IDs
- Original factory type 5: 4 stat IDs
- Original factory type 6: 1 stat ID
- Original factory type 7: 5 stat IDs
- Original factory type 8: 3 stat IDs
- Original factory type 9: 3 stat IDs
- Original factory type 10: 1 stat ID
- Original factory type 11: 3 stat IDs
- Original factory type 14: 2 stat IDs

## What this enables, and what it does not

An independent client may now safely choose a field reader **from the authenticated stat ID** instead of requiring a guessed factory type: `stats_wire_values::InspectFrameForRegisteredId`. The native fixture `independent_client_stats_original_type_registry_native` checks all 66 static/dynamic ID cases, boundary offsets, payload field counts, trailing opaque bits, unknown IDs and transactional failure.

This is **not** yet a working real-engine `CUserMessages::DispatchUserMessage`: the 24-byte proprietary `bf_read` adapter, live server senders, type-specific value mutations, UI callbacks and normal `VClient011::Init` remain missing. No original game binaries, raw source bytes or private run logs are included. The owner-retail terminal checkpoint remains `0x53494E74` until a new real process milestone is independently tested.
