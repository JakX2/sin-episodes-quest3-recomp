# Phase 152: whole original SiN Episodes DLL recovery atlas

**9 November 2026.** Objective: stop treating individual discoveries as an uncountable collection of phase reports. Build one searchable, auditable ledger of *everything currently identified* inside the original Windows 2006 game **client.dll and server.dll**, while preserving a rigorous difference between discovered addresses, original-byte verification, clean-room C++ behavior, and a genuine in-engine replacement.

## Binary identity

The owner-held 2006 original **client.dll** is 3,592,192 bytes, SHA-256:

    6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d

Original **server.dll** is 6,049,792 bytes, SHA-256:

    568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650

Both are native PE32 Intel x86 game modules, not ARM64 ELF libraries. The new atlas command **refuses** mismatching original hashes. Manually verified methods also require an exact original byte-region SHA-256 and length, both of which are checked against the actual private PE32 .text section when the atlas runs.

## Whole-image discovery currently available

| Evidence category | Original client.dll | Original server.dll |
|---|---:|---:|
| Distinct original TypeDescriptors (RTTI) | 799 | 1,499 |
| Classes with validated RTTI inheritance | 797 | 1,485 |
| Validated MSVC virtual tables | 1,428 | 1,750 |
| Virtual slot references, including inherited slots | 61,653 | 123,162 |
| Distinct executable virtual targets | 5,919 | 8,664 |
| Unique direct CALL target addresses in .text | 6,099 | 9,579 |
| Candidate function entrypoints (vtable union CALL targets) | 11,804 | 17,808 |
| Verified original InterfaceReg constructors | 55 | 10 |
| Verified 22-byte entity-factory registrations | not this DLL | 510 |

The code-census counts are the limits of known *discovery methods*, not true exhaustive counts of machine-code functions. Nonvirtual functions reached only by indirect calls or tail jumps, jump-table targets, callbacks, and code hidden from the linear disassembler may be absent. A candidate can instead be a thunk or a bad linear decode. Inherited virtual slots cause massive duplication. Neither a total candidate count nor a total phase count yields a truthful percentage of decompiled SiN game logic.

## Other code and data already inventoried

- **Client/server interface layout:** engine-facing VClient011, ServerGameDLL003, ServerGameEnts001, ServerGameClients003; original vtable counts and initial factory dependencies. The source game's old x86 interfaces are **not** interchangeable with SourceVR/HL2VR modern ARM64 versions.
- **Network tables:** 210 distinct client RecvTable names by known constructor shape, 1,079 recognized client properties out of 1,107 declared in that pattern; 40 unique server SendTable constructor calls, 270 recognized server properties out of 275. The server also contains at least 183 distinct DT_ strings, so the 40 recognized constructors are not the entire server network protocol.
- **Gameplay entity registrations:** 510 server entities matching the verified 22-byte registration pattern, including NPCs, weapons, triggers, props, AI, environmental effects and I/O helpers. They are *factories*, not 510 independently completed gameplay classes.
- **Actual gameplay-specific RTTI:** weapons, SinTEK grunt/jetpack/Quadralex/Jessica NPCs, poison/antigen, difficulty, phone, vehicle physics, predicted client weapon classes, client HUD and animation.
- **Original code regions partially reconstructed:** rifle and grenades, AI schedules, scene I/O, combat and vehicle rules, client viewmodels, initialization, loading UI and shutdown. The Phase 148–151 difficulty chain alone now connects raw stat samples through float ring history, gates, CStatAdvisor decisions, and original DDAdvisorScores messages received in client.dll.

## New searchable catalogs

The owner-local atlas generator creates seven evidence-only artifacts:

1. SiN_2006_Both_DLLs_Recovery_Atlas.json: per-DLL discovered/verified/reconstructed status counts, fingerprints, caveats.
2. SiN_2006_Function_Recovery_Atlas.csv: every candidate VA and its seed evidence, RTTI class *hints*, incoming call count, string *hints*, and explicitly reviewed recovery level.
3. SiN_2006_RTTI_Class_Atlas.csv: class/type descriptor labels, verified hierarchy availability, vtable counts and virtual target counts. Covers RTTI TypeDescriptors as well as classes with inheritance metadata.
4. SiN_2006_Server_Entity_Factory_Atlas.csv: every factory recognized by the validated compiler idiom.
5. SiN_2006_Engine_Interface_Atlas.csv: original 55 client and 10 server InterfaceReg constructor registrations.
6. SiN_2006_Network_Table_Atlas.csv: all constructor-backed tables recognized by the two existing network analyzers; server DT_* strings without a verified constructor are separately labelled **string-only**.
7. SiN_2006_Network_Property_Atlas.csv: every named property initializer from recognized tables, plus each unresolved declared slot, with actual original client/server x86 object offsets only where validated.

