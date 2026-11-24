# Phase 667: native engine skill bridge

Base: Phase666, 705570e6dbfede56f614ff7e3b4695199fdf71db. This adds an explicit port-specific Win32
entry point to the actual independent server.dll, not an original export.

- Use VEngineCvar002 const FindVar slot 3, with the existing checked
  x86 virtual-method ownership helper. The caller must supply a live,
  correct interface on the game thread; memory checks are not type proof.
- Read the original-guided ConVar parent at +0x18 and its cached integer
  at +0x2c (Phase335 native console ABI), require a self-parent root named
  skill, and reject anything outside 1..3. No registration or write into
  the engine. No pointer retained. No fallback to a made-up difficulty.
- BeginFromEngine starts the existing DLL-owned campaign; SyncFromEngine
  updates that same state. Failed lookup, invalid values and invalid
  lifecycle calls leave state untouched. Reentrant mutation during
  FindVar is refused. Phase666 ABI version and table layout unchanged.
- Loaded-module Win32 test supplies a synthetic native VEngineCvar002
  with an alias whose integer differs from the root; verifies root
  selection, changes, idempotence, rejection, damage and restart.
  Existing portable Phase666 test still runs on Linux.

Original DLLInit and GameInit remain fail-closed. No retail engine
invocation or automatic game-frame/level callback wiring is claimed.
This is the explicit integration boundary for future authenticated
engine-owned lifecycle callbacks, not a playable replacement.
