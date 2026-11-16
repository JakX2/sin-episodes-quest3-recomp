# Phase 253: Hosted automated Ghidra 12.2 analysis via GitHub Actions

16 November 2026. Stacked draft above Phase252 PR #308. The user explicitly chose GitHub-hosted automation because they do not have a dedicated server and do not want to leave their own Windows PC running.

## New workflow

- .github/workflows/sin-ghidra-headless.yml adds actual Ghidra headless smoke testing on synthetic executable files in pull requests, and private-original PE32 analyses on scheduled or manually dispatched DEFAULT-BRANCH executions.
- Ghidra is pinned to official version **12.2**, downloaded from NationalSecurityAgency/ghidra releases, with official full ZIP SHA256 **2c2cd776db3c1f6a097ba418922378ac3649cadd5e844a6f5f088ae628a7be67**. JDK 25 is required.
- Hosted schedule: **02:23 and 14:23 UTC daily**, subject to GitHub Actions usage, availability and retention limits. The existing separate ChatGPT hourly automation remains enabled.
- Only the original public Ghidra ZIP is cached. **Original SiN DLL ZIPs, original DLLs and private Ghidra decompiled C are never cached, committed or uploaded.**
- Pull-request smoke jobs have NO access to original Drive secrets and import only harmless synthetic ELF code.
- The real-binary job reads two Drive file IDs from repository Actions secrets, downloads the original Drive archives to ephemeral runner scratch, verifies exact whole-file original DLL SHA256s, invokes Ghidra, compares inferred entries against the 403 original-SHA-reviewed functions, and publishes ONLY metadata CSV and comparison JSON as a 7-day Actions artifact. The private binaries and ZIPs are removed from runner scratch.
- The original Ghidra postScript is invoked in metadata-only mode; optional bulk decompiler pseudocode output is never used in CI.

## Mandatory original hashes

| Retail PE32 module | Size | Full original SHA256 |
| --- | ---: | --- |
| client.dll | 3,592,192 | 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d |
| server.dll | 6,049,792 | 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650 |

## One-time setup the repository owner must perform

The available GitHub connector does **not** have an Actions-secrets writer. Navigate to the repository Settings > Secrets and variables > Actions > New repository secret and create exactly:

1. **SIN_ORIGINAL_DRIVE_BIN_ZIP_ID**: Google Drive file ID for the existing bin.zip file in SiN Ep Recomp > sin files.
2. **SIN_ORIGINAL_DRIVE_BIN_1_ZIP_ID**: Google Drive file ID for the existing bin 1.zip file next to it.

Enter just the IDs, not file contents, game DLLs, or bearer tokens. The original two Drive files currently have an existing **anyone** read permission. GitHub secrets avoid advertising their IDs in public workflow source but do NOT revoke existing Google Drive link sharing. If the owner later restricts Drive access, a proper authenticated download mechanism is needed; no access restrictions should be bypassed.

The workflow must also be **merged to the default branch**. GitHub scheduled workflows and workflow_dispatch are not active merely because this file exists on a stacked draft pull request. The project rule is to leave this PR DRAFT and never merge it automatically.

After those two steps, run the workflow once manually under Actions, then allow it to run at its daily scheduled times.

## Added support code

- research/tools/stage_original_pe_from_drive_zips.py: safe independent ZIP member handling, full-file SHA and sizes, refuse duplicate/wrong archives, stage only exactly matching original DLLs with owner-only file permissions.
- research/tests/test_stage_original_pe_from_drive_zips.py: seven synthetic-only ZIP regression tests, including mismatch, malformed Drive HTML, duplicates, unsafe ZIP paths and no overwrite.
- research/tools/install_pinned_ghidra.sh: official build SHA256 pin and headless analyzer installation.
- research/tools/compare_ghidra_original_atlas.py: Ghidra metadata vs separately verified method catalog, WITHOUT treating inferred functions as SHA-reviewed.
- research/tests/test_compare_ghidra_original_atlas.py: synthetic metadata tests for overlap, missing functions, duplicates and malformed addresses.
- .github/workflows/sin-ghidra-headless.yml: synthetic smoke on PR, original-only batch on schedule/manual from default branch.

## Limits

This phase does not itself add new original whole-native-code SHA-reviewed functions. The trusted catalog remains 403 (client 256, server 147). A successful synthetic Ghidra smoke is NOT proof that the owner-original DLLs were already analyzed on GitHub. The real original Ghidra hosted job requires the two GitHub secrets and merging the workflow file to default. A public-repository metadata artifact may be readable by visitors; do not include original binaries, complete native instruction listings, or C-like decompilation text in it.

Priorities remain an independent non-forwarding Windows x86 gameplay DLL boot first, Android ARM64 second, Quest 3 VR third.
