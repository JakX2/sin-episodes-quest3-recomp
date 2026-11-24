# Phase 710: shared client entity initialization

Fresh SHA-matching original client inspection confirms all five Phase708
primary vtables (242b44d4, 242b416c, 242c4654, 242c56c4, 242c2ff4) use
slot1 240c46b0 and slot37 240c8fe0. Slot17 map getters are 240d5dc0
(player), 24054e80 (hand), 24062070 (weapons). All addresses hex, evidence only.

Slot37 original:
1. +4 secondary vslot0 24054ec0 returns primary pointer, no stack cleanup.
2. Registry insertion 241280a0/24127f50 (Phase709).
3. 24119570 registers collision subobject +16c through an engine service
   and stores a 16-bit service handle at subobject +26. Remains explicit
   host operation; no collision implementation.
4. Primary vslot17 returns class-dependent interpolation map (may lazily
   initialize it); 240c8e90 traverses maps through +18, reads count +c,
   12-byte entries at pointer +0. Entry +0 kind, +4 data, +8 watcher.
   Calls watcher vslot0(data, kind), vslot5() returns flags, helper
   240c88e0 computes duration, watcher vslot1(float duration).
5. Stores index at +50 and returns true.

Independent adapter uses the actual native registry and verified
secondary/map/watcher virtual dispatch, delegates collision registration
and live interpolation duration calculation to host. Maps preflighted
with 32-map/256-entry limits, rejects cycles and malformed storage before
watcher dispatch. Follows derived map then parent. No original code
addresses called. On failed host operation, explicit host rollback
unwinds attempted collision/interpolation side effects, then registry
removes entity. If removal cannot complete, returns -1 and retained
handle; owner must not free object. Callback host contract requires
valid writable objects, stable maps and vtables, no registry mutation.
Final index write only on success. Paired PE32 fixture verifies exact
callback order, two-map traversal, null map, collision and duration
failure, cycle rejection, registry lookup during callbacks and rollback.
No original physics/model/prop work, no retail engine registration claim.
