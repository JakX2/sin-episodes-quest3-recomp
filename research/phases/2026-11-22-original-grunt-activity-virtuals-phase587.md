# Gameplay Phase587: original Grunt animation-activity virtuals and source-verified ACT remaps

## Scope and original-only provenance

This phase directly analyzes the untouched 2006 **SiN Episodes: Emergence** `server.dll` and `client.dll` from the owner's private `SE1/bin.zip`. It continues the original gameplay-reconstruction branch after Phase586. No generic HL2 physics, surface parameters, model/prop reading or rendering, native Windows DLL boot, or Android/Quest boot work is undertaken. Commercial DLLs and direct bulk assembly are **not** committed.

- Original `server.dll` SHA256: `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`.
- Original `client.dll` SHA256: `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.
- Native `server.dll` image base: `0x22000000`, original Grunt primary vtable: **`0x2249AAA4`**.
- Grunt vtable **`+0x338 → 0x2235A250`**; Grunt vtable **`+0x33C → 0x222A53E0`**. The main Phase584 resolver calls these two virtuals in its source-verified five-iteration normalization loop.

## Four newly authenticated complete original methods, 855 distinct code bytes

| Original complete method | Bytes | Inclusive native end | Whole-function SHA256 | Original role |
|---|---:|---|---|---|
| `0x2235A250` | **326** | `0x2235A395` | `aacea02b9e14ff6ba7a060bbbac75dd33bca28db374218029b1cb0f8e91d5351` | Grunt's own private activity remapping, state writes, component delegate or inherited fallback |
| `0x222A53E0` | **143** | `0x222A546E` | `b80322d471115481e1b2b70b0e7e006930b103bd7af6c13d64dbdd7e107e9b21` | Serialized entity-handle activity translator, native virtual `+0x47C` and invalid-handle flag clear |
| `0x2203CFC0` | **170** | `0x2203D069` | `a6898e05cf04d849b2c110e66f2878e5b4565d3b55d13b02f6452521562c5e56` | Inherited conditional activity map and entity/schedule-gated fallback |
| `0x222AA840` | **216** | `0x222AA917` | Whole-method digest pinned in the auditor | Grunt child weapon name/owner query over 48 serial-checked handles |

All four original function ends are followed by authentic `INT3` padding, and all `.text` and `.rdata` native addresses in the auditor are checked through PE32 section mapping. Native whole-function authentication does **not** imply full runtime-equivalent decompilation of every invoked engine dependency.

## Exact private Grunt remapping `0x2235A250` (`+0x338`)

The method takes an integer activity argument, writes certain Grunt fields, then usually delegates the potentially changed activity to another object. It returns `ret 4`.

| Input | Grounded state condition | Raw native output / activity name before delegation |
|---|---|---|
| `16 / ACT_RANGE_ATTACK1` | Owner byte `+0xCA0 != 0` | `199 / ACT_SHOOT_ON_KNEES` |
| `16 / ACT_RANGE_ATTACK1` | `+0xCA0 == 0`, `+0xC64 == 0` | `18 / ACT_RANGE_ATTACK1_LOW` |
| `66 / ACT_RELOAD` | Owner byte `+0xCA0 != 0` | `200 / ACT_RELOAD_ON_KNEES` |
| `66 / ACT_RELOAD` | `+0xCA0 == 0`, `+0xC64 == 0` | `67 / ACT_RELOAD_LOW` |
| `1 / ACT_IDLE` | `+0xCA0 != 0` | `196 / ACT_ON_KNEES` |
| `1 / ACT_IDLE` | `+0xCA0 == 0`, `+0xC64 == 0` | `45 / ACT_CROUCHIDLE` |
| `1 / ACT_IDLE` | Neither case above, and raw owner `+0x850` is **2 or 3** | `72 / ACT_IDLE_ANGRY` |
| `10 / ACT_RUN` or `6 / ACT_WALK` | Any state | Set owner byte **`+0xC64=1`**, retain input *for subsequent transformations* |
| `17 / ACT_RANGE_ATTACK2` | Special unconditional branch | Look up **`weapon_grenadelauncher`**, return original runtime global **`0x2259C4A8`** if found, **`0x2259C4AC`** if absent; **does not** run the usual delegates |

If the owner object at raw `+0xDBC` has virtual `+0x4A8()` returning the address of owner `+0xDB8`, the original method applies **additional** remaps *after* the rules above:

- `1 → 72` (`ACT_IDLE` → `ACT_IDLE_ANGRY`)
- `6 → 7` (`ACT_WALK` → `ACT_WALK_AIM`)
- `10 → 11` (`ACT_RUN` → `ACT_RUN_AIM`)

The caller then checks owner raw pointer `+0xC18`: if nonnull, invoke that object's virtual **`+0x7C(activity)`**; otherwise invoke original inherited **`0x2203CFC0(activity)`**. The final returned value therefore can differ from the Grunt's intermediate mapping. The private fields' conceptual names are not independently recovered from original C++ declarations; they remain referred to by offset.

The special `ACT_RANGE_ATTACK2` branch does **not** return the literal shared ACT ID directly; it looks up a named child weapon and returns a **runtime-global activity ID**. Exact runtime values for those writable globals cannot be inferred from the raw DLL file and are **not invented** in the host model.

## Original `+0x33C` and weapon handle dependencies

`0x222A53E0` reads owner handle **`+0x78C`** and validates it through native global table `0x224F5620`: low 12 bits index a 16-byte slot and upper bits must match the slot serial. If unresolved, the method preserves the incoming activity and clears the optional output flag byte, then returns `ret 8`. A valid slot forwards the input activity and flag to the referenced entity's **virtual `+0x47C`**. Its intermediate branches may dereference invalid null pointers in the retail binary; the safe reference returns the original activity when the target is invalid and does not claim crash equivalence.

`0x222AA840` searches **48 raw handle slots** starting at owner `+0x6CC`, each with the same generation-checked entity semantics. It first checks entity `+0x0C` string identity with the requested `weapon_grenadelauncher`, or calls original **`0x221387F0`** to perform a name match, then requires that entity's **virtual `+0x2E4`** equal the supplied owner/context pointer (`0` in Grunt's caller). The returned pointer is a matching entity, or null. The portable host model uses entity IDs and explicit injected classname and ownership checks rather than raw native pointers.

## Inherited `0x2203CFC0` is a distinct gameplay condition

- Calls owner virtual **`+0x5C0()`** and checks bit **`0x08000000`**. If unset, returns the original incoming activity without other changes.
- With that flag set, activity `66 / ACT_RELOAD` resolves serial-checked owner handle **`+0xA18`** and invokes owner virtual **`+0x718(targetEntityOrNull)`**.
- Activity `3`, or activity `1 / ACT_IDLE` when owner raw byte `+0x988 & 2` is set, calls original **`0x22039130`**, passes its result to owner virtual **`+0x714`**, and tests the returned code via **`0x220EEBD0`**. A failed registration result of `-1` substitutes **activity `1`**.
- No invented named activity is assigned to code `3`, and no assumptions are made about the global capability flag's developer name.

## Exact original ACT name provenance

The original shared registration initializer **`0x220E7040`** (13,780 bytes, owner SHA256 `be2d64d38bfffb9d6285fbb36268bc308a5d3d03100caf75dd6081dcb4b82206`) contains **921** registered `ACT_*` activities. A targeted independent audit checks **16 actual `push numeric ID`, `push original string VA`, `E8 0x220E6F60` triples** and the original NUL-terminated strings, including all values in the Grunt conversions above. This gives stronger attribution than generic Source SDK nomenclature. Its full table is **shared engine evidence**, not newly reconstructed Grunt-specific method bytes.

The event/schedule and final sequence that consumes each activity is still a separate original-gamecode dependency. These activity names do not alone prove what animation file, sequence index or actual pose a released model plays.

## Evidence, verification and limitations

- **Four newly authenticated complete native original methods, 855 distinct source bytes**, two primary Grunt vtable slot pointers, **five real E8 call-target pairs** across these functions, **46 distinct opcode/field witnesses**, named weapon string and **16 source-verified shared ACT numeric/name registrations**.
- Original-private client/server exact SHA256 pins, four original method whole-body SHA256 digests and original `INT3` boundaries: **PASS**.
- Original-free synthetic E8 opcode and rel32 byte mutation rejection plus wrong-DLL refusal: **PASS**.
- Independent original-free portable C++17 model and regressions: strict GCC, Clang `-Werror`, GCC ASan/UBSan, standalone CTest **2/2 PASS**. Tests cover Grunt attack/reload/idle/movement activity mapping, persistent state flag, kneeling cases, component identity, early return, inherited fallback, serial mismatch, null output flag, 48-slot weapon scan limit and owner match.
- The C++ reference supplies safe typed callbacks for unresolved native virtuals (`+0x4A8`, `+0x7C`, `+0x5C0`, `+0x47C`, `+0x2E4`, etc.). It is not bitwise-identical original PE32 execution, exact memory layout, full engine activity/sequence registration, or an engine-loadable new DLL.

Reproduce:

```bash
python3 research/tools/audit_sin_original_grunt_activity_phase587.py --self-test
python3 research/tools/audit_sin_original_grunt_activity_phase587.py --bin-zip /private/SE1/bin.zip
cmake -S . -B build && cmake --build build -j2 && ctest --test-dir build --output-on-failure
```

**Next direct original-gameplay boundaries:** original registered Grunt sequence selection (including the `+0xC18` object's `+0x7C` implementation) and entity delegate `+0x47C` via raw `+0x78C`, plus original activity-to-sequence lookup callsites. These require vtable/object ownership proof rather than assuming matching modern HL2 activity behavior.