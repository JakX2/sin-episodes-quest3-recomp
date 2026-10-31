# Recovered research notes: Phases 23–43

These notes backfill **findings** that were missing from the repository's research index. They are not copies of the original phase ZIP bundles, which are not available in the current execution session. Treat these as a reconstructed research inventory grounded in prior project notes, with selected important signatures **rechecked** against the original 2006 SiN DLLs during this upload.

## Recovered areas

- [Phases 23–34: vehicle, phone, poison and antigen systems](phase23-34-gameplay.md)
- [Phases 35–36: assault rifle, scattergun, flak and grenade projectiles](phase35-36-weapons.md)
- [Phases 37–43: Magnum DUAG, offhand grenades and ADS](phase37-43-magnum-ads.md)
- [Fresh baseline verifier](../verifiers/phase23-43-backfill-verify.py): newly authored, self-contained Python script that accepts a **user-owned** original 2006 `bin.zip`. It checks original DLL hashes, 23 fixed instruction signatures and three ZoomFOV virtual dispatch slots. It does not require redistributing game binaries.

## Provenance, limitations and archive status

These pages preserve only **recoverable findings**, not original research packages. Original phase folders, mock Source-SDK builds, 2006 x86 disassembly excerpts, and many historical test suites are **not present as source files in the current session**, and have not been silently invented or relabeled as archival originals. Details not supported by the retained notes are explicitly described as unresolved. The additional verifier was independently executed against the project-provided original DLLs in the current session, but that does not replay every test made during the historical research.

The original 2006 SiN binaries have SHA-256:
- server: `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`
- client: `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`

All virtual addresses in these notes refer to the **2006 x86 Windows build**, not SourceVR or ARM64. The project has **no playable SourceVR or Quest 3 port** at this stage. Do not commit any original DLLs, game assets or full disassembly dumps.

**Intentionally not reconstructed:** Phase 30 investigated incidental multiplayer artifacts; the owner explicitly requested the **released single-player campaign only**.

Further missing historical packages, especially Phases 2–21 and original 23–43 archives, should be recovered only from genuine retained sources, and tagged with their own provenance if they become accessible.
