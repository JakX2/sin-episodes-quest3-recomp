# SiN Episodes: Emergence (2006): Jessica's actual entity velocity impulse

**Research phase 105, November 8, 2026.** Static reverse-engineering and independently authored C++17 reference logic. Original game binary SHA-256:
`568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650` (`server.dll`, x86 PE32, base `0x22000000`).

## Correction to preceding phase

The previous Jessica ballistic phase correctly recovered the math helper at
`0x2204C850`, but incorrectly described its final `0x2214BA20` call as
setting an entity's **initial velocity**. The disassembled original
function **adds an impulse** and has a separate physics branch. Replacing
preexisting velocity changes the actual SiN behavior.

The original function is `0x2214BA20..0x2214BAD0` (177 bytes,
SHA-256 `5eb87e06301e647f9ab83b4463e940e6bc14b5e78eae08897a5723ad58e5a3f4`).
Jessica calls it at `0x22362529`, after computing ballistic launch delta.
The exact original C++ function name was not recovered.

## Directly verified machine-code behavior

| Address | Observation | Reconstruction consequence |
| --- | --- | --- |
| `0x2214BA29..0x2214BA5E` | x87 compares the three input components with global vector data at `0x225885B8..C0`. An all-matching vector returns. | Skip the zero-vector sentinel, with the caveat that a globally reassigned sentinel has not been ruled out. |
| `0x2214BA60` | Compares entity `+0x204` with movement type `6`. | Two distinct execution paths. |
| `0x2214BA69..0x2214BA7A` | Reads physics interface pointer at `entity+0x218`; pushes a zero/null secondary argument, the linear vector, then invokes physics vtable byte offset `+0xB0`. | Host must dispatch a physics linear velocity addition and null angular input. Exact native ABI must be verified before real use. |
| `0x2214BA82..0x2214BA92` | Non-physics: tests bit `0x1000` in entity flags `+0xB8` and conditionally calls `0x2213D750`. | Ask host to refresh dirty absolute velocity before reading it. |
| `0x2214BA97..0x2214BAC0` | Adds each requested component to existing float fields `+0x250`, `+0x254`, `+0x258`. | `new_abs_velocity = current_abs_velocity + impulse`, not an overwrite. |
| `0x2214BAC4` | Calls `0x22141A00` with computed three-component vector. | Delegate absolute-velocity side effects to a compatible host. |

The shared global vector is not file-backed in the private PE's mapped initialized-data sections. It is consistent with Source's global zero vector, but original runtime writes elsewhere have not been exhaustively excluded, so the portable implementation's **zero** interpretation is a documented assumption. The host-ready implementation also adds finite/overflow and missing-entity guards that are not claimed to exist in Ritual's original x87.

## Implementation

- `include/sin_compat/original_jessica_impulse.hpp`: clear `IOriginalImpulseHost` interface for exact movement-type and velocity behavior, independent of 2006 MSVC binary pointers.
- `src/original_jessica_impulse.cpp`: two branch actions, dirty-velocity refresh, three-axis addition and validation.
- `include/sin_compat/original_jessica_throw.hpp` and `src/original_jessica_throw.cpp`: **replaces the misnamed `set_initial_velocity` contract** with additive impulse processing after the already-reconstructed ballistic solution.
- `tests/test_original_jessica_impulse.cpp`: zero/negative-zero, non-physics addition, dirty-refresh ordering, type-6 physics dispatch, rejected hosts, missing fields, overflow, NaN and thrown host exception.
- `tests/test_original_jessica_toss_host.cpp`: full original-free Jessica animation -> weapon creation/ammo -> geometry -> physics/nonphysics impulse test.
- `research/tools/audit_sin_original_jessica_impulse.py`: exact original-DLL SHA identity, original full-function digest, nine instruction anchors and three call-target checks.
- `research/tests/test_audit_sin_original_jessica_impulse.py`: synthetic PE32 malformed-input regression.

Private verification (local game-owned files only):

```sh
python3 research/tools/audit_sin_original_jessica_impulse.py \
    --server /path/to/original/server.dll
```

Portable test command:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

## Remaining evidence gaps

This phase does **not** reconstruct the precise C++ signature of the player virtual call at `+0x214`, identify all native physics interfaces, prove every possible assignment of the shared sentinel vector, recreate `0x2213D750`/ `0x22141A00`'s full host engine side effects, or load a real SiN map.

This is still a host-agnostic C++ library on a stacked draft PR. It is not a working Quest APK, original game client/server, SourceVR engine module, or gameplay-level demonstration. No original proprietary DLL, original model/map/script, or disassembly dump is committed.
