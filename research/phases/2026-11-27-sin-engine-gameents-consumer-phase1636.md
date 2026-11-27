# Phase 1636: Authenticate original engine consumer of ServerGameEnts001

Stacked on fully green Phase 1635, PR #1643. This phase examines the original 2006 Windows **engine.dll** in addition to the previously authenticated original server.dll. The owner-private original engine SHA-256 is f3add5b41300e6d709fd0b492bfcdd37b9b3d20d9744f874c0b860bf021cec81 (PE32 image base 0x20000000). This value was verified against the engine.dll supplied through the owner's Drive archive. No commercial executable bytes are committed.

## Original engine factory requests

At engine VA 0x2015e04d, the engine queries ServerGameDLL003 and stores the result into global 0x203e4484. In the same factory-acquisition routine, after a successful first request, at VA 0x2015e07e it queries ServerGameEnts001 and stores its result in global 0x203e4488. A null result triggers an explicit local failure path, returning false from **this routine**. Whether that result is ultimately fatal to retail startup remains unproven, since the calling route and fallback behavior have not been exhaustively checked. Do not equate this local result with an observed game boot failure.

Nine original, bounded instruction windows (two factory requests and seven dynamic dispatch callsites) have SHA-256 witness metadata and PE32 structural checks. All six operational original vtable slots are reached by engine code:

| Offset | Operation | Original engine callsite VA |
| --- | --- | --- |
| +0x04 | SetDebugEdictBase | 0x20150f73 |
| +0x08 | MarkEntitiesAsTouching | 0x20172510, 0x20172780 |
| +0x0c | FreeContainingEntity | 0x20110e50 |
| +0x10 | BaseEntityToEdict | 0x20092a70 |
| +0x14 | EdictToBaseEntity | 0x20092a50 |
| +0x18 | CheckTransmit | 0x20153186 |

The original deleting-destructor slot +0x00 was authenticated previously on the server side; it is not one of these seven operational engine callsite witnesses. These seven callsites do not include every possible inlined or indirect use.

## Repeatable auditor

research/tools/audit_sin_original_engine_gameents_consumer.py supports two modes:

- --engine with an owner-private original 2006 engine.dll: verify the entire SHA-pinned PE, file-backed factory strings, 2 complete factory windows and 7 complete operational callsite windows, their global loads and their vtable offsets. Report metadata only.
- --self-test: fabricate unrelated PE32 and generic x86 instruction sequences to test valid decoding, malformed headers, truncated sections, wrong global pointers, missing required null checks, wrong virtual slot offsets and corrupted instruction fingerprints without any game binary.

The self-test is wired into the existing original decompilation synthetic workflow. A local owner-private run of the full audit also succeeds against the authenticated 2006 engine.dll.

## Integration consequence

The consumer-side evidence corroborates that the engine actually uses the six operational methods assembled during Phases 1633 to 1635. A working retail factory cannot be enabled merely because the vtable shape is known. A real engine adapter still needs verified edict/networkable ownership, entity generation, contact allocation, transmit-buffer extent and owner-lease semantics. No engine factory advertisement, Windows boot, Quest runtime, generic physics/surfaces/models/props changes or newly recovered server.dll functions are claimed.
