# Phase 530: all 17 owner-observed missing NPC names are registered in original SiN server.dll

## What was verified, from original binaries

Owner-provided unmodified `bin.zip` contains the real 2006 SiN
Episodes: Emergence Win32 game DLL pair. These have been independently
SHA256-checked in this investigation:

- original `server.dll`:
  `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`
- original `client.dll`:
  `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`

The earlier owner Source-style map runtime logs, summarized in
`research/phases/sin-original-map-runtime-unknown-entity-logs-nov08.md`,
show 1,531 repeated unknown-entity error lines across five console
logs, involving 49 unique classnames. Exactly **17** of those
unknown names start with `npc_`, making 656 of the 1,531 error
line occurrences. The occurrences include repeated loads and must
not be presented as unique BSP entity placements.

This phase tests all **17/17 original NPC registration routines**,
not merely presence of a recognizable string in read-only data:
- each original 22-byte compiled method's complete SHA-256;
- exact machine-code registration instruction shape;
- exact non-forged signed CALL rel32 target back to original
  singleton getter `0x2228B110`;
- the **correct original classname literal** at a source
  `.rdata` location, including its NUL terminator;
- the matching distinct original native factory object pointer
  in original server DLL's `.data` section;
- correct section/image base and both game-DLL SHA identities.

## NPC registrations that are now accounted for

| Original classname | Original registrar VA | Original factory object VA |
|---|---:|---:|
| npc_druglab_grunt_pistol | 0x223C6FE0 | 0x22536840 |
| npc_druglab_grunt_assault_rifle | 0x223C7020 | 0x22536848 |
| npc_druglab_grunt_elite | 0x223C6940 | 0x225346A0 |
| npc_druglab_grunt_scattergun | 0x223C7000 | 0x22536844 |
| npc_generic_actor | 0x223C6B90 | 0x22534BDC |
| npc_civilian | 0x223C6630 | 0x2253342C |
| npc_druglab_jetpack | 0x223C68B0 | 0x22534514 |
| npc_recombinant_major | 0x223C74E0 | 0x225372B8 |
| npc_jessica | 0x223C6E10 | 0x22535964 |
| npc_recombinant_minor | 0x223C7630 | 0x22537788 |
| npc_securitybot | 0x223C7700 | 0x22537B2C |
| npc_druglab_worker | 0x223C68E0 | 0x22534544 |
| npc_druglab_helicopter | 0x223C77E0 | 0x22537DB0 |
| npc_quadralex | 0x223C7350 | 0x22536CF8 |
| npc_jc | 0x223C6DF0 | 0x22535938 |
| npc_hologram | 0x223C6D70 | 0x22535794 |
| npc_sintek_grunt | 0x223C77A0 | 0x22537CEC |

All 17 native registration functions call the same original
registry singleton getter `0x2228B110`. Factory object pointers
are different for these 17 names, including the nearby
`npc_druglab_grunt_pistol`, `npc_druglab_grunt_scattergun`,
`npc_druglab_grunt_assault_rifle` registrations.
Do **not** infer from their proximity that these share all
constructors, behavior, weapons, animations or Source vtable ABI.

### What this changes about the porting priority

Our prior unknown-entity logs came from an environment that did NOT
contain working SiN server game code. They do NOT show missing classes
in the original retail game. This audit establishes that every
owner-observed `npc_` name has a genuine native original factory
registration. The next gap is reproducing actual class allocation,
spawn, AI and networking in an engine-compatible server module.

The four weaponized druglab grunt classnames alone account for
**433 repeated unknown-entity error occurrences**:
pistol=195, assault rifle=103, elite=69, scattergun=66.
That makes grunt factory/constructor/weapon wiring a more productive
next research and engineering target than Source engine physics,
model loading or prop rendering, which the owner has already found
working in their Half-Life 2 map port.

Prior useful gameplay code already in repo includes source-backed
assault/weapon C interfaces and the real SiN dynamic/secret
trigger Spawn/Touch. This phase adds no fake `npc_*` aliases;
it only proves the original factory registration basis before
writing any AI that might otherwise be fabricated.

