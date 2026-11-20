# Phase 474: direct owner-retail evidence for independent Stats user-message hooks

## Motivation

The user-provided 2026-11-20 Phase 473 results ZIP
`SiN_FileSystem_Test_Results_20261120-023819-4d62f137bf.zip`
was inspected directly (private ZIP not committed). It proves genuine
original-engine launches at 0x53494E68 / 0x53494E6B / 0x53494E70,
counts 66:62:8:156:66, seven modeled steps, and SHA-pinned
restoration plus removed temporary clone. The staged trace alone
does not explicitly prove WHICH newly reconstructed independent
Stats user-message hooks were registered.

This owner-only opt-in probe makes that proof a separate terminal
milestone. It uses the exact previously authenticated CUserMessages
message IDs from the shared client/server 39-entry catalog:
`StaticStat=27`, `DynamicStat=28`, `ResetHUD=5`.

The rebuilt client first repeats successful original-engine
VFileSystem012 presence/read and independently owned Stats staged
rb/GAME plus rt/null operations. The owned Stats registry records
three registrations; the new hook proof logs the exact order
`27:28:5` and count 3. It does NOT register these handlers into
the actual engine CUserMessages object, parse real bitbuf payloads
or dispatch gameplay messages.

## Owner test

Extract the complete GitHub Actions ZIP outside the installed game
folder, double-click **RUN_STATS_HOOK_PROOF.bat**, select the original
game root containing SinEpisodes.exe, and send the resulting
`SiN_FileSystem_Test_Results_*.zip`.

All independent PE32 DLLs and scripts are contained inside the ZIP.
No manual DLL staging, user preparation or original game modification.
The runner tests prerequisites in order, using three fresh process
sessions in one temporary game clone. Original SHA-verified DLL backups
must be restored after EACH launch; uncertain cleanup retains the copy.

Expected third-session success evidence:

* original `engine-fs-staged-counts:66:62:8:156:66`
* original `engine-fs-staged-stage:7:7`
* `engine-fs-hooks-entered:CUserMessages:owned-only`
* `engine-fs-hooks-ids:27:28:5`
* `engine-fs-hooks-count:3`
* `engine-fs-hooks-result:three-owned-hooks-bound-not-dispatched`
* `engine-fs-hooks-terminal-stop:independent-only`
* **new terminal exit 0x53494E74**, a separate
  `OWNER_CLIENT_ENGINE_FS_HOOK_RESULTS` same-session line, and
  `CLIENT_ENGINE_FS_HOOKS_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF` verdict.

0x53494E75 means owned-hook binding refused. A missing earlier
presence/read/staged count is NOT converted into a hooks success.

Only the new BAT opts into hook proof. RUN_SIN_TEST.bat and
RUN_STAGED_STATS_TEST.bat retain their old meaning and exit codes.

## Limits

The phase remains a deliberate terminal diagnostic; there is still
no complete normal VClient011::Init return, actual Stats network
payload decoding, original KeyValues construction, game menu,
map spawning, or Android/Quest 3 runtime. A successful 0x53494E74
would verify independently owned callback-ID registration after
authentic staged filesystem access, not network gameplay.

The new code is synthetic CI-tested, not yet owner-retail verified
until a resulting real Windows trace is supplied.
