# SiN Episodes: Emergence (2006): Jessica and JC NPC decompilation, round 07

**Scope:** Read-only, address-anchored static analysis of the original shipped PE32 x86 `server.dll` and a negative class-name scan of `client.dll`. This is a reconstruction of *compiled behavior*, not the original source code. No Quest 3 port files were changed. Does not redistribute either DLL or extracted code sections.

## Binary identity and confidence standard

- `server.dll`: SHA-256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`, preferred image base `0x22000000`.
- `client.dll`: SHA-256 `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`, preferred image base `0x24000000`.
- **Verified:** A literal string, RTTI parent, static table field, or original instruction/call is checked against these exact files.
- **Reconstructed:** C++-like summaries inferred from *multiple* verified instructions; names beyond literal identifiers are explanations.
- **Open:** No original class prototype, animation model data or running game observation establishes the complete runtime result.

## 1. Two distinct server-side character designs

| Entity class | Immediate RTTI parent | Primary vtable | Entries | Entries changed versus parent |
|---|---|---:|---:|---:|
| `CNPC_Jessica` | `CNPC_PlayerCompanion` | `0x22497A14` | 514 | 29 |
| `CNPC_JC` | `CAI_BaseActor` | `0x22496FBC` | 497 | 6 |

Other relevant ancestors for Jessica: `CAI_PlayerAlly`, `CAI_BaseActor`. `CNPC_PlayerCompanion` is an inherited companion framework in this binary; do not classify all its behavior as uniquely Ritual-authored without comparing source histories.

Useful override slots:

| Slot (byte offset) | Jessica implementation | JC implementation | Evidence and interpretation |
|---:|---:|---:|---|
| `+0x64` | `0x223628E0` | `0x22361C10` | Character-specific spawn routine, including hard-coded HP assignment |
| `+0x68` | `0x22361E10` | `0x22361BB0` | Model/precache-related setup, as indicated by calls and model-name handling |
| `+0x2C0` | `0x22362420` | `0x22361B90` | Animation-event dispatch; JC directly jumps to shared NPC handler `0x2204A3B0` |
| `+0x370` | `0x22362640` | inherited | Jessica-specific alive-damage callback, including `TLK_INPOISONCLOUD` |

`JC` also has two overrides that merely return zero: `0x22361B80` and `0x22361BA0`. The two characters are **not** identical implementations using different models.

### Spawn defaults established by original instructions

- JC: fallback model `models/characters/JC/jc_torso.mdl` at `0x22496F1C`; `0x22361C2C` selects the model when the model name is empty; health is written as integer **8** to `this+0x9C` at `0x22361CC3`.
- Jessica: fallback model `models/characters/Jessica/Jessica.mdl` at `0x224977A0`; `0x22361EBF` selects fallback; `0x22362962` writes health **80** to `this+0x9C`; `+0x1450` and `+0x1454` temporary ammo fields initialize to **-1**.

These are *spawn code values*. A map or other runtime logic might modify them. JC's model name contains `torso`, but there is no proof here of its visual presentation or whether it was used in released levels.

## 2. Jessica's custom map inputs recovered from her actual datamap

The datamap initialized at `0x22362A8D` contains **six records** starting at `0x225359D4`, spaced `0x34` bytes. Five hold useful metadata and the last is empty. The four I/O handlers below are confirmed by the exact function pointers in the records (not just strings).

| Backing member or input-handler name | Map-facing name | Address / offset | Role |
|---|---|---|---|
| `m_throwName` | (internal) | `this+0x144C` | Entity classname/string for what to throw |
| `InputThrowItemAtPlayer` | `ThrowItemAtPlayer` | `0x223622D0` | Store throw target name and activate condition |
| `InputSetPrimaryAmmoCountToThrow` | `SetPrimaryAmmoCountToThrow` | `0x22362320` | Set integer at `this+0x1450` |
| `InputSetSecondaryAmmoCountToThrow` | `SetSecondaryAmmoCountToThrow` | `0x22362350` | Set integer at `this+0x1454` |
| `InputHolsterWeapon` | `HolsterWeapon` | `0x22362300` | Resolve player entity and invoke a player virtual method at `+0x348` |

`InputThrowItemAtPlayer` tests the incoming variant's type code against **2**, stores the string handle to `+0x144C`, and uses AI condition `0x47`. If the variant type does not match, it falls back to an empty value. Ammo inputs test variant type code **5** and store their integer payload; wrong-type input results in zero.

## 3. A complete custom throw-item AI chain

The original binary includes **four** exact names:

- `AE_JESSICA_THROW_ITEM`: animation event, registered at `0x22362C10`; the assigned runtime ID is stored at `0x2259C9A8` (not a universally fixed event number).
- `COND_THROW_ITEM`: AI condition **`0x47`**.
- `ACT_THROW_ITEM`: registered animation activity.
- `SCHED_THROW_ITEM`: AI schedule **`0x59`**.

The compiled schedule string at `0x224978C0` has these tasks, in order:

```text
TASK_FACE_PLAYER        0
TASK_PLAY_SEQUENCE      ACTIVITY:ACT_THROW_ITEM
```

The original schedule text lists no interrupts after its `Interrupts` marker.

### Verified flow

1. The mapper invokes `ThrowItemAtPlayer` on Jessica. Handler `0x223622D0` records the string at `this+0x144C` and triggers AI condition `0x47` through helper `0x22039760`.
2. Jessica's custom schedule selection at `0x22362380` tests condition `0x47`. When present, it returns **`0x59`** (`SCHED_THROW_ITEM`). It otherwise delegates to her companion base implementation at `0x2236E6E0`.
3. The schedule turns Jessica toward the player, then plays `ACT_THROW_ITEM`.
4. The animation system supplies `AE_JESSICA_THROW_ITEM`; at `0x2236242C`, the class's animation handler compares the incoming ID with the registered event ID.
5. Handler `0x22362420` looks up attachment **`anim_attachment_RH`**, reads `m_throwName`, and invokes creation helper `0x22144DA0`.
6. It attempts an RTTI-based cast from `CBaseEntity` to `CBaseCombatWeapon` via `0x223A5BAB` using TypeDescriptors at `0x224D88D4` and `0x224DE300`.
7. On a compatible weapon object, nonnegative temporary ammo counts are copied to the spawned weapon fields **`+0x4D4`** (primary) and **`+0x4D8`** (secondary). The corresponding Jessica fields reset to **`-1`** after copying.
8. The handler uses the local player and a movement/velocity calculation (`0x2204C850`, `0x2214BA20`) to send the resulting object into motion. A floating-point constant **15.0** is pushed during this calculation. The complete final motion vector and exact meaning of that scalar are not established.

### Reconstructed logic (illustrative, not original code)

```cpp
void Jessica::InputThrowItemAtPlayer(InputData in) {
    m_throwName = in.isString() ? in.stringValue() : STRING(0);
    SetCondition(COND_THROW_ITEM);
}

