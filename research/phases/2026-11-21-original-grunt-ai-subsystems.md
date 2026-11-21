# Phase 534: original MobGrunt/Druglab Grunt post-construction CAI subsystem initialization

## Scope and proof boundary

This continues the exact owner-private original 2006 SiN `server.dll` and
`client.dll` from Phase533, without replacing Source/HL2 physics, model,
prop, or material loading. The original DLLs remain private and are NEVER
committed. The owner's Drive `SE1/bin.zip` matches the pair of SHA-256s:

- `server.dll`: `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`
- `client.dll`: `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`

The original Grunt primary vtable `0x2249AAA4` carries MSVC RTTI name
`CNPC_MobGrunt`, not `CDruglabGrunt` (earlier project prose used that
informal term). Its post-constructor virtual `+0x3F8` at `0x22359210`
calls `0x22031D50`. We fully checked **276 bytes**, SHA-256
`575ffffba81df6bc80a78e347c3c8323f73412852556a9d632f79cf8ef75ebbd`.

## Seven original virtual factories in *actual execution order*

The original method calls a factory virtual, writes the resulting pointer
into a distinct owner field, then checks EAX for NULL. A NULL immediately
returns false without entering the later wiring phase. The original
method does not visibly roll back already-created components.

| # | Virtual byte offset | Native factory VA | Stored field | First allocation | RTTI-confirmed type |
|---|---|---|---|---:|---|
| 1 | `+0x3FC` | `0x22048F90` | `+0x898` | `0x78` | `CAI_Senses` |
| 2 | `+0x404` | `0x2216E9C0` | `+0x908` | `0xF0` | `CAI_BlendedMotor` |
| 3 | `+0x408` | `0x22031ED0` | `+0x8FC` | `0xD4` | `CAI_LocalNavigator` |
| 4 | `+0x400` | `0x22031EA0` | `+0x904` | `0x44` | `CAI_MoveProbe` |
| 5 | `+0x40C` | `0x2216DDD0` | `+0x8F8` | `0x9C` | `CAI_Navigator` |
| 6 | `+0x410` | `0x220379F0` | `+0x900` | `0x18` | `CAI_Pathfinder` |
| 7 | `+0x414` | `0x22031F10` | `+0x9D8` | `0x10` | `CAI_TacticalServices` |

These are not guessed type labels. The actual factory implementations and
associated constructors install vtable pointers whose MSVC x86
Complete Object Locator, TypeDescriptor and decorated RTTI class strings
were checked against the genuine original PE32. In this build the RTTI
TypeDescriptor objects live in `.data`, while COL and vtables live
in `.rdata`; an auditor MUST allow that proven layout. The Navigator
factory (`0x2216DDD0`) delegates to `0x22031F50` for the actual
`0x9C` initial allocation and `0x220747A0` construction.

All **seven complete factory methods** are individually SHA-verified.
Together with `0x22031D50`, the total verified complete native function
bytes are **616**, and the auditor additionally checks the seven
vtable targets, seven pointer-store witnesses, seven RTTI types, an
embedded-member dispatch witness, shared-context global reads, and
11 original direct-call edges. The first allocation size is *not* the
transitive cost of nested constructor allocations.

## Exact subsequent wiring (after all seven factories succeed)

1. Dispatch virtual `+0x04` on embedded NPC member `owner+0x9E4`,
   forwarding the NPC object pointer.
2. Call `0x2206BF10` on the object stored at `owner+0x908`, passing
   `owner+0x8FC` component pointer adjusted by `+0x8`.
3. Call `0x220674D0` on the object stored at `owner+0x8FC`, passing
   `owner+0x8F8` component pointer adjusted by `+0x8`.
4. Invoke virtual `+0x10` on the object at `owner+0x8F8`, passing
   the pointer stored at global `0x225490C8`.
5. Call `0x22083CD0` on the object at `owner+0x900`, passing the
   same original global pointer.
6. Call `0x220AB0D0` on the object at `owner+0x9D8`, passing the
   same original global pointer.
7. Return true to `0x22359210`, which requires additional original
   virtual `+0x76C` success before invoking `+0x7AC` and member
   `+0xCC0` initializer `0x22358FA0`.

The C++ host interface models **only proven call order and roles**.
It does not cast original PE32 virtual pointers into the port, does not
invent Source CAI_ object layouts, and does not assign a speculative
semantic label to `0x225490C8`. Its host receives the exact native
slot, field, type and initial allocation request as metadata and must
supply genuine owned instances. The host can refuse on any stage.
Its no-rollback behavior for partial success mirrors the observable
original method, rather than claiming safe automatic reclamation.

## Reproducible checks

- `python3 research/tools/audit_sin_original_grunt_ai_subsystems.py --self-test`
  uses two invented relative CALLs and exercises all five byte mutations
  for each, the seven-slot metadata contract and whole-SHA refusal.
- `python3 research/tools/audit_sin_original_grunt_ai_subsystems.py --bin-zip <owner-private-bin.zip>`
  requires both exact SHA-matched DLLs and prints bounded metadata only.
- `sin_original_grunt_ai_subsystems_tests` uses a synthetic host to
  verify exact order of 7 factories plus 6 wiring calls, failure at
  each of the 13 callback stages, reentrancy refusal, exception
  handling and successful retry.

**No real Windows game-module startup is claimed.** These methods
still require a compatible Windows x86 `ServerGameDLL003` initialization,
engine-owned AI objects/edict state, native `ServerGameEnts001`, send
tables, and a map-level runtime test. Android/Quest 3 follows successful
Windows x86 integration. This phase deliberately does not revisit
models, props, physics or surfaces.
