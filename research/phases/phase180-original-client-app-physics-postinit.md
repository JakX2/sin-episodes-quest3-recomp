# Phase 180: Original SiN 2006 client app-system startup, cleanup, and physics interface gate

**11 November 2026.** Draft continuation stacked on Phase179 PR #232. This report is based on the user's **actual** original *SiN Episodes: Emergence* Windows x86 PE32 DLLs, not a Source SDK interpretation.

## Provenance and exact original x86 evidence

- Original `client.dll`, 3,592,192 bytes, SHA256 `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`, original image base `0x24000000`.
- Companion original `server.dll`, 6,049,792 bytes, SHA256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`.
- **Three previously uncatalogued complete original client functions, 199 bytes total**:

| Original native VA | Bytes | SHA256 of complete original function | Verified role |
|---|---:|---|---|
| `0x241640F0` | 48 | `93dbf327403a2963eba2ee6341b0aa5bfc0a7fef581840e57b71b418d7ae78d2` | Linked app-system callback cleanup |
| `0x24164120` | 54 | `645a9002d77b476692fe8688d691e87c4d2ebf8a61976be9eda071e41d7d3b7f` | Linked app-system creation/registration and failure rollback |
| `0x24182C60` | 97 | `46e079ea76e0cfee721efe3a421d68920cf980079aea4d76d7542f6f9b5281f2` | Three mandatory client physics interface queries, then opaque callback |

The read-only auditor checks **51 non-contiguous native instruction witnesses**, **two internal native `CALL rel32` edges**, **two incoming original client Init call edges**, three original interface strings, all three exact original storage globals and the three full-region native digests. The original DLLs and full disassembled bodies are excluded from the repository.

## Original app-system linked list and failure behavior

Original global head DWORD at `0x24369374` points to a singly-linked 16-byte node. The native fields are **DWORD +0** destination-pointer address (may be null), **DWORD +4** create callback, **DWORD +8** cleanup callback, **DWORD +12** next link. These are **not** the 12-byte `CreateInterface` registration nodes from Phase177.

At native `0x24164120`:

1. If the head is null, return true without invoking any factory.
2. For each node, invoke the pointer at `node+4` and read its result from EAX.
3. If result is nonzero, native reads `node+0` and writes EAX to the pointed-to global. It does not check for null output address in this success path, which is an **unsafe native precondition**.
4. If result is zero, check `node+0`. If it is null, skip this optional item and continue. If nonnull, call native cleanup `0x241640F0` over the **entire original list** and return false.
5. Continue through `node+12`; return true after the last node.

At native cleanup `0x241640F0`, each node with a nonnull output address reads the current output value. When nonzero, it pushes that value as the argument to callback `node+8`, then **writes zero to the output location**. When already zero, it still clears the output location. A node with null output address is skipped. The native function does not alter the linked-list head.

A portable host safety difference is deliberate: if an opaque create callback returns nonzero despite node output address being null, the reference throws instead of executing an invalid native write. **This is not claimed original behavior.**

Original caller at `0x240FE39B` calls `0x24164120`; native failure at `0x240FE3A2` jumps to client Init's shared `0x240FE59E` false exit. The earlier Phase179 Init function is *not* counted a second time.

## Original client physics bootstrap at `0x24182C60`

Native callback factory arrives on the stack. It makes three **ordered, immediately fatal-on-null** calls with null query status pointer:

| Order | Exact original interface string | String VA | Original global write |
|---:|---|---|---|
| 1 | `VPhysics030` | `0x242A2338` | `0x24374398` |
| 2 | `VPhysicsSurfaceProps001` | `0x242A2320` | `0x243743A8` |
| 3 | `VPhysicsCollision007` | `0x242A2308` | `0x243743A0` |

The original code first **writes the returned pointer into its module global**, then branches to the false return if the pointer is null. A successful third request loads the existing global DWORD `0x2433F6BC` (earlier client `VFileSystem012` pointer from Phase179) and the newly published surface-props DWORD `0x243743A8`, pushes those two raw pointer values, then calls **native `0x241927A0`**. We do **not** claim to have decompiled that target or identified its exact service-side effects. The native function returns true thereafter. Original incoming client Init `CALL` at `0x240FE540` leads to `0x24182C60`, with a failure conditional branch to the shared `0x240FE59E` Init false return.

These three physics strings are distinct from the 21 early + three late interface names in Phase179. It is a deeper startup stage beyond the export list and first engine handshake.

## Reproducible tests and recovered code

- `include/sin_compat/original_client_postinit.hpp`: portable, host-neutral C++17 behavior for all three original control-flow graphs, exact native node layout metadata and VA constants.
- `tests/test_original_client_postinit.cpp`: GCC and Clang C++17 `-O2 -Wall -Wextra -Wpedantic -Werror` passing, including first/middle/last failure cleanup, null-output optional entries, callback order and physics success/failure for each original lookup.
- `research/tools/audit_sin_original_client_postinit.py`: native whole-DLL hash checks and original 3-function SHA256 checks, call edges, selected opcode/branch anchors, physics strings and globals; additionally chains Phase179 owner-only Init/original-DLL verification when run as a complete script.
- `research/tests/test_sin_original_client_postinit.py`: **10 passing original-free Python tests**, including per-opcode, per-original-CALL and per-physics-string mutation and full-original-function SHA rejection for synthetic fixtures.
- Root CMake/CTest target `sin_original_client_postinit_tests`; actual Windows x86 proxy CI also builds and runs `sin_original_win32_client_postinit_contract` using MSVC with assertions active in Release.
- Unique original-function atlas **131 → 134**, covering **74 server / 60 client**. These are reviewed functions, not percentages of the whole original DLLs.

Owner-only local audit invocation:

```sh
python3 research/tools/audit_sin_original_client_postinit.py \
  --client /path/to/original/client.dll --server /path/to/original/server.dll
python3 -m unittest discover -s research/tests -p test_sin_original_client_postinit.py -v
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --target sin_original_client_postinit_tests
ctest --test-dir build -R sin_original_client_postinit_tests --output-on-failure
```

## Still unresolved

The original physics coupling function `0x241927A0`, engine-owned interface implementations and memory ownership, other post-Init UI systems, the original game's runtime class ABI and original executable running our replacement DLLs remain unresolved. This is a tested **partial original startup reconstruction**, not a playable Windows DLL, Android gameplay implementation or Quest 3 port. Next trace the exact native `0x241927A0` callback and the server's post-emitter initialization gates, then prove native Windows original-engine initialization with real interface vtables.