# Phase 684: native ScatterGun ammo-change and pump behavior

Authenticated unchanged original server.dll SHA256
568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650.
CWeaponScatterGun primary vtable 0x2247a1e4. Constructor
0x2231ba90 sets snapshots +0x57c/+0x580 to -1, +0x584 and
pending +0x589 to zero. Slot209 0x2231bb00 copies the primary
clip +0x4ec into snapshot before base tail-call 0x222cc340.
Slot216 0x2231c3e0, 104 bytes, SHA256
5e5668894afb65fd7dd45078abe9d58cae147714e302b7e5952411b9fb91af43:
base 0x222cd050 first, then snapshot update, secondary only
while zoom byte +0x578 set. First -1 snapshot does not latch.
Deferred function 0x2231c2f0, 240 bytes, SHA256
a1150899b9ecb8e31f9c5b93d749cc6c93abe7529fc5a2ccdbf0f6749efc8916:
pending required, primary deadline +0x49c <= gpGlobals+0xc,
signed clip >0, +0x4ac reload false, owner resolved. Only after
these gates does original clear pending, call weapon virtual
+0x3c4 with 13,0, choose normal activity 0x126 or lookup literal
ACT_VM_ZOOMED_PUMP (0x2247aaf0) through 0x220ecb40, then
virtual +0x314 with returned activity. Then duration helper
0x220eede0(this,+0x3ec) and refreshed curtime. Owner deadline
+0x60c is set via 0x2203d190; its actual resulting value is
read back before 0x22173260 writes weapon +0x49c and again
before 0x221732a0 writes weapon +0x4a0. All three setters have
original dirty notification branches; they remain host work.

Native module reuses Phase165 ammo snapshot logic within the
Phase683 map-owned scattergun. The host supplies actual live
ammo/owner/time observations after base frame, and delivers
the sound/activity/timer requests through transient callbacks.
The module does not duplicate live ammo, infer single-round
reload mode, write engine memory or claim completed attack.
Callbacks occur after consuming pending, and failures are
reported as uncertain with explicit observed-state resync;
this is a port safeguard, not original exception handling.
Tests exercise both activity modes, first snapshots, deferred
gates, pre-deploy update, callback order and actual readback
of owner timer, no replay on failed callback, stale identity.
No original binary or script bytes committed.
