# Phase 1594: original Recombinant Minor touch transitions

Direct disassembly of authenticated original server.dll SHA256 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650. Original touch markers 2237c980 and 2237ca30 are values in actor+7c, not callable pointers. First handler clears+c41, runs 22256b20 (other+218 flags&4 -> local-player release operation), then if other+94 and +9c nonzero stores original handle in +c30 and invokes 2237af10. That helper checks schedule82(false), clears68 and returns if already matching; otherwise sets68, sets zero velocity and movement(0,0), schedules current think. Main handler clears touch+7c and installs default think22044fa0(0,0). Nonliving contact clears touch if grounded or, after now>=+c38, if other+11c nonzero and +120 excludes bits4/0x28, detaches then clears touch. Detached handler clears+c41, grounded branch clears touch, installs default think, schedules now; airborne branch only calls detach on same cooldown/solid gate, retaining detach marker. Existing native phase1593 detach is reused. Generic velocity/movement and player object release remain hosted. Bounded native dispatch rejects unknown original marker and truncated touched entities; no raw original function address invoked. Windows test exercises exact cooldown equality, callback ordering, living target branch, schedule short-circuit, grounded and airborne contact, and original marker transitions.

VA/bytes/SHA256:
2237c980/162/67259c8a2a7ec96f236a3642ce49db35aba99cb74110bf55ebbc6f5c2280d472
2237ca30/129/979b84a5b8ce9d21b414b6134438e5eac708e11440eb7b1379f9a479b1d01627
2237af10/112/718f67cfea07d5ea714f0ec901dd1c8ddb55f2747075a01c41c2f9e5e67f5c44

No original bytes committed. CI pending.
