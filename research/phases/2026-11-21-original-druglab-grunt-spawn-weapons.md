# Phase 532: original Druglab Grunt Spawn weapons and game-network state

## Why weapon selection is not a constructor property

The user has already loaded SiN Episodes maps/models/props in Half-Life 2,
so we continue with actual original gameplay classes, not duplicate Source
physics or model readers. Phase 530 authenticated 17 missing original NPC
factory registrations. Phase 531 recovered the shared native factory path
for the three `npc_druglab_grunt_*` weapon variants, allocating a
0xFD8-byte native object, installing +0/+0x790/+0xC14 vtables, invoking
constructor initialization and returning an offset +0x160 object view.

**New finding:** the native `CDruglabGrunt`-derived primary vtable
`0x2249AAA4` slot **25** (byte +0x64) points to
`0x22369E40`. This is the **per-variant Spawn method**, distinct
from the factory's virtual +0x70 initialization bridge
`0x22031B10` (slot28). Original full Spawn method is
**1246 bytes** (VA 0x22369E40 through 0x2236A31D)
with SHA256
`99dbbb09eeebe19d7260e707c4fb917ab4c4941f1b066d6368eba53b485ccff7`.

Private primary evidence: original SiN Episodes 2006
`server.dll` SHA256
`568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`,
`client.dll` SHA256
`6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.
Source code, byte arrays, model/asset payloads and retail DLLs are not
published here. The auditor checks the whole original method digest,
not merely strings.

## Exact source-authenticated class/weapon/mode mapping

The native method reads its original classname pointer at object
`+0x0C`. It compares that string case-sensitively to three fixed
class names. It reads the assigned weapon string from `+0xA24`.
If it is *not already equal* to the class's original weapon name,
it replaces the native weapon symbol using original intern-string
helper `0x221F3DE0`.

| Original NPC classname | Forced weapon when different | Desired original +0x3B0 |
|---|---|---:|
| `npc_druglab_grunt_pistol` | `weapon_npc_pistol` | 1 |
| `npc_druglab_grunt_scattergun` | `weapon_scattergun` | 0 |
| `npc_druglab_grunt_assault_rifle` | `weapon_assault_rifle` | 2 |

The source-native names and addresses are
`0x2249AA20`, `0x2249AA00`, `0x22490EA0`
for classnames, and
`0x2249A974`, `0x2249A988`, `0x2249A99C`
for weapons. The three native field values are **original exact code
literals**, not assigned by the port from assumed difficulty levels.

The +0x3B0 slot is also a native *network-marked* integer:

- For pistol and scattergun, the original method compares the
  current +0x3B0 value, then, only when changing it, marks byte
  +0x200 if byte +0x1F0 is set; otherwise marks bit0 in the
  current `+0x16C` edict pointer when one exists; finally stores
  the desired +0x3B0 value.
- For assault rifle, the original method invokes existing native
  setter `0x220F10C0` on a pointer to +0x3B0 with desired value
  **2**; that setter checks value change and performs the same
  +0x1F0/+0x200 or +0x16C network-dirty branch.
- If the field already equals the desired value, neither native
  dirty path should fire. If no edict exists and +0x1F0 is false,
  the field can change without a published dirty marker.

The semantic game-logic property **name** for +0x3B0 remains
unproven; it must NOT be labelled "difficulty", "weapon slot" or
"NPC team" solely because the original constants are 0/1/2.

## Remaining original Spawn order

After variant weapon and +0x3B0 setup, the original method calls:

1. Native primary object virtual byte `+0x68`, observed
   callsite `0x2236A0D9`.
2. Native virtual byte `+0x1C`, uses its return pointer
   as the input to native virtual byte `+0x6C`,
   observed `0x2236A0E6` and `0x2236A0FA`.
3. Inherited NPC Spawn helper `0x2235F2A0`,
   callsite `0x2236A0FF`.

Only **after these inherited calls** does it recheck the
classname and load one of three per-variant runtime global
pointers `0x2259CC50` (pistol), `0x2259CC98` (scattergun),
or `0x2259CCE0` (assault rifle). Each is dereferenced
and a **float32 at global object +0x28** is loaded.

The original x87 path sets control-word rounding control to
**11, truncate toward zero**, and performs `FISTP` to a
signed 32-bit integer. It compares that value to original
NPC fields at +0x98 and +0x9C. If different:

- For field +0x98, it calls native object virtual byte
  `+0x1B0` then directly writes the int32 into +0x98.
- For field +0x9C, it calls native object virtual byte
  `+0x1B4` then directly writes the int32 into +0x9C.

The three branches share this shape but read different original
global objects. The particular gameplay meanings and ConVar
**names** are still unknown; this is not a license to label them
"health", "ammo" or "difficulty".

**Source invalid-scalar distinction:** the actual original x87
`FISTP` on NaN/out-of-range can produce an integer-indefinite
result with the native x87 exception policy. The independent
adapter explicitly refuses nonfinite/out-of-range inputs
instead of claiming to emulate those platform-specific details.
For finite in-range numbers, its truncation matches x87 RC=11.

## New host-neutral implementation

`include/sin_compat/original_druglab_grunt_spawn.hpp` and
`src/original_druglab_grunt_spawn.cpp` provide a pure
`plan_spawn()` and an opt-in `SpawnReference::apply()`.

Input is an explicitly observed **real engine-host snapshot**:
`classname`, original weapon, +0x3B0, +0x98, +0x9C, native
dirty-flag byte +0x1F0, edict presence +0x16C, and the
correct per-variant global float. It never guesses a default
ConVar value or dereferences an original PE32 native pointer.

The planner determines exact class weapon, whether the original
weapon symbol must be replaced, desired mode field, which dirty
marker should be updated, and whether each of the two native
numeric setters must be invoked. The host adapter then issues
callbacks in the **original verified Spawn order** and reports
the first failed stage instead of falsely returning success.

The underlying original native host still owns:
actual string interning, sendtable dirtiness, edict lifetime,
virtual +0x68/+0x1C/+0x6C inherited behavior, inherited
`0x2235F2A0`, ConVar values and final class networking.
The port has not implemented those engine callbacks. It refuses
unknown classnames, including `npc_druglab_grunt_elite`.
The conservative reentrancy/failure guard is a new safety policy.

## Verification

The new original-only metadata auditor
`research/tools/audit_sin_original_druglab_grunt_spawn.py`
requires both SHA-pinned retail DLLs, checks the 1246-byte
method SHA256, original virtual slots 25/28, all six source
class/weapon string operands, three native per-variant global
float sources, six original direct CALL edges, original
virtual call order, native +0x1B0/+0x1B4 field notification
sites and x87 RC=truncate instructions. It returns metadata
only. `--self-test` uses fabricated pointers and independently
mutates synthesized rel32 and class/weapon strings.

Native CTest
`sin_original_druglab_grunt_spawn_tests` verifies each of
three source weapons and desired +0x3B0 values, dirty-marker
precedence, class-sensitive exact existing-weapon preservation,
zero-netprop no-op, truncation of positive and negative f32,
int32 range boundaries, invalid scalar refusal, strict
nonaliases, every host failure stage and nested callback refusal.

Owner-private verification:

```sh
python3 research/tools/audit_sin_original_druglab_grunt_spawn.py \
  --client /private/original/client.dll \
  --server /private/original/server.dll
```

Public CI:

```sh
python3 research/tools/audit_sin_original_druglab_grunt_spawn.py --self-test
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build --target sin_original_druglab_grunt_spawn_tests
ctest --test-dir build -R 'sin_original_druglab_grunt_spawn' --output-on-failure
```

## Scope limit

This reconstructs actual **SiN-only NPC gameplay decisions**, but
`ServerGameDLL003::DLLInit` still does not complete, the original
engine has not instantiated the reconstructed NPC from a map,
and actual Source NPC AI/weapon-spawning, save/restore and
network class tables are not implemented. Windows native game
startup precedes Android and Quest 3 gameplay.
