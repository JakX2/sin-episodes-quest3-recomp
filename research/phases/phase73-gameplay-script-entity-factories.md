# Phase 73: Original SiN entity registration and campaign scripting audit framework

**Date:** 31 October 2026. **Type:** Original binary registry verification + independently authored, static map inspection tool. **Runtime scope:** No in-engine SiN level, scripted encounter, or Quest 3 execution has been established by this phase.

## Findings verified in the original 2006 SiN server.dll

We checked the user-owned `server.dll` and `client.dll` against pinned SHA-256 identities (`server`: `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`; `client`: `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`). This phase confirms **32 selected original entity-class registrations**, not a complete registry.

Each checked `server.dll` registration routine consists of a call to shared registry getter **`0x2228B110`**, `mov edx,[eax]`, a push of the actual classname C-string pointer, a push of a specific data-section entity-factory object pointer, `mov ecx,eax`, `call [edx]`, and `ret`. The new verifier checks **all 32 code shapes, all literal classname bytes, all registry call targets and all distinct factory-object pointers**, with explicit `.text`, `.rdata`, `.data` section validation. The original PE32 x86 VA addresses are build-specific and are **never** an Android/ARM64 object-layout claim.

| Category | Original confirmed class registrations | Count |
|---|---|---:|
| Logic and state | `logic_auto`, `logic_relay`, `logic_timer`, `logic_case`, `logic_branch`, `logic_compare`, `math_counter`, `env_global` | 8 |
| Transition | `trigger_changelevel`, `trigger_transition`, `info_landmark` | 3 |
| General trigger | `trigger_multiple`, `trigger_once`, `trigger_teleport`, `trigger_hurt`, `trigger_look` | 5 |
| SiN-specific | `trigger_dynamic_difficulty`, `npc_sintek_grunt`, `npc_sintek_jetpack` | 3 |
| NPC scheduling/cinematic | `npc_maker`, `npc_template_maker`, `scripted_sequence`, `aiscripted_schedule`, `logic_choreographed_scene` | 5 |
| Spawn and equipment | `point_template`, `env_entity_maker`, `game_player_equip` | 3 |
| World interaction and paths | `func_door`, `func_button`, `func_movelinear`, `path_corner`, `path_track` | 5 |
| **Total** | **Selected original server factory registrations** | **32** |

Sample exact instruction locations from that list: `logic_auto` registration at `0x223BC2F0`, classname at `0x2241F360`, factory at `0x224FB288`; `trigger_changelevel` at `0x223BE170`, classname at `0x22437EAC`, factory at `0x2250B3C0`; `trigger_dynamic_difficulty` at `0x223BE4A0`, classname at `0x2243CAF4`, factory at `0x2250B404`; `npc_sintek_grunt` at `0x223C77A0`; `npc_sintek_jetpack` at `0x223C77C0`. The verifier contains the remaining exact addresses.

This evidence establishes **registration of entity classnames against original server factory objects**. It does **not** by itself establish output firing semantics, original NPC AI behavior, trigger conditions, specific campaign entity counts, correct map transition timing, or a working SourceVR replacement. The Source engine may also register many entities outside our **selected 32**; unknown names in a later map audit must not be labeled unsupported.

## Executable tools added

### 1. Original binary verifier

```sh
python3 research/tools/verify_gameplay_entity_registry.py /path/to/lawfully/owned/server.dll /path/to/lawfully/owned/client.dll --output original-factory-verification.json
```

This emits the verified 32-entry registry with VAs. It requires exact original hashes by default. All original 32 signatures passed against the provided 2006 PE files. No DLL bytes, executable instruction dumps, map assets, or data models from the game are committed.

### 2. BSP or extracted entity-lump dependency audit

```sh
python3 research/tools/audit_source_map_entities.py /path/to/lawfully/owned/map.bsp --output my-map-static-audit.json
# Or an independently extracted plaintext Source entity lump:
python3 research/tools/audit_source_map_entities.py /path/to/private/level.ent --output my-level-audit.json
```

