# SHA-authenticated SiN Episodes filesystem implementation (July 2006)

## Original owner evidence

The original private `FileSystem_Steam.dll` was inspected directly from
the owner's connected Drive, under `sin files/bin` (separate from the
game client and server binaries). The original is a 135,168-byte MSVC
PE32 x86 DLL with a July 14, 2006 linker timestamp.

Exact SHA256:

`1a1675fa4b459df19f95a85ea9059eecea01f7862d2b153051385171da79d14b`

This matches the local owner-only audit. Only pointer/ABI metadata and
an independently written verifier are committed, **not** the original DLL,
binary bytes, disassembly or reconstructed implementation.

A private debug-path string references the Ritual SiN Episodes source
branch under `Se1/code/filesystem/filesystem_stdio`. This is additional
original-build provenance, not proof the source files are available.

## Authenticated actual original interface

Both `VFileSystem012` and `VBaseFileSystem007` are explicitly registered
inside the original module. Their factory thunks both return the same
original owner object (RVA `0x1F4F0`), initialized with two vptrs:

| Item | Authenticated original RVA |
|---|---:|
| Primary IBaseFileSystem virtual table | `0x1B630` |
| Secondary IAppSystem virtual table | `0x1B61C` |
| VFileSystem012 registration thunk | `0x19900` |
| VBaseFileSystem007 registration thunk | `0x19920` |
| Shared-object-returning factory thunks | `0xD6E0`, `0xD6F0` |

The actual primary table contains the following read-related original
function entries, separately checked against the owner SHA:

| ABI operation | Slot | Function RVA | First verified x86 return pop |
|---|---:|---:|---:|
| Read(buffer, size, handle) | 0 | `0x7C90` | 12 bytes |
| Open(path, mode, pathID) | 2 | `0x8D00` | 12 bytes |
| Close(handle) | 3 | `0x7920` | 4 bytes |
| Size(filename, pathID) | 6 | `0x8DF0` | 8 bytes |
| Size(handle) | 7 | `0x2D60` | 4 bytes |
| FileExists(filename, pathID) | 10 | `0x7FD0` | 8 bytes |

The distinction between the overloaded `Size` methods is no longer just
an MSVC x86 synthetic inference. It is consistent with the original
2006 functions' argument access and callee-cleanup code.

The first fourteen original primary slots and five secondary slots are
checked as an uninterrupted contiguous table. The complete virtual
interface is larger; this audit does **not** claim the remaining entries
are semantically identified.

## Reproduce with owner-owned private binary

```powershell
python .\audit_sin_original_filesystem_steam.py "C:\Path\To\FileSystem_Steam.dll"
```

The auditor refuses all other SHA256 values before parsing interface
metadata, reports only nonsecret identifiers and numeric offsets, and
does not transmit or write the supplied original DLL.

Original-free public CI runs:

```powershell
python .\audit_sin_original_filesystem_steam.py --self-test
```

That mode builds a fabricated PE in memory and rejects seven targeted
mutations across names, factories, dual-vptr initialization, both
virtual tables and Size overload return metadata.

## Evidence boundary

This is **original static ABI evidence**, not proof that our independent
client DLL has made a live IFileSystem method call inside the 2006 game.
It improves confidence in PR #450's source-guided adapter and PR #452's
engine-backed document reader. The next owner retail action should
still begin with the presence-only filesystem probe in a disposable
game installation, before enabling bounded reads.

The last publicly documented owner-verified original Windows runtime
checkpoint remains `0x53494E63` (sound-system Init true). No complete
`C_StatsSystem::Init`, normal `VClient011::Init`, map load or playable
engine startup is established by this audit.
