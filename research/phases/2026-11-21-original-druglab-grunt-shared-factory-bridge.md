# Phase 531: shared original Druglab Grunt NPC factory constructor reference

## Source provenance

The user's exact original 2006 `server.dll` was verified by SHA-256
`568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`.
Phase530 independently checked all 17 NPC class registrations that
were reported unknown by the previous Source map-port logs.

Three DISTINCT original NPC class factory registrations share one
RTTI-backed factory vtable at original `0x2249A9F8`:

- `npc_druglab_grunt_pistol`: registered factory `0x22536840`.
- `npc_druglab_grunt_scattergun`: registered factory `0x22536844`.
- `npc_druglab_grunt_assault_rifle`: registered factory `0x22536848`.

The original factory vtable has method0 `0x2236A320`, method1
`0x22369E30`. Source-audited original `0x2236A320` calls
`0x2236A340` to allocate a native NPC, then returns the object's
subobject pointer at `+0x160`. The 87-byte constructor:

1. Requests an original allocation of `0xFD8` bytes through
   `0x22138940`.
2. Calls an original base constructor `0x22360D80`.
3. Writes native vtable pointers at object offsets +0x0,
   +0x790 and +0xC14 (exact original addresses are retained
   in metadata-only Phase530 audit, NEVER called on ARM64).
4. Invokes native object virtual byte +0x70 with the factory's
   forwarded argument.
5. Returns its original native allocation pointer, leaving the
   wrapper to add +0x160.

The second factory method delegates to its non-null argument's
native virtual byte +0x18. These facts are derived from three
complete original function SHA checks and three exact CALL rel32
witnesses, not inferred from NPC models or classname similarity.

## New independently authored code

`include/sin_compat/original_druglab_grunt_factory.hpp` and
`src/original_druglab_grunt_factory.cpp` implement a native-host
**control-flow reference**, with explicit roles instead of pretending
that original 2006 32-bit vtable addresses are valid in new game
DLLs, Android or Quest:

- Recognizes EXACTLY the three original registered shared-constructor
  classnames. The elite and regular SinTek grunt classnames are
  excluded, since their native factory objects are different.
- Calls the host's allocator for 0xFD8 bytes, then base-constructor,
  vptr roles at +0/+0x790/+0xC14, then virtual +0x70 with the
  opaque original argument, before returning base+0x160.
- Preserves the original second factory dispatch as a distinct
  virtual18 host callback with null argument as a no-op.
- When a callback fails, refuses publication and attempts to
  reclaim the unpublished native allocation. If reclamation fails,
  returns an explicit rollback-failure status. If a host returns
  a pointer that cannot safely be adjusted by +0x160, it fails.
- Rejects reentrant callbacks instead of attempting to allocate a
  second NPC from a partially initialized first one.

**Original vs port distinction:** the original native x86 code's
allocation-failure path is not safe to blindly reuse. Failure
rollback, integer overflow checks and reentrancy rejection here
are independently authored conservative constraints, not alleged
original behavior. A modern host's `establish_native_vtable`
is passed an offset and a symbolic role, NEVER a raw original
PE32 code pointer.

`tests/test_original_druglab_grunt_factory.cpp` runs original-free
native C++ tests of all three classnames, expected constructor
callback ordering and exact offsets, original argument forwarding,
pointer adjustment, delegate virtual18, missing native allocator,
failed base/three vtable/post-init stages, failed rollback,
overflow and callback reentrancy. CMake registers
`sin_original_druglab_grunt_factory_tests`.

## What is still missing

This does **not** instantiate a genuine `CBaseNPC` inside the
original game. The native engine must create the appropriate actual
class object, resolve original `0x22360D80` base-class semantics,
implement the `+0x70` post-initialization behavior, register
network sendtables, handle engine-owned edicts, integrate think/schedules,
AI and weapons, plus Save/Restore. The three aliases' *shared*
factory path does NOT establish their initial weapon configuration
or prove that they run identical AI.

That research and integration is materially closer to missing SiN
gameplay than recoding physics, surface properties or models that the
user already ported successfully into Half-Life 2. Windows x86
`ServerGameDLL003::DLLInit` and a true native map-host remain
blocking before Android/Quest 3 playability.
