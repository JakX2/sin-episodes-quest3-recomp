# Phase 707: authenticated original client registration census

The SHA-pinned owner client.dll was inspected locally without execution.
All 191 client classes were identified by two exact inlined/static record
initializer patterns. The 191 matching CRT initializer pointers are unique
and appear in ascending registration-code order in the original .data
pointer sequence, from RVA 0x2f3008 through 0x2f47a0 inclusive. Thus the
actual linked-list order is the reverse of those initializer positions,
not an assumed alphabetical order. 125 registrations initialize four
record fields inline; 66 start from static records. 149 entity creators
and 42 event creators. All 164 server names are present on the client,
leaving 27 client-only classes.

Canonical digest of ordered class name, table and class storage RVAs,
initializer RVA, entity and event creator RVAs, CRT slot and form:
79a42f076c80900a6b755fd531b03b26d94328c9b06c8c837e94cf8d5adaad73.

The earlier table constructor scanner excluded CKeypad because the
original table name is DTKeypad (without underscore) on BOTH DLLs.
Client table constructor call 0x2421bbd6 and server 0x222fc07a.
CWorld uses DT_WORLD on server versus DT_World on client.

Inspect-OwnerOriginalClientClasses.py independently authenticates the
binary, VClient011 slot 5 getter, record stores, mapped storage, exactly
one executable creator, CRT pointer uniqueness, count and digest. Public
CTest uses invented records only; owner originals never committed or run.
The aggregate class subset is not yet published to the retail engine.

Next concrete creator boundary: CHL2_Player allocation 0x1250 bytes,
CPlayerHand 0x480, CWeaponMagnum/AssaultRifle/ScatterGun 0x830 each.
Original factories allocate zeroed storage via 0x240c4ff0, construct
class-specific objects, invoke primary vtable slot 37 (+0x94) with
entity number and serial, return IClientNetworkable subobject +8.
All five use original slot37 implementation 0x240c8fe0, which registers
the secondary +4 handle interface, initializes collision-related
host state, updates a class-dependent value and stores entity index
at +0x50. This is evidence for host construction, not an implemented
original class constructor or call into the original game.
