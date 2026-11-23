# Phase 658: Original 2006 SiN game-module initialization dependency acquisition

## Goal and provenance

Phases 655–657 reconstructed the complete nested name registry source-model
pipeline and authenticated six original game-module `CreateInterface`
registrations. Phase 658 moves **inside the original `ServerGameDLL003` and
`VClient011` initialization methods**, before attempting an actual
Windows original-engine replacement `server.dll` / `client.dll`.

This work uses the owner's unchanged 2006 *SiN Episodes: Emergence* retail
PE32 binaries from Drive. Original game executable bytes are **never
committed**, shared with GitHub Actions, or reused as public synthetic tests.

- `server.dll`: SHA256
  `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`
- `client.dll`: SHA256
  `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`

All work focuses on missing SiN-specific game-DLL initialization/engine
interfaces. Already-working HL2-equivalent physics, surface properties,
model/prop loading and rendering are deliberately out of scope.

## Entire original x86 methods authenticated (2,095 bytes)

| Original DLL | Original VA | Size | Whole original function SHA256 |
|---|---|---:|---|
| `server.dll` | `0x221E7640` | 875 bytes | `abb6eb2fc523fc0cb6301630e7ba9f0b61e70e48f15d5809145171250898b680` |
| `client.dll` | `0x240FE0E0` | 1,220 bytes | `14d81449866220aaef1f5f906b577179ea107df78a424a364c715261adbb35db` |

These methods occupy executable `.text` in their respective original
PE32 images. They end respectively in `RET 16` and `RET 12`, each
followed by an original `INT3` padding byte. Their original singleton
vtable entries were verified in Phase 657.

**Proof boundary:** Whole original methods are now statically
authenticated; this phase only implements their **first, clearly bounded
interface-acquisition prefix**. It does *not* decompile, implement or claim
runtime equivalence for the rest of either initialization body.

## Exact retail ABI and dependency order

The native server method takes **four stack DWORD arguments** following
`this` (`__thiscall`, `RET 16`). The first is an engine factory
function pointer; the **third** is a separate filesystem factory, and
the fourth is a global-variables pointer.

The original client method takes **three stack DWORD arguments** following
`this` (`__thiscall`, `RET 12`). The **first** is its engine factory,
and the **third** is a global-variables pointer. Unlike the server, the
client requests `VFileSystem012` using the main engine factory.

The exact ordered list of all **37 required factory queries** is tracked
in `research/data/original_module_init_phase658.json`, with the native
instruction address, exact original name literal VA, per-DLL storage
global VA, and factory provenance.

### Server: 16 mandatory factory queries

In order, with missing-interface branches to the shared original failure
exit `0x221E79A1`:

`VEngineServer021`,
`VoiceServer001`,
`VEngineCvar002`,
`VEngineServerStringTable001`,
`StaticPropMgrServer002`,
`VEngineRandom001`,
`IEngineSoundServer003`,
`SpatialPartition001`,
`VModelInfoServer001`,
`EngineTraceServer003`,
`VFileSystem012` (**third filesystem factory only**),
`GAMEEVENTSMANAGER002`,
`VEngineCache001`,
`VSoundEmitter001`,
`ServerUploadGameStats001`,
and `ISERVERPLUGINHELPERS001`.

Every acquired EAX is stored into its original DWORD global **before**
the immediately following `JE` abort check. A failed query leaves
earlier globals in place; there is no native rollback here.

After all 16 queries, the engine factory is passed to the
`VSoundEmitter001` object's virtual slot zero (`0x221E77E6`).
Failure of that callback's AL result also branches to the same failure
exit. Only after this gate does server store the fourth stack argument
into global `0x22572190` before further original startup calls.

A later `VDebugOverlay003` factory query is present at `0x221E7942`,
stored at `0x225721CC`, **without an immediate null-abort check**.
It runs much later, after other initialization and gameplay gate calls:
it is an *optional late query*, not part of the initial required sixteen.

### Client: 21 mandatory factory queries

The first is **`VEngineClient012`**, distinct from the client DLL's
exported `VClient011` interface from Phase 657.

The original client stores its third stack argument to
`DWORD[0x242FA458]` before making that first factory call.

The client then requires:

