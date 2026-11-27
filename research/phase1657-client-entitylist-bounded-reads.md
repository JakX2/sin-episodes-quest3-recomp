# Phase 1657: bounded diagnostic behavior for all nine client entity-list virtuals

This phase is a **native Windows x86 original-free implementation fixture**,
not a new original-method discovery, replacement `client.dll` registration,
or retail Windows startup claim. It is stacked on fully green Phase 1656
PR #1665, SHA `05ae98cdd06b3fa8c1c1a72ba9ec4e805fe071b2`.
It deliberately avoids physics/surfaces/models/prop work.

## Original 2006 PE32 evidence

The owner-provided unchanged `SE1/bin.zip` was statically inspected with
`objdump -M intel`, with SHA-pinned `client.dll` matching
`6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.
The original whole-method SHA audits from Phases 229, 230 and 1656
already authenticate all nine original entries. The *original* interface
is a secondary C++ subobject at owner + `0x10014`, confirmed independently
by the MSVC RTTI hierarchy in Phase 1656.

Original secondary `this`-relative field identities:

| Original field read/write | Offset from secondary `this` | Owner offset |
|---|---:|---:|
| Networked entity count | `+0x04` | `+0x10018` |
| Maximum entities | `+0x08` | `+0x1001C` |
| Secondary-flagged count | `+0x0C` | `+0x10020` |
| Highest networked entity index | `+0x10` | `+0x10024` |
| Networkable mirror entry #0 | `+0x14` | `+0x10028` |
| Raw entity record #0 | `-0x10010` | `+0x04` |

The parent raw table has **4096 records** of **16 bytes** each. The
secondary mirror is **4096 records** of **8 bytes** each, spanning owner
`+0x10028` through the next region at owner `+0x18028`.
**Only the original networked live-count/highest-index accounting uses a
separate 2048-entity guard.** That limit must NOT be applied to the
networkable mirror's total indexable storage. A raw entry's
first two DWORDs hold the original unknown pointer and the serial.
The remaining raw-entry fields are not reconstructed by this diagnostic.

The original EHandle resolver at `0x241074D0` first rejects
`0xFFFFFFFF`, masks the low 12 bits for the raw table index, shifts
the handle right 12 for the serial and returns the raw unknown pointer
only if the stored serial matches. Underlying native address arithmetic is
equivalent to `parent + 0x04 + 16*(handle & 0xFFF)`; it is not the start
of the secondary object's mirror table.

## Nine original virtual methods

| Slot | Original function VA | Original effect | Prototype in bounded model |
|---:|---:|---|---|
| 0 | `241056F0` | Networkable mirror lookup at `+0x14+index*8` | `void* GetClientNetworkable(int)` |
| 1 | `24105BB0` | Slot 2 handle resolver → unknown vcall +`0x10` | `void* GetClientNetworkableFromHandle(uint32_t)` |
| 2 | `241074D0` | Serial/index checked handle → unknown | `void* GetClientUnknownFromHandle(uint32_t)` |
| 3 | `24105AF0` | Raw index → unknown vcall +`0x18` | `void* GetClientEntity(int)` |
| 4 | `24105BE0` | Slot 2 handle resolver → unknown vcall +`0x18` | `void* GetClientEntityFromHandle(uint32_t)` |
| 5 | `24105700` | Count + flagged count *only when byte argument is 1* | `int NumberOfEntities(bool)` |
| 6 | `24105740` | Highest index `+0x10` | `int GetHighestEntityIndex()` |
| 7 | `24105720` | Max setter `+0x08` | `void SetMaxEntities(int)` |
| 8 | `24105730` | Max getter `+0x08` | `int GetMaxEntities()` |

All slots except 6/8 use x86 `__thiscall` with a four-byte callee-cleaned
argument. Slots 6/8 use no stack argument. **Slot 0 is not a destructor.**

## Independent implementation and protections

`pc/independent_slot0/native_client_entitylist_bounded_reads.h`
declares a nine-virtual interface and a separate *diagnostic-only*
`BoundedReader`. `State` is entirely caller-owned, and converts unknown
pointers through explicit synthetic host callback functions. It does not
reinterpret original engine C++ objects or invoke retail vtables.

The original methods do **not** consistently check array bounds or
validate counters. The independent model intentionally refuses negative
or out-of-range indices, stale serials, absent callbacks, invalid count
metadata and invalid maximum-entity values. These are documented safe
deviations, **not claims of retail crash behavior equivalence**.

`test_native_client_entitylist_bounded_reads.cpp` uses actual compiled
MSVC x86 virtual dispatch and inline-assembly caller ESP checks. It covers
all nine methods, both mirror boundaries, 4096 raw records, serial mismatch,
reserved handle `0xFFFFFFFF`, stale entity reuse, callbacks on valid
unknowns only, default and optional counts, highest index, max setter,
and corruption/missing callback refusal. It also **differentially compares**
native EHandle resolve and the unknown-to-networkable/entity methods
(virtual slots 1 through 4) against the independent, previously SHA-audited
portable Phase230 byte-buffer reference. It checks valid/stale/invalid
handles and signed indexed entity lookups, using 32-bit process-local
tokens only. No original DLLs or contents are linked or run in CI.

## Owner-only startup-state readback

The Phase 1656 owner-original InterfaceReg probe now also emits
`original_client_entitylist_startup_state` after the original's own
`DllMain` and already SHA-gated vtable readback. The four read-only DWORD
fields are anchored at the following **module RVAs**, not hard-coded
process pointers:

- `0x34FDE0`: networked entity count (owner +`0x10018`)
- `0x34FDE4`: maximum entities (owner +`0x1001C`)
- `0x34FDE8`: secondary-flagged count (owner +`0x10020`)
- `0x34FDEC`: highest live index (owner +`0x10024`)

The readback never executes an entity-list virtual method. It prints only
four counters, successful read count, and a bounded status label. The
read-only adapter rejects unreadable memory, 32-bit module-base wrapping,
counts exceeding 2048/4096, and an impossible empty/nonempty highest-index
combination. This is a conservative observer: some intermediate or unusual
runtime transitions may fail the invariants without implying that original
retail code is defective. Such a failure must be investigated, not
fabricated as a successful boot.

`test_original_client_entitylist_state_snapshot.cpp` uses only fabricated
memory to exercise initial zero counters/highwater -1, a valid populated
snapshot, each unreadable field, overflow, and corrupt-count cases.
The prior existing owner factory package still requires explicit confirmation
that the original files are in a disposable copy. No actual owner-original
DLL was loaded by public CI.

## Test gate and explicit non-goals

This branch adds two native CTests. The expected native suite is
**261/261** when all prior tests remain green. It is not marked verified
until GitHub Actions on the *final commit* passes.

- `VClientEntityList003` is **not** registered with the independent
  `CreateInterface`; the existing refusal test remains active.
- Real engine entity lifetimes, networking, class factories, world
  ownership and callback objects are **not** implemented by the model.
- No Windows map boot or Android/Quest 3 gameplay is claimed.
- The next work should connect authentic engine-owned entity lifetime
  events to a verified client ABI only after a real owner Windows startup
  trace. A fake successfully returned interface pointer is not a boot fix.
