# Phase 198: four original scene/model-sounds cache-tree rotations

13 November 2026. **Stacked draft on Phase197 PR #250.** This report describes binary-grounded research using the owner's exact, privately held *SiN Episodes: Emergence* (2006) original Windows i386 PE32 `server.dll` and `client.dll`. The original game binaries, assets, complete disassemblies and complete original native machine-code bodies are not committed to this repository. Windows PC first, Android next, Meta Quest 3 last.

## Complete original-function provenance

Both private originals match the existing full-file SHA256s:

- `server.dll`, 6,049,792 bytes: `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`
- `client.dll`, 3,592,192 bytes: `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`

| Actual native method | Entry VA | Complete original x86 length | Complete original method SHA256 |
| --- | --- | ---: | --- |
| Scene-cache left rotation | `0x221A6BB0` | 237 | `418e0affabf3ad24336f65de55a333ad822b4e486cfaeff9324dd44208b6ef93` |
| Scene-cache right rotation | `0x221A6CA0` | 240 | `5c19981d1f75b91c07cd390555412d0861bbc9843c5d1a7ed90e6c780b05aa53` |
| Model-sounds left rotation | `0x221409B0` | 237 | `d4e2f2b7fdf594c9c344511b8627451dbef34d0937112b6c70d54033c9e94214` |
| Model-sounds right rotation | `0x22140AA0` | 240 | `3b64fe5f8546d860f24d029ac9faf813a53adfd61b73c72719db8b8bb6a0e4ed` |

These are **four new complete original x86 functions totaling 954 bytes**, beyond the prior 191 function entries. Both left methods normalize to byte-for-byte identical machine code after zeroing **only nine resolver CALL rel32 displacements in each original body**. Full normalized left-body SHA256: `791ffa6910dfd63dcced4bb931e17c28c1288667106f27b808966cedef95eed9`. The same normalization of the two right methods gives full normalized SHA256 `ed7fc558c70d7259b710518c668610195b6e4335684cd746f991239c891f2717`. The original left/right methods are **different**, mirror-image algorithms and must not be conflated.

Each original function ends with `ret 4`, taking the tree via ECX and one 32-bit stack node-index parameter (whose low 16 bits index the 24-byte nodes). The left methods are followed by three original INT3 padding bytes. The right methods end directly before different subsequent original function entrypoints with independently pinned instruction prefixes.

## Verified native structural behavior

Native tree object field `+0x04` points at 24-byte indexed nodes; root WORD is `+0x10`. Each node has:
- left child WORD `+0`, right child WORD `+2`, parent WORD `+4`, color WORD `+6`;
- record key starts at `+8`; the Phase190 insertion callback ultimately owns payload DWORD `+20`.

The new disassembly supports the **standard rotation pointer rewrites**, without inventing record semantics:

**Left rotation at x:** Resolve y = x.right and beta = y.left. Assign x.right = beta. If beta is not `0xFFFF`, set beta.parent = x. Set y.parent = x.parent when the old parent exists; replace x by y in its old parent's left/right child link, or set root = y when x was the root. Assign y.left = x and x.parent = y. The original implementation calls its native node resolver `0x221A4D90` or `0x2213E990` during these operations.

**Right rotation at x:** Mirror the above: y = x.left, beta = y.right, assign x.left = beta; if beta exists set beta.parent = x; set y.parent = x.parent; redirect the root or parent child pointing at x to y; finally assign y.right = x and x.parent = y.

Both functions preserve node colors, key/payload data, and the unchanged nonrotated child. Rotation does **not** alone blacken the root or rebalance/recolor the tree. The original native methods themselves have no reliable array bounds/consistency guards. The separately authored host reference intentionally checks topology and all indexes **before mutation** and raises on invalid trees instead of emulating native memory corruption. It does not reproduce the exact number/timing of resolver calls or the Phase195 resolver's lazy sentinel-init side effect.

## Precise original function call evidence

- Left scene method `0x221A6BB0` makes **nine** exact native `CALL rel32` dispatches to `0x221A4D90`; left model-sounds `0x221409B0` makes nine to `0x2213E990`. Their local CALL offsets: `0x0B,0x17,0x36,0x45,0x62,0x82,0x8E,0x9B,0xB0`.
- Right scene `0x221A6CA0` and model-sounds `0x22140AA0` make nine to the corresponding resolver, at `0x0B,0x16,0x35,0x45,0x63,0x83,0x8F,0x9D,0xB3`.
- The original Phase197 balancers call each direction twice. Scene: from `0x221A8F20+0x34C/+0x541` to left and `+0x38A/+0x503` to right. Model-sounds: from `0x221429D0+0x34C/+0x541` to left and `+0x38A/+0x503` to right.

The auditor validates **36 original outgoing resolver calls and 8 original incoming balancer calls**, four complete native-function SHA256 hashes, both complete normalized-mirror identities/hashes, **72 sparse instruction witnesses**, all native return instructions and original method boundaries.

## Independently implemented code and tests

- `include/sin_compat/original_server_cache_tree_rotations.hpp`: reusable C++17 left/right rotation routines using the existing Phase191 native 24-byte slot and low16 parent layout; fixed-address scene/model-sounds configs. Mutations preserve color WORD at `+6`, all keys and payloads.
- `tests/test_original_server_cache_tree_rotations.cpp`: root promotion and inverse rotation round-trip, non-root promotion through either side, beta-child parent movement, original raw32-to-low16 index behavior, in-order key preservation, unchanged color/record data, and host-only fail-before-write cases for invalid pivot, promotion, beta, or parent.
- `research/tools/audit_sin_original_server_cache_tree_rotations.py`: original SHA-gated strict code verifier, including the private whole-DLL hashes and all four entire original function hashes and cross-phase CALLs.
- `research/tests/test_sin_original_server_cache_tree_rotations.py`: synthetic witness corruption tests (eight unittest groups), rejecting altered original opcodes, calls, method boundary, normalization, fake whole-DLL and complete-function hashes.
- Root CTest and Windows native x86 MSVC CTest targets are registered. Hosted results should be checked on this exact draft head before claiming they pass.

Original functions have been verified locally against the SHA-matched DLL bytes and all mapped native callsites, including the non-INT3 right-method boundaries. The tests and auditor are independent of the original proprietary game data for public CI.

## Scope and next delivery milestone

Reviewed original functions **191 -> 195**, comprising **103 server and 92 client**, not an overall decompilation-completion percentage. Phase197's **full red-parent balancing and recoloring loop remains unrecovered**, even though its four actual rotation helpers now have a source-grounded portable pointer-update model.

Next priority is reconstructing that Phase197 fixup control flow with strong invariants and synthetic insertion/find tests, plus running an **owner-only, reversible retail Windows executable startup trace** via the existing original-DLL forwarding proxies. Neither the original Windows game using recompiled SiN modules nor Android/Quest 3 gameplay has yet been proven. Keep the DLL first, Android second, VR third strategy.
