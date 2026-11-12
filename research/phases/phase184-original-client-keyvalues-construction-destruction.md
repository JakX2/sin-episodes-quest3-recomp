# Phase 184: Original 2006 client KeyValues construction and recursive destruction

12 November 2026. Stacked above draft Phase183 PR #236 in the independent *SiN Episodes: Emergence* DLL recovery project. This phase uses the owner's original Windows i386 `client.dll` and `server.dll`; their original bytes and complete disassembly are **not** included in the public repository.

## Exact original executable provenance

- Original `client.dll`: 3,592,192 bytes; SHA256 `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`; PE32 i386 image base `0x24000000`.
- Original `server.dll`: 6,049,792 bytes; SHA256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`.
- Three entire original client native x86 functions, **231 original bytes**, matched against their complete native-region SHA256s:

| Native VA | Complete bytes | Original full-function SHA256 | Role |
|---|---:|---|---|
| `0x24165010` | 61 | `f87d6d3f9161a19024319d7e4e2bfd5cef780bd85850b523ab4807d95844a8df` | KeyValues `thiscall` name constructor |
| `0x24165050` | 35 | `03b60f2d8adfedcfced3fc591eb1e8c1f07df082d727e21dbafbf4958e99865d` | MSVC scalar deleting destructor wrapper |
| `0x24166620` | 135 | `8aa139f82c4f1499a950f2782e457327f5d9d95749a340027bbef554b1788c6e` | Recursive KeyValues contents/tree destructor |

The native byte checker also pins **40 disjoint original instruction witnesses**, **five exact original internal x86 CALL rel32 edges**, and **two independently verified incoming caller edges** from previously researched functions. All data is drawn from the *actual original DLLs*, not Source SDK assumptions.

## Name constructor `0x24165010`

Original function receives `this` in `ECX` and a name pointer on the stack; returns the original `this` pointer and uses `RET 4`. It first writes name symbol sentinel DWORD `0xFFFFFFFF` at `+0`, then clears DWORD fields `+0x10` (tag), `+0x1C` (first child), `+0x18` (peer), `+0x20` (fallback), `+4` (narrow buffer), `+8` (wide buffer), `+0x0C` (numeric payload), and byte `+0x24` (flag), in that exact relative order. It calls the KeyValuesSystem factory stored in import slot `0x24276230`, invokes the service **virtual byte offset `+0x0C`** with the caller name, and stores the returned interned name DWORD at `+0`.

Original code does **not** initialize DWORD `+0x14` or bytes `+0x25..0x27`. The reference preserves these rather than silently initializing undocumented fields. The entire native record is **0x28 bytes** according to previous original allocations, but only the observed fields have assigned names.

The constructor is directly called from the Phase181 original manifest routine at `0x241927BB`. KeyValuesSystem's real factory, symbol intern table, and virtual methods remain outside this phase.

## Deleting destructor `0x24165050`

Original MSVC x86 thiscall wrapper calls `0x24166620` on `this` unconditionally. It then checks the low bit of the stack deletion flag. Only for `flag & 1` does it call the original KeyValuesSystem factory via `0x24276230` and its virtual **`+0x08`** node-release method with the instance pointer. It returns the original `this` pointer via `RET 4` even on a free branch. This is not proof that it is safe for a host to dereference that returned pointer afterward.

## Recursive contents destructor `0x24166620`

This function visits its current node's **first-child list** (`+0x1C`) and then the current node's **peer list** (`+0x18`). For each owned entry it saves the next-peer address, clears that entry's peer DWORD (`+0x18`), recursively calls the same routine, and releases the child/peer node through the KeyValuesSystem service's **virtual byte offset `+0x08`**. On the peer traversal the original compares each candidate to `this` and stops when equal; this guard is present in the original instructions, not an invented host policy.

After both traversals it calls original routine `0x2416FD60` **twice**, once for the node's narrow string at `+4` and once for its wide string at `+8`, clearing each buffer field to zero. The calls are made even if the buffer address is null. The original code **does not clear the current node's child or peer pointer at the end** and does not touch `+0x20` fallback or the interned name identifier.

Exact calls checked:

| Native call site | Actual target | Meaning |
|---|---|---|
| `0x24165053` | `0x24166620` | Deleting-destructor calls contents cleanup |
| `0x2416663F` | `0x24166620` | Recursion from child list |
| `0x24166670` | `0x24166620` | Recursion from peer list |
| `0x24166689`, `0x24166697` | `0x2416FD60` | Two native original string frees |
| `0x241927BB` | `0x24165010` | Original surface manifest entry constructs KeyValues root |
| `0x24166057` | `0x24166620` | Phase181 original cleanup bridge |

KeyValuesSystem calls use the original imported pointer and vtable slots; those private functions have **not** been decompiled here. Do not confuse the KeyValues name-symbol store with the distinct entity classname registry recovered in prior server phases.

## Implementation and proof

- `include/sin_compat/original_client_keyvalues_lifetime.hpp`: typed 40-byte original MSVC x86 field snapshot, a constructor/initialization reference retaining untouched bytes, and a separate portable heap graph model for original destructor and deleting-destructor decisions. It uses injectable engine callbacks, not hard-coded 64-bit host pointers as native game addresses.
- `tests/test_original_client_keyvalues_lifetime.cpp`: asserts original field initialization order, untouched fields, nested child then peer cleanup, original two frees per node, low-bit delete flag behavior for flags 0 through 3, and original peer-self guard. Compiled and passed optimized GCC and Clang C++17 `-Wall -Wextra -Wpedantic -Werror -UNDEBUG`.
- `research/tools/audit_sin_original_client_keyvalues_lifetime.py`: owner-only full client/server SHA256 gate, original PE32 section mapper, exact three native function hashes, 40 instruction witnesses, five internal and two incoming CALL edges.
- `research/tests/test_sin_original_client_keyvalues_lifetime.py`: **11 passing original-free synthetic mutation/regression tests** covering each native opcode and each call, entire-region hash rejection on synthetic fixtures, malformed PE rejection and layout checks.
- CMake/CTest native test target plus Windows Win32 x86 proxy-project MSVC CTest integration.

**Explicit portable safety differences:** The host model detects cyclic/shared-node input and excessive depth, and clears the *host* node's child/peer pointers after reclamation so its stale pointers are not accidentally traversed. The original x86 function does not perform those protections/clears. The host callback representing `free` is not the real KeyValuesSystem allocator and must not be treated as interchangeable with the engine's memory management.

## Next steps and limits

1. Reconstruct the actual original KeyValuesSystem interface factory and node allocator/free ABI, rather than guessing from a newer SDK.
2. Recover original client `0x2416FD60` buffer free and the KeyValuesSystem symbol intern routine and compare the live service vtable layout.
3. Connect original `CreateInterface` and engine lifecycle into executable Windows x86 replacement game modules, run under the legally owned original Windows game engine, and demonstrate map/player/weapon/HUD; only then attempt Android and Quest 3.

**This is not a playable Windows `client.dll`, Android game or Quest 3 port.** A whole-function hash proves code provenance and boundaries; the tested callback models reproduce **only** verified behavior and are not binary-compatible drop-in replacements.