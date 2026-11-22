# Gameplay Phase588: original Jetpack Grunt activity override and exact class RTTI

## Native original 2006 DLL identity

Original owner-private **SiN Episodes: Emergence** `SE1/bin.zip` has the unchanged original SHA256 identity:

- `server.dll`: `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`.
- `client.dll`: `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.
- Original PE32 preferred server image base: `0x22000000`.

This phase follows Phase587's `CNPC_GruntBase` activity virtual **`+0x338 → 0x2235A250`** into an original Jetpack-specific override. The original binary is not redistributed. Generic HL2 physics/surfaces/models/props/rendering and Windows/Android/Quest boot are excluded.

## One complete native method, 274 new code/table bytes

- **`0x22364700–0x22364811`, 274 original bytes**, whole-body SHA256 **`b9d06b726a6036fba8a32cafc3bcb4b5c1d762483b7d26aae226a66a7a113173`**.
- The original method includes executable x86 and two in-body data tables: three original DWORD target pointers at **`0x223647C4–0x223647CF`** (12 bytes) and **66 one-byte activity-to-case selectors** at **`0x223647D0–0x22364811`**. The byte at `0x22364812` is authentic `INT3` padding.
- The sole original direct `E8 rel32` call in this override at **`0x223647B7 → 0x2235A250`**, into the Phase587-original Grunt activity converter. Native indirect call through original `0x2251B1C4` resolves to the original x87 square-root function **`0x222680E0`**.
- The previously recovered complete **326-byte** original `0x2235A250` is SHA-authenticated again as a crossdependency and **not counted as new bytes**.

## Eight exact original class identities, not guessed names

The original PE32 `.rdata` primary vtable has a pointer to an MSVC RTTI Complete Object Locator at **`vtable[-4]`**, whose original `+0x0C` points to a TypeDescriptor with a mangled name at `+0x08`. For each of the following original vtable addresses, the auditor checks **both native virtual slots `+0x338` and `+0x33C`**, and the exact NUL-terminated original RTTI name.

| Original RTTI class | Native primary vtable | Actual `+0x338` activity method |
|---|---|---|
| `CNPC_DruglabJetpack` | `0x2248C514` | `0x22364700` |
| `CNPC_EliteJetpack` | `0x2248E83C` | `0x22364700` |
| `CNPC_JetpackGruntBase` | `0x2249964C` | `0x22364700` |
| `CNPC_SintekJetpack` | `0x224A4A4C` | `0x22364700` |
| `CNPC_EliteGrunt` | `0x2248DF04` | `0x2235A250` |
| `CNPC_GruntBase` | `0x224954A4` | `0x2235A250` |
| `CNPC_MobGrunt` | `0x2249AAA4` | `0x2235A250` |
| `CNPC_SintekGrunt` | `0x224A416C` | `0x2235A250` |

All eight independently share the already authenticated `+0x33C` entity-handle activity translator **`0x222A53E0`**.

This is **original game class/virtual ownership evidence**, not an inference based solely on HL2 SDK types or Source-named activity labels. It does not by itself prove the registered entity factory names in maps.

## Jetpack override exact original code-path conditions

The method executes its attachment/vector prefix **before the activity switch**, including for `ACT_RELOAD` or delegated fallback. It reads original Grunt owner raw handle **`+0x1024`**, resolves via the original 16-byte serial slot table global at `0x224F5620` and reads three float coordinates of the attached entity at **`+0xC18/+0xC1C/+0xC20`**. Original code squares, sums and takes the x87 sqrt of that three-vector; a float distance is compared against the literal **`50.0f`** at original `.rdata` **`0x223D5830`**.

The 66-entry original jump-index table maps **only four specific shared activity IDs** to special branches. Other in-range IDs, and all out-of-range IDs, call the **older Grunt converter `0x2235A250`**:

| Original input activity | Native branch | Original output |
|---|---|---|
| `1 = ACT_IDLE` | 0 | `213 ACT_JETPACK_MOVE` if measured length **strictly greater than 50**, otherwise `208 ACT_JETPACK_IDLE` |
| `7 = ACT_WALK_AIM` | 0 | Same magnitude-dependent `213/208` selection |
| `16 = ACT_RANGE_ATTACK1` | 0 | Same magnitude-dependent `213/208` selection |
| `66 = ACT_RELOAD` | 1 | Always `218 ACT_JETPACK_RELOAD` |
| All other activity IDs | 2 | Call original Grunt `0x2235A250(activity)` and return its result |

The original `fcomp 50.0; fnstsw ax; test ah,0x41; jne` makes the >50 branch strict. **Unordered x87 comparisons also choose the idle branch**, but the independent safe C++ reference explicitly marks nonfinite input as unsupported rather than asserting that it reproduces the original exceptional x87 state. The original model also dereferences the resolved entity without a safe null guard; the portable reference reports `MissingNativeAttachment` instead of simulating an unsafe dereference.

The length's physical semantic meaning (velocity, movement impulse, animation-state vector or another field) is **not independently proven**, so we label it *attached-entity vector magnitude*, not speed.

## Source-proven activity string labels

The owner-original 13,780-byte `0x220E7040` shared activity initializer SHA and seven genuine `push ID / push original name VA / CALL 0x220E6F60` triples confirm:

- `1 ACT_IDLE`, `7 ACT_WALK_AIM`, `16 ACT_RANGE_ATTACK1`, `66 ACT_RELOAD`;
- **`208 ACT_JETPACK_IDLE`, `213 ACT_JETPACK_MOVE`, `218 ACT_JETPACK_RELOAD`**.

The named registry is generic Source/Ritual activity registration data and is counted **only as supplemental evidence**. No 13,780-byte shared initializer is spuriously added to the recovered game-specific function byte count.

## Verification and limits

Read-only original PE32 audit: `research/tools/audit_sin_original_jetpack_activity_phase588.py`.

- Original owner `server.dll` and `client.dll` SHA256 pair, one complete new native method SHA256, its true method terminator, **three absolute jump destinations, all 66 selector entries, 19 independent opcode/field witnesses, native x87 sqrt pointer and float 50.0**: PASS.
- **Eight complete Grunt/Jetpack original RTTI class identity strings, 16 checked original vtable entries, seven ACT name/registration source triples**, and one actual call into the original Grunt base activity method: PASS.
- Original-free synthetic E8 mutation and fake-DLL rejection: PASS.
- Independent authored C++17 host reference and regression tests: GCC, Clang `-Werror`, GCC ASan/UBSan, standalone **CTest 2/2 PASS**. Covers original strict threshold, original 4 special input IDs, fallback delegation to Phase587 model, required attached vector even for reload, stale/missing attachment and host-safe nonfinite rejection.
- Original exact x87 precision, native factory/animations/assets, actual attached-entity type and original invalid-handle crashes are not claimed source-equivalent. Native original function extent includes **78 bytes of jump-table data** rather than 274 bytes of translated C++ instructions.

The Phase588 GitHub source only adds its new audit, report, C++ header and tests. Phase587's C++ header is already available on the stacked parent branch; it is copied into the standalone research package solely to make isolated compilation reproducible.

**Next original-gameplay boundary:** identify entity `+0x1024` field semantics and the production sites of attached vector `+0xC18`, then trace the remaining Grunt-private activity-to-sequence selection and `+0xC18` component delegate rather than implementing generic physics/renderer systems.