# Gameplay Phase581: complete original Grunt-specific animation-event override and second switch

## Scope and original file authentication

This original 2006 SiN Episodes: Emergence research follows gameplay **Phase580**'s `GRUNT_ANNOUNCE` / `GRUNT_GO_ALERT` / `GRUNT_FLANK` event-source mapping *one handler upward*. It analyzes the independent Grunt-related event override at original `server.dll` VA `0x22351C20`. Generic HL2 physics/surfaces/models/props, Windows boot, Android and Quest 3 integration remain excluded.

The owner-private commercial pair is authenticated *before* the auditor accesses internal functions:

- `server.dll` SHA256: `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`.
- `client.dll` SHA256: `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.
- PE32 original preferred base: `0x22000000`.

## One newly authenticated complete x86 native method extent: **1,330 bytes**

| Native VA | Extent | Whole-body original SHA256 | Role |
|---|---:|---|---|
| `0x22351C20` | **1,330 bytes, through 0x22352151 inclusive** | `770d75ba487fb923eddd5f20bf200d72ac7dd3b66085d9aab007366c1838363d` | Grunt-specific animation-event override with native two-level jump-table dispatch, event-specific side effects and generic fallback |

The extent includes *executable branch bodies and two embedded switch tables*: 10 four-byte address words at `0x223520B0–0x223520D7` and 122 one-byte case indices at `0x223520D8–0x22352151`, a total **162 table bytes**. The next original byte at `0x22352152` is `INT3` padding. Full method hashing includes both tables; **do not disassemble table bytes as if they were x86 instructions**. Native method body including original table data accounts for all **1,330 newly authenticated bytes**. This is not a claim of 1,330 independently translated C++ instructions.

Three original `.rdata` references (`0x2248B1F8`, `0x2248B990`, `0x2248C128`) contain the exact original method VA. These are original code pointers. Their precise Ritual C++ class/vtable ownership is not assigned without independent original registration evidence.

## Source-proven dispatch contract

The original entry reads the event pointer from the method's single stack argument, extracts its original numeric first field, computes **`event_id - 0x1B`**, and requires **unsigned result `<= 0x79`**, i.e. event IDs **0x1B–0x94 inclusive**. It uses a **122-byte map table** to choose among 10 original code destinations via a 10-entry 32-bit pointer table, rather than a simple contiguous dense `switch`.

| Event ID | Byte table case | Native branch VA | Grounded behavior bound |
|---|---:|---|---|
| `0x1B` | 0 | `0x22351E37` | Dedicated original Grunt branch; external callbacks not yet translated |
| `0x4A` | 1 | `0x22351DDC` | Dedicated Grunt target/event branch; external callbacks opaque |
| **`0x8C`** | **2** | **`0x22351C47`** | Original alert chance and two other calls, reconstructable prefix |
| `0x8D` | 3 | `0x22351F3D` | Dedicated native motion/vector and state branch; model deliberately opaque |
| `0x8F` | 4 | `0x22351FFE` | Dedicated branch with spatial/vector helper; model opaque |
| `0x90` | 5 | `0x22351C82` | Dedicated Grunt target-location branch; model opaque |
| **`0x91`** | **6** | **`0x22352080`** | Original signal/notification 0x1E and +0x368 -> +0xC30 copy |
| `0x92` | 7 | `0x22351D7F` | Dedicated state and timer branch; model opaque |
| **`0x94`** | **8** | **`0x223520A5`** | Immediate common epilogue, no original body work |
| Every other event ID | 9 | `0x2235209D` | Directly calls original shared NPC event dispatcher `0x22050150` |

The **nine dedicated Grunt event IDs are individually present once each** in the 122-byte table. The other **113** original in-range IDs all map to case 9, the native base dispatcher. Out-of-range IDs also branch directly to the same fallback. Both raw tables, every index and every case pointer are verified by the auditor, not just inferred from an apparent enum.

## Important additional original voice event: 0x8C

At `0x22351C47` the original code:

1. Calls the engine-global random integer interface at global `0x225721B0`, virtual `+0x08`, with arguments **1 and 4**. It compares the result to exactly integer **1**.
2. **Only if the returned value is 1**, calls the owner's Grunt-capable virtual **`+0x590`** at `0x22351C5F`. Gameplay Phase580 independently authenticated the original Grunt override **`0x22358970`** as `GRUNT_GO_ALERT`, with its own strict **10-second** cooldown. An event 0x8C random draw of 1 therefore does *not* guarantee an alert sentence if the Grunt's own cooldown gate denies it.
3. Regardless of random result, calls exact native method **`0x22350D30`** at `0x22351C67`.
4. Then passes a value from original global `0x2259BF3C` into exact original native method **`0x22049C20`** at `0x22351C75`, and returns.

Assuming (without independently proving) that the original random interface distributes all four values uniformly, the optional `+0x590` invocation would occur approximately 1 in 4 calls. The original code proves the **1..4 arguments and equality-to-1 test**, not the distribution, and still does not guarantee an actual played line. Independent host tests preserve the **random -> optional virtual alert -> original helper -> original state callback** ordering. The precise semantic names of the latter two callees remain unproven.

## Additional original Grunt-specific event 0x91

At `0x22352080`, native code requests original `0x22049C20` with literal integer **0x1E**, then copies owner raw dword `+0x368` into owner raw dword **`+0xC30`**, and returns. This is a narrow original field and call-order fact. It is *not* evidence that `+0xC30` is definitively a position, alarm type, or combat enum.

Event `0x94` deliberately maps to the common return epilogue without invoking generic fallback. The independent C++ host model reproduces that difference.

## Why this matters for `GRUNT_FLANK`

Gameplay Phase580 established the *shared* NPC event `0x54`: the original base handler reads a float parameter from animation event `+4`, temporarily selects x87 truncation, and calls Grunt virtual `+0x5A8`. The Grunt override there uses custom parameter `0` for assault speech and `1` for the `GRUNT_FLANK` path-length gate.

**Phase581 adds the missing upper edge:** original Grunt-specific handler `0x22351C20` maps event ID **`0x54` to its generic-base fallback at `0x2235209D`**, directly calling `0x22050150`. That gives a complete source-proven *dispatch route* across two authentic tables and the Grunt virtual. It still does **not** prove which released model/animation resources actually emit event ID `0x54` with a float parameter of `1.0`, or the original named enum identifier.

## Audit, source reference and limits

Read-only SHA auditor: `research/tools/audit_sin_original_grunt_event_switch_phase581.py`.

- Whole unchanged owner client/server DLL identity: **PASS**.
- Exact complete **1,330-byte** method SHA (including both data tables), authentic `INT3` method terminator and `.text` original range: **PASS**.
- All **10** original switch jump destinations, **122** original per-event indexes, exactly **nine specific IDs / 113 generic fallback indexes**, **three** original `.rdata` code pointers: **PASS**.
- All **44 native direct E8 callsite/target pairs**, including `0x22351C67 -> 0x22350D30`, `0x22351C75 -> 0x22049C20`, `0x22352084 -> 0x22049C20`, and original default fallback `0x223520A0 -> 0x22050150`: **PASS**.
- **27** distinct native branch/opcode witnesses, original `call [vtable+0x590]`: **PASS**.
- Original-free selftest rejects every single-byte E8 opcode/rel32 mutation across those 44 calls and refuses unsupported original pairs: **PASS**.

Independent reference: `include/sin_compat/original_grunt_event_switch_phase581.hpp` and `tests/test_original_grunt_event_switch_phase581.cpp`. The reference maps all nine Grunt-specific IDs, returns `NeedsOpaqueNativeBranch` for the six unimplemented engine-dependent cases, models exact known event `0x8C`, `0x91`, `0x94` effects, and leaves other IDs for **explicit base dispatch**. It uses the independently-authored Phase580 alert method via a header dependency, not any commercial game code. It rejects out-of-contract random results solely as a **host safety guard**, not as original 2006 behavior. Exact original call ABI, animation asset names, gameplay target allocation/physics and full large-handler translation are **not** claimed.

Strict GCC and Clang C++17 host regressions **PASS**; GCC ASan/UBSan **PASS**; standalone CMake/CTest **2/2 PASS**. No original commercial binary, entire assembly dump, game model/map or native executable payload is distributed.

## Reproduce

```bash
python3 research/tools/audit_sin_original_grunt_event_switch_phase581.py --self-test
python3 research/tools/audit_sin_original_grunt_event_switch_phase581.py --bin-zip /private/SE1/bin.zip
cmake -S . -B build
cmake --build build -j2
ctest --test-dir build --output-on-failure
```

The **next strongest Grunt-specific research target** is the six remaining active event branches in `0x22351C20`, particularly their state flags/target dependencies, or the released animation-resource source of outer event `0x54` and its float parameter `1.0`. The event dispatcher's original raw schedule-name association remains unknown.