# Phase 516: post-registration physics surface count and index readback

## Goal and evidence boundary

The owner's 2026-11-21 genuine SiN Episodes (2006) retail engine test
confirmed all three physics Factory queries and the original manifest's
two surface scripts, 93 definitions / 71 base references. PR #515 adds
an opt-in original physics `ParseSurfaceData` virtual slot1 experiment,
but **two call returns alone do not prove the engine created material
records**.

The *next* narrowly scoped test is to inspect the engine's surface table
after both ParseSurfaceData calls return, using **read-only** methods
`SurfacePropCount` and `GetSurfaceIndex`. This does not implement
actual `VClient011::Init`, physics simulation or server map loading.

## Source evidence and confidence

Private, SHA-authenticated original SiN 2006 client:
`6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.

Original native helper `0x24191EC0` has 136 full-region bytes with
SHA256 `f5e1bbb19b7d8305690aa65992653d244ee0e8aaaf105e057fb760a889c218b5`;
its call to `IPhysicsSurfaceProps::ParseSurfaceData` via virtual byte
offset `+4` (slot1) is original-game-authenticated.

The public historical Source SDK 2004
`Source-SDK-Archives/source-sdk-2004/src_mod/public/vphysics_interface.h`
and Source 2006 compatibility
`Source-SDK-Base-Legacy-Project/source-sdk-2006/src/public/vphysics_interfacev30.h`
define *the same* `VPhysicsSurfaceProps001` interface as:

| Win32 MSVC virtual slot | Name | Signature |
|---|---|---|
| 0 | virtual destructor | do not invoke |
| 1 | ParseSurfaceData | `int(const char*,const char*)` (mutating, Phase 515) |
| 2 | SurfacePropCount | `int()` (read-only, SDK-guided Phase 516) |
| 3 | GetSurfaceIndex | `int(const char*)` (read-only, SDK-guided Phase 516) |

**Critical distinction:** Slots 2/3 have not yet been directly observed
running against the owner's original vphysics.dll. They are justified by
the matching legacy SDK interface and virtual-slot layout, not by an
independently SHA-authenticated native callsite within SiN's original
`client.dll`. If either call crashes, it is a failed ABI hypothesis.

## Authentic name witnesses

I privately inspected the owner's original `SE1/scripts.zip` through
the existing connected Drive, *without copying original scripts to the
public repository*. The exact two SHA matches are:

- `scripts/surfaceproperties.txt` (23485 bytes,
  `86bb98fc2874d0e3ece18ff606fc4ee668c0a795596a393009e5534f372289a8`):
  includes `default` and `solidmetal`.
- `scripts/surfaceproperties_hl2.txt` (2812 bytes,
  `125ff956d7d3cb9bf5cdacbd04a0bdde2243c0d31106adf72e71e15be5c34068`):
  includes `metalvehicle`.

These three *name tokens* are used as positive registry witnesses. No
retail resource body or original proprietary function is committed.

## New code

- `client_original_surfaceprops_readback.h` contains a strict MSVC x86
  vtable pointer verifier, virtual slot2 Count and virtual slot3 Index
  invocation, bounded count 0..16384 and signed index validation. It
  retains no engine objects or pointers after the call.
- `test_client_original_surfaceprops_readback.cpp` uses a completely
  synthetic 32-bit IPhysicsSurfaceProps object with a virtual destructor
  and the original expected slots, plus invalid-count, missing-index and
  out-of-range cases. The fake registry's values `93:0:1:84` are **only
  invented test values**, not runtime results.
- The original-hosted independent client calls these read-only methods
  **only** after the separate explicitly enabled `ParseSurfaceData`
  attempt returns from both original source scripts. Three new ordered
  events `engine-fs-readback-entered/result/terminal-stop` and a separate
  same-session classifier result are introduced.
- The owner launcher adds `-SurfacePropsReadback` (must also specify
  `-SurfacePropsRegister`), with another environment opt-in, cleanup,
  proof chain and text-only summary. Old read-only commands remain
  unchanged.

## Interpreting the next owner test

New optional command for a **CI-verified** package extracted outside
the installed game:

```bat
RUN_SOUNDSCAPE_CATALOG_TEST.bat -SoundscapeCvarLookup -PhysicsFactoryLookup -SurfacePropsRead -SurfacePropsRegister -SurfacePropsReadback
```

Only `original-physics-surface-readback-three-in-range` with a positive
count and all three indices in `[0,count)` proves the live engine
answered the SDK-guided calls with coherent values. Importantly this
does **not** prove the independent client caused each entry to appear:
the engine may have initialized some of its own surface data before
this diagnostic. Nor does a count of 93 necessarily follow, as earlier
registrations and overrides can change the total.

**If Phase 515 native parse fails, Phase 516 is skipped**. A blank
readback result is not evidence that any SDK vtable method ran. No full
game boot is claimed.
