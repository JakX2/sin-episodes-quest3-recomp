# Phase 87: SiN original client/server subsystem registration, initialization and rollback

## Original binary authority

Inspected the user's original 2006 Windows x86 PE32 `client.dll` (SHA-256 `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`) and `server.dll` (SHA-256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`). The verifier chains Phase 86's dual-DLL SHA-pinned original initialization audit.

**Nine original complete function regions are SHA-256-pinned**, in addition to **73 exact x86 opcode sites, 16 relative CALL edges, two relative tail JMP edges and five NUL-terminated source strings**.

| Library | Purpose | Original VA | Bytes | Full function-region SHA-256 |
|---|---|---|---:|---|
| client | registration setup | `0x24164120` | 54 | `645a9002d77b476692fe8688d691e87c4d2ebf8a61976be9eda071e41d7d3b7f` |
| client | registration rollback | `0x241640F0` | 48 | `93dbf327403a2963eba2ee6341b0aa5bfc0a7fef581840e57b71b418d7ae78d2` |
| client | VGUI initialization | `0x241EDFB0` | 123 | `64cbfa079f093b1e577a59721396d73742eb4543b836e7fb0cc55d724783417d` |
| client | particle allocation | `0x2417F680` | 82 | `702b2e6cd174ce3abc939e91d99710a3669a0c23b82aa50b2904064445a3cf5a` |
| server | game systems startup | `0x221F5700` | 277 | `1e2a6e068b24b061fbd5cba16741f8ca8f470421b7ccbca3fa0239f579a9c530` |
| server | repository A forwarding | `0x221AD8A0` | 10 | `83762183370ab5370d0c099c4350b20768c57043ed99066edb8e1b1804da49e0` |
| server | repository A implementation | `0x221AD3E0` | 576 | `7b9db352473a8fc2d5343473b673ac52c89a8c98320847b0b4dd451dcf8e4e69` |
| server | repository B forwarding | `0x22149B60` | 10 | `47793d5818d074eb9ad8526102a131bb60fd50ab5fcaaa52aa5afe6e940852d9` |
| server | repository B implementation | `0x221498C0` | 576 | `94b55dbf7e21d7b45362d277505a8a7c02537667c3c4c4afbeeded9c40ec2218` |

## client.dll: linked setup and teardown registry

Phase 86's verified `VClient011` initialization call `0x240FE39B` reaches **`0x24164120`**. Original global head `0x24369374` is a linked list of callback nodes; x86 node fields are:

| Byte offset | Meaning |
|---|---|
| `+0x00` | pointer to 32-bit storage slot (may be null) |
| `+0x04` | initialize function pointer |
| `+0x08` | destroy function pointer |
| `+0x0C` | next node pointer |

The setup loop invokes each node's `+0x04` function. For nonnull return, it stores the object through `*slot`; for null return **with nonnull slot pointer**, it calls original **`0x241640F0`** and returns `AL=0`. If both the returned object and slot pointer are null, that node is skipped and setup continues. When all nodes complete, it returns `AL=1`.

The cleanup routine visits the **entire list**, calling each node's `+0x08` destructor with the value already stored through its slot (including when that stored value is null), and then zeroes that slot. It is **not limited to the successful prefix**. The reference C++ adapter reproduces this order for well-formed mock nodes. A malformed mock returning nonnull with a null destination slot is rejected safely rather than reproducing the original null-pointer crash.

## server.dll: registered game-system initialization

Phase 86's required original call `0x221E7927` reaches **`0x221F5700`**. It reads pending linked head `0x22572CC0`, with the next pointer at object `+0x04`, and maintains a dynamic array:

- data pointer `0x22510A58`
- element count `0x22510A64`
- capacity `0x22510A5C`

The original routine compares **object pointers**, avoids inserting duplicates into the array and calls an existing diagnostic function with the original string `AutoGameSystem already added to game system list!!!` when duplicated. The routine uses a vector growth helper `0x221F51A0` and a memory mover `0x223A5D10`, without assuming a particular Source STL implementation.

