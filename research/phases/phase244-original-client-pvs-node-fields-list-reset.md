# Phase 244: original client PVS node fields and auxiliary list reset

16 November 2026. Stacked draft on Phase243 PR #298. Original 2006 SiN Episodes: Emergence retail PE32 DLL provenance:

- client.dll: 3,592,192 bytes; SHA256 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d
- server.dll: 6,049,792 bytes; SHA256 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650

No proprietary DLLs, complete original native method bodies or disassembly dumps are committed.

## Eight NEW authenticated complete original x86 functions

| Original client VA | Role | Entire native bytes | Complete original SHA256 |
| --- | --- | ---: | --- |
| 0x24105D10 | map payload pointer, base+index*16+8 | 18 | c1940e69b4f114920d06d56700083f2cd9edaf9d6145fdd2020485eb2ad84188 |
| 0x24105DD0 | 8-byte auxiliary list free-chain reset | 81 | be605e319bc1bc6c2ac86feea8b724e3b9d051d4f59dc4752deb527031d106a9 |
| 0x24105E80 | separate 16-byte linked-list active predicate | 46 | 89b9061da55ddeadc3179e6ee9c9235b1e75ba17b5c773920d0472f810078763 |
| 0x24105EE0 | 16-byte map node parent WORD+4 setter | 24 | b1c6c757afdf5106f71ba3452c69c46ee0713d822bcc6ebb238132c6e7eafbfb |
| 0x24105F00 | 16-byte map node left WORD+0 setter | 23 | 49174d895e56856ee7877c8eab60c667cf613ea4254052d7f20067fd7bb510f7 |
| 0x24105F20 | 16-byte map node right WORD+2 setter | 24 | ed3757cb13ea83780882d9e45819bb069f2c58ae86a7750d2371fcb85765403b |
| 0x24105FA0 | map node color WORD+6 getter via native resolver | 17 | d7b08a570e5450bdaf0fc9aaadf9ebaf09d07d7701b79e5cadf1e4be2ed21c75 |
| 0x24105FC0 | map node color WORD+6 setter | 24 | ed5b194512b3e405f7b96a020481c590fc1a29f87c59fb1b1be4ae80ddf135ef |

**Reviewed full-original-SHA catalog 353 → 361: server 147, client 206 → 214; +257 complete original x86 bytes.** This is not a recovered percentage or proof of game DLL load. Original-only audit gates on both full DLL hashes, all eight whole native function SHA256s, 38 sparse original opcode witnesses, nine original return paths, trailing INT3 boundaries and three original direct E8 rel32 edges.

## Critical native layout separation

Three distinct original native structures are involved; they are **not** interchangeable:

1. **Eight-byte auxiliary list, reset at 0x24105DD0**. Manager base pointer DWORD+0, active head WORD+0xC, tail WORD+0xE, free head WORD+0x10, live count WORD+0x12, capacity WORD+0x14. Each node has payload DWORD+0, prev/self WORD+4, next WORD+6. For capacity greater than zero, reset builds the free chain from index 0 to last, with prev=self, next=index+1 or FFFF, free head 0, active head/tail FFFF and count zero. Capacity zero returns **without changing fields**. The original does not clear payload DWORDs.
2. **Sixteen-byte auxiliary list, active check at 0x24105E80**. Manager capacity WORD+0x14; each record has prev WORD+0xC and next WORD+0xE. A record in range is inactive only when **both** links equal its own index. Out-of-range index is inactive. A node with both links FFFF is considered active unless its index is also FFFF, which is out of range.
3. **Sixteen-byte red-black PVS map nodes**. Manager storage pointer DWORD+4, left child WORD+0, right WORD+2, parent WORD+4, color WORD+6 and stored key payload +8..+15. The four native setters accept arbitrary 16-bit values; host models bound-check only the node index. This map is not an auxiliary list.

Native payload-address helper returns a pointer to record+8. C++ host returns a checked byte offset, never a fabricated original game process pointer. Native color reader at 0x24105FA0 calls the previously SHA-reviewed original node/sentinel resolver 0x24105F40; nil index FFFF returns global sentinel color black=1.

## Authenticated direct original E8 call edges

| Original site | Native target | Role |
| --- | --- | --- |
| 0x24105FA5 | 0x24105F40 | color getter → native node/nil resolver |
| 0x241069B3 | 0x24105DD0 | auxiliary manager initialization reset |
| 0x241074A3 | 0x24105DD0 | containing-object teardown/reset call |

## New source and regression tests

- research/tools/audit_sin_original_client_pvs_node_fields_list_reset.py: original-only dual full-DLL SHA gates, eight complete method SHA digests, 38 opcode witnesses, native RET/INT3 boundaries and three E8 edges.
- research/tests/test_sin_original_client_pvs_node_fields_list_reset.py: synthetic-only mutation regressions. Synthetic bytes never earn original-method credit.
- include/sin_compat/original_client_pvs_node_fields_list_reset.hpp: C++17 independent host reference for distinct eight-byte list free reset, 16-byte list active predicate, indexed map field writes, color read and payload byte offset.
- tests/test_original_client_pvs_node_fields_list_reset.cpp: zero-capacity no-op, varied free-chain lengths with payload preservation, 16-byte active self-marker semantics, map parent/child/color writes and integration with Phase240 tree rotations.
- Portable Linux and genuine MSVC Win32 x86 CTests registered; reviewed method catalog updated.

Run the original owner-private audit:

    python3 research/tools/audit_sin_original_client_pvs_node_fields_list_reset.py --archive /private/bin.zip

The original Windows engine's full entity-list object lifetime, global CRT initialization, real game allocator and independent executable client/server gameplay DLL replacement remain unresolved. Platform priority is **retail Windows PC engine first → Android ARM64 second → Quest 3 VR third**.
