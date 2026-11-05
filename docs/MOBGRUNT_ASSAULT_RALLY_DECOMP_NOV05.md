# Original SiN MobGrunt assault rendezvous and grenade-input control flow

**Nov 5, 2026, original 2006 game source evidence.** This is a repeatable read-only decompilation audit against the owner's hash-pinned Windows x86 `server.dll`, supplemented by the **original SiN Episodes SDK FGD**. It is not native Quest 3 gameplay or recovered complete C++.

## Runtime source observations

**`CNPC_MobGrunt::InputAssault`, original VA `0x22357E50`:**
- Reads a typed input variant and uses a string conversion path (`0x221561D0` when necessary).
- Resolves the resulting name through a string/token conversion call (`0x221F3DE0`).
- Passes it and integer `3` to a behavior subobject at **MobGrunt + `0xDB8`**, with a call at `0x22357E8B` to **`0x220CFC00`**.
- A diagnostic string referenced by the destination identifies it as **`CAI_AssaultBehavior::SetParameters`**, and a nearby original error says a named **rally point** could not be found.

The original SDK's `base.fgd` supplies the authoritative map-input description:

> `input Assault(string)`: “Start an assault. Parameter passed in should be the name of the rally point.”

The SDK's `SinEpisodes.fgd` also declares an `ai_goal_assault` `AssaultCue` choice with **3 = “Don't wait for a cue.”** This strongly corroborates the machine-code argument `3`, but does not independently establish that every path uses the identical enum or that the NPC immediately enters combat without other conditions.

**Original map-assault entity definitions from `SinEpisodes.fgd`:**

| Classname | Relevant fields |
|---|---|
| `assault_rallypoint` | `assaultpoint`, `assaultdelay`, `rallysequence`, `priority`, `OnArrival` |
| `assault_assaultpoint` | `assaultgroup`, `nextassaultpoint`, `assaulttimeout`, `clearoncontact`, `allowdiversion`, `nevertimeout` |
| `ai_goal_assault` | `actor`, `rallypoint`, `AssaultCue`, `Activate`, `Deactivate`, `BeginAssault` |

The **three assault classes are registered in the actual 2006 `server.dll`**, not merely declared as editor-side types:

| Native entity class | Registry thunk VA | Factory object VA |
|---|---|---|
| `assault_rallypoint` | `0x223B9190` | `0x224E4BB8` |
| `assault_assaultpoint` | `0x223B91B0` | `0x224E4BBC` |
| `ai_goal_assault` | `0x223B91F0` | `0x224E4BC0` |

All three 22-byte x86 thunks call original registry getter `0x2228B110` and register a distinct classname/factory object. The verifier checks the complete instruction shapes and target C strings, rather than treating string presence as registration proof.

This is how the original map's assault-order logic is intended to connect to AI navigation and rally points. The user's desktop HL2 test verified the map-defined `roof_grunt_1_seq1` with temporary `npc_combine_s` and `weapon_ar2`; **it did not verify Ritual's native rally-point schedule**. The original relay `begin_roof_battle_seq1` sends an `Assault` input naming `seq1_assualt_point_start` to `roof_grunt_2_seq1`, not directly to the converted grunt #1. Before implementing that encounter, inspect this exact target's BSP classname and whether it is a rallypoint or assaultpoint, without silently correcting the original spelling.

## Decompilation of rally-point candidate ranking

