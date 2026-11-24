# Phase 699: native dynamic animation layer vector graph

Original server SHA-256 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650 and client SHA-256 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d freshly confirmed from owner archive.

DT_Animationlayer server constructor 0x220f37cb, client 0x240bfb9d. DT_OverlayVars server 0x220f4df7, client 0x240c36e9. Vector factories 0x2225f3b0/0x241271b0 create a direct parent m_AnimOverlay at offset zero, 16-child _ST_m_AnimOverlay_15, and a first lengthproxy datatable with one lengthprop15 in _LPT_m_AnimOverlay_15. The remaining children 000..014 reference DT_Animationlayer. Their index is stored in SendProp +0x24 / RecvProp +0x30 (stride), not offset. Server child flags 0x200. Original format literals in server 0x22453c30/3c48/3c54 and client 0x24298c24/8c3c/8c48.

| Field | Server offset | Client offset | Send encoding |
|---|---:|---:|---|
| sequence | 0x08 | 0x00 | unsigned 10 |
| cycle | 0x0c | 0x14 | float 15 ROUNDDOWN 0..1 |
| prevcycle | 0x10 | 0x04 | float 15 ROUNDDOWN 0..1 |
| weight | 0x18 | 0x08 | float 8 0..1 |
| order | 0x34 | 0x0c | unsigned 4 |

Server header at entity+0x494, stride64; client +0x7a4, stride32. 20-byte x86 vector header pointer/capacity/grow/count/elements. Server length proxy 0x2225f2f0 caps logical count at 15, reserve table proxy 0x2225f2d0 invokes 0x220f4180, which ensures allocation (0x220f3960) then initializes dormant slots (0x220f2f40), without changing count. Child proxy 0x2225f370 indexes allocated memory. Client length proxy 0x24127100 delegates to 0x240c3500, updating both the layer vector and parallel interpolation vector (+0x7b8, stride0x44) with unregister/rebind; reserve 0x24127120 calls 0x240c0780/0x240c0270 and zero-initializes dormant slots. Child proxy 0x24127180 indexes allocated memory. Original pointer arithmetic does not validate capacity or malformed indices.

Port-owned native descriptors implement this hierarchy with explicit host reserve/resize operations (allocation, construction, interpolation side effects remain host responsibilities). Reject malformed/failed callbacks rather than forwarding unsafe addresses. Tests use host-owned native headers and storage, exercise reserve without changing count, length change/no-change/shrink, 15*5 raw proxy transfers and rejection of invalid length/index, composition with actual animation/flex graph, and lifecycle. Not yet original entity allocation, actual wire encoding or engine registration. BaseEntity remains external. No binary bytes committed.
