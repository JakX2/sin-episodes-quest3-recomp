# SHA-grounded two-stage stats resources for the independent client

Previous synthetic coordinator PR #452 verified the original seven
initialization callsites but read both documents together at the
fourth display-layout stage. PR #455 recovered the original 66
compiled stat identifiers and a safe separately owned text binding.

Direct SHA-matched 2006 original client.dll evidence:

- C_StatsSystem::Init calls owner catalog helper at 0x241FC5A0.
- After allocating 66 pointer records, that helper constructs a
  KeyValues instance and reads scripts/stats/stats_definitions.txt
  using the original game filesystem and the GAME path group.
- The loaded KeyValues child records are traversed and each name,
  long_text, short_text and percentage_of is processed at 0x241FBD20.
- 0x241FFEE0 resolves names through a private 66-entry registry
  constructed by 0x241FFB60 from the original RVA 0x31E428 table.
- The next original stats Init stage consumes display-layout text
  scripts/stats/stats_display_layout.txt separately.

New independent staged mode:

1. At original Init stage 3, reset the independently owned 66
   catalog records, fetch and parse the definitions text, resolve
   names against the original 66-ID table, and bind safe owned
   strings plus percentage-index integers. Invalid/missing input
   refuses at attempted=3, completed=2.
2. At stage 4, fetch display-layout text separately; parse and
   validate both definitions and layout, publishing the two existing
   Documents vectors atomically only on successful parsing.
   Invalid/missing layout refuses at attempted=4, completed=3.
3. Remaining original UI and event-registration callbacks remain
   separately injected and fail closed when missing or refused.

An optional Hooks::read_stats_resource callback supplies only an
exact whitelisted path and raw text, using independent host string
ownership. Hooks::stats_resource_context is separate from the
game-system host pointer. The new ReadForStagedStats adapter uses
the SHA-authenticated original FileSystem_Steam.dll ABI's
Open, Read, Size(handle) and Close slots. It reads at most 256 KiB
per document, closes all successfully opened handles and never
writes to game files. It is not retail KeyValues construction or
the game's original parser.

Conflicting old combined-reader, filesystem-root, or injected
resource-loader choices are rejected before the first Init step.
Other old tests and the existing source-free fixture path retain
the earlier behavior unless the new callback is explicitly supplied.

Native synthetic tests cover normal seven-stage order, 62 bound
definitions, four compiled-only IDs, definition-stage read refusal,
layout-stage refusal, malformed definitions and layouts,
published-data retention, preflight source conflicts, and retries.
An additional test invokes the staged coordinator through the
actual independent SDK2004 VFileSystem012 x86 adapter with a
synthetic dual-vtable engine object.

This code is NOT wired into retail client.dll normal startup.
Retail FileSystem method invocation, original KeyValues, callback
registration, complete normal VClient011 Init and gameplay boot
are not yet verified. Source PC startup first, Android second,
Quest 3 third. No original binaries or scripts are committed.

## Exact original mode and path group evidence

Original SHA-matched client.dll's KeyValues load at 0x24164A50
is reached from original stats catalog 0x241FC5A0 with path group
"GAME" and Open mode "rb" for stats_definitions.txt.
The next display layout helper at 0x24204D90 uses Open mode "rt"
and a null path group for stats_display_layout.txt. The staged
reader now selects those original parameters based on the two
whitelisted resource names; the older combined reader keeps its
existing explicit binary/null probing behavior. Native synthetic
VFileSystem012 checks reject an incorrect mode/pathID combination.

Owner retail behavior is still unverified: in particular, text-mode
line-ending translation may cause Size(handle) to differ from the
number of returned bytes. Until a real owner test confirms it, the
strict 256 KiB exact-read and parser safeguards fail closed rather
than accepting incomplete content.

## Fail-closed exact original source counts

The owner SHA-pinned original source scripts contain 62 definitions,
eight percentage_of references, 156 paired display entries and 66
divider lines. The new staged-mode parser validates those counts
BEFORE permanently publishing either Documents vector. Correct-looking
but shortened source documents must be rejected at the corresponding
third or fourth original Init step. Existing combined-document probes
and explicit loose-file workflow are unchanged. These strict counts
are appropriate for reconstructing the unmodified SiN Episodes retail
content; supporting custom stats mods can become a separate option
after authentic Windows startup is established.