Inside original `CAI_AssaultBehavior::SetParameters` at `0x220CFC00`, the server iterates candidate rally entities. The initial best-priority sentinel is `-1` (`0x220CFC62`). At `0x220CFCA1` it reads each candidate's integer field **`+0x3A0`**, compares it with the best priority and keeps the higher value. When priorities match, the function measures squared three-dimensional distances using the owner's and candidate's position fields at **entity `+0x2BC`**; the x87 compare and conditional replacement are visible at `0x220CFDD7`. Equal priority and equal distance retain the earlier candidate. On success, it stores a selected entity handle in the assault-behavior subobject at **`+0x18`**. On failure the original diagnostic is `%s Didn't find a best rally point!`.

This is consistent with the original SDK FGD's `assault_rallypoint.priority` field, which explicitly says **“Higher priority rally points get selected first.”**

**New direct class-data confirmation:** the `CAssaultRallyPoint` original 52-byte `typedescription_t` records establish these exact fields:

| Rally-point member | Original object offset | FGD/Hammer name |
|---|---:|---|
| `m_AssaultPointName` | `0x394` | `assaultpoint` |
| `m_RallySequenceName` | `0x398` | `rallysequence` |
| `m_flAssaultDelay` | `0x39C` | `assaultdelay` |
| `m_iPriority` | `0x3A0` | `priority` |
| `m_OnArrival` | `0x3A4` | `OnArrival` |
| `m_hLockedBy` | `0x3BC` | Internal entity handle |

Crucially, `SetParameters` checks the original `m_hLockedBy` handle at **`0x220CFC70`** before ranking. If the handle refers to an existing entity, the candidate is skipped. This resolves the main rally-point eligibility check using **both the real data map and original x86 handle-lookup branch**. The standalone C++ reference takes `eligible` as host-supplied because only a real Source engine can resolve a live entity handle.



Readable reconstruction for valid, eligible rally candidates:

```cpp
best = none;
for (candidate : rally_points_with_matching_name) {
    if (!candidate_is_eligible(candidate)) continue; // skip locked point
    if (!best || candidate.priority > best.priority ||
        (candidate.priority == best.priority &&
         squared_distance(candidate, npc) < squared_distance(best, npc))) {
        best = candidate;
    }
}
```

The ranking instruction witnesses are now checked by the original-DLL audit. A standalone **engine-neutral C++17 reference** implements only this behavior in `include/sin_compat/mobgrunt_rally_rank.hpp`, tested by `tests/test_mobgrunt_rally_rank.cpp` and CTest. It does not resolve actual engine entity handles or implement the original candidate discovery, AI navigation, or `CAI_AssaultBehavior` schedules. It rejects invalid coordinates and negative priority instead of copying x87 corner cases into the future ARM64 code.

## Original assault-point chaining, timeout and diversion data map

The native **`CAssaultPoint`** class (map classname `assault_assaultpoint`) has separate 52-byte original x86 `typedescription_t` entries:

| Original member | 2006 x86 offset | Hammer key / input |
|---|---|---|
| `m_AssaultHintGroup` | `0x394` | `assaultgroup` |
| `m_NextAssaultPointName` | `0x398` | `nextassaultpoint` |
| `m_flAssaultTimeout` | `0x3B4` | `assaulttimeout` |
| `m_bClearOnContact` | `0x3B8` | `clearoncontact` |
| `m_bAllowDiversion` | `0x3B9` | `allowdiversion` |
| `m_bNeverTimeout` | `0x3BA` | `nevertimeout` |
| `m_OnArrival` | `0x3BC` | `OnArrival` |

Two genuine original scripted input handlers are now decoded:
- `InputSetClearOnContact` at `0x220CDE70`: checks variant type `6` and sets boolean **`this+0x3B8`** from the input, or resets it to zero when the type differs.
- `InputSetAllowDiversion` at `0x220CDEA0`: same pattern, writing boolean **`this+0x3B9`**.

These fields independently match the shipped FGD definitions of assault-point progression and conditional behavior. The native server binary verifies the **data and input behavior**, but the exact use of `m_flAssaultTimeout`, conditions for clearing a point, and transition to `m_NextAssaultPointName` remain to be traced. Do not interpret the existence of the field as proof of a finished Quest AI schedule.

## Grenade-related toggle and timing, original server x86

The original `InputThrowGrenades` handler at `0x22357EA0` checks the variant type against **6**, resets member **`+0xC70`** (`m_flGrenadeHeight`) to zero and writes the input byte to **`+0xC6C`**, clearing both fields for another variant type.

Two original MobGrunt virtual/local accessors use **the same flag** to choose floating-point values, verified from original x86 instructions and the precise original literals:

| Routine | `+0xC6C` is nonzero | `+0xC6C` is zero |
|---|---:|---:|
| `0x22357FA0` | `0.25f` | `1.0f` |
| `0x22357FC0` | `2.5f` | `6.0f` |

Another original MobGrunt initialization/update path at `0x2235F3DA` checks this flag, selects the first pair of constants, adds the chosen value to a global game-time field, and stores the result in **this + `0xC80`**. The same setup sequence later resets flag `+0xC6C`. This demonstrates actual use of the conditional values in time-related state, **not** a proof that the two accessors correspond to grenade fuse, throw cooldown, recoil, or damage.

The original DLL also contains distinct `InputLookOn` / `InputLookOff` handlers. Their full AI consequences and `CNPC_EliteGrunt` equivalents remain to be decompiled.

## Reproduce against owner's original game files

Use `research/tools/audit_sin_mobgrunt_assault_rally_timing.py`, which rejects changed original server DLL hashes/instructions and can optionally cross-check both original SDK FGDs:

```powershell
py research\tools\audit_sin_mobgrunt_assault_rally_timing.py --server "D:\SiN\bin\server.dll" --sdk-zip "D:\SinEpisodesSDK.zip" --output "D:\SiN_Test_Only\mobgrunt_assault_rally_audit.json"
```

Adjust the SDK ZIP path to the actual owner's folder. Output is read-only evidence in **a new file only**; the game DLL and SDK ZIP are never modified or redistributed.

Synthetic regression tests under `research/tests/test_audit_sin_mobgrunt_assault_rally_timing.py` exercise expected code shapes, calls, conditionals, original FGD semantics, and fail-closed altered instruction values. These tests run in `.github/workflows/sin-original-decomp-tests.yml`. Independently, the researcher also executed the equivalent byte-level audit **against the supplied original 2006 `server.dll`** and inspected the original SDK archive locally. Synthetic CI cannot substitute for an owner's original-asset check.

## Next code reconstruction step

Trace `CAI_AssaultBehavior::SetParameters` further through its rally-point search and schedule selection, then reconstruct the map-aware `InputAssault` behavior using a legally buildable Source-compatible native game-server module. Preserve the original distinction among `CNPC_MobGrunt`, `CNPC_EliteGrunt`, the original `weapon_assault_rifle` and `grenade_ar` factories. The stock SourceVR content-only mod loader cannot load replacement server-game DLL code.
