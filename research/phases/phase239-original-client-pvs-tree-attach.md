# Phase 239: original 2006 client PVS map tree attachment frontend

15 November 2026. Stacked draft on Phase238 PR #293. This reviews the **owner-original SiN Episodes: Emergence (2006)** Windows x86 client.dll, not an HL2 surrogate.

## Mandatory complete owner-original PE32 provenance

| Original DLL | Entire original DLL bytes | Complete original SHA256 |
| --- | ---: | --- |
| client.dll | 3,592,192 | 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d |
| server.dll | 6,049,792 | 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650 |

The original binary auditor checks both complete owner-original DLL digests BEFORE granting any code recovery credit. Do not commit original retail DLLs, complete proprietary native bodies, whole original disassemblies, or commercial assets.

## One new entire native function SHA review

| Original client entry | Whole original code bytes | SHA256 of whole original code | Original exits | Original alignment |
| --- | ---: | --- | --- | --- |
| **0x241076E0** | **106** | **fd141c9025d7975a43a24b398a6b5bee1f34cead227171e5319c269e78c8df9f** | three RET 12 at +0x4A, +0x59, +0x67 | six trailing INT3 at 0x2410774A..0x2410774F |

**Reviewed original method catalog: 343 → 344 (server 147, client 196 → 197). +106 whole-original-SHA-reviewed native x86 bytes.** This is a code review inventory, not a percentage of the full DLLs or a runnable gameplay DLL.

### Original x86 method behavior

Thiscall ECX is the original 16-byte map manager. Three stack arguments are new node WORD index, parent WORD index, and BYTE direction; native stack cleanup is **RET 12**. The method:

1. Computes the newly allocated node address using **map record storage DWORD+0x04 + node_index*16**.
2. Writes new record **parent WORD+0x04**, **right WORD+0x02 = 0xFFFF**, **left WORD+0x00 = 0xFFFF**, and **state/color WORD+0x06 = 0**. The function does NOT copy the eight-byte key payload at record+0x08.
3. If parent==0xFFFF, sets manager **root WORD+0x10 = new index**.
4. Otherwise uses parent record address (same 16-byte stride) and the third stack argument's side BYTE: nonzero writes parent.left WORD+0; zero writes parent.right WORD+2.
5. **On each of the root, left and right paths**, PUSHes new index and calls original **0x24106E30**, the tree-balancing routine, then returns via RET 12.

Critically, Phase236's original insertion frontend **0x24108750** calls this tree-attachment frontend before its own copies of the eight-byte payload at record+0x08. This is why the Phase239 regression observes the prior payload inside the balancer callback for a recycled index and the new payload only after the frontend returns.

The native tree balancer's body **0x24106E30** is substantially larger and NOT recovered in this phase. Our separate C++17 **BalancingBackend** is an explicitly synthetic injected callback that observes attachment and may implement host-owned balancing later; a no-op spy is not a native algorithm. Original x86 does not validate indices; the host model adds fail-closed bounds and self-parent guards. Original state/color WORD is represented as the upper half of the synthetic record's existing DWORD+0x04; do not conflate that with real engine-owned object layout.

### Four exact original E8 rel32 CALL edges

| Native CALL instruction | Native target | Role |
| --- | --- | --- |
| 0x24107724 | 0x24106E30 | left-branch tree balancing |
| 0x24107733 | 0x24106E30 | right-branch tree balancing |
| 0x24107741 | 0x24106E30 | root-branch tree balancing |
| 0x24108786 | 0x241076E0 | existing Phase236 insertion frontend invokes attach |

Auditor independently authenticates **one complete original native code body SHA256**, **19 sparse original opcode witnesses**, **all three original RET12 exits**, **all six trailing INT3 bytes**, and **all four direct E8 rel32 call destinations**, after the two entire retail original DLL SHA gates.

## Added code and integration tests

- research/tools/audit_sin_original_client_pvs_tree_attach.py: exact whole-retail-DLL SHA gate, complete original native method SHA, 19 independent sparse x86 witnesses, RET12, INT3 boundary, four native CALL relationships. Synthetic fixtures cannot receive original-method credit.
- research/tests/test_sin_original_client_pvs_tree_attach.py: six synthetic-only test groups that flip native instruction witnesses, every return operand, padding, all five bytes of every original E8, PE32 base and code-section boundaries.
- include/sin_compat/original_client_pvs_tree_attach.hpp: C++17 original root/left/right attachment model deriving from Phase237's attachment interface, with injected balancing callback, original node-header layout and host safety checks.
- tests/test_original_client_pvs_tree_attach.cpp: root/left/right branches, native fields, exactly-once balancing callback, no premature payload copy, invalid/sentinel node protections and a **Phase236+237+238+239 integrated grow→insert→lookup→release→reuse→reinsert** synthetic regression. The final callback witnesses the recycled node's old payload before Phase236 copies its new eight-byte key.
- Root CMakeLists.txt and pc/windows_x86_proxy/CMakeLists.txt: portable C++17 and genuine MSVC Win32 x86 CTests, not the original retail engine executing rebuilt gameplay modules.
- research/catalog/original_dll_verified_methods.json: one new original SHA-reviewed client function, taking the total to 344.

Private original retail auditor:

    python3 research/tools/audit_sin_original_client_pvs_tree_attach.py --archive /private/bin.zip

Synthetic-only evidence mutation tests:

    python3 -m unittest discover -s research/tests -p test_sin_original_client_pvs_tree_attach.py -v

## Next genuine recovery dependencies

The original tree-balancing function **0x24106E30** and original removal/rebalancing **0x241083C0** are not reconstructed. Likewise the original allocator internals and original game engine-owned runtime classes are not executable in a rebuilt gameplay module yet. Proxy forwarding, synthetic CTests and Android ABI loading are not proof of the original game running without proprietary client/server gameplay DLLs.

Platform order remains Windows PC original retail engine → Android ARM64 → Quest 3 VR.
