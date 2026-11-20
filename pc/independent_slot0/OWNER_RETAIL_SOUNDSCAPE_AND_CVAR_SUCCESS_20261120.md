# Owner-retail Windows x86 result: soundscape catalog and original ICvar read-only lookup verified

**Result status:** OWNER-OBSERVED, **not** a synthetic CI result. Captured
2026-11-20 in the user's independently run standalone diagnostic
package built from [Windows run 44823443084](https://github.com/JakX2/sin-episodes-quest3-recomp/actions/runs/44823443084),
corresponding to the original client.dll and server.dll independent
build chain through PR #511.

## Evidence and handling

The owner supplied the small text-only result ZIP directly to the project.
Its **SHA256** (for local private verification only) is
`f5499e2a0f5984ed331ec1abfc1d143566ee29468b622723eea7e09f3917637c`.
It contains eleven files: SUMMARY.txt, three owner phase transcripts
(presence/read/staged), three runner result files, three isolated
client logs, and the copy/restore summary. Archive integrity passed.

**Only the redacted findings below are committed.** The original ZIP,
its complete engine logs, local usernames, installation paths, temporary
paths, session identifiers, process IDs, absolute file addresses,
and the original 2006 commercial binaries are NOT committed.

The original engine's diagnostics were run through three independent
client-only launches, each using a separate original-SHA-verified,
disposable game copy. The complete staged launcher separately enabled
the optional original soundscape/catalog read and read-only ICvar query.
The expected original pair was restored and the disposable game directory
removed after the diagnostic. There was a first-try transient file lock
during staged and read restoration, followed by a successful retry and
verified cleanup. Do not confuse that retry with an engine ABI failure.

## Actual retail exit codes

| Diagnostic process | Owner-observed Windows exit | Meaning |
| --- | --- | --- |
| Presence | `0x53494E68` | Native `VFileSystem012` presence diagnostic |
| Read | `0x53494E6B` | Original statistics resources read and parsed |
| Staged + soundscape + ICvar | **`0x53494E76`** | **Owned catalog verified from original retail resources; diagnostic terminal** |

The previous highest owner-observed checkpoint **0x53494E74**
is now **superseded by 0x53494E76**. The 0x53494E77
refusal path was not taken in this run.

The session-scoped staged transcript independently classified:

- Original 2006 client engine factory supplied `VClient011` and
  interface `VEngineCvar002`.
- Actual `VFileSystem012` source file presence and read completed.
- Independent 66-stat source asset model verified, with source counts
  `66:62:8:156:66` and all seven owner modeled initialization stages.
- Three independent user-message hook ID declarations bound in
  source order `27:28:5` (`StaticStat`, `DynamicStat`,
  `ResetHUD`). **Not** genuine engine user-message dispatch.
- Corrected original file-search scopes from Phase 494:
  `rb/GAME` for the manifest, `rb/nullptr` for each source script.
- Original manifest parsed; **seven of seven individual soundscape
  scripts read successfully** and independently catalogued.
- Native result:
  `owned-soundscape-catalog-verified-original-files`.
- Source-shape counts:
  **7 documents**, **301 unique definitions**,
  **242 referenced soundscapes**, **3 unresolved reference edges**.
- Detailed source stage:
  `complete:owned-engine-soundscape-catalog-ready-no-game-init:owned-soundscape-manifest-ready:independent-soundscape-catalog-ready:7:7`.
- Optional native source-derived `VEngineCvar002` virtual slot 3
  `ICvar::FindVar` lookup for
  `snd_soundmixer` and `dsp_volume` returned:
  **`original-soundscape-both-vars-cached:2:2:1:1`**,
  two calls attempted, two returned non-null, both cache fields occupied.
  No ConVar values were read, changed, or registered.

The raw client session log contained exactly one entered/counts/result
set for the soundscape stage and exactly one entered/result/terminal
set for the ConVar stage. The ordered source event stream was complete,
ended with `engine-fs-soundscape-terminal-stop`, and the owner's
same-session PowerShell classifier emitted:
`CLIENT_SOUNDSCAPE_CATALOG_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF`.
This is independent evidence from the user's real retail engine,
not proof that genuine original `C_SoundscapeSystem::Init` ran.

## Important boundaries

1. The **client** independent DLL executed in an owner retail engine
   diagnostic context. This result reports **no server execution**.
2. The special `0x53494E76` process exit is an **intentional
   terminal probe**, not a return value proving normal `VClient011::Init`.
3. The catalog is an **independently owned parser**, not the engine's
   original KeyValues object graph, audio scheduler, mixer or DSP.
4. The 27:28:5 hook test is independent registration metadata,
   not user-message dispatch or in-game HUD.
5. Both ConVars were **found** using original interface ABI in a
   read-only diagnostic. They were not configured or invoked as audio.
6. The runtime monitor reported `OWNER_ENGINE_MODULE_NOT_OBSERVED:
   client.dll` despite collecting the independent client trace;
   its sampling does **not independently establish a module-load
   snapshot**. Do not inflate the monitor claim beyond the engine
   exit code and client-side event evidence.
7. **Normal standalone Windows startup remains fail-closed.**
   No actual map, graphics, entities, sounds, gameplay or Android/VR
   runtime became operational as a result of this diagnostic.

## Next implementation priority

The practical blocked milestone is an independently runnable normal
Windows x86 client.dll/server.dll pair that can proceed past startup
*without terminal opt-in diagnostics*, and then load at least one
real map. Specifically:

1. Preserve the now observed `0x53494E76` and original two-ConVar
   found result as a **regression baseline**: don't change original
   manifest/per-script path IDs or CVar slot3 ABI.
2. Recover the exact remaining real `VClient011::Init` and
   `ServerGameDLL003::DLLInit` owner-interface and callback sequence.
   Model the original AutoGameSystem registry, but don't claim the
   isolated fake-state list/dtor tests automatically fulfill
   production game-system startup.
3. Wire real owned engine calls into the appropriate production
   Init branches only with source-authenticated ABI/lifetime evidence.
   Failure must remain explicit and fail-closed, not fabricated success.
4. Build the next **opt-in original Windows runtime startup diagnostic**
   that checks an actual next callback after initialization and reports
   a unique terminal code. Request a new owner-run ZIP for that
   next code, not a repeat of the proven soundscape/CVar stage.
5. Continue the Android/Quest 3 work only after actual Windows
   client and server map startup is independently demonstrated.

The full owner ZIP is retained privately by the conversation's upload,
not the public repository.
