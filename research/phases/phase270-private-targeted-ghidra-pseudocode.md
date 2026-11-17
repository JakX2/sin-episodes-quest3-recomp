# Phase 270: opt-in targeted Ghidra pseudocode, owner-private only

## Purpose
The existing `emit-c` option attempts to decompile every inferred
function, which is expensive and unnecessary for the next Windows x86
engine-compatibility milestone. A **separate, bounded `targeted-c` mode**
now decompiles only reviewed factory-address candidates for:

- `ServerGameDLL003`, `ServerGameClients003`, `ServerGameEnts001`
- `VClient011`, `VClientPrediction001`, `VClientEntityList003`
- `GameMovement001` and `GameClientExports001`, where present

The Ghidra 12.2 Java decompiler is driven by the reviewed PE32 interface
registration manifest, with at most eight functions per module.
`SeedReviewedInterfaceFactories.java` first tries to establish missing
boundaries without clearing existing Ghidra functions. Exact addresses must
still correspond to the two authenticated owner-original files.

**C pseudocode never goes into public GitHub source or the normal
Ghidra Actions metadata artifact.** Only a harmless synthetic ELF is
decompiled in hosted PR smoke tests. Status CSVs and C files from retail
DLLs remain on the owner's local private filesystem. Every successful
Ghidra decompilation is approximate; none is original C++ source, a
SHA-verified complete function body, a verified vtable slot, or a working
game DLL implementation.

## Owner-only invocation

Prerequisites:
- installed Ghidra 12.2 (with `support/analyzeHeadless`);
- legally owned original client/server DLLs in a private directory;
- a private output directory **outside** this repository.

```bash
bash research/tools/run_ghidra_original.sh \
  /private/tools/ghidra_12.2_PUBLIC \
  /private/sin-extracted/bin \
  /private/sin-targeted-output \
  targeted-c
```

The runner verifies the entire bytes of **both** original PE32/i386 DLLs
against the pinned SHA-256 digests before it invokes Ghidra. It then
builds local reviewed factory CSVs from
`research/catalog/original_dll_interface_registrations.json`,
runs targeted function seeding and the Java decompiler, and removes
scratch Ghidra projects and seed files. It refuses any targeted-C output
path nested inside this public repository, uses a restrictive Unix
umask, and writes private reports under the specified output directory.

Private output shapes:

```text
/private/sin-targeted-output/
  original_pe32_identity.json
  client.dll_factory_seed_results.csv
  server.dll_factory_seed_results.csv
  client.dll_targeted_decomp_status.csv
  server.dll_targeted_decomp_status.csv
  client.dll_PRIVATE_TARGETED_DECOMPILATION/
    VClient011_<native-address>.c
    ...
  server.dll_PRIVATE_TARGETED_DECOMPILATION/
    ServerGameDLL003_<native-address>.c
    ...
```

The status CSV explicitly records `decompiled_candidate`,
`missing_inferred_function`, `not_executable`,
`empty_decompilation`, or `decompilation_failed`.
A successful decompile cannot by itself determine `thiscall`,
`cdecl`, object layout, complete vtable slots, nor verified native
behavior. Manual analysis and independent ABI tests remain required.

## Synthetic CI

The existing Ghidra Actions PR job now decompiles the harmless
`SyntheticEngine001` function to a **separate runner temp directory**
and checks that its C pseudocode and status report were written.
This temp directory is outside all artifact upload globs. The test
does not read original game DLLs and cannot publish retail pseudocode.
Synthetic fail-closed tests also check wrong SHA, missing input
and unsupported modes.

```bash
python3 -m unittest discover -s research/tests \
  -p test_targeted_ghidra_runner.py -v
bash -n research/tools/run_ghidra_original.sh
```

## Remaining work
Review the resulting PRIVATE pseudocode and graph evidence for factory
thunks and their actual returned interface objects. Verify function
boundaries, x86 calling conventions and vtable slots, then implement a
non-forwarding replacement interface with a real retail-engine lifecycle
test. Retail Windows gameplay boot remains unproven; Android ARM64 and
Quest 3 remain downstream.

No new original methods are credited as whole-body-SHA-verified and no
new independently retail-engine-tested methods are claimed in this phase.
