# Original 2006 C_DynamicDifficulty::Init, 11 binding registrations

Complete original client.dll function: 0x241FAC90..0x241FAD79
(234 bytes). Source SHA256:
`e8d18d75c634b4e3a479db3d9e63b0af21ad666a95730183fbfd7c304b01b10e`.

This function is the virtual Init slot zero of original
`C_DynamicDifficulty`, the **ninth** direct-registered game system.
It reads original registry global `0x243010A8` and makes exactly
eleven ordered calls to original `0x241CFF70`, passing a named
registration key and a function callback address. It does not check
registration return values, then returns true.

The original key order is GamePlayVarValue, Progression, DDInfo,
DDAdvisorChannels, DDAdvisorScores, DDDecisionMade,
DDDecisionsPerf, DDSuccessModifiers, ArenaMode, NewGameInfo, Cheating.

`client_dynamic_difficulty_init.h` models this exact sequence,
with original callback addresses preserved as **opaque metadata**.
They must not be invoked as functions from the rebuilt DLL.
A host registry callback records/installs each association.
The original registrar `0x241CFF70` includes native symbol tables
and dispatch storage not yet rebuilt, so the game-side callbacks and
full original registry need further work.

The native test covers order and exact callsite provenance,
repeated calls (the native function has no once flag), fail-closed
missing registry, and integration with the real AutoGameSystem
first-false Init gate.

This adds one more complete independent Init method reconstruction.
Total: **8 of 12**, four remaining complex methods:
CClientShadowMgr, C_StatsSystem, C_SoundscapeSystem,
CPerfVisualBenchmark.

No claim is made that this can be wired into the original retail
process or that VClient011::Init now returns successfully.
