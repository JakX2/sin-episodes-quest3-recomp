# Phase 252: automated original PE32 function atlas and optional headless Ghidra

16 November 2026. Stacked draft on Phase251 PR #307. This phase changes strategy from tiny, individually researched native helper PRs to reproducible bulk analysis and independent Windows gameplay DLL entrypoint milestones.

## Verified original binary provenance

| Original PE32 DLL | Original size | SHA256 |
| --- | ---: | --- |
| client.dll | 3,592,192 | 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d |
| server.dll | 6,049,792 | 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650 |

Both original DLLs were independently rehashed from the private bin.zip. No proprietary complete DLLs, whole native x86 code bodies, disassemblies, Ghidra decompiler output, or original assets are committed.

**New complete-original-SHA-reviewed methods in this phase: ZERO.** The existing ledger remains 403 (client 256, server 147). Automated candidates are intentionally NOT silently added to this ledger.

## Results: first successful original-only bulk PE scan

The original retail PE files were analyzed in a single automated read-only pass using GNU objdump linear sweep and strict PE32 export/import/relocation parsing.

| Observed metric | client.dll | server.dll |
| --- | ---: | ---: |
| Exported symbols | **1** | **5** |
| Imported symbols | **154** | **115** |
| PE32 HIGHLOW relocations | **143,620** | **268,435** |
| Direct CALL instructions found by linear sweep | **36,845** | **61,913** |
| Direct JMP instructions found | **10,284** | **15,159** |
| Distinct potential entrypoint addresses | **26,290** | **38,296** |
| Original whole-SHA-reviewed catalog addresses found | **256** | **147** |

**Important: 26,290 + 38,296 = 64,586 address candidates, NOT 64,586 proven functions.** Linear sweep can decode embedded executable-section data as instructions. INT3 starts are weak guesses. Do not derive a function-count or decompilation percentage from these numbers.

Candidate ranking and confidence tiers from the completed original-only private report:

| Tier | Client | Server |
| --- | ---: | ---: |
| Already in SHA-verified catalog | 256 | 147 |
| Export / PE entrypoint not otherwise reviewed | 1 | 5 |
| Direct CALL target and relocated code pointer corroborate | 220 | 444 |
| Direct CALL target only | 5,725 | 9,061 |
| Relocated data/rdata pointer target only | 7,531 | 11,378 |
| After INT3 boundary only, weakest | 12,557 | 17,261 |

Original PE export names and precise function VAs:

- client.dll CreateInterface: **0x24164180**
- server.dll CreateInterface: **0x221FB5F0**
- server.dll ?Detonate@CGrenadeHomer@@UAEXXZ: **0x2230F4E0**
- server.dll ?Detonate@CGrenadePathfollower@@UAEXXZ: **0x22310EA0**
- server.dll ?Detonate@CGrenadeSpitMutant@@UAEXXZ: **0x223120B0**
- server.dll ?Detonate@CGrenadeSpitPoison@@UAEXXZ: **0x22312B70**

These are genuine original export-table entries but not newly recreated executable implementations. Both CreateInterface entrypoints are priority targets for a standalone original-free Windows x86 DLL.

## Offline batch inventory: tested against original DLLs

Run from the repository root with Python 3.10+, GNU objdump and the private owner-provided archive:

    python3 research/tools/inventory_original_pe.py --archive /private/bin.zip --catalog research/catalog/original_dll_verified_methods.json --out /private/sin-batch-output

The tool checks PE32 structure and scans executable sections for direct CALL/JMP targets. It parses true exports, imports, relocation records and address-taken code pointers; then ranks candidate evidence, distinguishes original SHA-reviewed catalog entries and writes:

- client_function_candidates.csv and server_function_candidates.csv
- client_direct_edges.csv and server_direct_edges.csv
- client_exports.csv, server_exports.csv, client_imports.csv, server_imports.csv
- summary.json and README.md

The CSV files contain only addresses, names, count evidence and candidate labels, never binary byte dumps or disassembly text. ZIP contents are opened into a self-deleting local temporary directory for objdump. The pipeline does not falsely assert that every candidate is a function.

## Optional headless Ghidra batch decompilation

**Ghidra is not installed in this development container.** The optional launcher and Ghidra postScript have been authored but not run or version-validated here. To enable them on a machine with Ghidra:

    unzip /private/bin.zip -d /private/bin
    bash research/tools/run_ghidra_original.sh /path/to/ghidra /private/bin /private/sin-ghidra-output

This invokes local analyzeHeadless on both DLLs and exports Ghidra-inferred function names, prototypes, sizes, original call/reference relationships and relocated code pointer candidates as CSVs.

Optionally produce C-like pseudocode as PRIVATE LOCAL FILES:

    bash research/tools/run_ghidra_original.sh /path/to/ghidra /private/bin /private/sin-ghidra-output emit-c

Never commit wholesale Ghidra decompilation output. It is approximate and still requires manual validation of object layouts, virtual calls, compiler optimizations and class relationships. This tool is an optional local accelerator, not a proven independently compiling client/server DLL.

## Safe synthetic-only CI

- research/tests/test_inventory_original_pe.py builds synthetic test PE32 files with a named export, import, HIGHLOW relocation and function pointer, checks parser errors, confidence tiering and safe report generation. Synthetic data earns no original method credit.
- .github/workflows/original-pe-inventory.yml automatically executes those tests and the CLI help under Python 3.11 without any retail binaries.
- .gitignore excludes private analysis, Ghidra output and project files.
- The SHA-verified original-method catalog remains untouched until per-function original binary evidence is reviewed.

## Next execution priorities

1. Trace exact native CreateInterface entrypoints on both original DLLs and build the smallest independent Windows x86 export-compatible CreateInterface/startup harness.
2. Automate bulk class/RTTI/vtable/registration mapping and clean-room subsystem grouping rather than hundreds of tiny single-function PRs.
3. Ground candidate function boundaries with Ghidra and original whole-native-body SHA evidence. Do not promote scanner guesses into reviewed-method counts.
4. Prove replacement gameplay DLL loading and engine callback dispatch without forwarding into retail original DLLs. Only then claim initial executable Windows port readiness.
5. Prioritize PC first, Android ARM64 second, Quest 3 VR third.

The existing hourly scheduled decompilation automation was updated to use this bulk-first strategy without changing its hourly schedule.
