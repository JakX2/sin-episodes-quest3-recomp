# Independently compiled PE32 Source-era interface diagnostic (Phase 289)

The `client.dll` and `server.dll` produced by this project are **independently
compiled**, do not load or forward to original retail DLLs, and export
`CreateInterface` for `VClient011` and `ServerGameDLL003` respectively.

**Exactly two virtual methods exist per DLL. This is NOT a playable game
implementation, nor a generally safe drop-in retail replacement.**
The engine may call other virtual methods that do not exist in these vtables.
Both initialization methods intentionally fail closed, so engine startup
should not proceed, even if the object is successfully returned by its factory.

## Why the Phase 289 change is necessary

Phase 288 validated x86 stack cleanup for virtual slot zero but had a
semantic type mismatch against the actual **Source SDK 2004** headers:

| Native interface | SDK2004 slot 0 | SDK2004 slot 1 | Original SiN ABI observation |
| --- | --- | --- | --- |
| `VClient011` | `int Init(CreateInterfaceFn appSystemFactory, CreateInterfaceFn physicsFactory, CGlobalVarsBase* pGlobals)` | `void Shutdown()` | slot0 callee pops 12 stack bytes |
| `ServerGameDLL003` | `bool DLLInit(CreateInterfaceFn engineFactory, CreateInterfaceFn physicsFactory, CreateInterfaceFn fileSystemFactory, CGlobalVars* pGlobals)` | `bool GameInit()` | slot0 callee pops 16 stack bytes |

The third argument to the **client** `Init` is a pointer to engine
**data**, not a third function pointer. The original `ret 12` instruction
cannot distinguish these two kinds of 32-bit pointer. We use the Source
SDK 2004 C++ declaration as a *candidate type hypothesis*, not a claim
of fully recovered SiN-specific method semantics.

These prototypes and method names are described by the archived
[SDK 2004 cdll_int.h](https://github.com/Source-SDK-Archives/source-sdk-2004/blob/master/src_mod/public/cdll_int.h)
and
[SDK 2004 eiface.h](https://github.com/Source-SDK-Archives/source-sdk-2004/blob/master/src_mod/public/eiface.h).
No SDK header bodies or commercial code are copied into this project.

The 32-bit native MSVC tests now check the type-correct client
globals-data pointer in the x86 call sequence, both initialized vtable
slots being owned by the newly built DLL, independent `__thiscall`
dispatch, stack balance, and fail-closed return. The two SDK2004
candidates deliberately query only the first **two client**
(`VEngineClient012`, `VEngineModel012`) or **three server**
(`VEngineServer021`, `VoiceServer001`, `VEngineCvar002`)
engine interfaces, stopping on the first missing one. The returned
factory objects are never dereferenced or retained. The version names
are taken from SDK2004, not yet independently confirmed for SiN.

For an opt-in owner diagnostic set `SIN_INDEPENDENT_TRACE_FILE=1`
*before starting the process*. The independent DLL logs
`request`, `found-interface`, `missing-interface`,
`invalid-init-arguments`, and `sdk2004-preflight-only` to
`%TEMP%\sin-independent-<PID>.log`. File tracing is OFF by default.
It is intentionally separate from the original-DLL forwarding proxy's
`SIN_PC_PROXY_TRACE_FILE` logs.

Native MSVC test fixtures exercise first/second interface misses,
all first dependencies found, invalid globals, fail-closed return and
the opt-in log. The client second method safely clears diagnostic
state; server `GameInit` remains false. No real SiN game engine was
run or asserted to succeed.

## Compile and test

```powershell
cmake -S pc/independent_slot0 -B build-slot0 -G "Visual Studio 17 2022" -A Win32
cmake --build build-slot0 --config Release
ctest --test-dir build-slot0 -C Release --output-on-failure
```

GitHub Actions builds PE32 native diagnostic DLLs and tests with fake
host callbacks; it never runs the original SiN game. The outputs are
separate from Phase 284's **forwarding original-DLL proxies** and
must never be confused with a playable release.

**Next meaningful integration work:** recover the real original
engine-factory query contracts and expand a verified, complete
`IBaseClientDLL`/`IServerGameDLL` interface shape. Then use a
reversible, isolated owner-side game startup test, recording
the exact next failure without publishing any original game binaries.
