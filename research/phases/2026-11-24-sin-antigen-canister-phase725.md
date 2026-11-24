# Phase 725: native CAntigenCanister leaves and behavior

Fresh SHA-matched server/client DLLs. Factory registrations at
223c46b0 and 223c46d0 for prop_antigen_explosive_barrel and
prop_antigen_canister, respectively. Original records 2252a0c0 and
2252a0c4 share the same factory vtable 2246ec50 and creator 223007d0
-> 223007f0. Both allocate 0x6a0 and initialize CAntigenCanister
primary vtable 2246f094, secondary +494/+498. Constructor 222fffc0
(65 bytes SHA256 a9cb80d50961008bea2a607ce2615979ee12660b7a89d465b2b31d5980bce84a)
calls CPropContainer 222ffb80 with flags 9, model
models/items/antigen/antigentank.mdl, zero float +634, one +638,
null +63c/640/644, then writes zero +648. Base sets amount +628
100.0, +64c zero, handles +668 and +69c invalid. Port initializes
only verified leaves; output-member construction remains host work.
Both aliases reuse the same leaf API, no duplicate factory behavior.

Amount setter 222ff6e0 (39 bytes SHA256
08dbd711f36f06dc26f3e66b92326b4fe1aa4728041336104b12b16ce0b775a8)
stores +628 then pose percentFull=amount*.01f, NO clamp.
A describe helper exposes actual amount and flags for Phase720
dispenser observation/adapter; the API is ready for host wiring,
not automatic factory registration. Original spawn 223002f0 calls
223000b0 base setup then same pose, installs 222ff740 idle, schedules
now+.25. Idle 222ff740 advances animation (slot165), RandomFloat
(.5,1.5) via import 223d2188, schedules now+sample+1.0. Original
capabilities 222ff800 calls 2218dce0, OR 0x40000000, calls carry
predicate 222474e0 with live convar float values, OR 0x10 on success.
Use 222ff850 checks nonnull player vslot72 then raw variant tag 3
and calls activator slot322 with object and variant payload +4.
Existing Source carry/physics predicate retained, not reimplemented.
Break slot188 22300030 calls 222ffd00 base container break then
22170f60 four times, count 1, model antigen_break01..04 in order,
raw floats 200,-200,25. Host performs base container effects and
fragment creation using existing Source services. Deferred base
removal must preserve object until all fragments emitted.
Native tests exercise observed defaults, unbounded pose, spawn/idle
timing, capability predicate, use gates, fragment order and partial
failure. No new model loading, physics, asset or binary bytes.
Full generic CPropContainer damage/leak lifecycle remains separate.
