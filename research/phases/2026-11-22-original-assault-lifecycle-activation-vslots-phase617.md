# Gameplay Phase617: original 2006 Assault activation lifecycle slots

## Direct original binary evidence

I read the owner's unchanged original SiN Episodes: Emergence `server.dll` and `client.dll` from the mounted `bin.zip`. Complete original SHA256:

- server.dll `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`
- client.dll `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`

The original authenticated `CAI_AssaultBehavior` vtable is at `0x223F5544`. Its `+0x18` eligibility method was recovered in Phase607, its `+0x20` shutdown in Phase611, and Phase604 established that switching into a newly chosen behavior invokes virtual `+0x1C` and then `+0x24` before deactivating the previous behavior and notifying the owner.

This phase recovers the **previously omitted actual contents** of Assault's `+0x1C`, `+0x24`, and `+0x28` methods. They are small but important: a host adapter previously invoked a generic external callback for both +0x1C and +0x24, mistakenly giving the real +0x1C no-op an opportunity to create an external side effect.

## Three COMPLETE original native methods: 9 bytes

| Actual original Assault slot | Original VA | Whole original bytes | Whole original SHA256 | Actual semantics |
| --- | --- | ---: | --- | --- |
| `+0x1C` | `0x220CDB40` | **1** | `ae3f4619b0413d70d3004b9131c3752153074e45725be13b9a148978895e359e` | Immediate `ret`, no side effects |
| `+0x24` | `0x220CD290` | **7** | `5250ce3c1823f4699bcf08fcd456150282d40a8864891740e49ecbb6c1492db3` | Read behavior pointer `+0x0C`, obtain its vtable, tail-jump to virtual `0` |
| `+0x28` | `0x220CD900` | **1** | `ae3f4619b0413d70d3004b9131c3752153074e45725be13b9a148978895e359e` | Immediate `ret`, no side effects |

All three original whole-body SHA256s, exact vtable slots, and genuine following native alignment were verified directly. The original `+0x1C` function is followed by **15 INT3 bytes**, next method `0x220CDB50`; `+0x24` has **9 INT3 bytes**, next method `0x220CD2A0`; `+0x28` has **15 INT3 bytes**, next method `0x220CD910`.

The two RET method bodies are byte-identical but exist at distinct real addresses. The seven-byte `+0x24` body is exactly shared by `CAI_BeserkBehavior` at its vtable+0x24. The same `0x220CD900` +0x28 RET address is shared by **six** of the original eight registered Grunt behaviors. These claims concern real original machine code, not guessed 2013 SDK C++ method names.

### Original eight-behavior lifecycle inventory

The new SHA-gated source auditor records all original `+0x1C/+0x20/+0x24/+0x28` vtable targets for:

- CAI_RappelBehavior
- CAI_FuncTankBehavior
- CAI_TakecoverBehavior
- CAI_BeserkBehavior
- CAI_ActBusyBehavior
- CAI_AssaultBehavior
- CGruntStandoffBehavior
- CAI_FollowBehavior

Their original RTTI-backed vtable pointers were established in Phase535. The auditor confirms **32 exact lifecycle slot pointers** against the untouched original `server.dll`. It does **not** count the other seven classes' method bodies as recovered or claim their state transitions are complete.

## Runtime source-fidelity correction

The Phase613 loadable Grunt behavior C ABI currently implements the previously verified Phase606 priority selection, Phase610 Assault eligibility and Phase611 shutdown. Before Phase617 it invoked its generic Host `behavior_lifecycle(...,stage0)` even when choosing Assault, although the real native Assault vtable+0x1C is just RET. Phase617 changes the adapter as follows:

1. On Assault activation only, the Phase617 `enter_1c_noop()` function substitutes for an external stage0 side effect. The engine host must **not** be told to perform any imaginary Assault +0x1C action.
2. Assault `+0x24` is modeled through an explicit `enter_24_bridge(Host)` call. Its adapter invokes the host's existing **stage1** callback, which the real host must implement by calling the validated engine-owned behavior+0x0C bridge object's virtual0. No retail pointer is passed across the C ABI.
3. All other original behaviors retain their existing host-delegated stage0 and stage1 callbacks until their exact method bodies are recovered. Original Assault +0x20 same-owner cleanup is unchanged.
4. The `+0x28` no-op is available in the new independent model but is **not** added to Phase604's transition sequence, which does not call that slot.

The five exported names, ABI v1 version, callback table fields, Windows export definitions, proxy discovery and phase615/616 opt-in loading **do not change**. The existing host callback table comment is updated to document the now-source-verified Assault-specific bypass.

## Tests and source auditing

Files:
- `research/tools/audit_sin_original_assault_activation_phase617.py`: both complete retail DLL SHA256s, all 32 native lifecycle vtable pointers, 3 complete original method body digests, method INT3 boundaries and next-method openers, shared Beserk dispatch, six original +0x28 users, with an original-free mutation test rejecting nine changed bytes.
- `include/sin_compat/original_grunt_assault_activation_phase617.hpp`: independent C++17 RET and opaque host-bridge models.
- `tests/test_original_grunt_assault_activation_phase617.cpp`: no-op vs exactly-once host invocation.
- Changes to `src/grunt_behavior_c_api_v1.cpp`, public callback documentation and `tests/test_grunt_behavior_c_api_v1.cpp`: test that first Assault activation delivers only `life5:1` and **never** `life5:0`, while a later Rappel takeover retains two generic callbacks, same-owner link cleanup and original notification order.
- Root and Windows x86 CMake add distinct CTests for the independent source model and the source auditor. Existing C ABI and Win32 dynamic load tests continue.

The source-native original hashes and slot inventory were independently inspected directly against `bin.zip`. GitHub's original-free CI does not receive commercial binaries.

Reproduce the original source audit privately:

    python research/tools/audit_sin_original_assault_activation_phase617.py --self-test
    python research/tools/audit_sin_original_assault_activation_phase617.py --bin-zip /private/SE1/bin.zip

**Phase617 adds three newly whole-authenticated native method addresses / nine original native machine-code bytes. Cumulative Phases601–617: 43 complete native original method addresses / 11,233 source-authenticated original machine-code bytes.** These include two byte-identical one-byte RET methods at distinct native addresses; do not confuse original-byte accounting with ported C++ line counts or a playable Windows/Quest/Android Grunt.

The true remaining in-game gate is still a real native Grunt factory, engine-owned NPC lifetime and actual game-process host callbacks. No physics/surface/model/prop work is duplicated.
