# Phase 348 client diagnostic bundle (not playable)

This artifact contains the independently built Win32 DLLs and matching
owner launcher/verdict scripts from a single CI checkout. The manifest
detects accidental file mixing or corruption; it is not a signature.

Extract the entire artifact away from your game directory. Use only an
existing disposable copy of SiN Episodes, outside steamapps/common, with
both original game DLLs intact and the prior explicit disposable marker.
Do not copy the DLLs over your live Steam installation.

In PowerShell from the extracted directory:

    .\Run-ClientCvarDiagnostic.ps1 -GameRoot "C:\SiN-Test" -DisposableTestCopyConfirmed 2>&1 | Tee-Object -FilePath sin-phase348-client-probe.txt

Replace the path with the existing disposable game copy. The wrapper
verifies the manifest then delegates unchanged to the original launcher,
which checks the marker, both original hashes and PE32 architecture,
stages only the client, launches the game, and attempts original restoration.

Expected success exit: 0x53494E55. The session-specific verdict must also
confirm the three variables and final consistency, and restoration must
report VERIFIED_COMPLETE. Exit 0x53494E56 is a refusal, not a success.
Include the full text report when reporting results. If restoration is
incomplete, follow the existing launcher's manual recovery messages.

The previously verified Phase 348 command remains the default. The
optional -EnableClientOwnedStartupProbe switch extends the diagnostic
with independently owned deferred-object and particle-pool lifecycle
checks. Its expected exit is 0x53494E57, or 0x53494E58 on refusal. This
is not original registry or allocator parity, VGUI initialization,
or a successful Init return. No original game data is included.

Optional VGUI factory-only diagnostic: add both -EnableClientOwnedStartupProbe
and -EnableClientVGuiProbe. The original engine's already-loaded vgui2.dll
factory is queried for the two exact legacy interface names. Success is
0x53494E59; refusal 0x53494E5A. No GUI methods or registration helpers run.
