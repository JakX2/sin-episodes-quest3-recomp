# Phase 502: fail-closed interface evidence queue gate

This opt-in Python 3 standard-library tool consumes a **reviewed** JSON array and emits a deterministic, source-free queue report. It does not parse proprietary binaries, call Ghidra, authenticate original functions, implement interfaces, or demonstrate retail startup.

Run `python tools/validate_engine_interface_evidence.py --self-test` for synthetic regression checks. Run `python tools/validate_engine_interface_evidence.py reviewed.json --output queue.json` on a locally reviewed manifest. Exit codes: 0 valid, 1 invalid evidence, 2 malformed input or I/O failure. A missing input is an error, never an empty successful queue.

Each JSON entry requires `interface`, `module` (`client` or `server`), integer PE32 `rva`, `tier`, nullable lowercase 64-character `body_sha256`, boolean `boundary_verified`, and boolean `native_engine_tested`. The six supported interface anchors are VClient011, ServerGameDLL003, ServerGameClients003, ServerGameEnts001, VClientPrediction001, and VClientEntityList003.

Candidate addresses cannot claim independently verified boundaries or engine execution. Higher tiers require a reviewed boundary and full method body SHA. Only `owner_retail` may set `native_engine_tested=true`; this remains a **reviewer assertion**, not cryptographic proof of engine execution. Duplicate (module, interface, RVA) entries fail. Canonical sorting and a report digest support reproducible batch review. Do not treat the digest as a hash of the original DLL.

This gate deliberately does not ingest unreviewed Ghidra metadata or promote any of the 64,586 historical candidates. The historical 403 SHA-reviewed methods are not reauthenticated here. It is an evidence hygiene prerequisite for future ABI queue integration, not a replacement for call-graph/vtable/RTTI analysis, real PE32 factory testing, or original executable/map evidence.

Next: connect a reviewed, source-free metadata export to this schema; independently verify original function boundaries and hashes; build true x86 `thiscall`/vtable lifecycle interoperability tests for VClient011 and server interfaces; verify Ghidra schedule events separately. No commercial binaries or disassembly are included.
