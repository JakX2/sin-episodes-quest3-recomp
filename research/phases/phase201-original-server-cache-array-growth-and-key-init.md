# Phase 201: original server cache array growth and key-descriptor initialization

13 November 2026. **Stacked draft on canonical Phase200 PR #254**, which adds separate 64-node balancing stress tests. This phase is a different original-binary recovery; it is not a replacement of the concurrent code. The owner's original *SiN Episodes: Emergence* (2006) Windows i386 `server.dll` and `client.dll` were read privately from `bin.zip`, and both complete original files match the existing SHA256s. No commercial binaries, asset files, full native method bodies, or disassembly are committed.

## Four new SHA-verified original functions

| Original server method | Native VA | Exact size | Complete original function SHA256 |
| --- | --- | ---: | --- |
| Scene 24-byte node-vector growth | `0x221A3A10` | 119 | `e4affe6087ab30ce324d99a085b63f7f6589c3532bfbf9969dc2a5dac389abd5` |
| Model-sounds node-vector growth | `0x2213AAC0` | 119 | `c8cd4dd9c3ea9eaf97582206ea260481dde62a981a0518fede75a4fa42986f8f` |
| Scene 16-byte key initialization | `0x221A3A90` | 36 | `f55c402b759c9af8cb82f01ca4ae3844018f8e2cf6e886e5e083b75d3b2ea53e` |
| Model-sounds 16-byte key initialization | `0x2213AB40` | 36 | `f55c402b759c9af8cb82f01ca4ae3844018f8e2cf6e886e5e083b75d3b2ea53e` |

**310 newly reviewed complete original native bytes, four distinct original method entrypoints**. The two entire 119-byte grow methods become byte-identical after zeroing **only two original allocator CALL-rel32 displacement DWORDs** in each body, at `+0x51` and `+0x69`. Their full normalized native SHA256 is `609ec40a2754642eccef8c7d8715049b146d0f8990e0ead2bd5827c7e7d07192`. The two key initializer bodies are directly byte-identical without normalization.

The original full server file SHA256 is `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`; original full client file SHA256 is `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.

## Native dataflow and ABI, directly grounded in original machine code

- The growth routine is original MSVC x86 `thiscall` taking one stack DWORD `additional`, returning `ret 4`. **Critically, its ECX points to `tree + 4`, not the complete tree.** Phase194 allocator's original `LEA ECX,[ESI+4]` and the exact incoming relative calls at scene `0x221A68AD -> 0x221A3A10` and model `0x2214091D -> 0x2213AAC0` prove the association.
- The vector header at grow-this offsets `+0`, `+4`, `+8` therefore maps to native tree `+0x04` backing pointer, `+0x08` capacity DWORD, and formerly unnamed **`+0x0C` growth policy DWORD**, respectively.
- If growth policy equals `0xFFFFFFFF`, original growth exits without changing backing pointer or capacity, without an allocator call.
- Otherwise original code computes `needed = capacity + additional` and uses a **signed x86 comparison**. While capacity is less than needed, when capacity is zero it first becomes **2**; with zero growth step and nonzero capacity it doubles; with nonzero step it adds the configured step until enough.
- After capacity calculation, the native function **always** calls a heap routine, even when the existing capacity was already sufficient. When backing pointer is nonzero, it calls native `0x2226E400` with `(pointer,capacity*24)` (realloc-like). When pointer is zero it calls native `0x2226E390` with `capacity*24` (malloc-like). In both cases it stores returned EAX back into pointer DWORD at vector `+0`. The function mutates capacity before the allocation call and does not check allocator failure.
- Growth methods have two native `ret 4` exits followed by nine `int3` alignment bytes. The key initializer methods check the incoming pointer for zero and, if non-null, write four DWORDs **`0, 0, 0xFFFFFFFE, 0xFFFFFFFF`** into 16 consecutive bytes; they return with plain `ret` and have 12 `int3` padding bytes. The Phase194 allocator also initializes these fields inline on its own path.

## Independently implemented code and scope

`include/sin_compat/original_server_cache_array_growth.hpp` adds exact-size, native-width structs and a host-neutral C++17 reference with two mock callbacks: `allocate(bytes)` and `resize(previous,bytes)`. It uses **synthetic 32-bit pointer tokens**, not real x86 heap addresses. Host-only signed overflow, byte-size and loop guards prevent malicious or malformed states, unlike the original x86. The reference also includes null-safe 16-byte descriptor initialization.

`tests/test_original_server_cache_array_growth.cpp` checks first allocation (2 slots, 48 bytes), doubling (2→4, 96 bytes), needless-but-native reallocation when the requested count is zero, fixed-step growth (4→7→10), disabled `0xFFFFFFFF` policy, empty allocation, overflow refusal, descriptor defaults and **actual composition with Phase194's cache slot `claim()`**. Cross-phase integration replaces the Phase194 test host's simplistic one-slot expansion with the original two-slot-first/doubling policy. It demonstrates first slot allocation grows to capacity 2, second slot reuses that capacity, third request grows to capacity 4, and a freed index reuses the free-list without growing. Root CTest and actual MSVC Win32 x86 CTest targets are registered.

`research/tools/audit_sin_original_server_cache_array_growth.py` validates both complete original DLL hashes, all four complete original native method SHA256s, **52 sparse original opcode witnesses, four exact native outgoing heap CALLs, two exact incoming Phase194 calls**, post-return boundaries and both complete twin-body comparisons. Owner-original binary audit passed locally.

`research/tests/test_sin_original_server_cache_array_growth.py` has **eight original-free mutation test groups**, corrupting each opcode witness, native call and method boundary and rejecting fake whole-body/whole-DLL hashes or unwitnessed differences. All eight pass in local Python unittest. Standalone C++17 model tests passed locally with GCC and Clang optimized `-Wall -Wextra -Wpedantic -Werror`. The **combined CMake/Win32 integration tests must be checked in hosted CI** before claiming they passed.

## Limits and priorities

The original heap allocation functions `0x2226E390` and `0x2226E400` have been identified as direct call targets but are **not** newly decompiled here. Their allocation ownership, native failure behavior, real filesystem key provenance and game-loader interactions remain unresolved. No engine-hosted Windows original-game DLL replacement or playable Android/Quest module has been demonstrated.

Unique verified original-function catalog **195 → 199 (107 server, 92 client)**. This is *research inventory*, not a percentage of game readiness. Next investigate original heap ownership and retail Windows engine startup, while gradually reconciling Phase197's 1,387-byte balancing loops with the portable references. Windows PC first, Android second, Quest 3 third.
