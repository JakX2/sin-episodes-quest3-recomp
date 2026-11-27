# Phase 1655: Original 2006 ServerGameClients003 full 15-slot vtable and player-limits ABI

**Stack:** Phase 1654 PR #1663, head `ca1381fb04963f8a5f88bf0dbc16f73120b38e0f`. Phase 1654 adds original client `GameClientExports001` 4-slot destructor/voice callback authentication and a native Win32 x86 fake virtual ABI fixture, and keeps incomplete exports unregistered. Its native final-head CI was still queued when this phase started.

## Authenticated retail PE32 server evidence

Read-only local inspection of the original owner `server.dll`, SHA256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`, PE32 Intel i386 image base `0x22000000`.

The previously recovered real server static `InterfaceReg` constructor at `0x221FB5D0` is used by the exact original 21-byte `ServerGameClients003` registration wrapper at `0x223BF230`. The string at `0x22443328` is the version name. The original factory thunk at `0x221E8320` returns the **static singleton `0x22510658`**, whose initialized vptr is `0x22443054`. These are **source-authenticated original VAs**, not inferred HL2 SDK structs.

The **complete original ServerGameClients003 table has 15 function-pointer DWORDs** at `0x22443054..0x2244308F` (60 bytes), whole table SHA-256 `5d0631d25abacc8fea804b22e5bf489b2d667ce240417c3ab20cc06787eb535f`. The next original .rdata starts `WriteBool called with no active message\n\0`; it is not a sixteenth entry. The 15 target addresses are authenticated individually by the new owner-private audit; no raw original code is committed.

| Original slot | VA | Source-backed interpretation |
|---:|---|---|
| 0 | `0x221E63F0` | `GetPlayerLimits`, exact original `__thiscall`, three integer output references, native `ret 0x0C` |
| 1 | `0x221E8330` | Original vcall wrapper; full higher-level signature not yet reimplemented |
| 2 | `0x221EA330` | Original original-game callback; signature not inferred from pointer alone |
| 3 | `0x221EA3A0` | Original original-game callback; signature not inferred from pointer alone |
| 4–14 | In the audit's `VTABLE_TARGETS` | 11 further original methods, not advertised as reconstructed |

The **complete original GetPlayerLimits implementation** occupies `0x221E63F0..0x221E6410`, exactly **33 bytes**, SHA-256 `5b5cbbc2f6e847b94658b3d595ffea6859a6a09d62124f13a838a7159b4785c3`. Its x86 code loads three stack-passed pointer arguments at `[ESP+4]`, `[ESP+8]`, `[ESP+12]`, writes respectively minimum `1`, maximum `32`, default `1`, and returns with `ret 12`. It does not read any engine or entity pointers in this method. The whole 33-byte original SHA is checked in owner-private audit mode.

## Concrete independently compiled replacement behavior

The independent Win32 `slot0.cpp` already has an opt-in `ServerGameClients003` probe with **15 virtual slots**, and already implements slot 0 `GetPlayerLimits` values 1, 32, 1 while guarding all other callbacks. This phase extends `test_server_factory_probe.cpp`: in addition to its existing typed virtual invocation, it now calls **that actual compiled replacement vtable slot** through MSVC inline x86 assembly with `ECX=this` and the three pointer arguments, checks `ESP` before/after, and checks all three outputs. This tests that replacement slot 0 matches the observed original **12-byte callee-cleanup calling convention**, rather than assuming a typed C++ call is enough.

It does **not** make guarded callback slots safe, nor does it register the opt-in probe for ordinary production startup.

## Owner-original Windows readback

The existing SHA-gated `sin_original_interfacereg_owner_probe.exe` also now reads the actual loaded original server's `ServerGameClients003` singleton vptr after original `LoadLibraryExA` and verifies **all 15 relocatable original method target RVAs** using `ReadProcessMemory(GetCurrentProcess())`. It prints only `original_servergameclients_vtable` result status and matched-slot count. It **does not call** any of the 15 virtual methods. The original game DLL's `DllMain` and a handful of simple original `CreateInterface` thunks *do* execute as part of this already existing owner-only loader probe; this is not an external passive snapshot. The owner should run it only against a controlled disposable copy of the exact retail DLLs and dependencies. No such owner-local original load is claimed in this session or public CI.

## Regression and limitations

`research/tools/audit_sin_original_server_gameclients_playerlimits.py`:

- Owner-private `--server <unmodified-original-server.dll>` mode validates the exact complete original file SHA, 21-byte static registration, original version string, six-byte singleton thunk, static vptr, every original method pointer/section, the whole vtable digest and its precise boundary marker, and whole original `GetPlayerLimits` digest. Emits only derived metadata.
- Original-free `--self-test` constructs fake PE32 pointer DWORDs and **invented different limit values** (4, 64, 5) for its x86 method fixture, tests all bytes of the synthetic 15-pointer table, static wrapper and 33-byte outparam/RET12 instruction sequence against corruption, and rejects fabricated original-server SHA.
- Linux decomp synthetic audit runs `--self-test`. Windows MSVC Win32 x86 CMake adds the Python original ABI CTest and a **no-original-file owner package guardrail CTest**, and recompiles the existing native server factory probe with its new explicit raw calling-convention check. Expected Windows native regression count: **256** once Phase 1654's 254-case suite is green.

### New downloadable original-owner Windows diagnostic package

Phase 1653 already compiled the SHA-gated `sin_original_interfacereg_owner_probe.exe`, but did not ship that executable to the owner in a stand-alone artifact. This phase sets its CMake `RUNTIME_OUTPUT_DIRECTORY` to the independent `bin/Release` output and adds the `SiN-Owner-Original-InterfaceFactory-Probe-Win32-NO-GAME-DLLs` GitHub Actions artifact, which contains exactly:

- `sin_original_interfacereg_owner_probe.exe` (native MSVC x86)
- `Run-OwnerOriginalInterfaceFactoryProbe.ps1`
- `OwnerOriginalInterfaceFactoryProbe-README.txt`

The package deliberately includes **no proprietary original DLLs**, maps, engine files or game content. Its PowerShell runner has a completely original-free `-SelfTest` validating path types, distinct client/server names, exact `bin` parent folders and the required `-OwnerDisposableCopyConfirmed` switch. Public CI both runs the synthetic CTest and parses/self-tests the packaged copy before uploading, with a three-file allowlist.

The explicit owner-only invocation, from an unmodified disposable game copy with its correct dependencies installed, is:

```powershell
.\Run-OwnerOriginalInterfaceFactoryProbe.ps1 -ClientDll 'C:\SiN-Test-Copy\bin\client.dll' -ServerDll 'C:\SiN-Test-Copy\SE1\bin\server.dll' -OwnerDisposableCopyConfirmed
```

This invokes the SHA-pinned C++ factory executable, which **loads the original DLLs and executes their real static initializers and selected creator thunks**. The C++ observer then reads the 55/10 original post-DllMain registry nodes, 4-slot client `GameClientExports001` vtable and 15-slot server `ServerGameClients003` vtable without virtual calls. The wrapper does not edit original game files. A successful result is still **only original-DLL factory ABI verification**, not proof that replacement client/server boot in the engine.

These are ABI and test advances, **not retail engine compatible original ServerGameClients behavior**. Client/Server complete initialization, physical engine ownership, gameplay on Windows and Android/Quest 3 startup all remain unverified. Under the user's explicit priority override, no generic physics, surfaces, props or model loading is added.
