# SiN Episodes: Emergence (2006) — Original server.dll entity damage and health handlers

**Scope:** Read-only reverse engineering of the original user-provided Windows x86 `server.dll`. This report continues the physics-impact threshold, deferred-damage and sound-queue investigations. It does not alter the Quest 3 port and does not include any proprietary executable bytes.

- Original binary: 6,049,792 bytes
- SHA-256: `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`
- Preferred PE base: `0x22000000`
- Reproducible audit: `sin_original_damage_handlers_trace.py` (39 passing independent checks, including nine RTTI-backed class/vtable mappings)
- Evidence: `sin_original_damage_handlers_evidence.json`

## Result in brief

**We have recovered the actual integer health-subtraction instruction sequence.** Collision-derived damage dispatched through `CBaseEntity::TakeDamage` (`0x2213F460`) can reach specialized player or NPC handlers; those routes invoke `CBaseCombatCharacter`'s living-damage routine (`0x222A7B20`). At `0x222A7CD6–0x222A7D0E`, this function converts a floating-point damage amount to an integer using x87's truncating rounding mode, subtracts it from the entity's stored health, and writes the new integer health value to `this + 0x9C`. Further flags, armor, game-rules hooks and NPC-specific filters can change the amount or whether damage is accepted, so this is not an unconditional end-to-end damage formula.

## 1. Vtable proof: what `+0xE0` actually reaches

All primary vtable addresses below were validated with their MSVC `CompleteObjectLocator` pointers, which themselves point to named RTTI TypeDescriptors. `+0xE0` is the entity-specific damage callback, and `+0x370` is a combat-character alive-damage callback for applicable classes. These are **byte offsets**, not function slot indices: `+0xE0` is entry 56 and `+0x370` is entry 220.

| Runtime class | Primary vtable | Virtual `+0xE0` | Virtual `+0x370` | Virtual `+0xE8` |
|---|---|---|---|---|
| `CBaseEntity` | `0x2240AC2C` | `0x2213F1F0` | not applicable | `0x22140E00` |
| `CBaseCombatCharacter` | `0x22459E5C` | `0x222A57C0` | `0x222A7B20` | `0x222A7850` |
| `CBasePlayer` | `0x2245222C` | `0x2224C020` | `0x2224B130` | `0x2224EF40` |
| `CHL2_Player` | `0x224AEC3C` | `0x2239FA00` | `0x2224B130` | `0x2224EF40` |
| `CAI_BaseNPC` | `0x223DA7AC` | `0x222A57C0` | `0x22040140` | `0x22042E70` |
| `CNPC_RecombinantMajor` | `0x224A0D8C` | `0x222A57C0` | `0x22379100` | `0x22377750` |
| `CNPC_RecombinantMinor` | `0x224A2974` | `0x222A57C0` | `0x2237D0C0` | `0x22042E70` |
| `CNPC_Jessica` | `0x22497A14` | `0x222A57C0` | `0x22362640` | `0x2208FAD0` |
| `CNPC_Quadralex` | `0x2249ED8C` | `0x222A57C0` | `0x22374250` | `0x22374CB0` |

**Interpretation of `+0xE8`:** Within the shared combat handler, this virtual method is invoked on an HP-exhaustion path (`0x222A598C`). In the NPC variant, `0x22042E70` explicitly sets `lifeState` to `1` (`0x22042EB1`), supporting a death-handler interpretation. The complete death behavior is class-dependent and has not been exhaustively reconstructed.

## 2. Exact call paths

### Players

```text
0x2213F460  CBaseEntity::TakeDamage
  -> virtual +0xE0 at 0x2213F64E
  -> 0x2239FA00  CHL2_Player damage handler
      [checks state, flags and plays/dispatches special effects]
  -> 0x2224C020  CBasePlayer damage handler (direct call at 0x2239FB77)
      [checks additional damage flags, armor, and player state]
  -> 0x222A57C0  CBaseCombatCharacter handler (direct call at 0x2224C264)
  -> 0x2224B130  player alive-damage override (virtual +0x370)
  -> 0x222A7B20  shared alive-damage implementation (call at 0x2224B145)
  -> health read/modify/write at 0x222A7CD6..0x222A7D0E
```

### Recombinant majors and minors

```text
0x2213F460  CBaseEntity::TakeDamage
  -> virtual +0xE0
  -> 0x222A57C0  CBaseCombatCharacter handler
  -> virtual +0x370 at 0x222A5961
      MAJOR: 0x22379100
      MINOR: 0x2237D0C0
  -> 0x22342CB0  CNPC_BaseMutant-level filtering/conversion
  -> 0x22040140  CAI_BaseNPC alive-damage override
  -> 0x222A7B20  shared alive-damage routine
  -> health read/modify/write
```

Verified relative calls: major `0x2237918F -> 0x22342CB0`; minor `0x2237D28D -> 0x22342CB0`; mutant base `0x22342D5A -> 0x22040140`; NPC `0x22040163 -> 0x222A7B20`.

These static paths **do not imply every input reaches the final write**. Every layer has potential early returns and conditionals.

## 3. Exact health write

The shared `0x222A7B20` routine includes this sequence:

