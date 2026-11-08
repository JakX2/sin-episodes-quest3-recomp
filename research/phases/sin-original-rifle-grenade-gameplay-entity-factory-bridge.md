# Source-verified 2006 SiN named entity factories: gameplay bridge

**8 November 2026 | Actual game-code implementation with fail-closed SourceVR boundary**

Primary source: owner's unchanged original 2006 SiN Episodes: Emergence
server.dll, SHA256:
568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650

Secondary evidence: five real owner-owned map-load console logs from:
https://drive.google.com/drive/folders/1ExaddVDbt0UTrvec4jJdg1lXVdn6AAEJ?usp=sharing

The logs contain 1,531 error-line occurrences spanning 49 distinct
unrecognized SiN game entity names. The full per-class aggregate is at:
research/phases/sin-original-map-runtime-unknown-entity-logs-nov08.md

Those errors show WHY a normal content-only map load fails without
SiN game entity factories. The logs alone do not specify original
C++ constructors, map object identities or native ARM64 interfaces.

## Two actual original server.dll registration functions

This phase verified the precise registry path of TWO names for which
the independent repository already contains actual gameplay logic.
These registry routines were not among Phase 73's separate selected 32
evidence rows. They are additional original entity-factory witnesses.

| Original classname | Registration VA | Literal name VA | Distinct native x86 factory object | Portable implementation |
| --- | --- | --- | --- | --- |
| weapon_assault_rifle | 0x223C4F60 | 0x22477C30 | 0x2252D4FC | original WeaponSession with original assault-rifle profile |
| grenade_ar | 0x223C4FC0 | 0x22477230 | 0x2252D500 | CGrenadeAR flight/think/detonation and original touch gate |

Each original function contains a direct CALL to the previously verified
global entity-registry getter at 0x2228B110, loads its virtual method,
pushes the exact classname string and original class-factory object,
calls the registry's first virtual method, and returns. The independent
original PE32 verifier checks these exact 22-byte instruction SHAPES,
literal name pointers, factory object data addresses, original
registration call targets, and full server.dll SHA256.

This proves the names ARE registered by the original game, but the
original MSVC x86 object vtables and pointers cannot be used directly
in a native Quest 3/SourceVR ARM64 engine.

## Actual new C++ gameplay entity registry, not dummy class aliases

New classes and methods:

- include/sin_compat/original_gameplay_entity_factory.hpp
- src/original_gameplay_entity_factory.cpp

GameplayEntityFactory installs two precise callback/classname pairs
through IGameEntityRegistrar. The future engine host MUST install those
callbacks in a GENUINE supported native game entity factory; the
portable bridge does not export IServerGameDLL or fake a current
SourceVR registration or game launcher.

On a live native host entity with generation identity:

1. weapon_assault_rifle: independently constructed original-name map
   weapon record. Can remain **unowned** when a real map spawns a
   world pickup. When a live player picks it up, the bridge checks
   owner identity and creates a real portable WeaponSession with
   OriginalAssaultRifle's profile (45-round primary magazine,
   ARNADE secondary ammo, model and registered weapon kind).
   WeaponSession is NOT created with an invented owner at map load.
2. grenade_ar: spawns actual CGrenadeAR Runtime with original
   in-flight think schedule, kinematics, explosion and AI sound
   behavior. Original network-like contact flag +0x795 begins
   separately cleared, as the original server.dll proved.
3. Original touch: the bridge reads its own stored +0x795 flag,
   evaluates the original contacted-object raw +0x11C/+0x120/+0xFC
   gates, and must receive a host-confirmed original flag-setter
   callback before invoking the existing one-shot explosion.
   The setter is NOT faked: host rejection blocks blast.
   Once accepted, the actual PR #151 projectile damage/AI events
   and removal callbacks are invoked in their tested order.
4. Engine identity: exact index+generation matches are mandatory.
   Reused index with a new generation must explicitly release the
   old instance first, so no stale weapon or blast state survives.
   Old-generation unregister, selection, ownership and reentrant
   callbacks fail closed. The bridge never assumes an original
   EHANDLE bit layout or a native engine entity class structure.
5. Registration failure: if the native host rejects the second
   classname, the first is explicitly rolled back. A failed
   rollback remains visible and can be retried, not silently
   treated as successful registration. The host and registrar
   must outlive the bridge; all callbacks run on one thread.

**Not provided:** class factories for npc_druglab_grunt_pistol,
npc_druglab_grunt_assault_rifle, trigger_dynamic_difficulty,
prop_exploding_barrel and the many other original map classes.
For example trigger_dynamic_difficulty has an independently
proven original factory at 0x223BE4A0, but its full gameplay
class behavior has not been connected to a native host.
The bridge DOES NOT register it or return a placeholder object.

The 49 logged missing classes remain a live native-engine issue
until they instantiate correctly on real game-map loads. Providing
portable C++ classes is not the same as eliminating the runtime's
unknown entity type console errors. Only weapon_assault_rifle from
the five logs is represented by one of these two new class factories;
grenade_ar was independently identified from original firing
code, but was NOT a logged missing classname in the five files.

## Evidence and regression

- research/tools/audit_sin_original_rifle_grenade_factories.py:
  original owner-only PE32 validator with pinned SHA256, exact
  registry instruction structure, classnames and factory VAs.
- research/tests/test_audit_sin_original_rifle_grenade_factories.py:
  8 fabricated PE32 mutation tests; original game assets NOT in CI.
- tests/test_original_gameplay_entity_factory.cpp:
  compiled test via fabricated host/registrar that actually creates
  a map world weapon, later binds the existing real WeaponSession,
  creates a grenade Runtime and invokes its original touch/AI blast,
  tests blocked contacts, a host-rejected flag mutation, double
  detonation protection and repeated-generation rejection.
  Tests exact names, unsupported missing-log names, registration
  rollback, binding failure, capacity/physics failure, reentrant
  callbacks and correct cleanup.
- CMake/CTest static gameplay library and executable, plus the
  official Android ARM64/API26 and Android x86_64/API29 Bionic
  build targets. The latter is executed within Android emulator
  CI and records a PASS artifact.

Private exact DLL check:

    python3 research/tools/audit_sin_original_rifle_grenade_factories.py \
      --server /private/original/server.dll

Host-independent desktop:

    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --target sin_original_gameplay_entity_factory_tests
    ctest --test-dir build -R '^sin_original_gameplay_entity_factory_tests$' --output-on-failure

GitHub has NO original code/assets and all emulator tests explicitly
use synthetic engine callbacks.

## Shipping blocker and next engineering gate

This is a FUNCTIONAL portable original-class game factory, **not**
a working SourceVR engine registration. The current inert diagnostic
libserver.so/libclient.so from PR #147 cannot load SiN maps. The native
SourceVR engine requires a verified, compatible 47-slot
IServerGameDLL and 78-slot IBaseClientDLL, correct full startup and
entity-creation hooks, or another legally supported game host.

For an actual Quest port, the next step is to prove one genuine
engine-owned entity can be instantiated through the compatible
native host and subsequently call this portable gameplay behavior.
Then progressively bring the other map-required classes and
NPC types online and retest the five supplied map-log scenarios.

No original DLL, proprietary game asset, SourceVR .so, BSP,
NPC model, original script, APK or private log was committed.
