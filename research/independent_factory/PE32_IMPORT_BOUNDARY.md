# Phase 257: independent PE32 static import boundary

This batch adds a fail-closed, read-only Python PE32/x86 import-directory
auditor to the independent synthetic Windows DLL smoke workflow. It rejects
malformed DOS/PE headers, non-x86 or non-PE32 images, missing DLL flag,
truncated tables, invalid section-backed RVAs and unterminated import names.
The Windows job now compiles the independent synthetic DLL and audits its
static imports against forbidden `client.dll` and `server.dll` names before
running the native Win32 loader. Five synthetic parser tests cover valid
imports, wrong machine, truncation, invalid RVA and name case-folding.

**Scope:** This is an executable boundary safeguard, not original-game
decompilation or proof of a retail-compatible interface. The check covers
normal static imports only. It does **not** prove absence of dynamic
`LoadLibrary`/`GetProcAddress`, delay imports, forwarded exports or runtime
forwarding. A later milestone should audit delay imports and export forwarders
and run a real engine handshake with independently implemented interfaces.

**Evidence tiers unchanged:** 403 previously SHA-reviewed original bodies
(256 client, 147 server); 64,586 candidate entrypoints; 0 original functions
independently load-tested; 0 newly authenticated original bytes in this batch.
Retail gameplay boot is not demonstrated. No original DLLs, proprietary
bodies or game assets are included.

The private SHA-pinned original-PE and hosted Ghidra workflows remain separate
from this synthetic CI. Windows x86 remains first, Android ARM64 second and
Quest 3 third.
