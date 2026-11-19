# SDK 2004 VFileSystem012 Win32 read-only engine experiment

Stacked on PR #449. The historical Source SDK 2004 reference
Source-SDK-Archives/source-sdk-2004, src_mod/public/filesystem.h,
blob SHA b01864b6b8aae8b300079c663fa4bb192d67debe declares
VFileSystem012 with IBaseFileSystem primary methods:
Read=0, Open=2, Close=3, Size(handle)=6, FileExists=10.
SiN's original client Init requests VFileSystem012 (21 original factory queries).
These are SDK-derived slots, **not yet owner-retail-verified** for SiN.

Two opt-in stages after the previously verified real sound-system Init:

1. Presence only: launcher switch -EnableClientEngineFsProbe sets
   SIN_INDEPENDENT_CLIENT_ENGINE_FS_PROBE=1; calls primary FileExists
   on the two paths scripts/stats/stats_definitions.txt and
   scripts/stats/stats_display_layout.txt with pathID=nullptr.
   Engine search mounts (including packages) may resolve these paths.
   Exit 0x53494E68=both found, 0x53494E69=missing, 0x53494E6A=invalid ABI.

2. Bounded reads: -EnableClientEngineFsReadProbe also sets
   SIN_INDEPENDENT_CLIENT_ENGINE_FS_READ_PROBE=1, requiring presence flag.
   Calls Open(rb), Size(handle), Read chunks, Close on all paths.
   256 KiB limit per file, read-only, counts validated with the independent
   scripts parser. Exit 0x53494E6B=original counts, 0x53494E6C=refused,
   0x53494E6D=valid syntax with unexpected counts.

Read stage is independently gated. First trial should test ONLY presence;
after verifying it against owner retail, test reads separately.
No loose-file content root is required for the engine variant.
No original commercial files or binary bytes are committed.

Native fixture verifies MSVC x86 method slots with a synthetic engine
object, missing paths, partial reads, size bounds, close-on-refusal,
count mismatch and retry. It is NOT a retail engine execution.
Use only an existing disposable game copy and the reversible owner launcher;
do not stage the independent DLLs into your Steam installation.
Original C_StatsSystem helper, engine resource list and event callbacks
remain unresolved; no normal VClient011::Init success or map load.
