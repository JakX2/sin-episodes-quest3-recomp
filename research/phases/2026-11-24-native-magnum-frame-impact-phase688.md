# Phase 688: original Magnum deferred frame and full DUAG impact ordering

Fresh local SHA-256 verification of owner-private bin.zip extracted
server.dll 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650
and client.dll 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d.
PE32 RTTI verifies CWeaponMagnum primary vtable 0x2247875c, COL
0x224cf560. Slot216 0x22316b30, 133 bytes, SHA256
427f6d70c4a86d53b5777df6d11d63eb9d49b3b0d783162d8e92f913c5aa926c.
Constructor 0x22316ac0 clears raw pending DWORD +0x57c; no
semantic source-level name assigned. Frame checks signed count >0,
then +0x580 <= gpGlobals+0xc (unordered skips). Saves +0x49c,
calls tracked setter 0x22173260 with current-time pointer, decrements
+0x57c, invokes weapon virtual +0x388 (slot226 primary), restores
saved +0x49c, re-reads count, if positive stores clock + original
0.05f at 0x22478748 into +0x580. Base 0x222cd050 always runs last
on ordinary paths. No assumption about who initially schedules count.

Slot102 0x22316910, 229 bytes, SHA256
6a1600855239228a361be4cdbe971ea1d8c9b965be2bf6d97ccf82d4c5e978a5.
Base 0x2214a120 is directly verified at 0x22316922 before DUAG
bit 0x00400000 and raw trace byte+0x42 bit4 tests.
Phase143 omitted a real intermediate effect:
- 0x222701c0 filter ctor, RTTI CPVSFilter vtable 0x22405970,
  filter population 0x222708e0 at trace+0xc.
- Three f32 differences end-start, normalization function pointer
  at 0x2251b1d0 initially 0x22268100. Default helper SHA256
  04f25cc78823d4c84e55b8e91427aee57b3e2bd5540abd7b2d7adf0c8bd8b92d
  divides by length+float epsilon (0x22454570). Runtime CPU-specific
  math dispatch/extended rounding equivalence not claimed.
- Original 0x224daaf8 -> CTempEntsSystem object 0x224daafc,
  vtable 0x223d51e4 slot30 (+0x78) -> 0x2200d940, forwarding
  0x2201f300. Global temp entity 0x22546230 initialized by
  0x223c98e0 with RTTI CTE Smoke (CTESmoke) vtable 0x223d6290.
  Matches owner SDK2004 ITempEntsSystem::Smoke argument shape.
- Original precache 0x22316850 uses sprites/blueglow1.vmt at
  0x22411da8, records returned model index in 0x225986d0.
  Effect arguments: CPVSFilter at trace end, normalized difference
  as the position vector, delay 0.25, precached model index,
  scale 2.5, framerate 10. This unusual position is preserved,
  not silently changed to trace end.
- Then decal virtual +0x110 with DUAGDecal1, DUAGDecal2, followed
  by filter teardown. Resource names independently verified.

The native map-owned Magnum now has an explicit before-base frame
and impact API. Host must supply authoritative pending count/time,
tracked timer setters, real primary/base callbacks, current trace
after base impact and temp/decal effect delivery. No original
object memory or absolute address is invoked. Finite-input
normalization approximates original default math; callbacks own
actual precache, recipient filter and effect construction.
Uncertain callbacks block both paths until explicit host
acknowledgment; no automatic rollback/replay. Loaded-module tests
cover due/skip/last/re-read count, exact timer ordering, base last,
impact gate, zero vector, smoke+two decals and partial failure.

Additional newly authenticated original secondary slot227
0x22316cf0 (1145 bytes, SHA256
6b89fc7e2b14bfcadfb1d9d7209cc1900358fde284a434c4efbc2f54744f0e4b).
Its initial clip gate tests equality to zero, unlike the
ScatterGun signed nonpositive gate. Owner +0x120 controls
later owner animation, direct/fallback vectors and ammo path.
Full secondary attack record and actual host firing remain
separate work; this phase does not claim that method implemented.
No proprietary raw bytes or assets committed.
