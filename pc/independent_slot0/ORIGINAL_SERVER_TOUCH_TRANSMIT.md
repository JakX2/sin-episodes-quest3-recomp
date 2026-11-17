# Original-guided entity touch creation and transmit policy (Phase 323)

Inspected the original owner-private server.dll, SHA256
568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650,
and the owner's SDK 2004 gameinterface.cpp, physics_main_shared.cpp,
shareddefs.h and edict.h. The following are independently implemented
behaviors, not a complete exported game entity interface.

## MarkEntitiesAsTouching

The original slot 2 at RVA 0x1E9380 resolves each edict through its
unknown pointer at +0xC, virtual slot 5, and exits if either base
entity is absent. It initializes a trace (helper 0x285A10) and
recomputes dirty absolute transforms for the second then first
entity (flags +0xB8, bit 11; helper 0x13FA60). It reads three origin
floats at +0x2BC through +0x2C4 and computes their midpoint using
the original 0.5 constant. The helper at 0x221080 publishes the trace
and runs two directed registrations through 0x21F350.

The directed routine checks self, move-parent identity, no-touch
flags, two nonsolid triggers, and pending deletion. It updates an
existing link's stamp and calls its touch handler, or allocates and
links a new contact and conditionally calls start-touch. Raw retail
entity, trace, and list layouts are intentionally not overlaid.
Independent safeguards require owned sentinels, validate identities
and reserve both new nodes before mutation. The test now creates
contacts, refreshes them, removes both directions through the previous
bulk/reciprocal cores, and recreates the links.

## CheckTransmit

Original slot 6 at RVA 0x1E94D0 obtains edict zero through the engine,
resolves the recipient (with an edict-zero fallback when its pointer
is absent), and snapshots player skybox area (+0x828) and HLTV state
(+0x92C). The original edict stride is 20 bytes. The state flags
mask is 0x38: ALWAYS=0x08, DONTSEND=0x10, PVSCHECK=0x20,
FULLCHECK=0. Transmit context bitset pointers are +0x2008 and +0x200C.

Original precedence is DONTSEND, already selected, direct ALWAYS
(sets bits without entity SetTransmit), then full ShouldTransmit
and PVS policy. A full-check result of ALWAYS calls SetTransmit(true).
PVS mode forces skybox-area matches; HLTV passes other areas without
PVS culling; ordinary clients require PVS membership otherwise.
The original virtual byte offsets 0x54 and 0x5C match ShouldTransmit
and SetTransmit, and area/PVS resolve through network property.

The independent policy takes a pre-resolved recipient, a bounded
entry span and explicit bitsets. It validates indices before writing,
skips missing candidate entities instead of unsafe dispatch, snapshots
the recipient, and delegates actual SetTransmit propagation and PVS
to its owner. Tests verify the direct bit-set path, callback precedence,
HLTV/skybox behavior, selection across a 32-bit boundary, duplicates,
parent/dependent selection via adapter side effects and invalid inputs.

## Remaining work

These cores are not yet bound to the original 7-slot exported
ServerGameEnts001 ABI. Recipient/player resolution, the actual
networkable/PVS implementation, full trace semantics and engine
entity allocation/callbacks are still missing. All tests use independent
owned state. No retail executable code or original binary is committed.
