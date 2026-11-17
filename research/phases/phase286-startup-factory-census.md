# Phase 286: bounded Init / DLLInit factory dependency census

**Purpose:** turn Phase 285's original x86 stack-cleanup evidence into a
narrow, reproducible research handoff for the independent Windows runtime.
Do not confuse method-entry identification with verified full ABI semantics.

## Inputs and boundaries

- Full-file SHA256-verified 2006 `client.dll` and `server.dll` from the
  existing private owner staging workflow.
- Phase 280 reviewed singleton/vptr/RTTI and slot-zero method seeds.
- Phase 285's original x86 return cleanup: `VClient011[0]` 12 bytes,
  `ServerGameDLL003[0]` 16 bytes. These establish three and four
  stack-passed arguments respectively, **not their actual types**.
- Archived Source SDK 2004 supplies only the method-name hypotheses
  `IBaseClientDLL::Init` and `IServerGameDLL::DLLInit`.

The new Ghidra 12.2 script
`research/ghidra/ExportStartupFactoryDependencyCensus.java` examines
**only these two original slot-zero method entries**. It requires
`SIN_PRIVATE_DECOMP_ACK=1`, the existing reviewed seed file, and an
allowlisted original module name. For each method, it measures the
Ghidra-inferred function extent, instruction count, direct/indirect call
site counts and strictly allowlisted interface-version string data
references. The script **never exports arbitrary strings**, disassembly,
machine code, C pseudocode, private paths or full call graphs.

The raw one-row CSV per original module remains under ephemeral
`sin-ghidra-private`. A separate validator
`research/tools/summarize_startup_factory_census.py` enforces exact
reviewed method entrypoints, bounded counters, exact schema and an
interface-version-only literal allowlist. Only sanitized
`startup_factory_dependency_census.json` may enter the metadata artifact.
Ten synthetic negative tests run on pull requests; no original game
binaries are required for these tests.

**Important:** Ghidra may miss indirect factory calls or interface
strings that are propagated through registers, pointers, non-data
references or other functions. The resulting counts are conservative
Ghidra observations, not a complete execution trace. The code deliberately
does not guess the first failing branch, argument values, return statuses,
or any actual engine factory query order.

## Runtime handoff

For the dedicated Windows runtime task, the immediate testable
contract remains:

| Slot | Hypothesized SDK 2004 method | Confirmed original callee stack cleanup | Unverified |
| --- | --- | --- | --- |
| `VClient011[0]` | `Init` | `ret 12` | Three argument types, engine factory names, first failure branch |
| `ServerGameDLL003[0]` | `DLLInit` | `ret 16` | Four argument types, three factory roles, first failure branch |

After the original-only Ghidra job has actually run with this PR integrated
into the default branch, inspect its two sanitized census records. Use
indirect call counts and literal candidates to focus **private** C-like
pseudocode review of these two methods, then corroborate with a real,
reversible Windows engine startup trace. Do not promote an inferred
string/call count to a verified method signature.

## Evidence and verification status

The existing Phase 285 CI completed successfully on PR #344. This
phase's PR smoke executes synthetic-only tests. The owner-original job
is intentionally default-branch-only and cannot be claimed as executed
until integration and a separately verified owner run. No independent
retail Windows game startup or complete original method implementation
is established by this phase.

Do not upload original PE32 binaries, Ghidra pseudocode or proprietary
instruction sequences to GitHub.
