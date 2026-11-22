# Gameplay Phase626: generation-safe original Elite Spawn -> animation event 0x43 host boundary

## Gap closed

Phase622 recovered a distinct original 2006 `CEntityFactory<CNPC_EliteGrunt>`. Phase623 recovered the complete 505-byte Elite `Spawn` method and its complete 13-byte virtual `+0x798` byte-field helper. Phase593 recovered the original 360-byte Elite animation-event override at `0x22355480`, including its event `0x43` branch.

Those are accurate independent native-behavior **reference implementations**, but their shared **original field `this+0xFDC`** was not carried by one owner/generation-safe host session. Phase626 adds the missing independent binary interface for a future actual Source engine-owned Elite NPC.

## The direct original source relationship

- Original Elite `Spawn` (source `0x22355820`) resolves the active weapon, checks RTTI for `CWeaponChaingun`, invokes typed virtual `+0x08`, and writes the returned word to **`this+0xFDC`**; if unsuccessful, writes `0xFFFFFFFF`.
- Original Elite event override `0x22355480`, case `0x43`, reads **`this+0xFDC`**, resolves its slot/serial handle only if valid, otherwise uses a null pointer, calls original gate `0x22315320` **even for the null case**, sets **byte `this+0xFD9`**, then forwards the animation event to inherited Grunt handling only when allowed.
- A cached Spawn handle can become stale. Therefore Phase626's event43 bridge asks the real host to **re-read the live NPC's current `+0xFDC` word at every event**, rather than treating the initial Spawn output as permanent. The cached value is retained only for diagnostics.
- Phase623's Spawn clears `+0xFD9` and, through its source-verified virtual `+0x798` target, clears `+0xCB2`. Those are available through exact per-entity host callbacks.

## New independent eight-export C ABI

New standalone shared library: `sin_elite_native_host_c_abi` (Windows `sin_elite_native_host_c_abi.dll`). Its public header is `include/sin_compat/elite_native_host_c_api_v1.h`.

Exports exactly:

1. `sin_elite_native_abi_version_v1`
2. `sin_elite_native_create_v1`
3. `sin_elite_native_destroy_v1`
4. `sin_elite_native_attach_v1`
5. `sin_elite_native_spawn_v1`
6. `sin_elite_native_event43_v1`
7. `sin_elite_native_snapshot_v1`
8. `sin_elite_native_detach_v1`

The host passes a canonical original classname `npc_druglab_grunt_elite`, authoritative actor `(index, generation)` and opaque original entity ownership token. The `confirm_existing_native_elite` callback MUST verify the real entity factory/class and current owner. Attach, Spawn, event and snapshot all recheck identity under a nonreentrant, single-threaded guard. A stale generation never reaches behavior callbacks; only the original generation may detach even after its real NPC died. An Elite actor's session can never silently become a new NPC using its recycled index.

**Host ABI properties:**

- Native engine still owns all game NPC objects. The bridge owns only its `std::map` of independent per-actor metadata.
- `Spawn` uses the Phase623 original-source `SpawnReference` and host callbacks for the original symbol, inherited Spawn, two independent ConVar reads and field+flag writes, typed active-weapon resolution, and `+0xFDC` assignment.
- Spawn is **one-shot per actor generation**, including failures with possible partial native effects. It refuses a second invocation rather than inventing a native rollback for earlier weapon, flag or inherited Spawn writes. A genuine new entity generation can attach after old-generation removal.
- Event43 supports **only the verified original event ID `0x43`**. It obtains the live serial handle from the engine host, conditionally resolves it, always invokes the original gate (including null token), publishes `+0xFD9` **before** the optional inherited event callback, and returns distinct BLOCKED/FORWARDED statuses. If inherited event forwarding fails after publication, the already-published predicate remains recorded; no fake rollback.
- Other original Elite events (notably `0x03` and `0x47` from Phase593) are **not yet bridged by this ABI**; the engine must leave those to its original implementation until their real host callback contracts are verified.
- Callback userdata stays host-owned. There is no invocation of retail PE32 pointers, native Source vtables, `CreateInterface`, `CBaseNPC` constructor or engine interception in the library.
- `snapshot` exposes last successful handle/predicate plus one-shot Spawn flags to help a future real engine host verify behavior. It refuses stale/expired actor identity without changing the output.

The original four-alias MobGrunt seven-export bridge (`sin_grunt_native_host_c_abi`) remains independent. The Elite class is deliberately **not added to the MobGrunt registry**, because its registered retail factory and constructor are distinct.

## Regressions

`tests/test_elite_native_host_c_api_phase626.c` is genuine C11. It uses a synthetic engine host, not retail code, to verify:

- Active original Elite identity, one-shot Spawn, two independent field+0x98/+0x9C scalar conversions, exact original 0x00100000 / 0x00002000 flags and source dirty path;
- Spawn's `+0xFDC` encoded handle feeding event43's resolution/gate/FD9/inherited dispatch;
- Changed `+0xFDC` in the native host between Spawn and event: event43 reads the current field and updates its diagnostic snapshot;
- Absent/wrong-type chaingun returns `0xFFFFFFFF`, skips serial resolution but still calls original gate with zero;
- Gate-denied event does not forward inherited event; native `+0xFD9` publication runs before allowed forwarding; publish/forward failure outcomes preserve partial native effects;
- Host reentry attempts to snapshot/destroy during an identity callback, denied with REENTRANT;
- Host identity revocation, invalid native class, stale generation, death-safe exact detach, index reuse and a one-shot Spawn after partial failure;
- Invalid ABI/struct sizes/required callbacks, missing data, nonfinite/out-of-range x87 inputs, and failure at all 12 original Spawn callback stages.

`tests/test_elite_native_host_dynload_phase626.cpp` uses real `dlopen/dlsym` (Unix/Android) or `LoadLibrary/GetProcAddress` (Win32) to verify the exact eight C symbols and version. Linux uses a version script restricting exports; Windows x86 uses a `.def` file with exactly those eight exports. Native C/C++ and Windows x86 tests are hooked into their existing CMake/CI configurations.

## Proof boundary and next action

This is an **actual callable DLL interface**, not a synthetic string-only model, but CI fixtures do not create real SiN engine NPCs. The Windows `server.dll` factory proxy continues to forward the original `CreateInterface` unchanged and does not automatically load or invoke the Elite bridge. The owner must still provide/verify a native host that sees the real original Elite factory, receives authoritative lifecycle/Spawn and animation events, and supplies safe engine callbacks. Only after that can in-level Elite NPC execution be claimed.

No game DLL bytes were added to GitHub. All earlier original-code hashes, source-byte totals, physics/model/prop work priorities and Android/Quest boot claims are unchanged.
