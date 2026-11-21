# Phase 533: original Druglab Grunt post-factory classname and edict/networkable handoff

## Why this matters for getting a real `server.dll` running

The user has confirmed that SiN Episodes 2006 maps, physics,
existing props and models function when loaded in Half-Life 2.
This phase therefore does **not** reimplement physics or 3D asset readers.

The key missing piece is a **genuine Source-owned SiN game entity**,
not a fake classname alias. Phase531 reconstructed the three-variant
Druglab Grunt factory constructor and Phase532 their separate
per-class Spawn weapon selection. This investigation follows the
actual constructor's original virtual +0x70 and proves how it
attaches a class symbol and an engine edict/networkable handle
before entering the NPC's own post-constructor subsystem.

## Evidence: exact original DLL hashes and 4 complete methods

Original unmodified server.dll SHA256
`568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`,
client.dll SHA256
`6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.

The code was disassembled directly from the original PE32 and
four **entire native method bodies** were SHA-256-authenticated:

| Original VA | Original bytes | SHA256 | Authenticated role |
|---|---:|---|---|
| 0x22031B10 | 27 | 8b4c539f3d96b95df7c70b4918c8af0d442635b4ac078a8c6e3b51b123c1a6ad | Grunt virtual +0x70 constructor initialization |
| 0x2213B140 | 197 | 46d43167fb58428dafe3d886f42c4f38004c214adfb5268bd87cd50672febe7c | base entity class symbol/edict binding |
| 0x2227F070 | 53 | 8959de0a65a8c2ce49ffbb7b6f79bc8bcbc8e2cf227857ef697fda5e9fa41bb2 | engine-owned edict-to-networkable association |
| 0x22359210 | 57 | 848486f30a6c0d1bf820eecbb544c451f91a362070ca3693f500165b74fbfbc6 | NPC post-constructor dispatch |

The original Druglab primary vtable `0x2249AAA4`
contains `0x22031B10` at virtual slot28, byte **+0x70**.
That method calls `0x2213B140`, then the same vtable's
byte **+0x3F8** function `0x22359210`, with no other
intermediate call. This confirms that **actual native edict
and class-name initialization precede the Grunt postinit**.

This is a strong reason NOT to simply call the Phase532
weapon Spawn code as though it were a constructor.

## Authenticated source control flow

The exact original method `0x2213B140`:

1. If the constructor's nullable classname argument is non-null,
   interns the string through `0x221F3DE0` and writes the
   original class symbol at entity **+0x0C**.
2. Reads entity DWORD **+0xB8**. If bit9 (`0x200`) is set,
   calls native helper `0x22164460` with the global
   `0x2255A8D0`, then `0x2214BF30(0)` and
   `0x2214B280`, and returns. It does not run the
   normal networkable/edict attachment on this branch.
3. Otherwise, if bit10 (`0x400`) is NOT set, invokes
   `0x2227F070` on owner networkable subobject **+0x160**,
   passing the current global edict context pointer
   from **`0x22558764`**, then clears that global.
4. Inside `0x2227F070`: when supplied edict is null,
   calls the *real engine's* virtual `+0x54` with -1,
   which requests a new edict. Associates the returned
   engine edict and source networkable, including original
   networkable+0xC and engine edict+0xC members plus
   a flags value derived from the networkable's virtual +0x24.
   The native code assumes valid engine-owned memory.
5. The base initializer then tests original owner
   **+0x16C** edict pointer. If non-null it calls an
   engine virtual `+0x48`, calls helper `0x22164440`
   and finally writes original **edict+0x08**
   backpointer to owner networkable subobject **+0x160**.
6. Calls `0x2214BF30(0)` and `0x2214B280`,
   then returns to `0x22031B10`.
7. The wrapper immediately invokes NPC virtual
   byte **+0x3F8**, native `0x22359210`.
   That method checks a deeper original AI initialization
   path `0x22031D50`, dispatches virtual +0x76C
   (then +0x7AC on success), and invokes an initializer
   on the embedded member at +0xCC0 via `0x22358FA0`.
   Full behavior and class-specific AI method semantics
   are not reconstructed here.

The two creation flag branches are important:
`0x200` takes precedence over `0x400` when both
are present. The +0x400 path skips ordinary edict attachment,
but may still operate on an edict already present at +0x16C.
These are **native object state bits**, not proven BSP KeyValue names.

## Independent host callback reference

`include/sin_compat/original_druglab_grunt_entity_init.hpp`
`src/original_druglab_grunt_entity_init.cpp`

The new `EntityInitializer` follows the exact known
source operation sequence using a typed `IHost` that
must interact with a genuine engine-owned class object.
The explicit callbacks cover:

- class symbol interning;
- special native +0xB8 bit9 registration branch;
- entity+0x160 networkable/edict association;
- clearing temporary original factory edict context;
- conditional engine +0x48/backpointer at edict+0x08;
- two base initializer helpers;
- final original NPC virtual +0x3F8.

The host must supply correct MSVC 32-bit layouts and
running engine pointers. The adapter never fabricates
edict allocations, networkable vtables, engine handles
or compiler-specific CBaseEntity object size.

**Port-only safety difference:** if networkable attachment
fails or throws, this independent adapter still attempts
to clear the temporary edict context to prevent a stale
global from leaking into a later entity. Original 2006
code clears only after the native helper returns normally.
Other failed host callbacks return an explicit stage name
and cannot claim that a real engine entity now exists.

The synthetic-host regression validates reused factory
edict context, auto-requested edict, bit9 special branch
including bit9/bit10 precedence, bit10 skip with an existing
edict, null classname argument, missing native networkable
binding, helper failures, clear-on-error and reentrancy.

The original-only auditor
`research/tools/audit_sin_original_grunt_entity_edict.py`
requires exact full DLL SHA hashes, verifies four
complete original function hashes, eleven original direct
rel32 call edges and source field/flag/vtable witnesses.
Public CTest uses **fabricated** synthetic relative
CALL pointers and source hash refusal only, never original
proprietary DLLs.

## Remaining native blocker

The original Windows engine has **not yet called these
adapters as true Source CBaseNPC methods**. Independent
`ServerGameDLL003::DLLInit` still fails closed before
successful game initialization, and neither
`ServerGameEnts001` nor the complete original
networked/server-class entity registrations are available
to instantiate a Grunt inside the real game.

Actual next work: verify native `DLLInit` lifecycle with
an owner-side reversible Windows test, recover real
Source edict/networkable object ownership, implement
original send tables, then integrate the already verified
SiN class factory, entity initialization and weapon Spawn
stages into the genuine game server. Android/Quest 3 follows
successful Windows x86 map/entity execution.

No physics, surface-property or model/prop loaders added.
