# Authenticated original server.dll: ServerGameEnts001 vtable inventory

## Source and scope

The owner-provided `SE1/bin.zip` contains original `server.dll`,
6,049,792 bytes, PE32 Intel i386 with image base `0x22000000`.
Its full SHA-256 is
`568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`.
All address observations below were derived by **read-only local PE parsing**
of this hash-verified original file. This document contains only public
interface names, relative metadata, and inferred virtual-method counts,
not proprietary implementation bytes, assembly, or pseudocode.

The first real owner Windows engine tests, using the independent DLLs,
called our server `CreateInterface` and accepted `ServerGameDLL003` but
rejected `ServerGameEnts001`. The engine still proceeded to client Init,
so the rejected request is **not proven fatal**. Neither run established
a server `DLLInit` invocation.

## Original factory registrations

| Name | Registration RVA | Factory callback RVA | Singleton RVA | vtable RVA |
| --- | --- | --- | --- | --- |
| `ServerGameDLL003` | `0x3BF140` | `0x1E7630` | `0x510650` | `0x442EFC` |
| `ServerGameEnts001` | `0x3BF210` | `0x1E82C0` | `0x510654` | `0x443030` |
| `ServerGameClients003` | `0x3BF230` | `0x1E8320` | `0x510658` | `0x443054` |

The actual `ServerGameEnts001` singleton's vtable has **seven** function
entries followed by a null terminator, immediately before the adjacent
`ServerGameClients003` table data:

| Zero-based slot | Method RVA | Preliminary ABI note |
| --- | --- | --- |
| 0 | `0x1E9360` | Destructor-like callback; stack cleanup 4 |
| 1 | `0x1E82D0` | One stack argument; cleanup 4 |
| 2 | `0x1E9380` | Two stack arguments; cleanup 8 |
| 3 | `0x1E82E0` | One stack argument; cleanup 4 |
| 4 | `0x1E94B0` | One stack argument; cleanup 4 |
| 5 | `0x1E82F0` | One stack argument; cleanup 4 |
| 6 | `0x1E94D0` | Three stack arguments; cleanup 12 |

The stack cleanup values are individual method-local observations, **not**
proof of exact C++ parameter types, return types, side effects, or object
ownership. Virtual inheritance, destruction semantics, and engine callback
ordering still need validation before advertising this interface from our
independent server. Never replace a rejected factory with an arbitrary
non-null pointer or a seven-slot table of untyped placeholders.

## Reproduce privately

On a machine with Python 3 installed, point the new standard-library
inspector only at the **unchanged original server.dll**:

```powershell
python pc/independent_slot0/Inspect-OwnerOriginalServerPE.py 'D:\SiN-DisposableTest\SE1\bin\server.dll'
```

The tool checks the exact original SHA-256 **before** inspecting data,
authenticates the three registration wrapper names and singleton pointer
relationships, and verifies each method RVA lies within the executable
code section. A synthetic fixture runs without original binaries:

```powershell
python pc/independent_slot0/Inspect-OwnerOriginalServerPE.py --self-test
```

The native Windows CMake build registers that latter case when Python 3
is available, and never downloads or executes the owner's game binary.

## Next steps

1. Recover method-level signatures and ownership of all seven original
   `ServerGameEnts001` slots; use Source SDK 2004 only as a comparison.
2. Recover the original `ServerGameDLL003` `DLLInit` call sequence with
   owner-binary evidence, not merely interface-name guesses.
3. Add independently implemented objects only once their required engine
   ABI and lifecycle are supported, with native x86 tests for each slot.
4. Continue symmetric client and server reconstruction toward Windows
   initialization, before Android and Quest 3.

Current independent server diagnostic still exports
`ServerGameDLL003` only and deliberately rejects initialization.
**No playable server has been demonstrated.**

## Typed ABI crosswalk and native tests (Phase 313)

The owner's `source-sdk-2004-master.zip` was inspected directly at
`src_mod/public/eiface.h`. Its `IServerGameEnts` declaration has a
virtual destructor followed by exactly six virtual methods. The order and
pointer signatures match the seven entries in the original SiN binary:

| Slot | SDK2004 signature | Original x86 callee cleanup |
| --- | --- | --- |
| 0 | `virtual ~IServerGameEnts()` | 4 bytes (MSVC deleting-destructor flag) |
| 1 | `void SetDebugEdictBase(edict_t*)` | 4 bytes |
| 2 | `void MarkEntitiesAsTouching(edict_t*, edict_t*)` | 8 bytes |
| 3 | `void FreeContainingEntity(edict_t*)` | 4 bytes |
| 4 | `edict_t* BaseEntityToEdict(CBaseEntity*)` | 4 bytes |
| 5 | `CBaseEntity* EdictToBaseEntity(edict_t*)` | 4 bytes |
| 6 | `void CheckTransmit(CCheckTransmitInfo*, const unsigned short*, int)` | 12 bytes |

