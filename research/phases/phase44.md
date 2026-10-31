> **Archival research record (phase44).** Transcribed unchanged below from `SiN_Phase44_Zoom_Input_Weapon_FOV_Reverse_Engineering(1).zip` (`README.md`). Its test and verification claims describe work performed in the original phase, NOT new tests run by this repository. Original disassembly excerpt files are not republished here; references to `evidence/`, `disassembly/`, or local scripts may point to material only in the original phase archive. All function addresses are build-specific. No original DLL or game asset is included.

---

# SiN Episodes → Quest 3, Phase 44: Iron-sight input and shared weapon FOV

**Scope:** the *released* 2006 *SiN Episodes: Emergence* game, extending Phases 37 and 43. This package reconstructs the verified link between `+zoom` input, an input button mask, weapon-script `ZoomFOV` and two player-FOV requests. This is static reverse engineering **not a running Quest/SourceVR mod**. Original proprietary DLLs are **not** distributed in the ZIP.

## Confirmed finding 1: zoom console commands drive a shared key-state structure

The original client DLL registers `+zoom` and `-zoom` at `0x2426B296` and `0x2426B2C6`, with command callbacks `0x24161DC0` and `0x24161DD0`. The callbacks operate on an input key-state structure starting at **`0x24368038`** with two owner slots and state flags at **`0x24368040`**. The `+zoom` path calls a generic key-down routine at `0x24160F40`, which sets the active/impulse flags (`OR 3`). The release routine clears the appropriate holder and flags when the remaining owner lets go.

In the user-command builder, `0x24162695` tests the button-state flags; `0x2416269D` ORs **`0x00080000`** into the outgoing button mask when active. **This ties the original zoom command to a dedicated input button**, not Magnum secondary fire.

The standalone `ZoomKey` model preserves the observed two-holder/impulse approach, but deliberately omits unverified engine edge cases (missing key IDs, warnings, reset-on-frame quirks).

## Confirmed finding 2: the three shipped firearms share a virtual `ZoomFOV` getter

Original `server.dll` reads the literal `ZoomFOV` during weapon-configuration parsing at `0x222B4906`; it writes the resulting integer into its weapon-info-like data structure at offset **`0x1BC`** (`0x222B4918`). The shared getter at `0x222AC030` obtains the current weapon-info pointer using the weapon's `+0x51C` field and returns the 32-bit `+0x1BC` value.

The RTTI/vtables give stronger proof of reuse: virtual **slot 272**, offset **`+0x440`**, points to the same **`0x222AC030`** function for:

| Original class | Primary vtable | Slot 272 target |
|---|---|---|
| `CBaseHLCombatWeapon` | `0x22459194` | `0x222AC030` |
| `CWeaponMagnum` | `0x2247875C` | `0x222AC030` |
| `CWeaponAssaultRifle` | `0x224776AC` | `0x222AC030` |
| `CWeaponScatterGun` | `0x2247A1E4` | `0x222AC030` |

The virtual mapping and getter body are confirmed; the friendly **`GetZoomFOV`** name in our C++ reference is descriptive, not a recovered symbol.

## Confirmed finding 3: explicit zoom-in and zoom-out FOV requests

The weapon-side helper at **`0x222CBB60`** contains an active-state guard at weapon offset `+0x578`, uses virtual slot `+0x440` to retrieve the weapon's zoom FOV and calls the player-side FOV routine **`0x2224E6F0`** with a **`0.25` second** transition (`0x222CBB88`); on success it sets `weapon+0x578 = 1`.

The companion helper at **`0x222CBBC0`** requests **target FOV = 0** with a **`~0.15` second** transition (`0x222CBBE6`) and clears `weapon+0x578` at `0x222CBC11` if the rest of its conditions are met. Target `0` is a sentinel in this call, **not a claim that the camera has literally zero-degree FOV**.

A larger weapon-side function at **`0x222CCD40`** branches on zoom eligibility and current state, sometimes calling the enable/disable helpers. The precise active state, user input conditions, animations and all three gun-specific pose transforms are **not** fully established. The observed transitions cannot yet be unconditionally assigned to every weapon in all gameplay contexts.

## Relationship to Phase 43 networking

SiN's existing replicated `m_bZooming` and `m_bZoomScope` fields remain independently registered in both original DLLs (Phase 43). **This phase has not identified the exact server-side writes of these fields or proven which is toggled by which branch.** The key-state input, weapon FOV and network booleans must remain independent layers in our reference.

## Implementation implications for Quest 3

- Preserve `+zoom` / `-zoom` as their own user-command input; do not wire them to Magnum DUAG alternate fire or the quick-grenade input.
- Read a per-weapon `ZoomFOV` parameter through a shared weapon interface; avoid inventing fixed numerical FOVs for the three guns until the actual weapon scripts are examined.
- Preserve the observed aim-in and aim-out *requested transition times* as initial compatibility defaults; respect engine-side FOV rules and ownership/eligibility checks.
- **VR-specific design, not original behavior:** let physical weapon sight alignment use tracked controller poses. The flatscreen FOV transition may need to be reduced or disabled for comfort; do not narrow the VR headset camera as an unquestioned port of the original PC behavior.

## What remains unresolved

1. Exact player-side `m_bZooming` and `m_bZoomScope` writers and when each flag becomes true or false.
2. Per-weapon sight model movement/animation, ADS sway, zoom overlay and ironsight camera alignment.
3. Original numerical `ZoomFOV` values from each shipped weapon script and any script overrides.
4. Whether the generic zoom FOV transition helper applies to each firearm in all gameplay situations, and the edge-case behavior of weapon switch/sprint/reload interruptions.
5. Precise Magnum reload, DUAG reload and ADS timing interaction.

## Reproduce (on your own original DLLs)

```sh
python3 phase44_verify.py /path/to/original/bin.zip --output evidence.json
c++ -std=c++17 -O2 -Wall -Wextra -Werror -Isrc tests/test_zoom_fov_contract.cpp -o /tmp/sin_zoom_test
/tmp/sin_zoom_test
```

The verifier checks two original SHA-256 digests, **22 fixed-address instruction signatures**, literal command/script strings, four identified class RTTIs and virtual slots, and FOV float constants. The C++ model passed **61 assertions**. The distributed code is a **reference contract**, not SourceVR-integrated gameplay code. All addresses refer to the **original x86 Windows modules**, not Android offsets.
