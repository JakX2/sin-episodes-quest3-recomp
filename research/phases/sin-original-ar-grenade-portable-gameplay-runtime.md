# Original SiN 2006 grenade_ar: portable gameplay implementation

**8 November 2026 | Reconstruction from the owner's 2006 server.dll evidence**

This is the first executable, independently authored **CGrenadeAR projectile
flight + detonation behavior** in the current portable compatibility tree.
It is deliberately separate from the existing inert SourceVR native
game-factory probes. It does not link a Windows DLL or call an ARM64 engine
interface whose method signature is unproven.

## Original primary-binary basis

Owner's original SiN Episodes: Emergence 2006 server.dll SHA256:
568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650

The already committed SHA-pinned original-binary audits prove:

- Exact registered class **grenade_ar**, RTTI **CGrenadeAR** derived from
  CBaseGrenade; model **models/weapons/grenades/ar_grenade.mdl**.
- CGrenadeAR's original vtable index **250** at VA 0x22314570 is
  Detonate. Touch VA 0x223142F0 and think VA 0x22314360 invoke
  that same slot under conditions that still need a complete CFG.
- Original spawn sets **+0x7AC=100.0** scalar damage, **+0x798=250.0**
  explosion radius, **+0x7C4=100.0** danger hearing radius and initial
  scheduled think at time +0.1 seconds.
- Active flight/think can send AI **SOUND_DANGER=0x8** for 0.2 seconds
  on channel 1, at **origin + velocity*0.5**. A gated original branch
  checks danger radius **<300** before increasing it by **15**.
  Recurring think schedule is +0.05 seconds. The comparison is NOT
  an unconditional clamp at 300; 295 can become 310.
- Detonate checks the one-shot flag at object **+0x795**, returns if
  unarmed, and clears the flag before continuing.
- The original effect uses a trace to select literal Scorch or
  SmallScorch decals; the exact trace/collision condition is not
  yet reconstructed.
- Flat-screen screen-shake request has original **amplitude=25,
  frequency=150, duration=1.0, radius=750**. The original helper
  clamps amplitude to 16.
- Original damage info receives **100 base damage** and
  DMG_BLAST **0x40**, while radius damage receives **250 base radius**.
- The radius-damage wrapper performs the damage call, then adds a
  separate server-side AI SOUND_COMBAT|SOUND_CONTEXT_EXPLOSION event
  **0x02000001** using radius **max(128, blast_radius*1.5)**,
  **1-second** lifetime and channel 0, then projectile removal follows.

Original reports:

- research/phases/sin-original-ar-grenade-lifecycle-and-ammo.md
- research/phases/sin-original-ar-grenade-detonation-effects.md
- research/phases/sin-original-grenade-ai-danger-combat-sounds.md

## Implemented gameplay logic

The independently authored
include/sin_compat/original_ar_grenade_runtime.hpp and
src/original_ar_grenade_runtime.cpp provide a self-contained
C++17 projectile with initial state, kinematics, next think time,
danger radius, blast damage/radius and a one-shot detonation gate.

It calls a host-owned IHost interface for:

1. In-flight AI danger stimulus (not a client audio sound).
2. Original trace/effect selected decal.
3. VR-safe flat-screen shake *request* (explicitly do not move
   the tracked HMD).
4. Server-side radius damage using blast type, magnitude, radius.
5. Explosion AI hearing stimulus (not an explosion wav/visual).
6. Host-owned entity removal after the damage/event callbacks.

Original event order and numeric constants are kept distinct.

**Unproven predicates deliberately stay external:** the caller must
supply whether the original conditional think radius growth and
danger emission happen, whether collision/think has approved the
detonation, and which decal a real trace selects. The component
does not invent game physics, owner identity, damage modifiers,
entity interfaces or networking. A game engine can adjust runtime
damage/radius only after separate validation by its actual game rules.

The runtime blocks invalid NaN/Inf kinematics/time, unrepresentable
think deadlines and invalid runtime damage/radius; rejects duplicate
detonation; and clears the armed flag before calling host callbacks
so recursive engine callbacks cannot apply double damage.

## Built and tested

- CMake static target sin_original_ar_grenade_runtime, source-only.
- CTest sin_original_ar_grenade_runtime_tests verifies exact
  initial values, original think cadence, discrete radius growth,
  300-comparison overshoot, source-grounded AI event origin/duration,
  missing original predicates, decal choices, shake comfort marker,
  ordered blast callbacks, radius fallbacks, invalid inputs and
  reentrant detonation protection.
- scripts/build_android_arm64.sh and
  scripts/build_android_emulator_x86_64.sh also cross-compile the
  gameplay target using the official NDK. They do **not** run it
  inside the actual SourceVR engine or a Quest headset.

To build without commercial files:

    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --target sin_original_ar_grenade_runtime_tests
    ctest --test-dir build -R '^sin_original_ar_grenade_runtime_tests$' --output-on-failure

## What remains before this projectile is playable

The original C++ engine entity lifecycle, real original physics
trace and detonation conditions, owner/attacker identity, prediction,
network send/recv tables, game-rules damage entrypoints and
SourceVR ARM64 entity/game-module ABI are not yet implemented.
The host callbacks in the new code are a deliberate port boundary,
not proof of an engine-compatible SiN server or complete Quest
release. Rendering, audible effects and controller-based throwing
still need separate reconstruction and on-device tests.

No proprietary DLL, APK, engine ELF, game assets or models committed.
Keep this change in a stacked DRAFT PR until real engine integration.
