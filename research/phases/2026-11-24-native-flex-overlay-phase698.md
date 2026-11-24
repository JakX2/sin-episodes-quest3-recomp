# Phase 698: native BaseFlex and BaseAnimatingOverlay bridge

Fresh inspection of the same pinned owner DLLs: server DT_BaseAnimatingOverlay 0x220f3170 (2), client 0x240bfc55 (2), parent pointers 0x224e88b4 -> 0x2254d11c and 0x242f7420 -> 0x243362a8. Both declare baseclass and overlay_vars, direct offset 0. DT_BaseFlex 0x220f6ef0 / 0x240ce4d0, 7 properties, base pointers 0x224e90dc -> 0x2254d5a4, 0x242f7cb4 -> 0x243374ac.

| Property | Server offset | Client offset | Original server input |
|---|---|---|---|
| baseclass | 0 | 0 | DT_BaseAnimatingOverlay |
| m_flexWeight | 4a8 | 860 | Array3, 64 floats, stride 4, 12 bits ROUNDDOWN 0..1 |
| m_blinktoggle | 5b4 | ac0 | 4-byte integer, 1 bit unsigned |
| m_viewtarget | 5a8 | 824 | vector COORD flag 2 |
| m_vecViewOffset[0] | 37c | e0 | float NOSCALE |
| m_vecViewOffset[1] | 380 | e4 | float NOSCALE |
| m_vecViewOffset[2] | 384 | e8 | float NOSCALE |

Hex offsets. Flex array server constructor 0x220f6e1e, client 0x240ce434. Quantized upper endpoint 4095/4096, multiplier 4096. COORD/NOSCALE unused range normalized. The new role-specific graph composes Phase697 BaseAnimating and Phase696 inventory with the original two intermediate tables. 64 named descriptors live with the graph. Paired native fixture checks raw scalar/vector/array callbacks and lifetime; no actual wire codec or engine registration claim.

The unresolved DT_OverlayVars is NOT a fixed Array3. Original server ctor 0x220f4df7 calls vector helper 0x2225f3b0 for m_AnimOverlay at +0x494, element stride 0x40, max 15, callback 0x220f4180, element table 0x2254d560. Client ctor 0x240c36e9 calls 0x241271b0, field +0x7a4, stride 0x20, max 15, callbacks 0x240c3500 and 0x240c0780, element table 0x24337464. The externally supplied overlay schema must remain live; no invented vector storage, resizing, length negotiation or layer indexing. DT_BaseEntity also remains external. No original executable bytes committed.
