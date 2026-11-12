# Phase 191: original server cached-repository 16-bit tree lookup pair

12 November 2026. Research stacked on Phase190 draft PR #243. Owner-original SiN Episodes: Emergence (2006) PE32 files privately inspected. **Windows PC gameplay first, Android second, Quest 3 third.** Do not commit commercial DLLs, extracted assets, or complete x86 disassemblies.

## Native provenance

- Original `server.dll`, 6,049,792 bytes, complete SHA256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`.
- Original `client.dll`, 3,592,192 bytes, complete SHA256 `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.
- Scene cache key lookup **`0x221A7FB0`, 260 native x86 bytes**, complete function SHA256 `8e6977a45630ac69cadd8b7597f752bdd24946981832ffc1e5eaa31274661038`.
- Model-sounds cache key lookup **`0x22142670`, 260 native x86 bytes**, complete SHA256 `8cc7e151e2ee8b7fcc7c8202b6a880b8542969c7a5bbeb424cbcc89d9b79a912`.

Both native regions end after two `ret 8` exits and before `int3` alignment. They are distinct original callable functions, not copied ledger entries. The original Phase190 record identity helpers call these exact lookup entrypoints.

**Strong twin-body finding:** After zeroing only **14 embedded four-byte absolute addresses** in each 260-byte region, the complete normalized native method bodies are byte-for-byte equal. Full normalized SHA256 `9dc7a270eed6024f374820355e3097d25f96d64547b3c38d29d25c502732ec41`. The only differences are independent static tree sentinel global addresses. Both entire unnormalized original bodies are hash-pinned.

## Exact observed lookup flow

1. Original object field `+0x10` is a 16-bit root index. `0xFFFF` is the empty/missing sentinel. An empty root immediately returns `AX=0xFFFF` without invoking the comparator.
2. Native object `+0` is an indirect comparator **function pointer**, called through `call dword ptr [edi]`, with two stack arguments. There are two comparator call sites in each native method.
3. Original object `+4` points at a contiguous array of **24-byte indexed nodes**. Node `+8` is passed **by address** as the key operand. The query pointer is passed from the first native stack argument. The exact key representation and comparator semantics are still opaque.
4. If `compare(query, nodeKey)` returns nonzero, native descends to the 16-bit child index at node `+0`. Otherwise, if `compare(nodeKey, query)` returns nonzero, it descends to node `+2`. If neither comparison is true it returns the current node's index in `AX` (match by comparator equivalence, not inferred byte equality).
5. Both descent branches lazily initialize a module-static sentinel record **once**. The scene pair is `0x2256E1F0` (8-byte record) with flag at `0x2256E1F8`; model-sounds uses `0x22558D50` / `0x22558D58`. The first three 16-bit sentinel words become `0xFFFF` and the last becomes `1`. Native globals are in virtually allocated `.data` and need not have file-backed bytes.
6. Descent repeats until the next 16-bit index equals `0xFFFF`, then returns missing. Original code has no host-bound vector protection, no cycle guard and no deep-validation check.

Original x86 method uses `ret 8` (two caller-passed stack slots), while this particular body reads the first argument as a pointer to query data. No native comparator class or C++ symbol name is asserted.

## Source and verification

- `research/tools/audit_sin_original_server_cache_tree_lookup.py`: strict double-DLL SHA256 gate, two full native function SHA256 checks, **72 sparse instruction witnesses**, four indirect comparator CALLs, four `ret 8` exits, **28 specific 32-bit native global addresses**, both native boundary markers, exact whole-body normalized comparison and normalized hash.
- `research/tests/test_sin_original_server_cache_tree_lookup.py`: eight original-free mutation groups individually corrupting witnesses, sentinel references and indirect compare/return and boundary evidence. Negative synthetic full-hash tests prevent conflation of fixtures with original code.
- `include/sin_compat/original_server_cache_tree_lookup.hpp`: host-neutral C++17 tree and 24-byte slot-layout reference. Keys are synthetic `uint32_t` comparison values, not original-record format claims. `std::vector`, bounds and cycle guards are host safety substitutions.
- `tests/test_original_server_cache_tree_lookup.cpp`: tests both independent sentinel global identities, empty and matching root, less/greater branches, missing keys, once-only sentinel initialization, cyclical and out-of-range synthetic data.
- Root CTest registration and MSVC Win32 x86 CTest registration in `pc/windows_x86_proxy/CMakeLists.txt`.

**Local verification:** SHA-pinned native audit passes against exact owner-original binaries. Optimized GCC/Clang C++17 `-Wall -Wextra -Wpedantic -Werror -UNDEBUG` test binaries pass. Eight Python original-free mutation test groups pass. Hosted MSVC Win32 CI has not yet been independently observed.

## Scope and next work

Reviewed function atlas **177 -> 179 (server 85 -> 87, client 92 unchanged)**. No game-wide percentage is implied. The original tree comparator, key format, allocator, tree insertion and cache record decoder remain incomplete. The actual Windows engine `CreateInterface` / `IFileSystem` and replacement DLL load/playability are still unproven. Next useful target: recover the comparator function pointers and their x86 call signatures from the original tree initializer and the original record key payload, or perform a controlled owner-only Windows original-engine ABI probe. No artificial claims of a playable build.
