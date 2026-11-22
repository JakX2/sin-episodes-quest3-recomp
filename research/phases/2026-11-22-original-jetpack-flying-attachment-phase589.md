# Gameplay Phase 589: original Jetpack Grunt flying-attachment provenance and bidirectional ownership

## Purpose and provenance

Continue the original **SiN Episodes: Emergence** 2006 `server.dll` gameplay recovery from Phase588's Jetpack override `0x22364700`, which reads owner `+0x1024` and the resolved entity's three-component float vector at `+0xC18/+0xC1C/+0xC20` to select `ACT_JETPACK_IDLE` or `ACT_JETPACK_MOVE` (strict magnitude > 50). This phase identifies the **actual original owner handle producer**, the attached-entity registration name, the owner-to-attachment backreference, and the constructor's initial invalid handle. It does **not** implement unrelated generic Source physics, entity rendering, models/props, Android, VR or Windows runtime boot.

Original owner-private unmodified retail DLL hashes:

- `server.dll` SHA256: `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`.
- `client.dll` SHA256: `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.
- Original PE32 base: `0x22000000`.

## Six newly whole-method-authenticated functions, 618 native bytes

| Original original-server VA | Bytes | Exact entire original method SHA256 | Grounded source role |
|---|---:|---|---|
| `0x22363D20` | 38 | `4ef9bb85842856c477d97383f502a68b6439ae49c86d636f6ac0e56b79a9da52` | Owner/attachment pointer to encoded serial-handle setter, null-to-`-1` |
| `0x22363D50` | 42 | `3e1f38e2f06c1fd7b15f8268e7eecf5e4e21067cfa73e45348e71183740c9fa6` | 12-bit index plus serial-generation resolver of encoded handle |
| `0x22363DD0` | 107 | `addceba4a412ccf8b2fdf0d265667f1aa6193b362182f09f55d7e3f9032e34cd` | Assign attached entity's original `+0xC50` owner-backhandle and copy owner vector `+0x36C` to attachment `+0xC24` |
| **`0x22365EE0`** | **248** | `ae554bb1fde7031138f9d918b998ad79095cfaba1cccbcaaf6f967b72faff636` | **Jetpack attachment acquisition** via original literal `ai_base_physics_flying`, store encoded handle at owner `+0x1024`, setup and attach-backreference, final virtual call |
| **`0x22368F60`** | **161** | `621c741c1b3c8d00a0e62ade0fc040b7a9bd078e1360e6502cc2e61fca8b6fbc` | Original Jetpack base ctor; writes primary Grunt vtable and initializes raw owner `+0x1024` and other handles to `-1` |
| `0x223C7B10` | 22 | `542fc31bda25e807bd2320ff4c85d32db21ea320fe7f5f93e5aafe6ff5095200` | Original engine registration of literal `ai_base_physics_flying` with factory-data pointer `0x22539A80` |
| **Total** | **618** | | **Six distinct complete original functions; zero overlap with Phase588** |

All six original next bytes are `INT3`. Whole-function native hashes are checked *before* claims about any instruction or field.

## Confirmed original data flow

### 1. Constructor initializes the exact original handle

`0x22368F60` writes primary Jetpack Grunt vtable `0x2249964C` (the Phase588-authenticated `CNPC_JetpackGruntBase` RTTI), secondary vtables at `+0x790` and `+0xC14`, calls underlying constructor and game-global revision initializer, and sets the following native fields: `+0xFE0=0`, `+0xFE4=-1`, `+0xFE8=-1`, `+0xFF4=-1`, `+0x1000=-1`, **`+0x1024=-1`**, `+0x1028=-1`, and `+0x1008=0`. It also calls distinct constructors for `+0x102C` and `+0x1074`. Only the verified member writes are emulated by the safe host helper, not all inherited engine allocations and global revision effects.

### 2. Original Grunt startup invokes the flight attachment lookup

The existing upstream Grunt startup method calls **`0x22367F3F -> 0x22365EE0`**. The attachment method supplies owner virtual `+0x1C4` and fixed input pointers to the original native lookup `0x22144DA0`, using the **actual literal NUL-terminated string `ai_base_physics_flying` at `0x224984E0`**. A nonnull lookup result is asked for its encoded handle via virtual `+0x08`, then the raw encoded handle is saved at owner **`+0x1024`**. A null lookup writes exactly `0xFFFFFFFF` there.

The same original string is separately passed into the engine's registration method at **`0x223C7B17`**, adjacent to a factory pointer `0x22539A80`. This independently supports that the identifier refers to a registered flying-physics-related entity, not an arbitrary animation token.

The original acquisition method resolves the freshly stored handle, calls original `0x2214D6F0` with three zero stack arguments using the resolved entity as `this`, resolves the handle again, calls original `0x22363DD0(attached, originalGrunt)`, then resolves a third time and invokes the attached entity's virtual **`+0x64`**. The strict native ordering is **lookup -> handle write -> resolve -> setup -> resolve -> backreference/vector copy -> resolve -> +0x64 finalization**.

The original function performs unsafe null dereferences if an expected attached entity is absent or stale at later stages. The portable reference explicitly returns `MissingOriginalFlyingEntity` / `UnresolvedNativeHandle` rather than simulating an original crash. These statuses are **host-only**.

### 3. The attachment stores a reference back to the Grunt

`0x22363DD0` takes an entity/owner pointer, reads its virtual `+0x08` encoded handle, and stores that raw value at the *attached entity* **`+0xC50`** (or `-1` if input pointer null). It then serial-resolves that newly assigned handle and invokes original `0x22269B30` to copy **12 bytes** from the owner's `+0x36C` to the attached entity's **`+0xC24`**.

This makes the ownership relation **bidirectional**: the Grunt's raw `+0x1024` points to the flying entity, while the flying entity's `+0xC50` points back to the Grunt through a separately serial-validated handle. The nature of the copied `+0x36C` vector is not inferred beyond its original byte layout and transfer.

### 4. Slot/serial resolver and Phase588 activity consumption

`0x22363D50` authenticates the same serial-handle format observed in Phase588: raw bits `[11:0]` index 16-byte slots in the global entity table at `0x224F5620`; higher bits are compared against the slot's generation field. A mismatch or encoded `-1` returns null. The handle-writing helper at `0x22363D20` stores `-1` on null and otherwise obtains the original entity's handle via virtual `+0x08`.

Phase588's fully authenticated 274-byte `0x22364700` is SHA-checked again **only as a cross-phase witness**. It reads this `+0x1024` handle and the resolved flying entity's raw 3 floats at `+0xC18/+0xC1C/+0xC20`, then computes the three-vector length. `ACT_IDLE`, `ACT_WALK_AIM`, and `ACT_RANGE_ATTACK1` choose `ACT_JETPACK_MOVE` only at ordered length **strictly greater than 50**, otherwise `ACT_JETPACK_IDLE`; `ACT_RELOAD` chooses `ACT_JETPACK_RELOAD`. The vector's exact physical meaning (velocity, force or another state) remains **unproven**. This phase does not count or reimplement Phase588 code.

## Authentication and reproducibility

Auditor: `research/tools/audit_sin_original_jetpack_attachment_phase589.py`.

- Original whole-file client/server SHA256 pair: **PASS**.
- **Six distinct complete native function SHA256s / 618 original bytes**: **PASS**.
- **Twelve genuine original E8 direct calls**, including `0x22367F3F -> 0x22365EE0`, `0x22365F90 -> 0x22363DD0` and `0x223642D5 -> 0x22363D50`, **45 original instruction/field byte witnesses**, native `INT3` boundaries, exact original class string and class vtable: **PASS**.
- Original-free synthetic mutation tests corrupt each byte in every authenticated E8 encoding and reject unsupported DLL pairs: **PASS**.
- Authored C++17 safe host model and tests: owner constructor field defaults, lookup/registration key, low-12 slot and serial high bits, original setup and backreference/finalize order, distinct missing/stale handle statuses, no mutation of attached raw vector, original owner-vector backcopy. Strict GCC/Clang `-Wall -Wextra -Wpedantic -Werror`: **PASS**; GCC ASan/UBSan: **PASS**; isolated CMake/CTest **2/2**.
- Original lookup and engine callbacks, true field/physics meaning of vector `+0xC18`, inherited class constructors, model sequences, actual Windows/Android/Quest executable behavior and original dereference failures remain **explicitly outside the host implementation**. No original commercial DLL or proprietary assets/disassembly committed.

## Suggested next Grunt-specific target

The startup callsite at `0x22367F20` also initializes another Grunt handle `+0x1028` through a second literal entity lookup. Researching its source role and the original Grunt's scheduling/event triggers is preferable to reconstructing generic Source physics/surface or renderer components. Separately, recovering the original producer of the flying entity's raw `+0xC18` vector would determine whether the Phase588 idle/move threshold is measuring speed, impulse, or another value, but this should only proceed if a **Grunt-specific** producer rather than HL2-equivalent physics code can be identified.