`VEngineClient012`,
`VEngineModel012`,
`VEngineEffects001`,
`VEngineCvar002`,
`EngineTraceClient003`,
`VEngineRenderView011`,
`VDebugOverlay003`,
`VMaterialSystem076`,
`VEngineCache001`,
`VModelInfoClient003`,
`VEngineVGui001`,
`VEngineClientStringTable001`,
`SpatialPartition001`,
`VEngineShadowMgr002`,
`StaticPropMgrClient004`,
`IEngineSoundClient003`,
`VFileSystem012`,
`VEngineRandom001`,
`VENGINE_GAMEUIFUNCS_VERSION004`,
`GAMEEVENTSMANAGER002`,
and `VSoundEmitter001`.

All 21 required EAX stores precede conditional jumps to common
original failure exit `0x240FE59E`.

Afterward, the client calls virtual slot zero of `VSoundEmitter001`
at `0x240FE320` with the engine factory and tests AL; failure aborts.

The next factory query for **`VMaterialSystemStub001`** at
`0x240FE347` is **optional**: its EAX is written to `0x2433F6A0`,
but no immediate null-abort check follows. The client then requests
the **required `MaterialSystemHardwareConfig012`** through a *virtual
method at byte offset +0x20 on the previously acquired
`VMaterialSystem076` interface*, not through the general
`CreateInterface` factory. The result is stored at `0x2433F6A4`
and checked for null before continuing.

## New source model, audit and tests

### Read-only retail audit

`research/tools/audit_original_module_init_phase658.py`

This authenticates the two exact whole original DLL SHA256s and
both whole original initialization method hashes, verifies method
boundaries, and checks all **37 required original per-query instruction
sites** including:

- Exact `PUSH 0, PUSH name, CALL` factory provenance (`ESI` or server
  filesystem `EDI`), stack adjustment, `TEST EAX`, exact global
  destination and original shared failure `JE` target.
- The client's special first query, which saves the global-vars
  argument before the initial engine query and cleans a distinct
  temporary stack-argument setup.
- Name strings read from their original PE32 literal addresses,
  and whether pointer globals reside in `.data`.
- Separate sound-emitter slot-zero gates, optional debug-overlay/stub
  queries, and the required material-system hardware-config gate.

The auditor has a **public invented-byte mutation self-test** with no
original game bytes. Run:

```sh
python research/tools/audit_original_module_init_phase658.py --self-test
```

The read-only owner audit is:

```sh
python research/tools/audit_original_module_init_phase658.py \
  --server ORIGINAL_2006_SERVER_DLL --client ORIGINAL_2006_CLIENT_DLL
```

### Bounded C++17 acquisition model

Files: `include/sin_compat/original_module_init_phase658.hpp` and
`src/original_module_init_phase658.cpp`.

The typed guest host supplies the original external engine factory
calls, original global DWORD writes, sound-emitter slot-zero callback,
and client material-system slot-eight callback. The model preserves
mandatory query order, factory choice, store-before-null-test
behavior, global-vars placement, partial writes and error provenance.
The client's optional material stub may be null without blocking the
acquisition prefix. On reaching the end, it returns
**`Code::PrefixAcquired`**, deliberately *never claiming that the full
original `Init` method succeeded*.

Defensive handling of null host factory arguments, reentrancy,
callback errors and exceptions is modeled host behavior and not
attributed to the original 2006 x86 binary.

### Regression coverage

The dedicated C++17 tests exercise the complete original-required
factory order, all **37 individual missing-required-interface cases**
including preserved partial global writes and first failing site,
separate server filesystem factory, client engine filesystem factory,
both sound-emitter gates, optional null stub, mandatory hardware
configuration, the original globals write timing, callback errors,
exceptions and reentrant interception.

Portable Linux/GCC and Win32/MSVC x86 CMake test targets are added,
alongside an independent **Clang AddressSanitizer/UndefinedBehaviorSanitizer**
workflow with public original-free opcode mutation testing.

## Next implementation boundary: the original startup tail

Neither game module is ready to replace a retail DLL yet.

The server's later block after the sound gate includes original
subsystem bootstrap operations, event-resource registration
(`resource/gameevents.res`, `resource/ModEvents.res`), and
additional boolean-returning helper gates. The client tail
after its material-system hardware gate performs many more
subsystem registrations, engine virtual calls and fail-fast
startup checks. These functions need separate ABI signatures and
side-effect reconstruction before a non-forwarding `Init` can be
attempted.

Original-engine Windows execution, a full working game DLL,
in-game NPC/weapon lifecycle, and playable Android or Quest 3
gameplay are **not** demonstrated by this phase.
