# Phase 1665: authentic CPrediction packet entity lifecycle slots 4 and 5

## Original provenance and scope

This phase is a read-only exact-source audit of the original 2006 PE32
SiN Episodes client.dll and an independent host-owned MSVC Win32 x86
behavior/ABI regression. The original owner-file dual full SHA gate is
inherited from Phase1658:

- original `client.dll` complete SHA256
  `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`
- original `engine.dll` complete SHA256
  `f3add5b41300e6d709fd0b492bfcdd37b9b3d20d9744f874c0b860bf021cec81`

Source profiles give the authentic method names. This phase follows
Phase1664 PR #1673, but does not touch unrequested physics, surfaces,
materials, models, props, or rendering.

Three original native methods have complete authenticated SHA windows:

| Original method | Original VA | Native bytes | Whole original SHA256 |
|---|---|---:|---|
| `CPrediction::PreEntityPacketReceived`, vtable slot4 | `0x24195F90` | 174 | `3ad38230016d32a96a657b96440112b51bc8535e420b7f0a8db7c17c73f7d0e0` |
| `CPrediction::PostEntityPacketReceived`, vtable slot5 | `0x24196040` | 153 | `12c676eee586917d9880b32cc134370e75e696052512b478bc4da9f217fd7090` |
| `C_BaseEntity` eligibility byte getter | `0x240C5500` | 7 | `eb493d88758526793c40c2cb3442a38fd0974612d4c95680b330121dce5a3228` |

The total authenticated original method extent is **334 bytes**.
Original prediction vtable cells at `0x242A364C` and
`0x242A3650` match those methods and carry genuine PE32 HIGHLOW
relocations. All three are native `.text` methods with source-verified
INT3 padding boundaries.

The complete original names live at profiling strings
`0x242A3738` (PreEntityPacketReceived) and
`0x242A3760` (PostEntityPacketReceived). These are not names inferred
from newer Source SDKs. Both methods enter/exit original tier0
profiling, using original `CVProfile::EnterScope` and
`CVProfile::ExitScope` imports.

## Source-authenticated PreEntityPacketReceived, vtable slot 4

The original method accepts exactly **two 32-bit stack args** and
cleans them with `RET 8`.

1. Enter the original scoped profiler.
2. Store **the second argument** at prediction `this+0x0C`.
   The same native slot17 getter, previously reconstructed and
   ABI-tested in Phase1658, returns this DWORD.
3. Read the original `cl_predict` numeric value from its
   ConVar parent pointer cell at VA `0x24374540` plus
   `m_nValue` offset `0x2C`.
   - If prediction is **disabled**, invoke original opaque
     `0x24195720`, then exit profiling and return `RET8`.
     The full semantics of that fallback method are NOT
     reconstructed here.
   - If enabled but no current local player at global
     `0x24338CF0`, exit without iterating entities.
4. Otherwise query original client entity-list global
   `0x242F7EC8`, invoking its **virtual offset +0x04**
   for the entity count. The native code snapshots that
   count before the index loop.
5. For each index `[0,count)`, invoke entity-list
   **virtual offset +0x00**, skip null entries, then check
   the source-authenticated eligibility method
   `0x240C5500`. Its original **7-byte** body is
   `mov al,[ecx+0x2E8]; ret`: a single BYTE flag at
   original `C_BaseEntity+0x2E8`, not the name or
   meaning of a new engine object.
6. Only for eligible entities invoke the original
   per-entity callback `0x240C5510`, with the
   **first original Pre argument** as its single stack
   DWORD. Exit original profiling when done.

This second-versus-first argument distinction is directly
observed from original method stack accesses, not inferred
from the semantic names.

## Source-authenticated PostEntityPacketReceived, vtable slot 5

The original post method has **no stack args**, and
completes via an imported `ExitScope` **tail JMP**,
with no additional caller stack cleanup.

