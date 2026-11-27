# Phase 1593: Recombinant Minor damage, detach and ground condition

Direct disassembly of original server SHA256 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650. Minor virtual +370 at 2237d0c0 copies 0x48-byte damage info via 2202f480. Source handle +24 resolves classification10 immunity. BaseMutant and Minor inherit 22342b50 (return10); Major overrides 22376f30 (also10). Native Major source classification now recognizes all three. If +c41 set, compare resolved attacker +28 to local player (null for >1 maxclients); equal pointers including null apply 0.05-scaled vector, squared >62500 invokes original detach +778; otherwise refresh velocity if +b8&1000 and add vector to +250. Detach 2237cac0 sets67, calls set velocity(0,0,-.01), clears+c41, stores original touch marker 2237ca30, schedules current-time think with context0. Burn8 on already-burning actor invokes color(5,30); health<=1 and odd signed entity index checks ideal80, installs80 if absent; every accepted burn invokes inherited Ignite(30,1,0,0,0,0); then inherited damage consumes local copy. Minor gather 2237c840 calls parent first, clears66, resolves ground+238 via serial registry. Only grounded +bc&1 with live nonzero-index ground sets66 for +f4 true, +120 true or object +218 virtual flags&4. Engine services remain explicit owner callbacks. No physics engine recreation, no original bytes committed.

VA/bytes/SHA256:
2237d0c0/473/435b75c09fad0865546377f80e189fa4ccb908556753560e14bb2cb843ba74da
2237cac0/87/2614aff327f3efffcec3d02301f29826ebd036c9281f0958f7f2304fe4fefc71
2237c840/121/e3bf264138b340bb126df5f01ea79115ab85763067f4bcc0f38290cad752bd99
2202f480/131/e3cadeac292ed0f3ddaaee2c1e3efa8d586ecff097949d66f1ddcfad6e74528a
2200deb0/65/e6b66aaa261c63238d1fa8a8e83c45b7bb5a911ce046a3a0346c481f04f4efae
2221f5b0/46/ffb3caf99f7592acdfc5891f27a109837c96c08a3c6f8c099637227c7136ad1c
22342b50/6/07297dca94be1b975a5dc696b5976f205b84057995fa3d3fbb157101b1ae8098
223d9cb0/4/0b9d79ff0fc4d0a77ef398763bbe45c445501e826219f2b6a1aa709411ab631d
22480204/4/5842ad76679a305c32da4ee93ce2f0f17b3453c5f33e514ec3c901594a76bffd

Loaded test checks source types, exact impulse boundary, stale attacker, null comparison, detach, burn odd/even branches and ground condition priority. Touch callback body and complete scheduler still pending. CI pending.
