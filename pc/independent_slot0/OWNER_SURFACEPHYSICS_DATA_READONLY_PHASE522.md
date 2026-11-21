# Phase 522: read original, preloaded physics material fields, without ParseSurfaceData

## Why

The real Windows owner result from 2026-11-21 02:47:49, ZIP SHA256
`56c9163dea74bb72ec17534fc66e24d56333fe0db4620f2fd060115dc33ffe37`,
shows `SurfacePropCount=94` and `GetSurfaceIndex` witnesses 0:1:85
**before** independent ParseSurfaceData calls and unchanged afterward.
Therefore, another parser mutation is not necessary just to access
the original engine's surface data.

## Exact original ABI evidence

Original SHA-pinned `client.dll` at 0x24184F62 invokes the
`VPhysicsSurfaceProps001` method at vtable byte offset `+0x14`.
Original server.dll independently calls vtable `+0x14` at
0x2220D9CF. Public Source SDK 2004 and 2006 `vphysics_interface.h`
name the method `GetSurfaceData(index)`. The SDK describes
`surfacedata_t::physics` as the FIRST 20 bytes, comprised of
`float friction, elasticity, density, thickness, dampening`.
These are not original game-source bytes; the public header
is used only to identify a binary ABI.

## New Win32 independent code

`client_original_surfacephysics_data_probe.h` first verifies
`SurfacePropCount` and `GetSurfaceIndex` through already
owner-observed slots2/3, then calls read-only vslot5 for the
owner-validated material names `default`, `solidmetal`,
`metalvehicle`. The returned engine-owned memory is checked for
readability, copied as five floats, and bounded to finite
numeric values. No native parser, material registration,
CVar write, game object, map or VClient011 Init-success path is invoked.

A *separate*, nonmutating `-SurfacePhysicsDataRead` switch in
the SHA-gated one-click launcher enables this stage only after
all preceding engine/filesystem/soundscape/physics 3/3 checks.
The new BAT `RUN_PHYSICS_MATERIALS_READONLY.bat` supplies those
prerequisites automatically, does NOT request
`-SurfacePropsRegister`, and reports a text-only archive with
the original 3 material physics tuples. Treat any real engine
success as a new ABI milestone, NOT normal startup or a VR port.

The original server's equivalent resource reader is a separate
read-only probe. No original server DLLInit owner result is yet available.

## Native original-free tests

The synthetic C++ fixture supplies an x86 six-slot vtable and
three invented 20-byte physics sample buffers; it checks no parser
calls, correct three material indices, bounded struct copy,
missing index, nullptr and NaN refusal. It doesn't use any
retail game DLL or material text.

Phase522 user-side data is NOT YET OBSERVED until the owner
runs the new package and sends its small ZIP.