The tool reads the **entity lump (lump 0)** of a Source `VBSP` with versions 17–31 when the entity data is plain and uncompressed. It also accepts extracted Source entity keyvalues `.ent` text. It preserves duplicate keys and outputs, handles `//` comments and quoted strings, parses both comma-separated and escape-delimited Source I/O payloads, and inventories:

- Exact entity classname counts and matches against the **verified subset** of the original SiN factory registrations.
- Literal `targetname` links for `On...` entity outputs; missing, duplicate/ambiguous, dynamic/wildcard and malformed references are kept in **separate categories**.
- `trigger_changelevel` map name and local `info_landmark` string references, without asserting they will load successfully.
- Broad subsystem counts for prioritizing a lawful, module-capable engine adapter.

This is **static dependency inspection**. A missing literal target could be deliberately spawned at runtime, match a wildcard or refer to a game state outside the map. A class name missing from the **32 verified subset** is *not* proof that it is absent from the original game or unsupported by any engine. The tool rejects malformed/truncated/unsupported or compressed BSP entity lumps instead of silently misparsing them. It does not extract files, decompile game scripts, load assets or dispatch any game entity input.

**No shipped `.bsp` maps were supplied for this phase.** Our actual gameplay entity counts, broken-map references and campaign-script implementation coverage therefore remain **unknown**. All synthetic map tests are self-authored fixtures, not measured SiN level results.

## Porting order: what this tells us to implement next

| Priority | Required compatibility surface | Gate before claiming working gameplay |
|---|---|---|
| P0 | Engine-compatible **entity factory and I/O dispatcher** with targetname lookup, delayed outputs, fire counts and save/load | A lawful Source engine game module creates one actual `logic_auto`/`logic_relay`/`trigger_once` chain and proves it executes in-engine |
| P0 | **Level transitions and landmarks** (`trigger_changelevel`, `trigger_transition`, `info_landmark`) | Execute one actual transition with legitimate player state preservation on a PC test map |
| P1 | **Spawners, NPC scripts and schedules** (`npc_maker`, `npc_template_maker`, `aiscripted_schedule`, `scripted_sequence`) | Spawn a real actor and execute a schedule/cinematic event in-engine |
| P1 | **SiN-specific NPC factory and dynamic difficulty** (`npc_sintek_*`, `trigger_dynamic_difficulty`) | Correct original behavior/state tests plus live encounter, not just a classname being registered |
| P1 | **Doors, buttons, movement paths and sequences** | Press a button, see door/path behavior, and verify save/restore in-engine |
| P2 | Campaign-wide parity audit | Run the tool on the user's lawfully installed real level BSPs, trace linked outputs and manually review dynamic targets; mark a level complete only after actual gameplay |

These are **proposed engineering priorities**, not claims that the port already implements these subsystems. The portable Phases 56–72 weapon/ABI tests do **not** make original `logic_relay`, `scripted_sequence`, NPC AI or level transitions playable. No authorized SourceVR engine source with module hooks has been identified (existing integration blocker issue #12).

## Test evidence and reproduction

New self-contained unit tests build an **original-free synthetic PE fixture** with the same registration signature shape and cover mutations of classname string, code shape, factory pointer and registry call. Additional synthetic `.ent` and `.bsp` fixtures cover entity linkage, duplicated `On...` outputs, `\x1b`-delimited outputs, wildcard and special targets, malformed outputs, transition landmark references, unsupported/truncated BSP headers and basic safety limits.

On this research container, all **32 selected original registrations** passed the SHA-pinned original DLL verifier and **20 new Python tests passed**. No original game map/runtime test took place. GitHub CI runs the new synthetic tests along with prior repository tests. No C++ runtime or gameplay engine code was changed.

## Next evidence required

1. Run the map auditor on **legally held SiN BSP map files** and reconcile static target references. Users should *not* upload proprietary maps or extracted assets to a public GitHub repository.
2. Identify a **lawfully accessible, module-capable** Source/Quest engine revision. Prove one tiny in-engine factory + delayed I/O test before treating the historical DLL registry as working native gameplay.
3. Recover original `trigger_dynamic_difficulty`, `scripted_sequence` and NPC scheduler behavior against pinned 2006 server code, then implement and measure it in the chosen host.
4. Continue ARM64 standalone runtime hardware testing independently; do not conflate it with game-script integration.
