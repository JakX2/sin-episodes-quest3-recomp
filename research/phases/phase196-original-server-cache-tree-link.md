# Phase 196: original server scene/model-sounds tree link wrappers

13 November 2026. New stacked draft on Phase195 PR #248. Private read-only investigation of the owner-provided original SiN Episodes: Emergence 2006 Windows i386 game DLLs. Original DLL/asset bytes and complete original native function bodies must never enter the repository. **Windows PC gameplay first, then Android, then Quest 3.**

## Original binary provenance

Whole original `server.dll` SHA256: `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650` (6,049,792 bytes).
Whole original `client.dll` SHA256: `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d` (3,592,192 bytes). Both match the project provenance. Native entrypoints extracted from exact owner originals:

| Original function | Native VA | Full length | Complete native function SHA256 |
| --- | --- | ---: | --- |
| Scene tree link and rebalance dispatch | `0x221AA890` | 109 x86 bytes | `7df2e686b670494a7683bd72793a5dc87594159ce7916ce8354ae8821cfc1fc5` |
| Model-sounds tree link and rebalance dispatch | `0x221441B0` | 109 x86 bytes | `2430a2e9c122c44c12371b9c63ed7aaeddfe2dd7add4fb42f61a0a914edf2e25` |

These are **two new independently callable original server.dll functions, 218 verified original native bytes**. Their complete 109-byte bodies are byte-for-byte identical after zeroing **only three CALL rel32 displacement DWORDs per body**, at local offsets `0x47`, `0x56` and `0x64`. Full normalized SHA256 `2b1dd748abfd04da2818f807c2ce84be6be4e28aef95c8d92f8ad6b41573d461`. Both have three `ret 0x0C` exits and then exactly three `int3` alignment bytes.

### Native direct CALL evidence

| Call origin | Target | Context |
| --- | --- | --- |
| `0x221ACD36` | `0x221AA890` | Phase193 scene insertion |
| `0x22146F26` | `0x221441B0` | Phase193 model-sounds insertion |
| `0x221AA8D7`, `0x221AA8E6`, `0x221AA8F4` | `0x221A8F20` | Scene rebalance, left/right/root paths |
| `0x221441F7`, `0x22144206`, `0x22144214` | `0x221429D0` | Model-sounds rebalance, left/right/root paths |

The six outgoing balancing calls are native `CALL rel32` and both incoming callers are exact native `CALL rel32`. The internal implementations at `0x221A8F20` and `0x221429D0` have **not** been recovered here.

## Verified branch behavior

Each native tree link helper uses `ECX=this`, three 32-bit stack arguments, and `ret 0x0C`. The first is a new slot index, the second supplies a **16-bit parent index**, and the third is inspected as a **byte** to select a child direction. The native body:

1. Indexes the 24-byte node array at object `+4` using **only the low 16 bits** of the incoming new index. The new node receives left and right child WORDs `0xFFFF` at `+0/+2`, parent WORD at `+4`, and flags WORD `0` at `+6`. Neither key data starting at node `+8` nor the payload pointer at `+20` is modified.
2. If parent is `0xFFFF`, writes the low 16-bit new index into tree root at `+0x10`.
3. Otherwise selects the parent node by `uint16(parent) * 24`. **Nonzero** selector byte writes the new low-16 index to parent's first/left WORD at `+0`; **zero** selector writes it to parent's second/right WORD at `+2`.
4. Regardless of which of the three branches runs, pushes the **full original 32-bit new index** and calls its repository-specific balancing method. The wrapper has no native bounds/cycle checks, no allocation, and no independent balancing logic.

This is now a concrete link between Phase193's insertion wrapper, Phase194's slot allocation, Phase191's tree traversal, and Phase192's exact unsigned first-DWORD comparator. It does **not** establish the balancing algorithm or tree colors beyond the observed zero-initialized WORD at node `+6`.

## Independently authored code and validation

- `include/sin_compat/original_server_cache_tree_link.hpp`: C++17 host-neutral direct branch model for the native tree root and parent-child association; typed 16-bit fields and explicitly injected balance callback, with safe host-only index guards.
- `tests/test_original_server_cache_tree_link.cpp`: root insertion, both child directions, nonzero-selector behavior, original 16-bit indexing versus full-32-bit balancing argument, preservation of payload/key fields, invalid-index guards, integration with Phase191 tree find, Phase192 unsigned comparator, and Phase193 insertion wrapper.
- `research/tools/audit_sin_original_server_cache_tree_link.py`: strict whole original server/client SHA256, both complete original function hashes, 36 sparse native opcode witnesses, 6 exact outgoing helper calls, 2 incoming Phase193 calls, `ret 12` exits and native alignment, whole normalized twin equality and digest.
- `research/tests/test_sin_original_server_cache_tree_link.py`: eight original-free mutation-test groups, including every sparse instruction witness and direct CALL, original function boundary, native address drift, normalized-body disagreement, and deliberately non-original fixtures.
- Root CTest and native Windows Win32 MSVC CTest targets registered.

Native-byte and SHA observations were inspected directly from the exact owner-provided DLLs. New GitHub hosted Windows, C++ and Android CI results must be checked independently at this branch head; a synthetic ABI test does **not** prove original-engine gameplay.

## Scope and next milestone

Unique reviewed original-function atlas advances **187 to 189 (server 95 to 97; client stays 92)**. This is research inventory, not a percentage of runnable game code. The real tree balancing helpers and entire original filesystem interface at byte `+0xD4` remain incomplete. A viable Windows executable-hosted original factory-forwarding proxy test is documented at `docs/PHASE147_PC_WIN32_ORIGINAL_FACTORY_PROXY.md`, but its actual **retail game launch** has never been verified. That owner-only runtime smoke, followed by actual game-interface vtable integration, remains a higher-value delivery milestone than accumulating isolated methods indefinitely.
