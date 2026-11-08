# SiN Episodes: Emergence (2006): Jessica's exact player collision-center target

**Phase 106, November 8, 2026.** Independently authored reconstruction from the owner's unchanged original Windows x86 `server.dll` (SHA-256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`). The original binary and copyrighted game assets stay private. This phase does NOT recreate the full Source engine or execute SiN on Quest.

## New direct binary evidence

Jessica's item-throw animation handler at **0x22362420** calls a player virtual method at **0x22362509**, through virtual-table byte offset **+0x214**. The method takes no explicit arguments and returns an address of a 3-float vector. Original MSVC RTTI complete-object locators and vtables independently confirm that three classes have an identical `+0x214` target:

| RTTI class | Vtable VA | CompleteObjectLocator VA | Slot +0x214 target |
| --- | --- | --- | --- |
| `CBasePlayer` | `0x2245222C` | `0x224C8498` | `0x2214BD40` |
| `CHL2_Player` | `0x224AEC3C` | `0x224D3FF0` | `0x2214BD40` |
| `CNPC_Jessica` | `0x22497A14` | `0x224D224C` | `0x2214BD40` |

The `CBasePlayer` and `CHL2_Player` entries are especially important because Jessica resolves a **player**, not herself, before issuing the virtual call. The Jessica entry is corroborating inheritance evidence, not proof that she calls her own method.

Disassembly of `0x2214BD40..0x2214BD89` loads `this+0xFC` (the embedded collision property), allocates a temporary vector from an indexed **128-element, 12-byte-per-entry scratch array**, calls **0x2214B020** to compute the local collision bounds midpoint and **0x2214B0D0** to convert it to world coordinates. The scratch vector is returned by pointer and should be copied promptly rather than retained across subsequent calls. The scratch array's runtime contents are not available in the file-backed PE; the code's indexing and pointer arithmetic are directly verified.

Original midpoint helper `0x2214B020..0x2214B0C1` reads collision bounds at `CCollisionProperty+0x08` and `+0x14`, and uses `0.5f` from binary VA `0x223D9778` to calculate:

```
local_center = mins + (maxs - mins) * 0.5
```

The original conversion helper `0x2214B0D0..0x2214B16D` selects a translation-only path when **collision flag bit 0x40** is set, the numeric solid type is **0 or 2**, or the absolute orientation is zero. Otherwise it uses the entity-to-world transform (the original virtual transform method at byte slot `+0x28`, followed by vector/matrix transform at **0x22268B20**).

This corresponds strongly to Source SDK **`WorldSpaceCenter()`**, whose documented purpose is to return the world-space center of an entity's collision OBB. The name comes from comparison with Valve's published Source SDK, not a symbol retained in the stripped SiN DLL. Do **not** confuse the computed point with EyePosition, eye-height view offset, model pelvis bone or bbox origin. The Source SDK implementation and vtable layout may differ between versions, so all addresses above come strictly from the *original SiN* binary.

### Evidence / reproducibility

- `research/tools/audit_sin_original_jessica_target.py` is a read-only, SHA-pinned PE32 verifier. It checks **three complete original function digests, three RTTI type-descriptor/class/vtable chains, three original relative call targets, twelve instruction anchors and the original 0.5f constant**.
- `research/tests/test_audit_sin_original_jessica_target.py` uses **synthetic** PE fixtures to guard malformed sections, wrong branch opcode, bad pointer bounds and SHA mismatch. Neither file includes proprietary binaries.

```
python3 research/tools/audit_sin_original_jessica_target.py --server /private/original/server.dll
```

### Independently authored C++17

- `include/sin_compat/original_jessica_target_center.hpp`
- `src/original_jessica_target_center.cpp`
- `tests/test_original_jessica_target_center.cpp`

The pure `compute_player_world_collision_center()` reference accepts **host-owned observations** (collision OBB, world origin/orientation, flags/type, and optionally a genuine `entity_to_world` 3x4 matrix). It implements the compiled branch distinction and returns a copy of the world position rather than a pointer into original engine scratch memory. `build_original_jessica_toss_geometry()` combines the computed target, Jessica's right-hand attachment world position and live gravity. Unit tests cover asymmetric OBBs, translation, 90° rotation, force-translation flag, solid modes 0/2, missing transform, nonfinite data and a cross-check with the Phase 104 ballistic solver that reaches the reconstructed center.

Safety checks for finite values and missing host-provided matrix are **new**; no claim is made that the 2006 x87 executable performed the same checks. The numeric solid-type and flag decisions are verified, but this phase does not assign speculative symbolic enum/flag names.

### Remaining uncertainties / next engine gate

- Confirm the exact returned vector's lifetime under the original 128-slot ring and all possible player subclasses.
- A **native host** must supply collision dimensions, correct world transform, current gravity and the real `CNPC_Jessica` attachment. There is no live engine entity here.
- The standalone math code is a reference, not native `IServerGameDLL`, a replacement for `client.dll`, a Quest 3 APK or a playable game map.
- Next narrow analysis opportunity: finish the engine's `0x22141A00` absolute-velocity setter and `0x2213D750` dirty-velocity refresh, including parent propagation, before assuming SourceVR ABI compatibility.
