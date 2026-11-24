# Phase 724: native original player healing and overhealth
Fresh local owner SHA256 client 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d,
server 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650.
Both CHL2_Player (224aec3c) and CBasePlayer (2245222c) slot57
22246860, 156 bytes SHA256 eec6c6ee25aeabdedcf48ca376cc06d1193de04ee23ad746580bbacd2f65e582.
If +94 nonzero clears +9d0 bits using ~(flags & 0xffc07fff).
If +12a1 byte zero and max +98 >100 calls vslot263(100)
(Phase722 native setter). Then combat-character 222a57a0
(19 bytes SHA256 0b26bb3840232fea7d07f72671fa44536d45227c53f1d14d47c4a815f59f3eb9)
refuses +94 zero; base 22140be0 (199 bytes SHA256
3ed676f823deff69136498895978aa954fee03d267f693b098806a1fda8abf41)
requires +16c nonnull and signed +94 >=2. If health >= max returns
zero. Otherwise x87 truncates float amount, adds DWORD to +9c via
2213e0d0 (vslot109 dirty before changed write). Player vslot72 gates
health event 222b5ec0/222b6260, with PRECLAMP health and delta,
even if the amount truncates to zero. Then re-read max, if current
above max use tracked setter 2203d0a0. Returns final delta.
Slot57 returns boolean, and on nonzero delta invokes 22287250
Fade (message string 22405f50) with RGBA 0,0,128,64, duration 1,
hold .5, flags 1. Finally if health >100 clears byte +12a1.
The byte is not a permanent overhealth indicator: later heal
can reduce max to 100 while current health stays above it.
Native uses actual compatible player offsets, no replacement engine
network/event/fade implementation. Host handles those original
effect boundaries. Finite/truncation guards and callback failure
reconciliation are port differences. Also clarifies Phase719
semantic integer dose adapter: original dispenser at 222f728b
stores floor result as float then pushes its bits into slot57.
Do not pass integer bits into real virtual call.
The new pickup path shares native heal body inside existing
pickup busy scope and reuses Phase722 maximum setter. Test asserts
damage mask, base gates, zero truncation event, preclamp event,
two health notifications, successful overhealth consumption,
subsequent maximum reset without current clamp, failed fade and
actual integrated MegaHealth plus subsequent heal. No physics work.
