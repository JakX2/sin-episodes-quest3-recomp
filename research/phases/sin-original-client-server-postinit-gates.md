# Phase 86: original SiN 2006 client/server post-core initialization

## Primary original binary verification

Original owner-held Windows x86 PE32 `client.dll` and `server.dll` were read directly, each confirmed against its known full SHA-256:
- client.dll: `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`
- server.dll: `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`

Phase 84 validates actual original `CreateInterface` / vtables. Phase 85 checks both whole slot-zero `Init` method hashes and **21 client + 16 server required core factory queries**. This new phase extends **both** original functions after their required sound-emitter virtual initialization gate.

Original post-core exact byte SHA-256:
- client `0x240FE32A..0x240FE594`: 619 bytes, `0617a6584da528bb2e3e5a7c9967b305a8fba83b732300fef1db7be2e846dd2d`
- server `0x221E77F0..0x221E79A0`: 433 bytes, `629d342cd1709e59b3473904650b70a57b32cfd2203e164270e7ffc496f468de`

The audit verifies each below CALL target, x86 return-value test, and precise relative conditional jump to the existing shared `Init` failure location. The full region hash additionally anchors all intervening instructions not individually decoded.

## Original client.dll required post-core checks

Original `VClient011` slot-zero Init method at `0x240FE0E0` has **seven more observable failure conditions** in this order after the sound emitter succeeds:

| Sequence | Call site | Verified behavior |
|---:|---|---|
| 1 | `0x240FE367` | Virtual +`0x20` query for exact `MaterialSystemHardwareConfig012`; null fails |
| 2 | `0x240FE39B` | Direct helper `0x24164120`; false `AL` fails |
| 3 | `0x240FE3B9` | Direct helper `0x2417F680`; false fails |
| 4 | `0x240FE3C7` | Direct helper `0x241EDFB0`; false fails |
| 5 | `0x240FE3E6` | Virtual +`0x08` query for exact `MatSystemSurface003`; null fails |
| 6 | `0x240FE4B3` | Direct helper `0x24156860`; false fails |
| 7 | `0x240FE540` | Direct helper `0x24182C60`; false fails |

Each guarded failure transfers control to **`0x240FE59E`**, which returns zero. The helper addresses are retained as identifiers: their actual semantics and every subordinate call remain to be established.

Two nonfatal **at the point of observation** paths also exist:
- A command-line probe for original string **`-textmode`** (string push at `0x240FE332`, virtual call `0x240FE339`) sets byte `0x2433F6FC` to one if present; no immediate failure if absent.
- A first-factory `VMaterialSystemStub001` request at `0x240FE34E` stores whatever pointer is returned at global `0x2433F6A0`, without an immediate null branch.

These observations **do not** imply every omitted client subsystem is optional throughout the whole game.

## Original server.dll required post-core checks

Original `ServerGameDLL003` slot-zero Init method at `0x221E7640` has **four more observable failure conditions**:

| Sequence | Call site | Verified behavior |
|---:|---|---|
| 1 | `0x221E784C` | Direct `0x2225BC00` lookup of exact `sv_cheats`; null fails |
| 2 | `0x221E7927` | Direct helper `0x221F5700`; false `AL` fails |
| 3 | `0x221E7930` | Direct helper `0x221AD8A0`; false fails |
| 4 | `0x221E7939` | Direct helper `0x22149B60`; false fails |

The common original server failure target is **`0x221E79A1`** (returns zero). Between those checks, real game initialization work takes place and must not be silently collapsed into a future adapter.

Original server invokes virtual slot `+0x04` on an interface for two **original resource strings**:
- `resource/gameevents.res` at `0x221E783A`, virtual call `0x221E783F`
- `resource/ModEvents.res` at `0x221E791F`, virtual call `0x221E7924`

These are verified calls; the audit does **not** claim those files were mounted or parsed successfully in a running Quest application.

The server queries `VDebugOverlay003` using the original first-argument factory at `0x221E7949`, stores it to `0x225721CC`, and performs **no immediate null-failure test**. Later it calls `0x2226E790` after pushing size `0x510`; the branch at `0x221E795F` handles null. Both nonnull (`0x221E7978`) and null (`0x221E7997`) observed branches return success `AL=1`; on null, the stored pointer at `0x2254B888` is explicitly zeroed. We have **not** assigned a guessed object/class name to this allocation.

## Recovered C++ reference and verification

- `include/sin_compat/original_postinit_contract.hpp`: type-safe, host-neutral ordered arrays of **seven client** and **four server** checks, both resource file names, two immediately nonfatal factory requests, and the server allocation branch.
- `research/tests/test_original_postinit_contract.cpp`: C++17 checks all gate order, success, failure at every position, early-abort semantics, resource ordering and both allocation branches.
- `research/tools/audit_sin_postinit_gates.py`: chains Phase 85 SHA-pinned original vtable + Init audits, checks **both complete post-core code-region hashes**, all ten direct helper callsites, two client virtual queries, server `sv_cheats` pointer branch, resources, `-textmode`, and nonfatal allocation paths. Outputs metadata, no proprietary DLL contents.
- `research/tests/test_sin_postinit_gates.py`: **11 synthetic original-DLL-free mutation tests**, including every callsite and checked failure branch.
- `CMakeLists.txt`: registers the new C++17 reference under CTest.

Local: original private binary audit passed; 11/11 Python synthetic tests passed; C++17 compiled and executed under `-Wall -Wextra -Wpedantic -Werror`. GitHub Actions outcomes are independently reported after publishing.

## What remains to reach native Quest 3 binaries

This is **evidence-backed original x86 initialization control flow**, not a recovered full Ritual `client.dll`/`server.dll` implementation and not an actual Android `CreateInterface` game module. Client helper classes and server helper semantics, entity registration, Init/Shutdown lifecycle, network tables, game rules, map startup, and real ARM64 SourceVR interface layouts remain open.

Next phase: trace the **original helpers called during Init** (especially `0x221F5700/0x221AD8A0/0x22149B60` and `0x24164120/0x2417F680/0x241EDFB0`) into actual initialized gameplay and client systems, while verifying Android SourceVR's corresponding interfaces rather than inventing version aliases.