These stack cleanup byte counts were independently checked in the original
SHA-verified `server.dll`, using each slot's method RVA. The first entry
is directly consistent with an MSVC x86 scalar deleting destructor.

The new independently authored `server_gameents_abi.h` declares exactly
this seven-slot contract. `test_server_gameents_abi.cpp` compiles a
synthetic implementation in **native MSVC x86**, exercises the seven
slots by direct vtable index, checks argument identity/return values,
and verifies the CPU stack pointer is balanced across every call.
The original-private metadata verifier also checks all seven original
return epilogues against the expected callee cleanup count, after
checking the exact original SHA-256.

**Deliberately not wired to exported `server.dll`:** The code currently
establishes ABI layout, not the actual entity lifecycle. In particular,
`MarkEntitiesAsTouching`, `FreeContainingEntity`, and `CheckTransmit`
must perform real game-state work rather than no-op; exposing
placeholder implementations would risk silent gameplay corruption or
a later crash. Original Win32 server startup still needs its
`DLLInit` implementation and other interfaces reconstructed.

## Three independently implemented, original-verified behaviors (Phase 316)

Read-only inspection of the owner-original server DLL at the previously
authenticated entity vtable method RVAs verifies these bounded behaviors:

| Method | Original binary behavior | Independent core |
| --- | --- | --- |
| `SetDebugEdictBase` | Save the supplied edict base pointer | Stores pointer in independent core state, supports replacement and clearing |
| `BaseEntityToEdict` | Return null if no entity; otherwise read the edict pointer at `CBaseEntity + 0x16C` | Reads that pointer from a synthetic x86 entity fixture, including null cases |
| `EdictToBaseEntity` | Return null for null edict/unknown; otherwise fetch unknown pointer from `edict + 0x0C` and invoke virtual method slot 5 (`GetBaseEntity`) | Mirrors this pointer chain with a six-slot `IServerUnknown` ABI fixture and validates the native x86 call |

The independent `server_entity_conversion_core.h` implements these three
**real isolated method behaviors** using x86 object-offset evidence.
The independent synthetic test `test_server_entity_conversion_core.cpp`
checks pointer identity, null paths, mutation, `IServerUnknown` virtual
slot order, and balanced `__thiscall` stack before returning. It
does not link against or execute the commercial DLL.

This is intentionally **not** an exported `ServerGameEnts001` object.
The real `CBaseEntity` allocation, `edict_t` storage, unknown-provider
lifecycle, collision/touch callbacks, entity deletion, and transmit-bit
behavior must be independently implemented before the interface can
safely be advertised to the engine. The hard-coded `0x16C` and
`0x0C` offsets are specific to the SHA-pinned original Win32 x86
layout. Android/Quest code must use an independently defined native
layout and must not reuse these offsets blindly.

The next meaningful server milestone is reconstructing and testing the
entity ownership/unknown-object lifecycle and then introducing a
complete, non-placeholder implementation for all seven slots.

## Bounded original FreeContainingEntity teardown sequence (Phase 317)

This phase inspected the owner's **SHA-verified actual original** `server.dll`
in a private local workspace, rather than inferring the behavior from
SDK headers alone. The `ServerGameEnts001` slot 3 wrapper at RVA
`0x1E82E0` forwards to a shared cleanup helper at RVA `0x266E60`.

The following observable behavior is recovered:

1. A null edict has no effect.
2. A null unknown interface at `edict + 0x0C` has no effect.
3. The unknown interface's virtual slot 5 retrieves the base entity;
   if that pointer is null, nothing is changed.
4. If a real base entity is found, the helper **clears the edict's
   unknown pointer**, then **clears the first 32-bit edict state word**.
5. It then passes that same base entity to **three cleanup routines
   in order**, at RVAs `0x21FFD0`, `0x220190`, and `0x286C10`.

The precise three cleanup routines' ownership and gameplay semantics are
**not yet established**. A premature direct implementation that skips them
would strand entity resources or corrupt the game's entity graph.

`server_entity_release_core.h` therefore implements the verified null
guards, disconnection and callback order using **three explicitly supplied
cleanup hooks**, refusing to mutate anything when one is missing. This is
a testable independent core, not an exported game interface. The native
MSVC x86 synthetic test checks all null and missing-hook branches,
pointer identity, ordering of state reset before any cleanup call,
preservation of unrelated edict fields, and repeated-call no-op.

The owner-private `server.dll` is not committed or loaded by tests.
The method-level contract still requires identifying and independently
implementing the three original cleanup behaviors, plus entity ownership,
before `FreeContainingEntity` can be wired to `ServerGameEnts001`.
