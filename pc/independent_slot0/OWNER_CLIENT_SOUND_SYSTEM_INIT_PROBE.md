# Original Windows sound game-system Init, gated after surface query

The owner-original 2006 client.dll SHA 6535f13e... demonstrates
CSoundEmitterSystem::Init at 0x241BC950 is 11 bytes and tail jumps into
slot 1 of sound-emitter engine interface held at global 0x2433F6E4.
Its RTTI, original object and exact slot method VA are verified by
research/tools/audit_sin_client_static_gamesystem_rtti.py.

The independent client captures its already-factory-acquired
VSoundEmitter001 pointer and (ONLY with the additional environment flag
SIN_INDEPENDENT_CLIENT_SOUND_SYSTEM_INIT_PROBE=1, plus all earlier
terminal gates) invokes the authenticated slot 1 via module and
VirtualQuery checks. Success returns diagnostic 0x53494E63; a false
return or refusal yields 0x53494E64. This runs after successful
MatSystemSurface003 query (owner-confirmed earlier checkpoint
0x53494E61) and therefore does not change previous default behavior.

This is a *single genuine game-system engine ABI operation*, not an
original gamesystem-vector pass, normal VClient011::Init return,
completed sound playback, level load or gameplay. No original code or
commercial binaries committed.

## Actual original-engine owner proof: 19 November 2026

The owner tested downloaded original-free Windows PE32 diagnostic bundle
`OWNER_DIAGNOSTIC_BUNDLE_COMMIT f1b86ee9e866276b2beeaddca27d800d1de9d801`
in disposable `D:\\SiN-DisposableTest`, with
`-EnableClientOwnedStartupProbe -EnableClientVGuiProbe
-EnableClientVGuiLayoutProbe -EnableClientEngineVGuiSlot1Probe
-EnableClientVGuiControlsBootstrapProbe -EnableClientMatSurfaceProbe
-EnableClientSoundSystemInitProbe -DisposableTestCopyConfirmed`.

Two uploaded files, `Pasted text(7).txt` (console transcript) and
`sin-sound-system-init.txt` (UTF-16 PowerShell Tee-Object log), show
the **same session** `ec8e0ac313f04c7590eb6734a4795439`,
process 4428 and exact terminal sequence:

- The retail engine accepted independent `VClient011` and entered Init.
- 21 engine interface requests were found, sound Connect succeeded;
  material, screen, random Gaussian, 3/3 console vars, independently
  owned objects/particle pool and VGUI preflights were verified.
- All eight original controls interface versions found, `IVGui::Init`
  returned true, `GetPanel(PANEL_CLIENTDLL)` returned nonzero.
- `VGUI_Surface028::IAppSystem::QueryInterface("MatSystemSurface003")`
  returned nonnull.
- `sound-system-init-entered CSoundEmitterSystem:VSoundEmitter001+1`
  preceded `sound-system-init-result called-true`, followed by
  `sound-system-init-terminal-stop original-slot1`.
- Real process exit decimal **1397313123** =
  **`0x53494E63`**, diagnostic
  `INDEPENDENT_CLIENT_SOUND_SYSTEM_INIT_TRUE_DIAGNOSTIC_STOP`.
- Ordered trace classifier:
  `CLIENT_SOUND_SYSTEM_INIT_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF`.
- Original `client.dll` reported `RESTORED_VERIFIED`, original
  `server.dll` `RETAINED_ORIGINAL_UNCHANGED`;
  `OWNER_INDEPENDENT_RESTORATION: VERIFIED_COMPLETE`.
- Module polling did not happen to observe `client.dll` during 39
  samples; this does not negate the factory, execution trace, and
  intentional exit. It is not a complete module residency audit.

This is owner-original **Windows retail-engine execution** of the
independent client invoking the real `VSoundEmitter001` virtual slot 1
and receiving true. The process stopped deliberately at the new
boundary. No successful normal `VClient011::Init` return, complete
twelve-system AutoGameSystem pass, level load, rendering or gameplay
has been demonstrated; no original game bytes are committed.
