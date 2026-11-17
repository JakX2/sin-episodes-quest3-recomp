# Phase 271: deploy reviewed factory Ghidra metadata to main

## Why this needs a standalone deployment

The owner-original `client.dll` and `server.dll` are already analyzed successfully by
the scheduled `main` workflow, but the upstream standalone branch did not yet
contain the separately reviewed static `CreateInterface` registration manifest.
Consequently, the routine run can repeatedly infer generic function starts
without probing the **65 known interface factory entry addresses**.

This standalone PR is based on `main`. It copies the already-synthetic-tested
metadata-only research pipeline from Phase 269, without merging the lengthy
stack of gameplay/runtime draft PRs or promoting unreviewed code.

## What will run after deployment on the default branch

1. Download the owner's two original ZIP files into ephemeral GitHub Actions
   scratch via existing repository secrets.
2. Match the **complete SHA-256 and sizes** of both original PE32 DLLs before
   preparing any interface addresses.
3. Build and validate factory seed CSVs from the 55 client + 10 server
   pre-reviewed `CreateInterface` registrations.
4. Run real Ghidra 12.2 headless analysis, attempting to create candidate
   function starts only at explicitly reviewed factory addresses.
   Existing overlapping functions are not cleared or rewritten.
5. Compare inferred Ghidra function entries against the 403 reviewed-method
   metadata ledger; do not increment independent original-method verification.
6. Fail closed on missing/duplicate/inconsistent factory seed outcomes.
7. Publish **metadata-only** CSV and JSON output: per-factory status, ranked
   engine ABI review queue, code pointer and call-reference candidate reports.
   Never publish original DLL bytes, game assets, decompiled C or disassembly.

The method ledger contains only addresses, labels, lengths and hashes for
previously reviewed original code. Referenced earlier evidence files may still
live on stacked research branches. This deployment does not newly validate
any original function body, virtual method slot, engine ABI or game boot.

## Synthetic PR testing

PR CI compiles a harmless synthetic non-PIE ELF and executes the real Java
factory-seeding script followed by the metadata exporter. Independent Python
tests verify reviewed seed generation, result reconciliation and priority queue.
No private Drive originals or secrets are used in PR smoke.

Scheduled original analysis remains restricted to the repository's default
branch; its existing cron is unchanged. The new pipeline only takes effect
on owner originals **after this draft is integrated into main**.

## Milestone state

- Original Ghidra first run: confirmed successful
- Previously reviewed ledger: 403 methods (256 client, 147 server)
- Factory registrations: 65 metadata anchors
- Newly independently verified method bodies by this phase: **0**
- Retail-engine load-tested independent gameplay functions: **0**
- Independent playable Windows game boot: **not demonstrated**

This PR is intentionally independent of Ghidra's opt-in private pseudocode
mode in Phase 270; no original pseudocode is exported to public CI artifacts.
