# Phase 497: opt-in owner-retail soundscape ICvar lookup

## What this adds over the synthetic-only Phase 496

Phase 496 implemented a real Win32 x86 \`VEngineCvar002\` virtual-call
adapter from the SHA-pinned original 2006 client method at 0x240F3C40,
61 bytes, complete SHA256
8a2bce6161d8de0973141bfae7d6c151091ee75933291d1c1d61b15d65b82040.
Its native tests used a fabricated engine object and did not prove
the source-original game's actual ICvar interface.

Phase 497 exposes that *already compiled and tested method* in the
existing **owner-only, disposable-copy and session-bound** diagnostic
chain. It is **OFF by default** and never changes the meaning of the
existing original soundscape catalog 0x53494E76/77 terminal exits.

Original 2006 source evidence at 0x240F3C40:

- C_SoundscapeSystem vtable at 0x24293C34, slot 3.
- This method checks cached original global pointer 0x2433E4AC;
  if null, uses ICvar::FindVar via VEngineCvar002 vtable byte
  +0x0C (slot 3) for "snd_soundmixer".
- It checks original global pointer 0x2433E4A8; if null, uses
  the SAME ICvar slot 3 for "dsp_volume".
- There is NO original ConVar::SetValue or console registration
  within the 61-byte body. It can find neither variable yet
  still returns void.

Source original owned client.dll SHA256:
6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d.
No original game binaries are shipped to CI or the public repository.

## Activation

From the latest Windows x86 standalone GitHub Actions artifact:

    RUN_SOUNDSCAPE_CATALOG_TEST.bat -SoundscapeCvarLookup

The existing command without the additional flag still only performs
the original soundscape catalog diagnostic. Both commands use the same
correctly restored, SHA-authenticated installed original game as
source, duplicate the game into a disposable Windows TEMP copy,
and run the diagnostic client/server DLLs only inside that copy.
No independent DLL is placed in the installed retail game.

The option is explicitly carried through
Start-SiNFileSystemTest.ps1 -> Invoke-OwnerFileSystemRetailGate.ps1 ->
Invoke-OwnerIndependentEngineProbe.ps1, with the existing
- StagedStats
- HookProof
- SoundscapeCatalog
and the prior restored presence/read session proofs REQUIRED first.

The SHA-pinned launcher snapshots/restores the PROCESS environment
variable SIN_INDEPENDENT_CLIENT_SOUNDSCAPE_CVAR_LOOKUP_PROBE. The
independent DLL checks the entire existing chain plus this additional
flag before calling the cache adapter. The current client's real
VEngineCvar002 pointer is collected from the already-authenticated
fourth app-system factory query, never inferred from a retail address.

The new code invokes only the **already-authenticated original
ICvar::FindVar virtual slot 3**. Its method address must pass the
native Win32 verified-method pointer and module-owner guards.
It does not dereference a returned ConVar pointer, register commands,
change mixer settings, start DSP, create original KeyValues objects
or call the normal game initialization path.

## Terminal and report invariants

After the original catalog stage result and failure stage details,
the opt-in new trace adds, in order:

1. \`engine-fs-soundscape-cvar-entered\`:
   \`VEngineCvar002:slot3:read-only\`.
2. \`engine-fs-soundscape-cvar-result\`:
   fixed \`status:requested:found:mixer-present:dsp-present\`
   with bounded 0-2 counters and 0/1 booleans.
3. \`engine-fs-soundscape-cvar-terminal-stop\`:
   \`no-convar-writes-no-game-init\`.

Then the EXISTING soundscape terminal message and process-exit code
execute normally. Presence of the two queried ConVars is **advisory**:
it does not modify the original 7:301:242:3 catalog proof nor the
true/false classification of real Windows soundscape source reads.
If neither ConVar exists at this point in the engine lifecycle,
the trace says so; this does not prove missing original runtime audio.

The owner-local transcript classifier emits the optional
\`OWNER_CLIENT_SOUNDSCAPE_CVAR_LOOKUP_RESULTS:\` summary line only for
a **properly ordered, exactly three-event, same-session** source trace
preceded by original staged Stats and hooks, followed by the actual
soundscape terminal. It rejects duplicate, forged or premature
records; the main catalog verdict stays unchanged.

The one-click result ZIP's \`SUMMARY.txt\` now includes
\`Original soundscape ConVar lookup (read-only): ...\` when requested.
When prior gates fail, it says NOT_RECORDED rather than claiming an
unobserved ConVar result. No ConVar value, pointer address, source
soundscape filename, installed machine path or original asset bytes
are serialized.

## Public regression tests

Existing native x86 test
\`test_client_soundscape_original_convar_cache.cpp\`
exercises exact ICvar slot 3 on a native fake engine object, no
ConVar writes, two-name lookup, cache/lookup retry, invalid pointer,
owner identity reset and callback exception. Phase 497 adds real
engine-call *activation* but public CI **does not** launch retail SiN.

PowerShell source-free synthetic fixtures also require:

- Default one-click \`-SoundscapeCatalog\` has no ConVar probe.
- \`-SoundscapeCvarLookup\` requires the existing one-click
  \`-StagedStats -HookProof -SoundscapeCatalog\` chain.
- The gate refuses the new mode outside Soundscape phase and
  cannot bypass a previous verified read transcript.
- Same-session orderly CVar result appears in text summary.
- Duplicated, out-of-order or inconsistent fake CVar traces
  do not become accepted summary evidence.
- The new environment flag is explicitly reset with the
  previous process value after the disposable launch.

## Limits and next actual retail evidence

**No owner-run retail ConVar lookup has been observed yet** as this
phase is authored. The most advanced user-observed real Windows
checkpoint remains 0x53494E74. The existing 0x53494E76/77
soundscape catalog exit codes remain prospective until the owner
runs the latest one-click artifact. This optional lookup is not
automatic gameplay, a successful VClient011::Init, working audio,
a finished Windows client or an Android/Quest 3 port.
