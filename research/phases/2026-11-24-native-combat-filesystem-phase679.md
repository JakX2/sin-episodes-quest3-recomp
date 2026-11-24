# Phase 679: native engine filesystem combat configuration

Authenticated unchanged original server.dll SHA256
568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650.
Direct disassembly of constructor 0x222d7e30 shows it passing the
filesystem global 0x225721a4, filename pointer global 0x22523608 and
GAME path literal 0x223ea728 to 0x22264200. On false it destroys and
clears its KeyValues owner. The original loader at 0x22264200:
- Open at 0x2226421f, primary virtual +8, path/mode rb/pathID.
- Size(handle) at 0x2226423d, +0x1c.
- Read at 0x22264258, +0, then a terminator at Size(handle).
- Close at 0x22264263, +0xc, before parsing 0x22266860.
The rb and GAME literals were verified directly. The original resource
scripts/rules_combat.txt from owner scripts.zip is 2920 bytes, SHA256
0d4c47cfa543435deb7098b4218e62707a02971351957260959ffd6016667f1e.

The native Win32 module now exports SiN_LoadAttackSpreadFromEngine:
a single read-only resource path, exact original binary mode/path ID,
preflight four vslots, bounded positive length, complete binary reads,
one close attempt on all successful Open paths, then the Phase678 parser.
Unlike the original code, short or oversized reads fail explicitly.
Close failure/exception is distinguished from parsing or read failure.
The old independent configuration survives failures; this is a port
transactional reload decision, not the original constructor failure
semantics. Callback reentry cannot mutate campaign or rule storage.
No live filesystem pointer or file handle retained.

Win32 loaded DLL tests use synthetic actual MSVC x86 virtual objects:
partial reads, exact path, missing/throwing open, oversized/zero size,
read zero/overcount/exception, close exception, parse rejection, and
reentry. Tests assert last known good lookup survives failures.
No new filesystem interface or generic Source physics/resource work.
The actual retail CSE1Rules constructor and DLLInit still need binding.
