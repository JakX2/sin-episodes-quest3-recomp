# Gameplay Phase621: Fourth retail CNPC_MobGrunt factory alias and unmodified default Spawn branch

## Why this phase exists

The owner's original `SE1/bin.zip` was retrieved from their Drive and extracted locally. Both unmodified 2006 PE32 DLL SHA256 fingerprints match `research/binary-provenance.json` exactly:

- Server `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650` (6,049,792 bytes).
- Client `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d` (3,592,192 bytes).

The prior Phase530 audit correctly enumerated **17 original map-missing NPC classes** and Phase531 correctly studied the three explicit weapon variants. The base `npc_druglab_grunt` was not in those missing-class reports, but retail code actually registers it as a **fourth object of the same original `CEntityFactory<CNPC_MobGrunt>` type**. This previously omitted alias was not recognized by the reconstructed Phase531 factory or Phase614/619 session registration.

## Original source authentication (one NEW complete registrar)

Original `server.dll` function `0x223C6FC0` is exactly 22 native bytes, ending in `ret`, SHA256:

`063e48f768558e1c251b08a9c79ba87af04137351a2b39851504b769bcb2398e`

The function uses the same 22-byte `call 0x2228B110; mov edx,[eax]; push classname; push factory; mov ecx,eax; call [edx]; ret` pattern as Phase530. Its classname string at `0x2249B2F4` is exactly `npc_druglab_grunt`; its unique factory object at `0x2253683C` stores the vptr `0x2249A9F8`. MSVC RTTI for that factory is `.?AV?$CEntityFactory@VCNPC_MobGrunt@@@@` at `0x225367F4`.

Its original CRT registration array also contains four **adjacent** initializer method pointers:

| Retail classname | Registrar VA | Original factory object |
|---|---|---|
| `npc_druglab_grunt` | `0x223C6FC0` | `0x2253683C` |
| `npc_druglab_grunt_pistol` | `0x223C6FE0` | `0x22536840` |
| `npc_druglab_grunt_scattergun` | `0x223C7000` | `0x22536844` |
| `npc_druglab_grunt_assault_rifle` | `0x223C7020` | `0x22536848` |

All four original factory objects store the **same** vptr `0x2249A9F8`. The original factory creation method at `0x2236A320`, underlying constructor at `0x2236A340`, and destroy delegation at `0x22369E30` were already SHA-authenticated by Phase530 and are **not counted anew**. The elite Grunt is still a different `CEntityFactory<CNPC_EliteGrunt>` registration and is not admitted to this MobGrunt registry.

## Default classname Spawn path, not a guessed loadout

Phase532 already SHA-authenticated the original complete 1,246-byte native Spawn-like virtual method `0x22369E40` (SHA256 `99dbbb09eeebe19d7260e707c4fb917ab4c4941f1b066d6368eba53b485ccff7`).

Two source control-flow witnesses prove the default classname follows the **common inherited Spawn** path, skipping its own weapon-variant overrides:

1. The three exact classname comparisons reach a final assault-rifle mismatch `jne` at `0x2236A07F`, which jumps directly to `0x2236A0D4` and **skips the explicit weapon symbol and field +0x3B0 assignments**.
2. The post-inherited-Spawn three-way classname comparison finishes at `0x2236A291`, where a near `jne` jumps to method epilogue `0x2236A317`. Therefore the default does **not** execute per-variant ConVar +0x28 to integer updates at fields +0x98/+0x9C in this method.

The common path still dispatches original virtual slots `+0x68`, `+0x1C`, `+0x6C` in that order and directly calls inherited original Spawn at `0x2235F2A0`.

**No evidence here proves the default Grunt is unarmed.** The inherited NPC initialization/Spawn path may provide other weapon/default behavior. The reference model therefore refuses to invent an initial weapon, ConVar, or mode for `npc_druglab_grunt`.

## Implementation

- `GruntFactoryReference::classify` now recognizes exact `npc_druglab_grunt` as `Variant::Base`; all four aliases follow the already-authenticated same native allocation/constructor reference. Exact-prefix lookalikes and `npc_druglab_grunt_elite` still fail closed.
- `SpawnReference::plan_spawn` uses the source-backed default branch to run **only the inherited common stages**. Existing weapon, mode +0x3B0 and numeric fields +0x98/+0x9C remain untouched, and a NaN per-variant ConVar float is intentionally not read for this non-variant path.
- Phase614 native identity registry and Phase619 seven-export native-host C bridge now accept and reattest the fourth exact classname. They still require a real engine host to confirm factory ownership, actor index/generation, class, and owner token. No NPC is constructed by these components.
- Synthetic reference tests cover the fourth factory, inherited-only Spawn, no made-up defaults, and C/CPP native-host registration and generation-safe teardown.
- `research/tools/audit_original_base_grunt_factory_phase621.py` validates the new complete 22-byte original registrar digest, the four factory objects, original factory RTTI, CRT initializer ordering, the existing Phase530 constructor and Phase532 full Spawn digest, and both authentic fallback branches. `--self-test` uses no original code.

### Private local reproduction

```sh
python3 research/tools/audit_original_base_grunt_factory_phase621.py \
  --server "<private-game-copy>/SE1/bin/server.dll" \
  --client "<private-game-copy>/SE1/bin/client.dll"
```

The original binaries remain **private** and are never committed. The established Phase601–617 accounting (43 authenticated functions / 11,233 bytes) is separate; this phase additionally authenticates **one previously omitted 22-byte original registrar**. It does not double-count the already-authenticated shared factory or Spawn.

## Remaining blocker

A source-backed factory alias is **not** an engine callback integration. The native host still needs authoritative real NPC spawn / AI think / removal signals and registered engine condition/schedule hooks. We do not guess original C++ vtables, replace original `CreateInterface`, or revisit already working HL2 physics, surfaces, models, props, or rendering. Real Windows owner-process proof and Android/Quest game startup remain outstanding.
