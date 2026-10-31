> **Archival research record (phase45).** Transcribed unchanged below from `SiN_Phase45_Zoom_Decision_Model.zip` (`sin_phase45_zoom_control/README.md`). Its test and verification claims describe work performed in the original phase, NOT new tests run by this repository. Original disassembly excerpt files are not republished here; references to `evidence/`, `disassembly/`, or local scripts may point to material only in the original phase archive. All function addresses are build-specific. No original DLL or game asset is included.

---

# SiN Episodes → Quest 3: Phase 45 (evidence-bounded zoom decision logic)

## Provenance and limitation

This analysis builds **only** on the disassembly excerpts shipped in the user's Phase 44 research ZIP. The original `server.dll` and `client.dll` are NOT present in the current chat/container, and the shared ChatGPT conversation URL was inaccessible. This package does **not** assert fresh binary discoveries, new executable hash checks, recovered asset coordinates, or a running Quest VR port.

## What the excerpts support

1. **Same zoom bit on both ends.** `client.dll` at `0x2416269D` ORs `0x00080000` into user-command buttons. `server.dll` at `0x222CCDCB` tests byte `player+0x93E` against `0x08`. In little endian, that is bit 19 of the 32-bit word starting at `player+0x93C`, i.e. `0x00080000`. This is a strong correspondence, but the *exact member identity* at `player+0x93C` needs tracing.
2. **Zoom eligibility.** Inside `0x222CCD40`, the player pointer from `0x222AD370` must be non-null and the player's virtual `+0x120` must return true. Eligibility is computed as `(weapon byte +0x4AC == 0) && (player virtual +0x3C8 returns false)`. Names for both prerequisites are unknown.
3. **Interrupt and restore.** If the weapon's `+0x578` zoom-active byte is true and eligibility fails, `0x222CCDC0` is invoked and `weapon+0x579` set to one. If the weapon is inactive, eligible and `+0x579` is set, `0x222CBB60` is invoked and `+0x579` is cleared. That re-entry path is reached **without** checking the zoom input bit.
4. **Regular toggle.** When eligible and not taking the automatic restoration branch, the `0x80000` player-field bit toggles `0x578` by invoking the enter or exit helper. The disassembly snippet alone does not establish whether that bit indicates a press-edge, a held state, or whether the containing function is invoked on an edge-only schedule. Treat it as an *event-qualified signal*, not proof of hold-to-ADS.
5. **An animation/activity hook.** The zoom-enter helper calls virtual offset `+0x314` (x86 slot 197) with integer `0xC0`. Zoom-exit calls that slot with `0x9D` **only if** weapon field `+0x4BC == 0xC0`. This strongly suggests animation/activity handling; the names of the IDs and actual weapon poses are **unknown**.
6. **FOV contract unchanged from Phase 44.** Enter uses per-weapon virtual offset `+0x440` (x86 slot 272) for `ZoomFOV`, requests a 0.25-second FOV change; exit requests target 0 sentinel over ~0.15 seconds. The helper sets/clears `+0x578` even on some paths without a successful player FOV request.

### Observed decision priorities

| Priority | Preconditions | Effect in excerpt |
|---|---|---|
| 1 | No player / player virtual `+0x120` false | No change within shown portion |
| 2 | Zoom active and no longer eligible | Disable zoom; mark restore pending |
| 3 | Zoom inactive and still ineligible | No change within shown portion |
| 4 | Zoom inactive, eligible, restore pending | Enter zoom; clear restore pending, regardless of zoom bit |
| 5 | Eligible and bit `0x80000` present | Toggle zoom |
| 6 | Otherwise | No zoom transition |

**Important scope boundary:** This models control flow through `0x222CCDE6`. The supplied excerpt ends shortly after that, and downstream logic is not analyzed here. Original engine calls can also have effects not represented by the standalone model.

## Quest 3 port implications

- Separate **controller-grip physical aim alignment**, **gameplay ADS permission/state**, and **camera FOV**. Keep HMD projection unchanged for comfort while handling SiN zoom state independently.
- Track temporary ADS interruption and auto-restoration only where original conditions permit. Make the controller pose the source of physical muzzle/sight alignment.
- Keep DUAG/secondary firing and quick grenades separate from the zoom input bit.
- Do **not** assign `0xC0` / `0x9D` to named Source activity enums or specific weapon animations without verifying against vtable targets and model sequence files.

## Remaining investigation with original DLLs and game assets

1. Find callers of `0x222CCD40` to establish its frequency and which player button member lives at `+0x93C` (edge or held).
2. Resolve weapon vtable slot **197** targets for Magnum, assault rifle and Scattergun; then trace the implementation and activity tables for `0xC0` / `0x9D`.
3. Inspect complete viewmodel/weapon pose client code, sequence descriptors and weapon scripts for per-gun sight transforms and actual `ZoomFOV` numbers.
4. Track server writes to `m_bZooming` and `m_bZoomScope` and weapon-switch/reload edge cases.

## Reproduce this package's local checks

```sh
python3 verify_excerpts.py
c++ -std=c++17 -O2 -Wall -Wextra -Werror -Isrc tests/test_zoom_transition_model.cpp -o /tmp/sin_zoom_phase45_test
/tmp/sin_zoom_phase45_test
```

`verify_excerpts.py` checks captured *text*, not original binary bytes. For a real binary check, run the Phase 44 verifier separately with the original matching `bin.zip`.
