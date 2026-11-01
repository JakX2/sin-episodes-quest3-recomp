# Phase 90: Original SiN combat spread rules from owner-supplied game scripts

**Source:** owner-uploaded original `SE1/scripts/rules_combat.txt` in `game stuff.zip`, alongside weapon, sound, response, stats, cfg and GameInfo files. This is a read-only private-source audit and an independently written portable C++17 lookup. **The commercial script is NOT committed.**

## Exact input provenance

| Original owner-held file | SHA-256 |
|---|---|
| `scripts/rules_combat.txt` | `0d4c47cfa543435deb7098b4218e62707a02971351957260959ffd6016667f1e` |
| `GameInfo.txt` | `476011d70e75fad4ca322689ac8b1613685f1c345b74bad3c8fe27abf0a51033` |
| `scripts/stats/stats_definitions.txt` | `a18afd2e8fba54653a9bd173c3eacf440fe016b3659b74578f2ecd1f37a156a8` |

The uploaded archive has 139 ZIP entries with approximately 1.27 MB uncompressed content, including 11 weapon scripts, 62 named statistics, and the game's original `GameInfo.txt`. Its `SteamAppId=1300` and `Game |gameinfo_path|.`, `Game SE1` search entries are original Windows Source configuration, **not proven Android library loading rules**.

## Combat attack spread table

The original file's `AttackSpreadRules` contains **10 attacker class mappings** to rule sections. The lookup hierarchy is:

```text
AttackSpreadRules
  class_names[attacker_class] -> actor_rule
  actor_rule.weapon_class_names[weapon_class or "*"] -> weapon_rule
  actor_rule.weapon_rule.target_class_names[target_class or "*"] -> target_rule
  actor_rule.weapon_rule.target_rule -> spread numeric value
```

The source comments annotate these stored values with nominal angular widths. **Use the stored numeric factor**, not the displayed degree label, for later engine integration.

| Original attacker class(es) | Target player | Other target |
|---|---:|---:|
| `player` | `0.0` | `0.0` |
| `npc_sintek_grunt`, `npc_druglab_grunt`, `npc_druglab_grunt_pistol`, `npc_jessica`, `npc_turret_ceiling` | `0.13053` (annotated 15°) | `0.04362` (5°) |
| `npc_druglab_grunt_assault_rifle`, `npc_druglab_grunt_scattergun` | `0.08716` (10°) | `0.04362` (5°) |
| `npc_druglab_jetpack`, `npc_druglab_grunt_elite` | `0.11320` (13°) | `0.04362` (5°) |

In the provided file, most weapon entries map to a generic rule through the `*` fallback; the file nonetheless has explicit per-weapon mapping syntax and the new loader must resolve that hierarchy rather than hardcode attacker-only spread values. An attacker class not mapped and lacking a wildcard must report **NoActorRule**, not silently inherit a made-up default.

## Portable C++17 implementation

- `CombatSpreadRules::load(std::string_view)` parses independently authored Source-style KeyValues syntax: quote-delimited fields, comments, braces, nested sections, string leaves and duplicate-key rejection.
- `lookup(attacker, weapon, target)` follows both explicit class mappings and `*` wildcards. It returns a typed Found / NoActorRule / MissingRule / InvalidValue result, instead of guessing missing data.
- The parser rejects malformed braces, unknown escapes, nesting depth over 16, oversized documents over 256 KiB, large tokens and nonfinite/out-of-range numeric spread values.
- All rules are **runtime-loaded**. No original game script or entire mapping tables are embedded in the library or test fixtures.
- The existing `sin_weapon_runtime` static library compiles this component on desktop **and Android ARM64 NDK**; no new exported host C ABI or gameplay DLL is claimed.

## Verification

One CTest `sin_combat_spread_rules_tests` uses an **independently authored synthetic** KeyValues tree to test player, standard, elite, actor/weapon/target wildcards, explicit weapon precedence and malformed documents. A separate optional executable `sin_combat_spread_private_audit` accepts the owner's actual `rules_combat.txt`, and compares seven distinct attacker/target scenarios against its independently observed constants.

The exact original file **passed all seven private lookup checks locally** and the synthetic test passed with GCC C++17 strict warning flags and AddressSanitizer/UBSan. GitHub CI runs the synthetic CTest, native build, Android NDK cross-build and other existing jobs; it does *not* have access to the private original source file.

Reproduction:

    cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
    cmake --build build --parallel
    ctest --test-dir build --output-on-failure
    ./build/sin_combat_spread_private_audit /private/SE1/scripts/rules_combat.txt

## Important limitations

This component **does not** perform muzzle-space spread sampling, recoil, NPC perception, actual Source weapon firing or dynamic-difficulty adjustment. We have reconstructed a **data lookup**, not the original combat solver. The file's annotated degrees are reference comments; the exact in-engine random spread generator and whether additional scaling applies are unverified.

The original `stats_definitions.txt` provides 62 tracked stat identifiers, including `STAT_TOTAL_TIME_IN_COMBAT`, `STAT_TOTAL_SHOTS`, `STAT_TOTAL_WEAPON_ACCURACY`, `STAT_TOTAL_DAMAGE_TAKEN`, and `STAT_EXPECTED_TIME`. These are candidates for Phase 91's **source-grounded stat schema**, but their mere names do not prove how the 2006 difficulty algorithm consumes them.

**Engine ABI issue #12, compiled campaign issue #20 and real Quest gameplay remain open. Physical VR headset testing remains deferred.**
