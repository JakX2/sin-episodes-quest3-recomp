# Phase 515: original-engine physics surface data call boundary

## Newly observed owner retail evidence (2026-11-21)

The owner-provided text-only `SiN_FileSystem_Test_Results_20261121-013258-e0483dc99d.zip` confirms the **original 2006 Source engine** completed both Phase 513/514 *read-only* checks. The independently authored client requested all three factory versions through its actual second (physics) Init argument:

- `VPhysics030`
- `VPhysicsSurfaceProps001`
- `VPhysicsCollision007`

Owner trace: `original-physics-three-available:3:3:1`. All three pointers were present at lookup time, but no methods were invoked.

The original engine's `VFileSystem012` then read the authentic `scripts/surfaceproperties_manifest.txt` in `rb/GAME`, `scripts/surfaceproperties.txt` and `scripts/surfaceproperties_hl2.txt` in `rb/nullptr`. Owner trace: `original-surfaceprops-retail-resource-shape-verified:2:2:93:71:0`. A total of 93 surface definitions and 71 direct `base` keys were found with zero duplicate names. The file copies were restored and disposable copy was removed. Exit `0x53494E76` is the existing diagnostic terminal stop, **not a production game boot**. No personal log paths or retail file contents are included here.

## Original source ABI evidence

SHA-authenticated privately held original `client.dll` SHA256: `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`. Native `0x24191EC0` (136 bytes, digest `f5e1bbb19b7d8305690aa65992653d244ee0e8aaaf105e057fb760a889c218b5`) invokes `VPhysicsSurfaceProps001` virtual vtable byte offset `+4` (slot **1** in x86) with `(filename, NUL-terminated raw file text)`. The original closes the file handle **before** the physics method call, ignores the method's return, and does not test the VFileSystem::Read byte count. The new bridge deliberately rejects short/invalid files rather than inheriting those unsafe assumptions.

The shipped retail sizes privately authenticated in Phase 514 are **23485** and **2812** bytes. The Phase 515 bridge checks both exact lengths and refuses embedded NUL bytes before dispatch. It processes them in manifest order and holds the buffers only during the bounded terminal diagnostic. It does **not** claim to reconstruct Source KeyValues or original physics data structures.

## Independently written new code

- `client_original_surfaceprops_registration.h`: bounded real-call-capable Win32 MSVC x86 bridge, source-correct vtable slot1 and argument order, transient engine interface ownership, two sequential calls.
- `test_client_original_surfaceprops_registration.cpp`: invented materials/filesystem/physics fixture. Checks exact ABI, call order, file closure, returns, refused malformed data, missing inputs and short reads. Does **not** invoke the retail engine in public CI.
- `slot0.cpp`: new `engine-fs-registration-entered/result/terminal-stop` trace after the established soundscape+ConVar+physics+surface-resource chain. Normal `VClient011::Init` continues to return failure; no normal game initialization is enabled.
- Launchers, classifier and tests: distinct `SurfacePropsRegister` / `ExplicitSurfacePropsRegisterProbe` / `EnableClientSurfacePropsRegisterProbe` flags, session-scoped attestation and environment cleanup. No earlier read-only command can accidentally enable this step.

**Important evidence distinction:** `original-surfaceprops-register-two-calls-returned:2:2:<first>:<second>` means two virtual calls *returned*. It does **not** prove that physics parsed or registered 93 surfaces, that game systems finished Init, or that a map can load. The original ignored this method's return. Fail-closed checks block missing source inputs before **either** parse call.

## Optional new owner-side experiment, after CI succeeds

A freshly packaged and CI-tested **Phase 515** diagnostic may be run from a folder outside the original game installation:

```bat
RUN_SOUNDSCAPE_CATALOG_TEST.bat -SoundscapeCvarLookup -PhysicsFactoryLookup -SurfacePropsRead -SurfacePropsRegister
```

Unlike the prior test, **this invokes a real engine-mutating physics surface registration virtual method** in a *disposable entire game copy*. A crash or non-diagnostic exit is possible until the live engine confirms ABI compatibility. The SHA-pinned reversible launcher and recovery checks remain mandatory. Do not run on an installed game or manually swap the original DLLs.

The result should be uploaded as the new small text-only `SiN_FileSystem_Test_Results_*.zip`, without original game DLLs. If a protected/unsafe method or an ABI mismatch prevents a complete return sequence, inspect the logs and revise the bridge, **not** silently advance to `VClient011::Init` success. The original 2006 `ServerGameDLL003` initialization and any playable Windows/Android/VR boot remain incomplete.