int Jessica::SelectSchedule() {
    if (HasCondition(COND_THROW_ITEM))
        return SCHED_THROW_ITEM;
    return Companion::SelectSchedule();
}

void Jessica::OnAnimationEvent(Event ev) {
    if (ev.id != registered_AE_JESSICA_THROW_ITEM) {
        Companion::OnAnimationEvent(ev);
        return;
    }
    // Obtain right-hand attachment and requested entity type.
    // Create the entity. If it is a combat weapon, transfer configured ammo.
    // Apply computed movement toward the player.
}
```

The pseudocode intentionally omits unresolved helper prototypes and trajectory math. The static evidence does **not** demonstrate the exact thrown object in any particular campaign scene.

## 4. Dialogue and other specialized Jessica behavior

Verified dialogue concept strings and executable references:

| Concept string | Relevant code | What we know |
|---|---:|---|
| `TLK_HELLO` | `0x22362990` | Invoked from character-specific spawn/setup code |
| `TLK_PLDEAD` | `0x2236262D` | Invoked from a callback that checks a condition and the state of a related entity |
| `TLK_INPOISONCLOUD` | `0x2236265F` | Emitted under a flag test in Jessica's alive-damage handler |
| `TLK_PLYR_PHYSATK` | `0x22361E6C` | Referred to in a special damage/interaction callback |

Another virtual override `0x22361EE0` looks up the pose parameter `head_yaw` and references `weapon_smg`. Under a conditional comparison it returns a float of **22.0** (`0x224977C8`). The precise animation/gameplay condition should be decompiled further before naming it.

## 5. JC: much smaller custom behavior surface

The original `CNPC_JC` virtual-method differences consist of destructor/instance-management, model/precache and Spawn, two trivial `return 0` functions, and a trampoline back to `CAI_BaseNPC`'s animation-event handler at `0x2204A3B0`. The JC `Spawn` code explicitly configures an actor with `jc_torso.mdl`, HP 8, and further flags. There is no comparable custom throw-item datamap or schedule in the recovered JC functions.

Do **not** infer that JC has no conversation behavior at runtime; inherited `CAI_BaseActor`/expresser behavior is substantial. Also, no gameplay encounter or model visual has been checked here.

## 6. Client DLL cross-check

The SHA-pinned original `client.dll` contains **zero literal occurrences** of `C_NPC_Jessica`, `C_NPC_JC`, `DT_NPC_Jessica`, `DT_NPC_JC`, `npc_jessica`, and `npc_jc`. It still contains the generic `C_AI_BaseNPC` RTTI name. This negative scan indicates **no obviously separate named Jessica/JC client class in the checked binary**, not that these server NPCs have no client-side representation. Ordinary entity/networking inheritance can represent them without explicit character class names.

## 7. Importance for the future recompilation

`npc_jessica` is **not** adequately modeled by simply creating a Source companion with Jessica's model:

- Entity inputs must exist and accept the correct `variant_t` types.
- `COND_THROW_ITEM`/`SCHED_THROW_ITEM` must be registered in the custom AI activity table.
- A model animation event must dispatch to the native throw handler.
- The thrown object must use a compatible entity class and preserve weapon ammo fields.
- The client needs the proper animation and attachment state even if it has no dedicated `C_NPC_Jessica` named class.

This is exactly the missing class of game-specific logic observed when running maps on a more generic HL2-based runtime. No port code was touched.

## 8. Next verification targets

1. Resolve `0x22144DA0`'s original creation helper signature and the final physics-motion helper's precise semantics.
2. Compare `AE_JESSICA_THROW_ITEM` against the model's compiled animation events to identify the actual moment of item release.
3. Find campaign `.bsp` entity I/O that calls `ThrowItemAtPlayer` and identify the item names/contexts.
4. Disassemble `CNPC_Jessica`'s other 29 changed vtable entries, then character relationships and dialogue response selection.
5. Crosswalk `CAI_BaseActor` and `CNPC_PlayerCompanion` with Source-era equivalents to separate inherited code from Ritual's additions.

## Reproducibility

`sin_original_jessica_jc_audit.py` checks original SHA-pinned binaries and writes `sin_original_jessica_jc_evidence.json`. Current result: **104 static verifications**, including six client string-absence checks. Use:

```bash
python sin_original_jessica_jc_audit.py ORIGINAL_SERVER.DLL sin_original_jessica_jc_evidence.json ORIGINAL_CLIENT.DLL
```

This script only reads the original binaries and outputs metadata, verified addresses, and interpretation. It does not run executable content or extract proprietary source bytes.