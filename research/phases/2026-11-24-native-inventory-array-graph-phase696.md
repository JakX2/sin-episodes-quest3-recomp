# Phase 696: native inventory Array3 graphs and combat recipient dispatch

Fresh SHA-matching original server/client disassembly:
SendPropArray3 0x2225efc0, RecvPropArray3 0x24126e90.
Original server 0x22249a82/0x22249a9d constructs m_iAmmo
unsigned 10-bit DWORD at +0x64c, 32 elements stride 4.
Client 0x240d398f/0x240d39bd constructs m_iAmmo[0] and
m_iAmmo parent at +0xc00. Original server 0x222a6344/635f
constructs m_hMyWeapons handle element +0x6cc, 48 stride4;
client 0x240c37bc/37ea element m_hMyWeapons[0] at +0xc80.
Both parent datatable proxies are direct. Original name
arrays 0x2251abf8 and 0x242fb110 resolve literal 000,001,...047.
The server factory allocates a copy of the element prototype
and sets parent arrayProp, both factories allocate numbered
child descriptors with offsets i*4 and parent name; child
table name is array name. Client parent array_prop is null.
SDK2004 dt_send.cpp/dt_recv.cpp corroborate.

Complete DT_BCCLocalPlayerExclusive (2) and
DT_BaseCombatCharacter (3) constructor calls server
0x222a6384/0x222a5274, client 0x240c3803/0x240c38b4.
Original base pointers 0x224e9570/0x242f87f8 resolve to
DT_BaseFlex. BaseCombatCharacter declares baseclass,
bcc_localdata, m_hActiveWeapon. Local declares next attack
and weapon array. Owner timer/active offsets from Phase693.
The 27-property DT_LocalPlayerExclusive remains unfinished;
the ammo property is exposed only as a component.

Original 0x222a50f0 BCC local proxy clears recipients,
direct player slot72, otherwise slot76 -> returned object's
slot2(0) -> candidate player slot72. Both branches get
edict +0x16c and engine slot18. Returns pData even when no
recipient. Original object-ID local player proxy 0x22258250
sets index-minus-one bit and returns pData. Port validates
ID range and keeps empty recipient mask for invalid inputs.

New actual role-specific inventory graph APIs compile into
native Win32 modules. Explicit nonempty external DT_BaseFlex,
borrowed live registry/engine/edict context. Same verified
handle and player-index code as Phase695; both BCC branches
implemented with checked original vslots. Numbered array
storage and server element prototype live as long as graph.
Destruction guards engine flags and decoder/precalc.
Paired native DLL fixture dispatches every 32/48 numbered
property, active handle and timer, checks original offsets,
parent-name/arrayProp semantics and direct/indirect player
recipient behavior with actual MSVC x86 fixture methods.
No full player table, native class registry, entity
creation, actual wire transport or retail initialization claim.
No original assets or binaries committed.
