# Phase 1590: original Recombinant Major burn response

Fresh direct disassembly of exact original server SHA256 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650. Major vtable +370 22379100, +2d4 22379ec0. Reject incoming source handle +24 resolving to classification10. Burn bit8 when state +bf&8 rejects unless flags include 0x20000000. Accepted bit8 invokes color helper 220f2900(5,50). 22379050 checks state again, accumulates amount +2c into actor+c3c, stamps +c40=now+5, calls Ignite(100,1,0,0,0,0) if accumulated >=0.1*max health (+98), then parent damage 22342cb0. 22379ec0 calls inherited Ignite first, reads health +9c as float, draws [-2,2], computes (draw+10)*5. If strictly above snapshotted health keep live health; otherwise second independent draw and truncate result (can raise health). Notify +1b4 before changed health write. Activity6/10/1 invokes pending112/113/111. Preschedule prefix 22379dd0 clears accumulated value at nonzero deadline <= now, not the deadline itself. Subsequent custom preschedule branches remain unimplemented; no generic engine burn/health/color or physics recreated. Native loaded tests cover exact equality, stale source, classification immunity, second draw, output order, expiry, type flags and activity changes.

Regions VA / bytes / SHA256:
22379100/153/06dbffd83e32e2bfeb1692df0f9402572f6a59ff8bee6172cdad054326cce85e
22379050/91/b245aa8ddcf6e28f516c89f96a124fc28fd2f075dbbedcc52932f8ca64ff7503
22379ec0/270/f80ff6841467cb0656c2eb4bbcf91f034aa03c4a44c86b7b2d3c106c9d088415
22379dd0/237/05a35ad8274e4f409882b0d62a9a9424f9e311a2a237db59bb34c89170db0a34
2249f628/4/80c8a717ccd70c8809eb78e6a9591c003e11c721fe0ccaf62fd592abda1a5593
223da4a8/4/fca31f1667a6aa1bba12fca4e4ea1becd503379d80da3213af07f6cc5702828d
223d8fe8/8/45d2b662d9d490ae9b932759c910b26b9a874065de620f4ac0a3a23a65e40b8c
224a10fc/4/ff3e30f656e28ef05aa7e5d040fd44a3a110abf7bfd3c0077d7bd2afa0b341bf

No original bytes committed. CI pending.