| VA | Verified instruction | Role |
|---|---|---|
| `0x222A7CD6` | `mov ecx, [esi+0x9C]` | Read old health |
| `0x222A7CE5` | `or ah, 0x0C` | Set x87 rounding-control bits to truncate toward zero |
| `0x222A7CEE` | `fldcw [...]` | Apply temporary rounding mode |
| `0x222A7CF2` | `fistp dword [...]` | Convert final floating damage to 32-bit integer |
| `0x222A7CFA` | `sub edi, edx` | Compute old HP minus integer damage |
| `0x222A7CFE` | `fldcw [...]` | Restore previous x87 control word |
| `0x222A7D08` | `call [eax+0x1B4]` | Notify a virtual state-change hook before write |
| `0x222A7D0E` | `mov [esi+0x9C], edi` | Write new health |

The write is skipped if the truncated integer amount makes no difference (`0x222A7CFC` comparison). The binary also contains the metadata field name `m_iHealth`.

**Pseudocode of this verified block only:**

```cpp
const int oldHealth = entity->m_iHealth;
const int integerDamage = trunc_toward_zero(finalDamageFloat);
const int newHealth = oldHealth - integerDamage;
if (newHealth != oldHealth) {
    entity->NotifyHealthChanged();  // virtual +0x1B4, exact name tentative
    entity->m_iHealth = newHealth;
}
```

The code prior to this block includes other calculations and dispatches; `finalDamageFloat` is not necessarily the original physics table result or raw impact damage. This is *behavioral pseudocode*, not original Ritual source.

## 4. Player armor: concrete evidence, incomplete total formula

`CBasePlayer`'s `+0xE0` handler at `0x2224C020` has a conditional armor path:

- At `0x2224C163`, it forms a pointer to `this+0xBC4` and at `0x2224C17D` reads its current integer value.
- The binary contains the field name `m_ArmorValue`.
- At `0x2224C1AF`, it multiplies part of its floating-point calculation by a literal `0.2f` stored at `0x223D9538`.
- The routine calls two generated integer-field update helpers at `0x2224C218` (`0x2224BB60`) and `0x2224C250` (`0x2224BBC0`); the latter performs subtraction from the field.
- The resulting damage-information payload is then passed to `CBaseCombatCharacter` at `0x2224C264`.

This verifies that armor is involved in player damage processing and that the `0.2f` scalar is part of the calculation. **It does not establish an unconditional 20% damage reduction**. Flags, an additional scaling value, and an armor-availability check affect the branch. Formal symbolic reconstruction and gameplay tests are still needed for numerical cases.

An additional special branch in `CBasePlayer` (`0x2224C07B–0x2224C0A0`) can set the health field to exactly `100` and returns early under specific flag and damage conditions. The meaning of those flag bits is not yet fully recovered; do not equate this branch with ordinary healing.

## 5. NPC and mutant-specific differences

The shared combat handler at `0x222A57C0` calls a virtual alive-damage method at `+0x370` (`0x222A5961`). This is why the major and minor can respond differently while retaining shared health arithmetic.

- **Major (`0x22379100`):** resolves the attacker handle, checks a virtual classification result against the integer `10`, tests damage flags and NPC state, may return without accepting damage, and otherwise forwards to mutant-base logic. The classification value's original enum name is not yet established.
- **Minor (`0x2237D0C0`):** performs a similar attacker classification check, but also branches on its state flag at `+0xC41` and can perform separate behavioral responses before forwarding to mutant-base logic.
- **Mutant base (`0x22342CB0`):** tests specific damage flags (`0x10000`, `0x400000`) and contains a special health write to `-5` for a conditional attacker/damage path before forwarding to `CAI_BaseNPC`'s alive-damage implementation. This unusual code is verified as present, but its purpose and practical trigger remain unknown.
- **NPC base (`0x22040140`):** forwards accepted damage to `0x222A7B20`, then performs additional AI-specific reaction work.

After the alive-damage routine completes, the combat handler checks `this+0x9C` (`0x222A5967`). If the health is no longer positive, a path invokes the class-specific virtual `+0xE8` handler (`0x222A598C`). The generic NPC handler sets `this+0x8C = 1`, matching the compiled metadata string `m_lifeState`. Major has its own death override at `0x22377750` which forwards to generic NPC death processing at `0x22377797`.

## 6. Where this fits the previous physics research

Previous findings (separately audited):

```text
physics collision
   -> table-based impact damage (0x22219290 / 0x22218E00)
   -> CTakeDamageInfo-like event construction
   -> deferred damage queue (0x22214460)
   -> post-simulation replay (0x222135F0)
   -> CBaseEntity::TakeDamage (0x2213F460)
   -> [THIS REPORT] target-specific +0xE0 handler
   -> class-specific +0x370 alive-damage handler
   -> +0x9C integer health subtraction
   -> if health exhausted, class-specific +0xE8 event/death handler
```

**Important boundary:** This is a verified *compatible chain of static functions*. It is not an instrumented runtime proof of the damage dealt by a specific prop launched by the major Recombinant.

## 7. Next reverse-engineering targets

1. Identify individual damage-flag bits and damage class enums in the original DLL, especially the two mutant early-return paths and the `-5 HP` branch.
2. Symbolically trace the complete player armor calculation and validate it on representative hit amounts/armor levels.
3. Decode the `+0xE8` event/death handlers, particularly the major's custom response and the minor's AI tasks near fatal damage.
4. Investigate NPC damage scaling/difficulty (`0x223A67A0` is directly called by the shared alive-damage routine) and compare with the already recovered dynamic difficulty system.

## Reproduce

```bash
python sin_original_damage_handlers_trace.py ORIGINAL_SERVER.DLL sin_original_damage_handlers_evidence.json
```

Expected output:

```text
PASS: 39 static checks; 9 RTTI-linked class vtables
```

Only descriptive metadata and verified addresses are emitted. No original executable data is copied into the output.