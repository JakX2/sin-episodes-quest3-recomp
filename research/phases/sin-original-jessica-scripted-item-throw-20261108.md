# Original 2006 SiN Jessica item throw: binary-verified reconstruction

**November 8, 2026.** One bounded original gameplay subsystem; **not** a native SourceVR NPC or functioning Quest 3 game module.

## Sources and provenance

This pass directly rechecked the owner's legally obtained original Windows x86 `server.dll` extracted from the supplied Drive `sin files/bin.zip`. **SHA-256:** `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`, preferred image base `0x22000000`. Predecessor: [Round 07 Jessica/JC audit](../2026-11-04-original-dll/reports/SiN_Original_Jessica_JC_AI_DeepDive_07.md). The separate client DLL was not decompiled in this narrow phase.

Run the read-only verifier on private, unchanged game files:

```sh
python3 research/tools/audit_sin_original_jessica_throw.py --server /path/to/original/server.dll
```

The verifier checks full-image SHA, **six original-function SHA witnesses**, exact attachment name and schedule ordering, plus four original x86 relative call targets. It prints metadata only. The original DLL, disassembly dumps, maps and assets must remain outside GitHub.

## What the original instructions actually establish

| Original VA | Binary-verified behavior |
| --- | --- |
| `0x223622D0` | `InputThrowItemAtPlayer`: accept type 2 string or empty; store `Jessica+0x144C`; set AI condition `0x47` even for wrong input type |
| `0x22362300` | `InputHolsterWeapon`: resolve player; if present call player virtual slot `+0x348` |
| `0x22362320` | Primary throw ammo: accept type 5 integer or zero; store `+0x1450` |
| `0x22362350` | Secondary throw ammo: accept type 5 integer or zero; store `+0x1454` |
| `0x22362380` | Select `SCHED_THROW_ITEM` / `0x59` if condition `0x47` is set, otherwise delegate to companion base class |
| `0x22362420` | On **runtime-registered** `AE_JESSICA_THROW_ITEM`: clear condition `0x47` first; obtain right-hand attachment; create selected entity; if it is a combat weapon, copy only nonnegative configured ammo and reset each copied input field to -1; invoke movement/launch. Nonmatching events delegate to base class |

The exact `anim_attachment_RH` string is at `0x22490254`. The original named schedule at `0x224978C0` contains `TASK_FACE_PLAYER` followed by `TASK_PLAY_SEQUENCE ACTIVITY:ACT_THROW_ITEM`, with no listed interrupts. Separate bit-array helpers `0x22039760` (set), `0x220397C0` (test), and `0x220398A0` (clear) are corroborated by their x86 instructions and call sites. The schedule ID `0x59` is **not** the animation event's runtime-assigned ID.

The original spawn default for Jessica's temporary ammo fields is -1 according to Round 07. Successful weapon-only, nonnegative ammo assignment writes weapon offsets `+0x4D4` and `+0x4D8`; if spawning fails or an object isn't a weapon, the ammo values are not consumed.

## New independently authored C++17

- `include/sin_compat/original_jessica_throw.hpp`
- `src/original_jessica_throw.cpp`
- `tests/test_original_jessica_throw.cpp`

`ThrowRuntime` implements the verified typed input behavior, condition-based schedule selection, live animation event binding, clear-before-spawn ordering, conditional ammo transfer, and holster forwarding via an explicit `IHost`. Synthetic tests prove callback ordering and edge cases. The host is responsible for actual NPC condition storage, model attachments, engine entity creation and player-targeted motion.

**Unknown:** the full original throw trajectory formula. A float `15.0f` appears in the x86 movement path, but its exact physical role is not yet proven; it is deliberately **not** treated as speed. The host may return spawn/launch failure rather than inventing game physics.

**Not yet integrated:** `npc_jessica` Source game-entity factory, native game AI scheduler, Android ARM64 game module, actor model, or any actual SiN episode level. Passing portable tests doesn't demonstrate any of those. Keep the code isolated until a verified compatible engine host can supply these services.
