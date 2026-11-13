# Phase 210: original 2006 client InitPredictable and SaveData slot selection

13 November 2026. **Stacked draft on Phase209 PR #263**. Original client.dll and server.dll are kept strictly owner-private. This phase verifies complete original Windows i386 PE32 native functions and reconstructs only their provable control flow; no proprietary binaries, full original machine-code bodies, game maps or assets enter GitHub.

## Two distinct new SHA-verified client functions

| Original native entry | Complete bytes | SHA256 of entire original code | Evidence |
| --- | ---: | --- | --- |
| `0x240CAF40` | 90 | `78c3167df98218dd8298d9e7f7c194fac9bc224a0d3c773d65e023e22ff2ed2e` | `InitPredictable` caller, 90 SaveData iterations |
| `0x240C5960` | 241 | `2c11e9007379162694493491f4f794c975019f3abd2c7119b228bcce84d7a58f` | `C_BaseEntity::SaveData` profiler/slot selector |

**Two newly recovered original functions = 331 full original x86 bytes.** Their complete native hashes, precise `ret` instructions and original `int3` alignment bytes (6/15) were checked directly against the private 2006 original `client.dll`, SHA256 `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`. The companion original `server.dll` provenance is `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`.

## Complete original 90-byte initialization call order

Original client entry `0x240CAF40` has original `ECX` = object pointer. It:

1. Sets the byte at **object+0x2E8 to 1**. This happens before any virtual or direct helper call.
2. Calls the object's **virtual byte offset +0x44** and forwards that result to **`0x240C8F00`**.
3. Calls the already recovered Phase209 **`0x240C8720`** to lazily allocate and zero **91 separate buffers**: the sentinel at object+0x454 and 90 DWORDs at +0x2EC..+0x450.
4. Reads object's **DWORD at +0xB4** and passes it to the separate global object at `0x242F7F78`, via `0x240CAC00`.
5. Calls object helper **`0x240C6900`** with explicit zero.
6. Loops the signed index **0 to 89 inclusive**. Each time, pushes `0`, the current index, and the original `.rdata` literal **`InitPredictable`** (at `0x242893DC`), then invokes **`0x240C5960`** as an object method. The loop check is native `cmp edi,0x5a; jl`.

The exact original direct `CALL rel32` pairs are:
- `0x240CAF53 → 0x240C8F00`
- `0x240CAF5A → 0x240C8720`
- `0x240CAF6F → 0x240CAC00`
- `0x240CAF78 → 0x240C6900`
- `0x240CAF8A → 0x240C5960`

**Subtlety:** outer 90-byte `InitPredictable` does **not** check whether the 91-buffer sentinel already exists. It calls the inner Phase209 initializer unconditionally. The inner initializer returns immediately if object+0x454 is nonzero, but the outer caller's other steps, including the **90 SaveData calls**, still execute on subsequent entries.

## Original 241-byte SaveData implementation

The original `0x240C5960` takes three DWORD stack arguments and returns with `ret 0x0C`. It uses an object-provided buffer pointer with these verified rules:

- If **index == -1**, select **object+0x454**, the original sentinel buffer.
- Otherwise, if the sentinel is null, select a **null buffer**.
- Otherwise calculate the signed x86 `IDIV` **index % 90** and read one DWORD at **object+0x2EC + 4 × remainder**. Thus index 0 uses the first of the 90 buffers, index 89 the last, and index 90 wraps to index 0. Negative indexes other than -1 can produce unsafe negative offsets; the portable C++ host model guards them rather than imitating invalid game-memory access.

This routine enters the original `tier0.dll` profiler scope with name **`C_BaseEntity::SaveData`**, uses **`vstdlib.dll!Q_snprintf`** into a 64-byte stack destination to construct one of two labels:

- `%s SaveData(original)` for index -1.
- `%s SaveData(slot %02i)` for other indices.

The original accompanying string **`Unaccounted`** appears in the profiling setup. It constructs an opaque snapshot/serializer object via **`0x241978A0`**, calls an object's virtual **+0x40** and another object's virtual **+0x24**, forwards those and the selected buffer into **`0x24199BA0`**, and exits the original tier0 profiler scope. The exact native calls are `0x240C5A13→0x241978A0` and `0x240C5A34→0x24199BA0`. The imported MSVC symbols and IAT VAs verified from the genuine PE32 import table are:

| Original imported API | Original client IAT address |
| --- | --- |
| `tier0.dll!g_VProfCurrentProfile` | `0x242761F4` |
| `tier0.dll!CVProfile::EnterScope` | `0x242761F8` |
| `tier0.dll!CVProfile::ExitScope` | `0x242761F0` |
| `vstdlib.dll!Q_snprintf` | `0x24276274` |

The actual original serializer's class definition, object lifecycle, buffer ownership, engine state and side effects are **still unknown**. The two SHA-pinned 2006 functions and their runtime control-flow fragments do **not** prove that the game's snapshot/restore semantics are fully decompiled.

## Portable C++17 model and falsification tests

- `include/sin_compat/original_client_prediction_savedata.hpp`: independent, **partial** host model of the full 90-byte InitPredictable sequencing and 241-byte SaveData's index selection and 64-byte label formatting. Reuses Phase209's `client_init_91_buffers` and Phase205's exact shared 16-byte per-DLL lazy allocator cache. The unexamined internal serializer, global helper and engine virtual callbacks remain explicit **opaque host callbacks**. No binary original game pointers are stored.
- `tests/test_original_client_prediction_savedata.cpp`: checks original virtual +0x44 preflag publication, complete callback order, 91 distinct zeroed buffers and exactly 90 named SaveData calls, once-only buffer allocation across repeated outer calls, repeat SaveData behavior, profile label formatting, sentinel `-1`, slots 0/89 and wraparound 90/181, safe refusal of negative invalid slots, host-side 64-byte string truncation, separate tagged owner context.
- `research/tools/audit_sin_original_client_prediction_savedata.py`: owner-only dual whole-DLL SHA256 guard, two complete native method hashes, 38 sparse x86 opcode witnesses, **7 exact direct native CALL edges**, both original method/INT3 boundaries, original five `.rdata` literals and four original named PE32 import bindings.
- `research/tests/test_sin_original_client_prediction_savedata.py`: ten original-free synthetic mutation groups corrupting every sparse witness, all call edges, original boundaries, literal pointers, original PE section/type, imported library/function/slot and fake whole-code/whole-DLL hashes.
- Linux/reference C++ CTest and real Windows x86 MSVC CTest registration. CI must be checked on the **exact latest Phase210 SHA** before reporting success.

## Importance and next gate

Unlike previous allocator alias batches, these functions expose **client entity prediction SaveData initialization** and a concrete host-neutral route from **91 lazily allocated buffers** to **90 named per-slot snapshots**, with the special original `-1` slot. Exact entity prediction object/class identity and real snapshot serializer behavior are not recovered. The original game-engine PC startup and gameplay with reconstructed client/server DLLs are still **not proven**.

Unique original-method atlas **233 → 235 (128 server, 107 client)**. This is SHA-reviewed native function inventory, **not game-port completion percentage**. Next candidate: original helper `0x24199BA0` and the original caller construction/destruction lifecycle associated with these 90 prediction buffers. Windows PC first, Android second, Quest 3 third.
