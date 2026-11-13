# Phase 204: Original 2006 SiN DLL PE32 import dependencies

13 November 2026. **Stacked draft on Phase203 PR #257.** This is a read-only, SHA-pinned audit of the owner's original `server.dll` and `client.dll`, targeting the Windows PC loader and original game-module ABI. No proprietary binary content, DLL function bodies, map assets or original disassembly is committed.

## Original provenance and exact PE dependencies

Owner-local `/mnt/data/bin.zip` (not included in GitHub) contains the original game DLLs, both independently verified using complete-file SHA256:

| 2006 original | Bytes | Original image base | SHA256 |
| --- | ---: | --- | --- |
| `server.dll` | 6,049,792 | `0x22000000` | `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650` |
| `client.dll` | 3,592,192 | `0x24000000` | `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d` |

**Original PE32 import directory, interpreted from file-backed sections, not a guessed SDK dependency:**

| Direct imported library | server.dll entries | client.dll entries |
| --- | ---: | ---: |
| `KERNEL32.dll` | 59 | 74 |
| `USER32.dll` | 3 | 6 |
| `vstdlib.dll` | 27 | 30 |
| `tier0.dll` | 26 | 16 |
| `WINMM.dll` | 0 | 3 |
| `GDI32.dll` | 0 | 22 |
| `ADVAPI32.dll` | 0 | 3 |
| **Total direct imports** | **115** | **154** |
| **Distinct direct DLLs** | **4** | **7** |

Both imports include the original 2006 `tier0.dll` and `vstdlib.dll` dependency. **Neither has a direct import descriptor for `engine.dll`**, which **does not prove independence from the engine**. The engine's interface factories, startup callbacks, allocator pointers and Source object vtables remain separate. Phase177 established original game `ServerGameDLL003` and `VClient011`, and Phase203 provides the first owner-testable Windows x86 proxy/factory handshake.

This snapshot is evidence about *direct PE import tables only*. It does not prove the complete set of later `LoadLibrary` calls, engine-supplied interfaces, runtime optional dependencies, or legal replacement DLL requirements. In particular, the Windows/API dependencies described here do not carry over to Android ARM64; those must be translated through a native host.

## Independently authored auditor

`research/tools/audit_sin_original_pe_imports.py` requires **both complete original DLL SHA256s** for a definitive report; no synthetic fixture can claim original recovery. It parses PE32 descriptors, lookup thunks, import-by-name and import-by-ordinal using the existing original PE section mapper. It validates the original machine type, image bases, **ordered original DLL names and each per-library import count**, and rejects malformed, truncated, duplicate or unbounded import directories. Output is summary JSON without reconstructing or emitting original code.

Owner-only offline invocation:

```powershell
python research/tools/audit_sin_original_pe_imports.py --archive "D:\OwnerFiles\bin.zip" --output "D:\OwnerFiles\sin-pe-import-audit.json"

# Equivalently, when the original DLL files have already been extracted:
python research/tools/audit_sin_original_pe_imports.py --server "D:\OwnerFiles\server.dll" --client "D:\OwnerFiles\client.dll"
```

`research/tests/test_sin_original_pe_imports.py` creates an independently authored, *fake* PE32 image in memory and covers **12 synthetic test cases**: named imports, ordinal imports, fallback from missing OriginalFirstThunk to FirstThunk, missing import terminator, absent directory, invalid RVA, reserved ordinal bits, unterminated DLL name, fake original SHA rejection, wrong PE32 class, empty thunk and duplicate DLL names. No original commercial bytes are included.

### Verification performed

- Full-file SHA gate and import metadata parsing executed locally with the original owner archive, both original hash checks passed, 4/7 library counts and 115/154 import counts exactly matched.
- Twelve original-free Python unit tests passed locally.
- CI discovery: the repository's `.github/workflows/ci.yml` runs `python3 -m unittest discover -s research/tests -p 'test_*.py' -v`. Hosted CI results must be checked on the exact Phase204 head before claiming pass.

## Windows-first integration implications

1. On the owner-only test machine, first check that the **original game's** 32-bit `tier0.dll`, `vstdlib.dll`, and required system DLLs resolve consistently with the owner's 2006 executable. Avoid substituting incompatible versions from other Source builds.
2. Use the Phase203 Win32 module preflight to identify module load, missing export or original factory-name errors, then collect debugger/engine logs from the actual Windows game process.
3. Distinguish a missing import/library or bitness problem from a `CreateInterface` version mismatch, loader filename assumption, allocator-object mismatch, or later game init failure. `engine.dll` absent from PE imports is **not** evidence of engine compatibility.
4. After the first verified Windows startup boundary, incrementally replace the owner-original forwarding modules with independently reconstructed server/client gameplay logic. Android then Quest 3 remain subsequent targets.

**No new distinct original x86 function bodies were recovered in Phase204**. The unique reviewed-function atlas stays **203 (111 server, 92 client)**. Most urgently missing: original Windows executable-hosted startup trace proving the engine loads and initializes replacement interfaces.
