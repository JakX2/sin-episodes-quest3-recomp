# Phase 272: ephemeral hosted Ghidra C-like decompilation and metadata-only report

## Development problem

The verified original DLLs can be fetched and analyzed by GitHub Actions,
but until this phase the scheduled pipeline only exported function/call
metadata. The separately tested Phase 270 Java postScript can decompile a
small reviewed interface-factory batch into C-like pseudocode. This phase
wires **that decompiler** into the existing authenticated owner-original
Ghidra run without publishing any game pseudocode.

## Production execution after integration into main

1. Download owner-original `client.dll` / `server.dll` archives to runner temp.
2. Authenticate **both complete original PE32 DLLs** against fixed SHA-256
   digests using the existing staging step. Fail closed if either differs.
3. Prepare the reviewed 65 factory seeds, 55 client and 10 server.
4. Only *after* authentication, create a private `RUNNER_TEMP` directory
   outside the checked-out repo and public metadata directory, set
   `SIN_PRIVATE_DECOMP_ACK=1`, and run Ghidra headless once for both DLLs.
5. During the same analysis: try seeding reviewed missing factory boundaries,
   run the bounded Java `DecompInterface` pass for up to eight priority
   interface factory candidates per module, and export the normal metadata
   atlas.
6. Store generated C-like text in `RUNNER_TEMP/sin-ghidra-private/
   <module>_PRIVATE_TARGETED_DECOMPILATION/`. These files are never placed
   in `RUNNER_TEMP/sin-ghidra-metadata`, committed, or uploaded.
7. Validate private *status CSVs* against reviewed factory identities with
   `summarize_private_ghidra_decomp.py`. The report contains only function
   virtual addresses, statuses and inferred function sizes, never C code.
8. Upload the normal metadata artifact with an additional
   `private_targeted_decomp_summary.json`. Remove original archives,
   staged game binaries, Ghidra metadata, private C files and seed CSVs
   from runner scratch on completion or failure.

Statuses include `decompiled_candidate`, `missing_inferred_function`,
`not_executable`, `empty_decompilation` and `decompilation_failed`.
The report explicitly sets newly SHA-verified original methods and
engine-load-tested methods to **zero**.

## Security/evidence boundaries

- Original data comes from the owner's archives and is SHA-256 authenticated
  *before* private decompilation is authorized.
- All factory VAs are compared against previously reviewed registration
  metadata. The sanitizer rejects missing, duplicated or unexpected
  status records, malformed addresses, unexpected outcomes, unreviewed
  provenance, and textual data in the inferred-size field.
- GitHub-hosted PR smoke **only** imports a harmless synthetic ELF,
  removes a synthetic function start, recovers it from a synthetic seed,
  and confirms the private C-like decompiler creates nonempty pseudocode
  outside the uploaded artifact directory.
- The runner refuses a configured private output path inside the repository
  or metadata directory and requires an explicit opt-in value.
- Ghidra's C-like output is *not* a faithful original C++ source dump.
  Even a success status is only a candidate, not a complete verified
  function, vtable method signature, known ABI or bootable game system.

The owner cannot review the generated pseudocode from an artifact because
the hosted files are intentionally deleted after the run. A separate
owner-controlled local private extraction remains available through
Phase 270 for code-level examination. The scheduled run answers only
which candidates are decompilable and at what inferred lengths.

## Synthetic CI

```sh
python3 -m unittest discover -s research/tests \
  -p test_summarize_private_ghidra_decomp.py -v
bash -n research/tools/run_ghidra_original.sh
```

The Ghidra PR smoke verifies actual Java decompilation using Ghidra 12.2.
Original retail decompilation will execute **only after Phase 271 and
Phase 272 are integrated into the repository default branch**, because
the owner-original job is explicitly main-only.

The next milestone is to use privately reviewed C pseudocode together
with original interface-registration and vtable evidence to implement
and test independent Windows x86 client/server ABI compatibility. This
phase does not establish retail Windows boot, Android or Quest 3 gameplay.
