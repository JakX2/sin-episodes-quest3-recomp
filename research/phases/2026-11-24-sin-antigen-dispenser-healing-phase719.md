# Phase 719: original antigen dispenser use/healing controller

Local original server/client SHA256 matches existing pins. RTTI
CAntigenDispenser primary vtable 2246aa6c, secondary +494 vtable
2246aa5c. Factory 222f6fa0 allocates 0x4e8; constructor 222f6820 after
base 220f15e0. No distinct network class in authenticated class lists.
Datamap getter 222f6410 -> 225285c4, lazy builder 222f7470 populates
14 descriptors at 2252866c. Names and offsets verified:
m_bLoaded +498 byte (default 1); m_fAntigen / Antigen +49c float 100;
m_fLastUse +4a0; m_fHealthGiven +4a4; m_rejectTimeStart +4a8
(all zero floats); m_hUser +4ac, m_hSteamJet +4b0, m_hVguiScreen +4b4
(all invalid handles). Output members +4b8 OnCanisterInserted and
+4d0 OnCanisterEjected remain host constructed.

Original 222f7f00 checks loaded, nonnull activator, player vslot72,
health +9c < max +98, original spatial predicate 220b3d00 (argument
80). If remaining <=0 plays dispenser.empty. Otherwise samples clock,
stores last use and packed user handle, calls 222f7890, installs think
222f7100, samples clock again for immediate scheduling.
Host reproduces original use predicate (not claimed identical to
the later squared-distance check). Start 222f7890 emits dispenser.use_loop
even if an existing steam handle resolves; if none, creates env_steam
and configures attachment/parameters. Host implements those services.

Think 222f7100 validates user handle (12-bit entry/serial); updates
absolute transforms, re-resolves user, computes squared distance with
rounded f32 differences. Strict <6400 (80 units). Elapsed = clock-last,
cap upper bound 0.1f only; dose = elapsed * 7.0f. Write last use;
subtract dose, clamp negative remaining to zero. Add dose to float
fractional accumulator. Compare unrounded intermediate to 1, then
floor using original CRT helper 223a67a0; subtract integer part from
stored f32 accumulator. If dose remains positive, resolve user again,
call health vslot57 with integer amount and flag 0x00400000. If
depleted at the integer threshold or health callback returns zero,
stop. Note original drains dose and consumes accumulator even when
health callback refuses, and can schedule with depleted dose when
fractional accumulator has not yet reached 1. Preserve both quirks.
Continuing samples clock again, schedules now+0.1f, updates pose.
Stopping calls 222f6eb0, zeroes last-use, invalidates user, updates pose.
Stop helper checks live steam, resolves again, removes, invalidates
steam, stops use_loop and emits dispenser.stop. Stale steam is a no-op.
Pose 222f64d0 uses clamped antigen*0.01f, name percentFull, then base
animation advance slot165. Uses original f32 constants from .rdata.
Double intermediates approximate normal x87 behavior, not full
extended-precision emulation for extreme values.

New server-only native API uses real field offsets and borrowed
services. Explicit callback failure blocks until host reconciliation,
invalid storage refused, no automatic replay. Finite-input guards
and null-checked repeated user resolution are port safety differences.
Constructor API only writes known leaf fields; output and base object
construction remain host work. No additional physics, models, or
network class fabricated. Native loaded-pair test covers original
gates, exact distance equality, rate/cap, fractional and integer dose,
depletion, rejected health, start/stop effect ordering and failure.
Canister insertion/ejection and screen spawn still to be implemented.