1. Enter the original PostEntityPacketReceived profile.
2. If `cl_predict` is disabled, leave immediately; unlike Pre,
   **Post does NOT invoke the Pre-only fallback**.
3. If there is no current local player, also exit.
4. Otherwise query the same entity-list global
   `0x242F7EC8`, get count once at vptr+`0x04`,
   fetch each entity through vptr+`0x00`, skip null
   or the flag at entity+`0x2E8` being zero.
5. For each eligible entity, invoke original direct callback
   `0x240C68C0` with **no explicit stack arguments**.
6. Exit profiling, using the original imported
   `CVProfile::ExitScope` tail jump.

The original callbacks `0x240C5510`,
`0x240C68C0`, the disabled fallback `0x24195720`,
and the actual client entity-list object remain
separate **opaque dependencies**. This phase does not
fabricate their effects or register an incomplete
`VClientPrediction001` factory.

## Original-free implementation and tests

The existing 23-slot diagnostic `Prediction23Fixture` now gives
slots4 and 5 their source-authenticated x86 native signatures:

- `PreEntityPacketReceived(uint32_t arg1,uint32_t arg2)`
  via actual vtable slot4, `RET8`.
- `PostEntityPacketReceived()` via actual vtable slot5,
  `RET0` with no caller arguments.

`PacketEntityOps` is an entirely host-owned callback contract
for invented `CClientEntityList` Count/Get, invented entities,
the original eligibility-byte read, pre/post opaque callbacks,
the disabled-pre fallback, and profile-entry/exit hooks.
No original engine callback or retail game DLL is invoked.

It maintains source-authenticated ordering, stores Pre arg2
at the original `this+0x0C`, forwards arg1 only to eligible
entities, and checks current-player and cl_predict gates
before enumeration. The synthetic version rejects missing
callback backends or suspicious negative/huge entity counts
(guard ceiling `4096`) rather than looping unsafely.
These guardrails are explicit safety divergences, not
claims about the original retail implementation.

`test_original_client_prediction_packet_entity_pair.cpp` uses
a real compiler-generated MSVC x86 prediction vtable and
inline assembly for ECX=this, precise stack argument order
and before/after ESP. It tests:

- Pre slot4 and Post slot5 ABI, including source-matched
  `this+0x0C` writes;
- one Count call, exact index Get order and null skip;
- a synthetic `C_BaseEntity+0x2E8` byte, nonzero
  values including `0xFF` treated as predictable;
- three eligible entities, correct per-entity callback
  ordering and forward-argument value;
- Pre-specific disabled fallback, Post disabled no-op,
  and enabled/no-local-player early exits;
- empty count and fail-closed negative/oversized count,
  missing backend, and detached packet-context refusal;
- no runtime original native DLL invocation.

The Python original PE32 auditor
`research/tools/audit_sin_original_prediction_packet_entity_pair.py`
authenticates dual original-file SHA, all three full-method
hashes, original prediction vtable pointers, exact profile
name literals, global relocation sites, source direct-call
rel32 targets and sparse x86 loop/gate/RET8/tail-JMP
witnesses. Public `--self-test` constructs invented
**non-executing sparse x86 byte islands**, performs covered
byte-corruption and length mutations, and rejects forged
retail-image SHA.

One Python CTest and one native MSVC x86 CTest raise the
stacked suite from Phase1664's **276 to 278 CTests**.
The original decomp synthetic GitHub Actions workflow also
runs the new packet auditor self-test.

## Explicit non-goals

This is concrete exact-source packet lifecycle *dispatch*,
not actual entity serialization, net packet simulation,
reconciliation, tick prediction, original Windows startup,
Android native executable or Quest 3 VR gameplay.
Full original CClientEntityList/ C_BaseEntity dependencies
remain unresolved where intentionally opaque.

No original commercial DLL files or whole copyrighted
machine-code bodies are committed to GitHub. All CI tests
are original-free. The PR remains open and stacked.
