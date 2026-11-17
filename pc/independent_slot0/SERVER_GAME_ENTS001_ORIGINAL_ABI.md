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
