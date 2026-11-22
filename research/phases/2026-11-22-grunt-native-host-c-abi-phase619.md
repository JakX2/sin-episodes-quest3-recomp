# Gameplay Phase619: Native Grunt attachment via a standalone C ABI

## Rationale

Phase613's `sin_grunt_behavior_c_abi.dll` exposes exactly five exports and can execute reconstructed Grunt behavior independently. Phase614 provides a C++17 registry which may attach only after a host confirms a real engine-owned Grunt. Phase618 checks that identity on every step/snapshot. A native Source server plugin should not have to link directly against our C++ Registry class, because its C++ ABI is not stable between toolchains.

Phase619 therefore exposes a **separate seven-export C DLL**: `sin_grunt_native_host_c_abi.dll`. It depends on the existing `sin_grunt_behavior_c_abi.dll`; neither the prior five-export ABI nor the original `server.dll`/client `CreateInterface` proxy is changed.

## Exact public calls

- `sin_grunt_native_abi_version_v1()`: returns 1.
- `sin_grunt_native_create_v1(host,userdata)`: creates an **independent session registry**, not an NPC.
- `sin_grunt_native_attach_v1(registry,attachment)`: supports only the three original factory-authenticated Druglab Grunt classnames and only when an engine host confirms a live, native-factory-owned actor.
- `sin_grunt_native_step_v1(registry,actor,now,selected,underlying)`: runs original-source-authenticated behavior priority and Assault paths only after Phase618 native identity reattestation. Reports both registry and underlying behavior status.
- `sin_grunt_native_snapshot_v1(registry,actor,out)`: refuses stale native identities without touching the output state.
- `sin_grunt_native_detach_v1(registry,actor)`: cleans up an exact-generation session after an NPC's removal; does not call the engine object destructor.
- `sin_grunt_native_destroy_v1(registry)`: releases registry-owned behavior sessions only.

Both callback and attachment tables have exact version and size checks. The host confirmation callback takes an immutable original classname, `(index,generation)`, and opaque owner token, and must check these against its authoritative engine entity registry. Callbacks use 1/0/-1 (success/not current/error). Host failure stops before executing AI. The behavior-host callback contract remains unchanged from Phase613.

## Minimum native host wiring required

A future **real Source engine module** must provide authoritative actual NPC spawn and destruction events:

1. Construct native engine-owned NPC through the correct entity factory. Only *after* a genuine created entity exists, collect its classname, `(index,generation)`, opaque owner token, host-registered condition IDs, and proper Phase613 callback table.
2. Create the session registry at the native host's initialization, attach the verified NPC after its true initialization, and step it from an actual NPC update callback.
3. Revalidate current native entity identity in the host confirmation callback on every step/snapshot. The host must also implement condition queries/updates, safe serial-handle lookups, behavior lifecycle and schedule/transition callbacks. Do not assume Source SDK 2013 vtable offsets match the original 2006 SiN.
4. On real entity teardown, detach the exact generation even when `actor_live` is false; destroy the registry before releasing callback userdata. All calls are serialized on the game thread; callbacks must not reenter or free the active registry.

These are integration requirements, **not proof that a real SiN Grunt has been spawned**. The original server proxy still only probes the behavior library. The new DLL does not intercept engine callbacks, synthesize `CBaseNPC`, replace `ServerGameDLL003`, or alter physics, materials, props or models. Original 2006 native functions: **unchanged at 43 verified methods / 11,233 bytes**.

## Original-free test matrix

The C11 executable links to the new shared library and tests ABI version/size rejection, supported and unsupported classnames, duplicate slot rejection, prior-generation refusal, engine confirmation, staged behavior callbacks, snapshot, owner identity revocation and host error, same-thread reentrant destroy/step rejection and safe detach/reuse. The independent loader smoke uses real `dlopen/dlsym` or x86 `LoadLibrary/GetProcAddress` for **all seven exports**.

The root CMake compiles the shared library using a PIC-compatible Phase614 registry and applies a strict Linux export map. The existing Win32 x86 CMake project produces a PE32 DLL and independently runs both C and real Windows loader tests. Commercial SiN DLLs, proprietary content and fake Source interfaces are not needed in CI.

### Validation

See the Phase619 branch's C++ reference CI and Win32 x86 compatibility workflow. These tests verify the C-host integration boundary, **not runtime execution of a genuine retail NPC**.
