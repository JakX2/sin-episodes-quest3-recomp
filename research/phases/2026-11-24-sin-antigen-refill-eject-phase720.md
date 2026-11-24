# Phase 720: native antigen container refill/eject

Rechecked SHA-matched server.dll. CAntigenDispenser spawn 222f6fe0
clamps +49c amount to [0,100], sets +498 loaded from amount>0, sets
bodygroup(1,loaded). Base setup and screen spawn remain separate.
Touch 222f6660 casts CBaseEntity to CPropContainer (RTTI descriptors
224d88d4 and 224eef2c, not a CAntigenCanister-only cast). Requires
flag bit 8 at container +62c and unloaded dispenser. If reject time
+4a8 plus 0.5 >= gpGlobals time, stores current reject time and
returns. Continuous touch refreshes the window. Otherwise transfers
container +628 float multiplied by 100.0f and 0.01f, sets bodygroup
(1,1), calls percentFull updater 222f64d0, then sets loaded=1.
Even a zero-amount accepted container sets loaded. Game rules slot
323 (+50c) receives inserted container, then UTIL_Remove-like helper
22286bf0, dispenser.load, OnCanisterInserted output +4b8 with
container activator and dispenser caller (deferred host removal).
Eject 222f7330 checks loaded, creates prop_antigen_explosive_barrel
at original transform, spawns, calls setter 222ff6e0 with amount
*100.0f*0.01f. Setter stores +628 float and percentFull=amount*0.01f
without clamp. Game rules slot322 (+508) receives container and origin.
Samples time into +4a0, clears loaded and amount, calls 222f6eb0
steam stop, sends entity message helper 22139c90 with arg1,
bodygroup(1,0), dispenser.eject, clears think, fires
OnCanisterEjected output +4d0, samples clock into reject time +4a8.
Neither user handle nor fractional accumulator is reset by eject.
Original callback assumes deferred removal preserves activator pointer.
Host contract must preserve it until output returns.

Native API reuses Phase719 validated state and stop/pose helpers.
Borrowed host effects preserve order; real CPropContainer RTTI cast,
creation/transform, output/game-rules/animation and sound remain host
services. Explicit failure guard prevents replay. No new physics,
model handling or network classes. Native test checks spawn clamp,
wrong RTTI/flags, loaded gate, rejection equality and rolling refresh,
transfer, both outputs, active steam removal, zero-amount insertion
and partial effect failure. No proprietary binary data committed.
