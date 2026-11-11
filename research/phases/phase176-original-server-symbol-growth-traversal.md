# Phase 176: Original 2006 server symbol-tree insertion traversal and record growth

11 November 2026. Stacks on Phase175 draft PR #228. Original executable evidence read directly from the owner-provided 2006 Windows x86 PE32 `server.dll` and matched with original companion `client.dll`. Neither binary, original complete x86 function bodies nor proprietary game content is committed.

## Original executable provenance

- Exact original `server.dll`: 6,049,792 bytes; SHA256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`.
- Exact original `client.dll`: 3,592,192 bytes; SHA256 `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.
- Four **complete original function bodies** totaling **546 machine-code bytes**, independently SHA256 hashed and verified directly from `server.dll`:

| Native VA | Exact body length | Original whole-region SHA256 | Native role |
|---|---:|---|---|
| `0x2228E200` | 115 | `bf973beee94fe5f7c2bd5e649e41eb973f99bb49dee68e28b9861757796adb2b` | 12-byte record dynamic-buffer reserve |
| `0x2228E280` | 112 | `1b91b29a1820e36300655fa1f9440dfc0f2653be30b61ca5a6f1ea44d6e8565b` | 4-byte record dynamic-buffer reserve |
| `0x2228E470` | 261 | `52909d48c6bde81a72b0e10e3f871e8d13de62ab03ca3a3aa2ffe6b145f3b871` | Separate symbol-tree insertion position / insertion-direction traversal |
| `0x2228E5B0` | 58 | `341f65a09dc5ce440ffbf168cdc6403706b1c409fd77f272c2adc041f1d5754b` | Guarded storage cleanup and pointer-cache refresh |

Phase176 private binary auditor checks **72 sparse original opcode witnesses**, **5 internal exact x86 `CALL rel32` edges**, **2 independently checked inbound caller edges**, and chains the complete owner-DLL and Phase175 native verification. There are no full-function code bytes in the public repo.

## Original array growth, two distinct stride variants

`0x2228E200` operates on a dynamic array subobject with `+0` allocation pointer, DWORD `+4` capacity, DWORD `+8` growth quantum or disabled sentinel `-1`. It is called from Phase175 symbol free-list allocator at `0x2228E63C`.

- `-1` growth quantum immediately returns with no mutations and no allocation.
- The original calculates a **modular 32-bit** target `capacity + requested_growth`, but compares `capacity` against that target using signed x86 `JGE/JL`.
- While the capacity is signed-less than the target, initial zero capacity becomes `3`; otherwise if the growth quantum is nonzero add it; if it is zero, double the capacity. Native code does not detect wraparound/nontermination.
- Multiply final capacity by **12 bytes**, then **always** invoke native `0x2226E400` to reallocate an existing pointer, or `0x2226E390` to allocate from null, *even when the old capacity is already sufficient*. Store returned pointer without an original null check.
- Model safety departure: a finite iteration guard raises if the native grow loop would not converge.

`0x2228E280` has exactly the same high-level branching but differs in **initial capacity 8** and **4-byte record stride**, including byte-size multiplication and allocation/reallocation. It is a distinct original function, not an inferred C++ template specialization. Both whole native code hashes are independently verified.

## Native symbol insertion traversal `0x2228E470`

This function is **not** the previous Phase174 exact-match tree search `0x2228EB40`. It receives a search key pointer, output 16-bit last index pointer, output direction byte, and opaque comparison argument. It reads its root WORD at object `+0x10` and an indirect comparator at object `+0x00`.

1. Write `0xFFFF` to the output index and `0` to the direction byte before checking the root. On empty root, return immediately with no comparator and without initializing the global sentinel.
2. For each current 16-bit index, store it to the output index; obtain current record from object DWORD `+0x04` with **12-byte stride**, and pass a pointer to record payload at **`+8`** to the comparator alongside search key and context.
3. When indirect comparator returns nonzero in `AL`, write direction byte **1** and move to node WORD **`+0`**. Otherwise write byte **0** and move to node WORD **`+2`**. Loop until next index is `0xFFFF`.
4. Initialize native global sentinel exactly once via guard bit0 at `0x22590134`; original words `0x2259012C`, `+2`, `+4` become `0xFFFF`, `+6` becomes `1`. The branch is reached after the first comparator, not on empty root. As in the other original tree routines, the guard is set before writing all four sentinel WORDs.
5. Original has no bounds or cycle checks. The host-neutral model guards invalid indices/cycles rather than dereferencing arbitrary memory. The comparator algorithm and original object allocation remain external engine callbacks.

Caller edge `0x2228F72B -> 0x2228E470` is checked against original binary, connecting this insertion traversal directly to the Phase174 symbol registration pipeline.

## Native storage reset `0x2228E5B0`

- Store zero at state DWORD `+0x0C` **before** checking the capacity/allocator guard DWORD `+0x08`.
- If guard equals `0xFFFFFFFF`, preserve current allocation and capacity; copy allocation pointer `+0` into cached pointer `+0x10` and return.
- Otherwise free nonnull allocation through exact native `0x2226E3E0`, clear allocation pointer, zero DWORD capacity `+0x04`, then store current (possibly null) allocation pointer into cached `+0x10`.
- Does not overwrite guard. This function is not proven to be a full C++/Source engine destructor.

## Code, tests, limits

- `include/sin_compat/original_server_symbol_growth_traversal.hpp`: C++17 host-neutral behavior models, explicit engine allocator and comparator callbacks, signed x86 comparisons and modular arithmetic, exact branch order.
- `tests/test_original_server_symbol_growth_traversal.cpp`: GCC and Clang optimized `-Werror` checked; initial/doubled/fixed growth, disabled sentinel, allocator invoked despite sufficient capacity, signed overflow, empty and populated symbol tree, lazy sentinel, per-direction output, guarded invalid index, and storage reset.
- `research/tools/audit_sin_original_server_symbol_growth_traversal.py`: owner-DLL original SHA and opcode/call verifier, chained through Phase175 full original dual-DLL checks.
- `research/tests/test_sin_original_server_symbol_growth_traversal.py`: 10 Python original-free mutation/independent-matrix tests, including 320-case signed capacity arithmetic matrix and native CALL mutation detection. Synthetic opcode fixtures cannot satisfy whole function SHA256s.
- Root CTest target `sin_original_server_symbol_growth_traversal_tests`.
- Original reviewed method atlas **114 -> 118 (68 server, 50 client)**. These are precise reviewed native methods, **not** full game completion percentage. All four conservatively `machine_code_verified` with unresolved allocator/comparator effects.

Owner-only full audit:

```sh
python3 research/tools/audit_sin_original_server_symbol_growth_traversal.py --client /private/client.dll --server /private/server.dll
python3 -m unittest discover -s research/tests -p test_sin_original_server_symbol_growth_traversal.py -v
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --target sin_original_server_symbol_growth_traversal_tests
ctest --test-dir build -R sin_original_server_symbol_growth_traversal_tests --output-on-failure
```

## Remaining work

Native comparator callback from the registered symbol object, real allocator bodies `0x2226E390/0x2226E400`, tree rebalance `0x2228EDF0`, `flak` gameplay physics, engine x86 ABI and working replacement DLL factory/runtime remain unresolved. The models are *not* an independently playable Windows game DLL pair nor a working Android/Quest 3 gameplay port. Prioritize actual playable Windows x86 DLLs first, then Android, then Quest 3. Keep the PR stacked, **draft** and **unmerged**.