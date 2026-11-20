# Owner-only original-mode staged Stats engine probe

## Why this test exists

The November 20, 2026 original Windows Source engine successfully ran the
independent replacement client.dll through **VFileSystem012** FileExists,
Open, Size(handle), Read and Close and independently parsed 62 definitions,
8 percentage references, 156 display pairs and 66 divider lines.
Terminal checkpoint: **0x53494E6B**. That combined probe used **rb/null**
for both files; it did NOT test the original 2006 client-specific
Open arguments, construct original KeyValues or bind 66 native stat IDs.

The original SHA-pinned client C_StatsSystem::Init separately loads:

- Stage 3: **scripts/stats/stats_definitions.txt**, Open("rb","GAME")
  from the catalog helper 0x241FC5A0.
- Stage 4: **scripts/stats/stats_display_layout.txt**, Open("rt",nullptr)
  from the layout helper.

The new opt-in retail trial calls those SDK2004 VFileSystem012 virtual
methods and passes the read text to separately owned, independent Stats
owner/catalog/asset types. The parser checks 62/8/156/66 and binds original
66 ordered registry identifiers, including preserving IDs 51..54 as
compiled-only entries. All original callbacks after reading layout are
**stubbed without side effects** in the independent probe. No engine
KeyValues objects, original client objects or unknown original callbacks
are passed to the host.

## Download, extract, double click

GitHub Actions for this PR publishes
**SiN-Episodes-Standalone-OneClick-Filesystem-Test-Win32** with its own
new **RUN_STAGED_STATS_TEST.bat** next to the prior RUN_SIN_TEST.bat.

1. Extract the ZIP anywhere **outside** your SiN installation.
2. Close the game.
3. Double-click **RUN_STAGED_STATS_TEST.bat**.
4. Choose the original game folder containing SinEpisodes.exe.
5. The launcher verifies the two retail DLL hashes, creates a temporary
   full game copy, checks both paths, runs the already verified combined
   binary-mode read, then invokes the new staged-resource diagnostic.
6. Send the produced **SiN_FileSystem_Test_Results_*.zip**. It contains
   text logs, NOT original DLLs or commercial content.

The pre-existing RUN_SIN_TEST.bat still runs only presence/read. Use
RECOVER_PREVIOUS_TEST.bat to recover old SHA-pinned TEMP copies. A
failed or locked original backup is retained rather than deleted.

## Expected diagnostics

Prerequisites in fresh, separately session-bound launches:

- 0x53494E68: VFileSystem012 FileExists of both files.
- 0x53494E6B: combined Open/Read and content counts 62:8:156:66.

New third-stage exit values:
- **0x53494E70**: staged original-mode read plus independent owned
  66-slot catalog and 62/8/156/66 parsed values verified;
- **0x53494E71**: stage-three definitions read/parse/bind refused;
- **0x53494E72**: stage-four layout read/parse refused;
- **0x53494E73**: engine ABI or independent validation refused.

Success trace is
`engine-fs-staged-entered:rb-GAME:rt-null:owned-only`,
`engine-fs-staged-counts:66:62:8:156:66`,
`engine-fs-staged-stage:7:7`,
and `engine-fs-staged-result:owned-staged-verified-not-engine-init`.
The owner gate additionally demands same-session trace, correct terminal
exit and authentic cleanup of the disposable-copy DLLs.

This is another intentional terminal diagnostic, NOT a normal
VClient011::Init return, original KeyValues Init, UI callback
registration, map load, server startup, playable PC build or VR port.

### Important potential text-mode issue

Open("rt") may alter line endings or Size(handle) semantics on some
original filesystem providers. The authenticated original layout file
is **8666 bytes with 262 CRLF pairs** (SHA256
850dd06942b459843236dd8c6770b40a5b28e6b6a0fcab7bf4e60dec55322dbe).
A CRT-backed text Read could return 8404 LF-normalized bytes even
when the physical Size(handle) remains 8666.

The independent reader keeps its strict 256 KiB limit and
close-on-refusal checks. It accepts either (a) a complete exact-size
read or (b) in **rt mode only**, a bounded EOF after positive progress
with no remaining CR octets and a physical-to-returned byte deficit
equal to the number of LF octets. The full original Stats parser
still requires all 156 layout pairs and 66 dividers. Any other
short read, including binary-mode early EOF, remains refused.
Native synthetic tests cover successful CRLF translation, an
additional unexplained byte of shortfall, and a successful retry.

If the layout stage still fails, the private trace should be
examined for deviations in the actual original filesystem provider.
This compatibility guard does NOT imply original KeyValues or game
initialization was successfully run.

## Evidence status

The staged original-mode trial is not confirmed on real retail Windows
until an owner runs this new package and provides the report ZIP.
Native Windows CI uses synthetic dual-vtable MSVC objects and original-
free fixture documents. Neither source assets nor retail DLLs are
published.
