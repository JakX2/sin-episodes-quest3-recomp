# Phase 692: original weapon networking scalar contract and live snapshot capture

Fresh SHA256 verification of both owner-private original PE32 binaries:
server 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650,
client 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d.

Original server SendTable calls: LocalWeaponData 0x222acb10,
LocalActiveWeaponData 0x222acfea (newly recovered), BaseCombatWeapon
0x222ade36. Client corresponding calls 0x24091076, 0x24090f9e,
0x24091196. 5,4,7 property slots respectively. The base table has
baseclass, LocalWeaponData and LocalActiveWeaponData before four
direct public scalar fields. Direct disassembly of all 13 scalar
property constructors establishes offsets, field order and flags.
SendPropInt 0x2225ebf0, wrappers 0x2227eba0/0x2227eb70, and
SendPropFloat 0x2225e8c0 / timer helper 0x2227ebd0 establish
bit widths and flags. NOSCALE timer stored nBits=0, payload f32.
Owner SDK2004 dt_common.h corroborates UNSIGNED=1 and NOSCALE=4.
No claim about overall bitstream framing or flattened property order.

Clip send proxy 0x2227eb10 (16 bytes, SHA256
28e1a5bd723c33d1180abd76dc424fb14afee9052eed304b4323d67b3826362a)
adds one; receive proxy 0x241a8c20 (17 bytes,
bbea8c9b7a7fce6219d595fb3305ce6b6137f5186e171df2ab93d3fe23323582)
subtracts one. Unsigned 8-bit supports -1..254. Port refuses
out-of-range values rather than assuming engine encoder clipping.
Owner handle send 0x2227ec60 verifies entity index/serial/live
entry, packs low 10 serial bits at bit11, invalid=0x1fffff.
Client 0x241a8db0 reverses wire serial shift and 11-bit entry,
invalid becomes 0xffffffff. Port refuses out-of-range native
entry and sentinel collision. Live resolution supplied by host;
truncated decoded handles are not generation-safe port identities.

Both local table proxies 0x222adba0 and 0x222adc60 have identical
177-byte SHA256 c3eb5e4c1ad3974451753f215836b1df36cb936caf05e2fdedfbaa3a95749c91.
They resolve weapon owner, test player eligibility, call engine
slot18 with owner field +0x16c, clear eight recipient words and set
the client bit (index minus one). Both use the same owner restriction;
no extra active-weapon equality gate in either. Host supplies the
resolved one-based client slot; port validates 1..256. No owner,
nonplayer or unresolved owner means no local payload. Base public
fields remain available. New portable payload is semantic scalar
words/mask, NOT the Source network bitstream or native table ABI.

Shared contract compiled into both reconstructed DLLs. New server
map-owned snapshot export requests authoritative host observations,
requests only public scalar fields for a nonowner, and never reads
private values on that path. No raw offset writes, no retained
host, transactional failure leaves output unchanged. Client-side
decode updates only fields in permitted mask; caller owns identity,
recipient authorization and resets on owner change. Actual loaded
module regression checks metadata and cross-DLL scalar conversion,
clip sentinel, owner serial truncation, recipient edge, malformed
payload rejection and stale identity. Linux sanitized service tests
also verify host capture and nonowner read suppression.

Further authenticated inventory anchors: DT_LocalPlayerExclusive
m_iAmmo 32 dwords at server +0x64c and client +0xc00, 10-bit unsigned
elements (server 0x22249a82/0x22249a9d, client 0x240d398f/
0x240d39bd). DT_BCCLocalPlayerExclusive m_flNextAttack server
+0x60c/client +0xbf8 and 48 handle array m_hMyWeapons server
+0x6cc/client +0xc80, original calls 0x222a631e/0x222a635f and
0x240c37a1/0x240c37ea. These inventory fields are research anchors,
not included in Phase692's weapon payload. No full client
prediction, server class registration, wire compatibility or retail
startup claim. No original bytes distributed.
