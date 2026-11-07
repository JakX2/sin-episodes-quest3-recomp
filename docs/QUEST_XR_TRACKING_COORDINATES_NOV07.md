# Quest 3 tracking-space to Source room-coordinate compatibility boundary

**7 November 2026. Native math-only integration target, not a VR runtime.**

## Newly inspected external reference in owner's private Drive

The owner-provided source ZIP, Sin-Episodes-VR-Port-main.zip,
was inspected read-only. Exact archive SHA-256:

    055b40dff0cb145664c9fd1c774306d42f3d735dc47157e4cf2c549ed4a9831a

The archive includes 96 ZIP entries, with a Windows SteamVR/OpenVR
SiN Episodes VR mod whose own README states that it uses injected
32-bit Windows code, a modified DXVK Direct3D 9-to-Vulkan renderer,
and SteamVR. It targets SteamVR-compatible headsets on Windows;
it is **not** a standalone Android/Meta Quest 3 game port.

Authorship/license: RototRobot, 2026, Apache License 2.0
(see the ZIP's LICENSE and NOTICE). The Apache license applies
to that project's own sources. Its content/ overrides and other
third-party components have separate terms. None of the original
Windows mod source files, game content, DXVK implementation or
third-party headers are copied to this MIT compatibility repository.

Relevant source-provenance waypoints within the private archive:

- src/sinvr/vr/openvr_backend.cpp lines 28-68: tracking +Y up/-Z
  forward, Source +X forward/+Y left/+Z up, HMD pose angles,
  position and linear velocity conversion.
- src/sinvr/vr/vr_backend.h: tracking-space HMD/controller pose
  interfaces, native runtime velocity, physical hands vs handedness.
- src/sinvr/vr_camera.h: explicit separation of body yaw,
  recenter yaw, head yaw and controller aiming.
- src/sinvr/render/stereo.cpp: Windows D3D9/DXVK per-eye
  projection and compositor draw path, unsuitable for direct
  reuse in a standalone Android native module.
- actions/sinvr_actions.json: Windows VR action/binding layer;
  not evidence of compatible Quest Android input bindings.

This inspection establishes reference **design/coordinate behavior
only**, not that the original SiN 2006 engine, Quest SourceVR or
OpenXR uses the exact same projection/camera ABI.

## New independent implementation

include/sin_compat/xr_source_tracking.hpp provides a small
C++17 helper that accepts a **row-major 3x4** rigid tracking pose
in the standard OpenVR/OpenXR convention and a caller-supplied
source_units_per_metre scalar. It returns portable Source
room-space pose and Source pitch/yaw/roll in degrees:

    Source position = (-tracking_z, -tracking_x, +tracking_y) * scale
    Source velocity = (-tracking_vz, -tracking_vx, +tracking_vy) * scale
    Source pitch = asin(clamp(matrix[1][2], -1, 1))
    Source yaw   = atan2(matrix[0][2], matrix[2][2])
    Source roll  = atan2(-matrix[1][0], matrix[1][1])

Angles are converted to degrees. Rotation is validated as finite,
approximately orthonormal and with determinant near +1 before
any result is returned; reflected, sheared, zero, NAN or
infinite matrices are rejected. Scale must be finite and positive,
and numerical overflow in position/velocity is rejected.

Source-unit conversion is **not** hard-coded: the eventual game host
must establish actual map/physics scale and room-space calibration.
The helper intentionally does not mutate entity angles, move the
player, process actions or submit stereo render targets.

The math is independently authored; the proprietary/private
archive's original code was not imported or relicensed.
We acknowledge the RototRobot project's coordinate reference.

## Input contract for future Quest OpenXR host

A future verified OpenXR host must:

1. Obtain valid runtime-local XrPosef position/quaternion,
   convert to the declared row-major 3x4 matrix, and supply
   the tested source-units-per-metre scale.
2. Use the same axis mapping for headset, controller position
   and controller linear velocity, without inferring velocity
   from noisy frame differences.
3. Compose *room-local* head pose with separate player
   movement/body yaw and recenter transform. A correct
   Source-relative room pose alone is **not** a full VR camera.
4. Handle recenter, tracking loss, guardian/playspace changes,
   input bindings, left/right handedness and motion-controller
   interactions via a real Android OpenXR/SourceVR integration.
5. Build stereo views using the actual Quest OpenXR projection
   and image swapchains. The Windows DXVK D3D9 path is not
   transplantable onto a Quest app by copying code.

The reference archive uses OpenVR HmdMatrix34_t and Windows
openvr_api.dll. OpenXR uses a position/quaternion pose instead,
so the genuine Android host still needs a verified conversion
between that quaternion and this matrix representation.

## Tests

tests/test_xr_source_tracking.cpp covers identity poses,
meter-to-Source scaling, all three coordinate axes,
linear velocities, yaw/pitch/roll rotations, realistic small
floating-point roundoff, nonfinite translation and vectors,
invalid scales, shear, reflections and degenerate rotations.
All assertions remain enabled in Release builds.

    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --target sin_xr_source_tracking_tests
    ctest --test-dir build -R '^sin_xr_source_tracking_tests$' --output-on-failure

GitHub CI also compiles an Android NDK ARM64 regression executable,
checks its ELF machine, and executes the same math assertions in
an Android Bionic x86_64 emulator. This is native Android math
testing, **not** a headset test or SourceVR factory load.

## Still needed for playable SiN on standalone Quest 3

- Verified Quest-capable SourceVR game module/loader ABI and legal source.
- Original SiN game-server/client entity factories, engine callbacks,
  world simulation, input and weapon/NPC reproduction.
- Actual OpenXR Android pose/VR input integration, stereo rendering,
  locomotion, controller models and physics calibration.
- Real one-level boot and end-to-end checks with user-owned game assets.

Neither this math helper nor the Windows VR mod archive demonstrates
a working original SiN executable on Meta Quest 3. No game binaries
or private ZIP contents have been committed.