## Reproducible audit

Owner-private binary verification:

```sh
python3 research/tools/audit_sin_original_npc_factories.py \
  --client /private/original/client.dll \
  --server /private/original/server.dll \
  --output /private/sin-original-npc-registry-metadata.json
```

The output is bounded metadata, not original compiled code, actor
models or proprietary game assets.

Original-free public synthetic verification (all original sample
code bytes are fabricated in a DIFFERENT address space):

```sh
python3 research/tools/audit_sin_original_npc_factories.py --self-test
ctest --test-dir build -R sin_original_npc_factory_17_registration_synthetic --output-on-failure
```

The synthetic regression independently mutates all 22 bytes of its
invented registration function, eight factory/singleton address
conditions, and refuses fake original DLL identities. CI does not
access the user's proprietary originals.

## Remaining proof before a playable map

1. Recover original grunt class-factory allocation paths, RTTI
   inheritance and class-specific constructors from the known
   17 factory addresses, without claiming shared vtables from
   classname or visual similarity.
2. Reconstruct NPC entity class registration under a real
   Windows x86 Source-compatible `ServerGameDLL003` host.
3. Implement engine-owned NPC Spawn/Think/schedules, player/weapon
   interactions, AI decisions, networking, map I/O and save/restore.
4. Only after verified Windows original-map execution attempt
   native Android and Quest 3 VR integration.

**This PR is a verified original factory inventory, not 17
implemented NPCs, normal original DLLInit, or playable gameplay.**

## New original construction lead: three Druglab Grunt variants share one factory vtable

Original private binary inspection established a further, more
actionable observation **beyond the 17 registration calls**:

| Native original factory object | Registered classname |
|---|---|
| `0x22536840` | `npc_druglab_grunt_pistol` |
| `0x22536844` | `npc_druglab_grunt_scattergun` |
| `0x22536848` | `npc_druglab_grunt_assault_rifle` |

Each of the three distinct objects stores EXACTLY the same original
virtual-table pointer `0x2249A9F8`. That native factory table
has RTTI object locator `0x224D23F4`, first method
`0x2236A320`, second method `0x22369E30`.

All **three entire original compiled function bodies** have been
SHA256-pinned:
- Factory creator `0x2236A320`: 23 bytes,
  digest `6eecf5ca8aa241b73c4f1557c98667941ac865d58014926bdfdd055381792de9`.
- Factory delegated destruction `0x22369E30`: 16 bytes,
  digest `ed8abac6973adb2766ebee7d4e8a19d917263843bcec4e4a25f52cc4c4203a21`.
- Shared allocator/constructor `0x2236A340`: 87 bytes,
  digest `4df0df1f66082cfa848b96edff3977f7c6cb35e4ff31d2a7918dd244028d7ad6`.

In the factory method, the game calls `0x2236A340` and adds
**`0x160` bytes** to the object pointer before returning a
subobject view. The shared allocator requests **`0xFD8` bytes**
from original `0x22138940`, calls original base constructor
`0x22360D80`, installs three native vtable pointers at object
offsets **+0x0, +0x790, +0xC14**, and then dispatches the
new object's virtual byte offset `+0x70` with the forwarded
factory argument. The second factory method invokes virtual
byte offset `+0x18` on its non-null argument.

These observations are independently anchored by three exact
native relative CALL edges, source-byte field/return witnesses
and full-function hashes in the expanded original-only verifier.
Private local verification passed all recorded checks. The
original null-allocation branch is unsafe to re-create verbatim
as a modern error-handling strategy; the port must fail closed.

**Do not infer from one factory vtable that the three actors
use identical initial weapons, AI schedules, model selections
or spawn KeyValues.** They remain separate original classnames;
the registered names or native entity fields may determine
their behavior later. The elite variant's original factory
object is *different* (`0x225346A0`), so it is not part
of this proven three-object alias set.

Next highest-value native decompilation targets are the
factory's `0x22360D80` base-constructor class identity and
the observed virtual `+0x70` initialization dispatch,
followed by class-specific weapon selection and the first
real engine-owned NPC creation in Windows x86.