CSV files are searchable with ordinary spreadsheet/terminal tools or can be imported into a SQLite database for later UI work. The full metadata files generated from the owner's original binaries stay **local to the owner**; only scanner sources and a small manually reviewed evidence ledger live in this public repository.

The reviewed SHA-pinned method ledger initially contains **26 entries**, 25 server and 1 client, selected from the exact original-byte evidence in Phases 85 and 148–151. The one client entry represents original Init code verification; it is *not* a completed reconstruction of the client. This ledger is intentionally an **initial sample of reviewed implementations**, not a claim that all previously verified phases have been ingested. Earlier verified systems must be reviewed and added with original address, exact code length/SHA, implementation, test and evidence document. Do not equate this sample with total project recovery.

Recovery status meanings:

| Level | Requires | Does NOT imply |
|---|---|---|
| candidate_only | Vtable or direct x86 CALL target from scanner | True function boundaries, identity, correct disassembly |
| machine_code_verified | Manually reviewed original VA/region hash + evidence document | Whole original function semantics or C++ replacement |
| behavior_reconstructed | Original code evidence plus reviewed C++ implementation and tests | ABI-compatible engine module or game execution |
| engine_runtime_verified | Real-engine run and demonstrated behavior, reviewed separately | Full game completion |

**Current atlas deliberately rejects engine_runtime_verified entries** until real original engine evidence is archived and explicitly approved. The generator does not automatically mark any method from the presence of an adjacent string or from a passing mock test.

## How to produce the complete currently known catalogs from owner copies

From repo root, using the real private 2006 DLLs, with GNU binutils objdump accessible:

~~~powershell
py research/tools/audit_sin_rtti_vtables.py --client "C:\SiN\bin\client.dll" --server "C:\SiN\bin\server.dll" --output "C:\private\sin-rtti.json"
py research/tools/audit_sin_entire_dll_code_census.py --client "C:\SiN\bin\client.dll" --server "C:\SiN\bin\server.dll" --rtti "C:\private\sin-rtti.json" --output-dir "C:\private\sin-census"
py research/tools/audit_full_sin_game_dlls.py --client "C:\SiN\bin\client.dll" --server "C:\SiN\bin\server.dll" --output "C:\private\sin-factories.json"
py research/tools/audit_sin_recv_tables.py --client "C:\\SiN\\bin\\client.dll" --server "C:\\SiN\\bin\\server.dll" --output "C:\\private\\sin-recv.json"
py research/tools/audit_sin_send_tables.py --client "C:\\SiN\\bin\\client.dll" --server "C:\\SiN\\bin\\server.dll" --output "C:\\private\\sin-send.json"
py research/tools/build_sin_both_dll_recovery_atlas.py --client "C:\SiN\bin\client.dll" --server "C:\SiN\bin\server.dll" --rtti "C:\private\sin-rtti.json" --census-dir "C:\private\sin-census" --factories "C:\private\sin-factories.json" --verified "research/catalog/original_dll_verified_methods.json" --repo-root . --output-dir "C:\private\sin-recovery-atlas"
~~~

The atlas builder performs no disassembly itself. Supplying both network reports enables the two network CSVs; leaving them out produces empty network CSVs with an explicit network_audit_supplied=false flag, rather than pretending to have checked those tables. It requires the already verified RTTI, code census and factory reports. Any mismatch between those reports and the original SHA-pinned binaries or the expected candidate-row counts is a **fatal error**. It also verifies every reviewed method's original code-region SHA directly from the private binary. Every output entry is metadata only, never copied executable bytes.

Synthetic test (does not require the original game files):

~~~powershell
py -m unittest discover -s research/tests -p 'test_build_sin_both_dll_recovery_atlas.py' -v
~~~

## What is still unknown or unimplemented

This is a **complete searchable atlas of the currently recognized code-entry candidates and class/factory registrations**, not a finished decompilation of all original instructions. Many original client presentation and prediction routines, unrecognized server send-table compiler patterns, nonvirtual helpers, callback dispatches, original gameplay event producers, client/server network types/flags, proprietary game engine integration and real campaign playback remain open. Source-level comments, original identifier spelling for stripped symbols and exact original build system cannot generally be recovered merely from optimized retail DLLs.

## Next research priorities using the atlas

1. **Expand the manual verified-method ledger across earlier gameplay phases**. Record evidence rather than inflating status from source files.
2. **Balance both binaries**: prioritize original client animation/prediction, HUD/weapons and networking, alongside server NPC/weapon state.
3. **Build coverage-relevant class and call cross-references**: original virtual owners, referenced strings and direct callsites are triage hints. Recover real control-flow graphs and nonvirtual helpers before writing C++.
4. **Recover the remaining server SendTable constructor patterns** to align complete client/server properties, types, flags and quantization.
5. **Validate real Windows gameplay DLL loading**, then replace proven behavior incrementally; Android and standalone Quest 3 remain later stages.

No original binaries are published; none of these outputs proves a playable Windows, Android or Quest game.