Crucial order: before invoking per-system methods, the original clears the pending head at `0x221F57D5` and sets global byte `0x22572CB8 = 1` at `0x221F57DF`. It then iterates all active pointers and calls each object's first virtual function slot `+0` at `0x221F57FA`. On the first `AL=0`, it returns failure immediately at `0x221F5810`; otherwise it returns `AL=1`. The pending head and initialized flag are **not automatically reverted** when a system fails.

The C++ reference uses host-owned vectors and explicit function pointers. This approximates observable ordering/deduplication, **not** exact 32-bit heap allocation, pointer layout, vtables, exception semantics or full engine lifecycle.

## Further original client setup: VGUI and particle manager

**VGUI** `0x241EDFB0` (called at `0x240FE3C7`) verifies a module-named step using literal **`CLIENT`**, then requests exact interfaces **`VGUI_InputInternal001`** and **`VGUI_System010`** from the obtained factory; failure to resolve either returns false. The resulting pointers are stored to globals `0x24378AB8` and `0x24378ABC` respectively. A one-time registration flag at `0x24378B28` gates call `0x24009340`, then helper calls `0x2422ECD0` and `0x2422ECF0` occur; the return of the latter is converted to boolean. The real VGUI ABI and the two helper meanings remain unproven.

**Particle setup** `0x2417F680` (called at `0x240FE3B9`) first resets state via `0x2417F3A0`; allocates **0x30 bytes** via `0x2416FCF0`, passes original **0x60** and label **`CParticleMgr::m_pParticleBucket`** to constructor helper `0x241704C0`, stores the constructed pointer into client object `+0x84` and returns whether that pointer is nonnull. The C++ header retains these verified constants but does not claim to implement Source particle allocation or lifetime.

## Further original server setup: two repository wrappers

Phase 86's later required calls lead to two 10-byte wrappers:

- `0x221AD8A0`: moves original singleton `0x22503EE0` into ECX and tail-jumps to `0x221AD3E0`.
- `0x22149B60`: moves singleton `0x224F0388` into ECX and tail-jumps to `0x221498C0`.

Both implementation bodies include a byte test at instance `+0x3D` returning success when already initialized; otherwise set that byte **before** validating a filename pointer at `+0x34`. They also use an independent state byte at `+0x3C` and external file-system calls. The exact repository schemas, parser data structures and all failure side effects **are not yet reconstructed**. This phase pins both entire implementation bodies and their short wrappers, but intentionally does not invent their game-system identities.

## Artifacts and verification

- `include/sin_compat/original_subsystem_startup.hpp`: standalone C++17 behavioral reference for client setup/rollback, server dedup/list/init ordering, VGUI required interface requests and original particle constants.
- `research/tests/sin_original_subsystem_startup_reference.cpp`: tests complete setup, failure+full rollback, optional missing slot, repeated registrations, duplicate server system, early failure without undoing state, VGUI requests and one-time registration.
- `research/tools/audit_sin_subsystem_startup.py`: SHA-pinned original dual-DLL read-only audit, nine code-region SHA-256, 73 opcode anchors, 16 direct CALL targets, two wrapper JMP targets and five original string anchors; outputs JSON metadata without original binary code.
- `research/tests/test_sin_subsystem_startup.py`: 11 synthetic/original-free tests mutating **each** checked opcode, CALL, JMP and string, plus full SHA guards.
- `CMakeLists.txt`: CTest target.

Both private original DLLs passed the chained audit locally. The strict C++17 test compiled and ran with `-Wall -Wextra -Wpedantic -Werror`; 11 Python tests passed. GitHub CI is checked separately.

## Progress and hard limits

Real progress on **both original client.dll and server.dll**, but this is still a *verified subsystem behavior reference*, **not** a completed C++ decompilation, game module for `CreateInterface`, or Android ARM64 SiN port. This does not provide original Source client/server entity classes, physics, savegames, map startup, complete VGUI or particle render logic. Nor has a Quest 3 device loaded these game modules.

Phase 88 should examine original Init Shutdown transitions and how the two server repositories feed entity/AI or resources; the exact SourceVR 0.5.5 native factory ABI is a separate prerequisite for real loading. Do not blindly alias 2006 SiN interface versions to newer Source engine versions.
