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

Optional read-only VGUI vtable anchor check: add -EnableClientVGuiLayoutProbe
together with -EnableClientVGuiProbe and -EnableClientOwnedStartupProbe.
This checks vtable memory ownership and the original-used slots 0/56
(InputInternal) and 0/24 (System) without invoking any virtual method.
Success 0x53494E5B, refusal 0x53494E5C. This does not prove complete GUI
initialization or a successful VClient011::Init return.

Next original engine-panel query: -EnableClientEngineVGuiSlot1Probe together
with all three preceding VGUI/owned startup switches. This invokes the
SHA-authenticated original VEngineVGui001 virtual slot 1 with integer
argument 2 only after the read-only VGUI anchor checks. Nonzero return
is 0x53494E5D, refused / zero is 0x53494E5E. Neither is proof of
GUI initialization or complete game startup.

Original 2006 VGui_InitInterfacesList stage (eight app factory interfaces and
IVGui.Init slot1) is separately gated by -EnableClientVGuiControlsBootstrapProbe
along with -EnableClientEngineVGuiSlot1Probe and earlier client switches.
Success plus a nonzero engine GetPanel(PANEL_CLIENTDLL) returns 0x53494E5F;
missing interface, unsafe method or zero panel returns 0x53494E60.
Both remain terminal diagnostics, not normal Client Init or gameplay.

Next original post-VGUI boundary: enable -EnableClientMatSurfaceProbe together
with -EnableClientVGuiControlsBootstrapProbe and all earlier client switches.
The original VGUI_Surface028 has a secondary IAppSystem base at +4 x86 bytes.
Its QueryInterface virtual slot2 is invoked with "MatSystemSurface003".
Exit 0x53494E61 means non-null; 0x53494E62 means refused. These are
reversible terminal diagnostics, not successful VClient011::Init or gameplay.

Independent original engine sound-system Init probe: add
-EnableClientSoundSystemInitProbe to -EnableClientMatSurfaceProbe and
all earlier client probe flags. This invokes original engine
VSoundEmitter001 slot 1, equivalent to the 2006 client
CSoundEmitterSystem::Init 11-byte tail call. Success 0x53494E63,
false or memory validation refusal 0x53494E64. Diagnostic-only stop;
not a normal VClient011::Init or full game boot.
