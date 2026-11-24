# Phase 676: original player damage gate, copy/scaling and fractional health stage

Direct inspection of the SHA256-pinned original server.dll
568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650
recovered these connected stages:

- CHL2_Player damage method 0x2239fa00, 389 bytes,
  ed4730225428e2c5841e609bc8cde3b094bbdaed0d444f3f2651cb94216c13fe.
  The initial lookup reads "gordon_invulnerable" at 0x224af48c,
  resolves the global via 0x221f4680/0x221f4690, and rejects state 1
  or nonzero player life state dword +0x8c. State 2 does not imply
  invulnerability. Missing global returns 0.
- The final copy branch uses original 72-byte copy constructor
  0x2202f480 (131 bytes,
  e3cadeac292ed0f3ddaaee2c1e3efa8d586ecff097949d66f1ddcfad6e74528a).
  At 0x2239fb51..0x2239fb70, resolved attacker and inflictor
  pointers are separately compared against self. Either match skips
  helper 0x221f8770 (24 bytes,
  03de2613f84d0403c9eb81e4e2153fe8199e866a9238f40652b65597d34f7fe6).
  This helper copies +0x2c damage to +0x34, then invokes game rules
  slot63 (+0xfc), original CSE1Rules 0x222d5420 (34 bytes,
  5c48c6832387452dabc568be81434c50394f250dd11346378d9c1dbd6b00a97a).
  That method bypasses scale when type+0x38 & 0x80024021 != 0.
  The original source damage record is not modified.
- Common alive health branch 0x222a7c51..0x222a7d13 (195 bytes,
  887fc7e6831f6a1d8da5ad28c2035fcef0f7168a36c6379fe3fe358f0a55b089).
  Entity +0x94 == 1 bypasses this stage. Otherwise the CRT floor
  helper 0x223a67a0 is used to obtain the fractional remainder,
  stored as f32, which adds to the per-character accumulator at
  +0x634. Constant at 0x223d22b8 is double 1.0. A carry >=1 adds
  one to the integer reduction and subtracts one from the stored
  fraction. Health change calls virtual +0x1b4 before writing +0x9c.
  Zero integer reduction does not invoke that virtual or write HP.
  This is more detailed than the earlier simplified Phase125 reducer.

New port CombatDamageApi integrates these stages into the actual
independent server module using its existing campaign and CGlobalState.
The player gate and copy stage are separate from unreconstructed
intervening player effects/armor/filters; host calls the gate early
and copy preparation only after those branches are satisfied. Host
supplies resolved equality observations, not guessed handle bits.
The final health stage accepts already filtered/scaled amount and
current health/carry, emits an explicit write/notification descriptor,
does not write entity memory or dispatch death. Double intermediates
model normal finite positive float inputs; reject negative/nonfinite,
out-of-range carry and overflow as independent port guards, not
original behavior. No full x87 exception/rounding-mode parity claim.

Actual loaded-module test covers shared env_global invulnerability,
self damage, exact copied fields, all bypass bits, difficulty change,
fractional accumulation, carry, events-only branch and refusal without
output mutation. No retail player, native armor, game rules vtable,
or actual engine entity writes are claimed. No original bytes committed.
