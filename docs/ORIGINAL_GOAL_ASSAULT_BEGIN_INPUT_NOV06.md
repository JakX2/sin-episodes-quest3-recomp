# Original SiN Episodes 2006: ai_goal_assault BeginAssault input and actor cue fanout

**November 6, 2026 | Direct x86 original server.dll and original SinEpisodesSDK**

The owner-provided original `server.dll` SHA-256 is
`568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`.
The analysis checked the original unmodified PE32 game DLL locally, and
corroborated editor-facing inputs against the owner's original
`SinEpisodesSDK/bin/SinEpisodes.fgd`. The original commercial
game DLL and all game assets remain outside GitHub.

## The previously missing bridge is now proven

The original `ai_goal_assault` is a genuine server entity
(factory registration previously recovered in PR #113).
Its original 52-byte `typedescription_t` entries identify:

| Goal entity original member | Original x86 object offset | Hammer key or input |
|---|---:|---|
| `m_RallyPoint` | `+0x3C8` | `rallypoint` |
| `m_AssaultCue` | `+0x3CC` | `AssaultCue` |
| `InputBeginAssault` | Input handler `0x220CFF30` | `BeginAssault(void)` |

These bindings are backed by **both** the actual native data-map
descriptor words and the exact original C-string literals.
The SDK defines `AssaultCue` values: **1 = Entity System Input**,
**2 = Gunfire**, **3 = Don't wait for a cue**.

### Original goal setup: configuring each actor

An original goal-entity virtual-table entry at `0x223F5280`
points to the helper at **`0x220CFF00`**. The verified
helper resolves an NPC's `CAI_AssaultBehavior` through
`0x220CDC80`, then passes the goal's `m_RallyPoint`
and `m_AssaultCue` to original
**`CAI_AssaultBehavior::SetParameters`** at `0x220CFC00`.

Its exact inherited C++ method name is not independently
established, so this note calls it a *goal actor setup hook*,
not a definitively named `Activate` implementation.
The native behavior call may reject or fail to find a valid rally
point, as earlier decompilation established.

### Original `BeginAssault`: delivers cue 1 to actors

The real `InputBeginAssault` at **`0x220CFF30`**
works across the goal's actor list, rather than writing one
global flag:

1. Checks/gathers the goal actor list, using original list state
   flags at `+0x3C4`; iterates its actor count `+0x3B8`.
   The indexed actor resolver at **`0x220CF120`** reads
   the actor handle array at **`+0x3AC`**.
2. For each valid actor, resolves its actual
   `CAI_AssaultBehavior` via **`0x220CDC80`**.
   Unresolved actors/behaviors are skipped without stopping
   the remaining actors' iteration.
3. If the behavior's owning NPC pointer is non-null, resets two
   original NPC fields at **`+0x864`** and **`+0x85C`**
   to zero. Their precise original member names have **not**
   been recovered.
4. **Writes `m_ReceivedAssaultCue = 1`** in that
   actor's `CAI_AssaultBehavior` at behavior `+0x20`,
   through the original instruction at **`0x220CFFA0`**.
5. Calls the original NPC condition setter **`0x22039760`**
   with numeric condition **`0x18`**, at
   **`0x220CFFAC`**.

The precise name of NPC condition `0x18` remains unproven.
The input does not check the configured cue mode before writing
`m_ReceivedAssaultCue=1`. It is the separately recovered
`CAI_AssaultBehavior` cue helper at `0x220CEA90` that
interprets the configured mode.

Therefore, a goal configured with **mode 1** can remain in
`TASK_AWAIT_CUE` until `BeginAssault` delivers value 1.
A goal configured with **mode 2** still queries NPC
condition `0x33` for its gunfire cue; the received value
1 does not override that separate condition. Mode 3 writes
received value 3 and succeeds without waiting.

### Important: the direct MobGrunt input is a different path

The earlier original `CNPC_MobGrunt::InputAssault` branch
at `0x22357E50` passes integer **3** and a named
rally point into the same `SetParameters` routine
through `0x22357E8B`. In contrast, an
`ai_goal_assault` uses the **map-configured cue mode**
and exposes **`BeginAssault`** to signal its list of
actors.

This distinction is essential for faithful finale scripting:
`Assault` on an individual MobGrunt should not be
blindly treated as synonymous with `BeginAssault` on an
AI goal entity. The original rooftop relay's
`Assault` output for `roof_grunt_2_seq1` is a
direct-NPC input and does not imply a goal entity was
required in that map.

## Machine-code verification

`research/tools/audit_sin_goal_assault_begin_input.py`
is a read-only original-DLL auditor verifying:

- The three exact native 52-byte typedescription records and
  their original Hammer/SDK labels.
- The original goal setup virtual slot `0x223F5280`
  and its function pointer.
- **24** short x86 instruction witnesses for actor iteration,
  cue setup, per-actor NPC field clearing, the `received=1`
  write, and condition `0x18` signaling.
- **Eight** exact native relative `CALL` destinations,
  including actor lookup, behavior resolution, goal actor setup,
  NPC condition setting, and the direct MobGrunt input call.
- Optional validation against the original `SinEpisodes.fgd`.

These exact expected bytes, the native object member names and
cross-checks were also checked against the actual owner-provided
original 2006 `server.dll` and SDK archive in this session.
All audit failures are explicit and fail closed.

`research/tests/test_audit_sin_goal_assault_begin_input.py`
provides synthetic fixtures and deliberate corruptions of
handler pointer, field offsets, cue state value, actor array,
NPC condition argument, setup hook and call targets.

Reproduce the verification with the original files:

```powershell
py research\tools\audit_sin_goal_assault_begin_input.py --server "D:\SiN\bin\server.dll" --sdk-zip "D:\SinEpisodesSDK.zip" --output "D:\SiN_Test_Only\goal_begin_assault_audit.json"
```

Change paths as needed. The report is written only as a **new**
file, never over an existing report, game DLL or BSP.

## Portable cue adapter and actual limits

`include/sin_compat/original_goal_assault_input.hpp`
connects this verified goal-input behavior with the prior
`OriginalAssaultCueGate` (PR #122), without pretending
that engine actor handles or native NPC fields are available.
It provides two separate host-facing operations:

- `configure_goal_assault_actor`: passes the original
  `rallypoint` name and cue mode to a host-provided
  `SetParameters`-equivalent callback, updating the
  portable cue only when that callback succeeds.
  This success Boolean is **new host adapter design**,
  not a claim about the original C++ method's return ABI.
- `begin_goal_assault`: iterates resolved actors,
  requests reset of the two native NPC state fields through
  a host callback, writes received cue 1 for that actor,
  then requests condition ID `0x18` through another
  callback. Skipping actors with an unresolved owning NPC
  is a new safety guard and does **not** prove the original
  DLL's behavior for a malformed behavior object.

`tests/test_original_goal_assault_input.cpp` verifies
per-actor success/failure, multiple actors, ordered effects,
missing actor and behavior, mode 1/2/3 distinction,
repeat inputs, and a simulated hold-rally schedule whose
`TASK_AWAIT_CUE` becomes complete after a delivered
`BeginAssault` cue.

These are not original NPC objects, game-time callbacks,
real Source entity I/O, or native Quest 3 gameplay.
The standalone SourceVR-compatible game module and original
NPC combat still require implementation.

## Read-only static BSP I/O verification extension

The earlier `research/tools/analyze_sin_assault_graph.py`
has been updated to recognize the original `BeginAssault`
input **independently** from the MobGrunt `Assault` input.
The report now includes `original_goal_begin_assault_signals`
and `directed_begin_assault_inputs`, plus class-specific
resolution statuses for the recipient goal entity.

A `BeginAssault` connection pointing to `npc_combine_s`
or original `npc_druglab_grunt_assault_rifle` is reported
as a wrong-class target (the original map input belongs to
`ai_goal_assault`). Duplicate targetnames are reported as
ambiguous. Dynamic `!activator`-style targets are not falsely
reported missing. The tool does not rewrite any entity.

The new synthetic test cases cover both original input types,
valid and missing goal targets, wrong-class/duplicate goal
targets, and ESC-separated Hammer outputs. This check **has
not been run against the original finale BSP** in this session.

## Next steps

1. Investigate original input/entity behavior that writes
   `m_ReceivedAssaultCue=4`, which the original cue
   predicate treats as an unconditional success override.
2. Trace original condition ID `0x18` and the two
   NPC state fields `+0x864`/`+0x85C` to authoritative
   native enum and data-map symbols, not guesses.
3. **Static BSP preflight now implemented:** the existing
   `research/tools/analyze_sin_assault_graph.py` separately enumerates
   `BeginAssault` outputs, checks that named recipients are actual
   `ai_goal_assault` entities, and flags missing, wrong-class or
   ambiguous recipients. It keeps runtime `!activator`-style targets
   labeled dynamic rather than falsely claiming they are missing.
   The original direct NPC `Assault` analysis is preserved separately.
   Tests use synthetic BSP entity lists; the user's original finale
   BSP was **not** available to run the new check in this session.
   The next task is to verify the original finale map and report any
   real map-level mistakes without changing the BSP.
4. Build a real native game-server adapter with
   engine-compatible SourceVR AArch64 registration,
   actor iteration, I/O, nav, cues and model activities.

No code here installs or patches the user's game.
