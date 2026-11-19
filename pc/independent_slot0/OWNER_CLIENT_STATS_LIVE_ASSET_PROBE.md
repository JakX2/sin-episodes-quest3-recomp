# Opt-in original-content stats asset probe after native sound Init

This is an additional **Windows x86 retail-engine diagnostic**, stacked on
PR #448's clean-room parser and loader. No commercial data is committed.

## What it does

The previously owner-verified independent \`CSoundEmitterSystem::Init\`
engine vtable call returned true (terminal code \`0x53494E63\`).
**Only after that succeeds**, a separate opt-in can read the existing
\`SE1/scripts/stats/stats_definitions.txt\` and
\`SE1/scripts/stats/stats_display_layout.txt\` files directly from disk.
The independent C++17 loader validates both documents and expects the
owner-pinned original counts: 62 definitions, 8 percentage references,
156 display pairs and 66 divider rows. It logs only counts and status,
not source data or local paths.

This checks whether real content is visible to our *replacement process*.
It is **not** the game engine's IFileSystem, the original KeyValues ABI,
the original C_StatsSystem::Init or a normal VClient011::Init return.

## Opt-in owner experiment

Use the existing disposable Windows retail test copy and its full
previously verified, session-bound probe environment, through
\`SIN_INDEPENDENT_CLIENT_SOUND_SYSTEM_INIT_PROBE=1\`.

Add both:

- \`SIN_INDEPENDENT_CLIENT_STATS_ASSET_PROBE=1\`
- \`SIN_INDEPENDENT_CLIENT_STATS_ROOT=<absolute-path-to-SE1>\`

The root MUST be the content directory *containing* \`scripts/stats\`.
The environment variable value is a path, not a literal string with
angle brackets or surrounding quote characters.
Keep \`SIN_INDEPENDENT_TRACE_FILE=1\` and the required 32-character
lowercase-hex \`SIN_INDEPENDENT_TRACE_SESSION\`.
If the previous sound slot returns false the new probe never runs.
If the new flag is absent, previous sound terminal codes are unchanged.

Terminal codes:
- \`0x53494E65\`: both files parsed; original stat counts verified;
- \`0x53494E66\`: no absolute root, filesystem/parse refusal or exception;
- \`0x53494E67\`: files parsed but counts differ from the owner originals.

The count result does not by itself authenticate the file bytes. The
previous owner SHA256 fingerprints remain the separate reference.
Restore original client.dll after the disposable test. Do not use these
diagnostic DLLs for normal gameplay.

Synthetic-only native CTest verifies valid counts, missing/relative path,
missing and malformed files, mismatched counts, and successful retry.
No new owner retail trial or successful game boot is claimed by this PR.
