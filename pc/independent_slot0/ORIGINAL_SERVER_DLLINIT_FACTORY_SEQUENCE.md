# Original SiN ServerGameDLL003::DLLInit first ten engine dependencies

## Evidence provenance

Recovered through bounded read-only PE32 analysis of the owner's
SHA-256-verified original `server.dll`:
`568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`.
The `ServerGameDLL003` singleton vtable is RVA `0x442EFC`; its virtual
slot zero leads to original `DLLInit` RVA `0x1E7640`.

The method starts with ten consecutive requests through its
`engineFactory` argument, passing the interface name and a null optional
status pointer. Each is tested for a non-null result before proceeding.
The following *exact order* has been checked in the original code:

| # | engineFactory interface requested | Source of evidence |
| --- | --- | --- |
| 1 | `VEngineServer021` | Original retail DLLInit |
| 2 | `VoiceServer001` | Original retail DLLInit |
| 3 | `VEngineCvar002` | Original retail DLLInit |
| 4 | `VEngineServerStringTable001` | Original retail DLLInit |
| 5 | `StaticPropMgrServer002` | Original retail DLLInit |
| 6 | `VEngineRandom001` | Original retail DLLInit |
| 7 | `IEngineSoundServer003` | Original retail DLLInit |
| 8 | `SpatialPartition001` | Original retail DLLInit |
| 9 | `VModelInfoServer001` | Original retail DLLInit |
| 10 | `EngineTraceServer003` | Original retail DLLInit |

The next original request is `VFileSystem012`, but it uses the separate
`fileSystemFactory`. The original method performs additional initialization
after these first ten queries, so this table is **not a complete DLLInit**.

## Independent reconstruction status

Phase 314 extends the non-forwarding MSVC x86 independent server DLL's
**diagnostic-only** `DLLInit` to request these ten versions in order.
It stops immediately at the first missing interface, never dereferences
or retains any returned interface pointer, and always returns `false`
even if all ten are available. The synthetic Win32 native test exercises
all ten query names, confirms the early-stop boundary at each unavailable
version, and validates stack cleanup of the four-argument `DLLInit` ABI.

The owner-local SHA-pinned `Inspect-OwnerOriginalServerPE.py` now also
validates the original `DLLInit` vtable entry and first ten request
sequence, without exposing original binary contents. It refuses a
different hash or a changed call order.

**Retail evidence limit:** In both observed owner Windows game tests,
the engine accepted independent `ServerGameDLL003` but there was no
recorded server `DLLInit` callback. The client diagnostic intentionally
returned zero, so the server's ten factory requests have not been
exercised in the retail engine yet. The test changes are compatible with
later development but do not make the game boot or implement any server
entity behavior.

## Remaining work

Reconstruct the later filesystem and other factory dependencies,
the required server initialization side effects and state, plus actual
`ServerGameEnts001` and `ServerGameClients003` method behavior.
Do not simply change `DLLInit` to return `true` before these are
supported, and do not inject dummy engine interface pointers.
