# Phase 695: native resolved-owner weapon send proxies

Direct read-only recheck of original SHA-pinned server and engine:
server.dll 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650
engine.dll f3add5b41300e6d709fd0b492bfcdd37b9b3d20d9744f874c0b860bf021cec81

Both original 177-byte local weapon proxies 0x222adba0 and
0x222adc60 resolve the raw weapon EHANDLE at pData+0x494, use
original list global 0x224f5620 with 16-byte record and 12-bit
index/20-bit serial. Original +0xfc virtual on the resolved handle
object yields a base entity; that entity's +0x120 virtual must
return true. Its +0x16c field supplies the edict pointer to
VEngineServer021 slot18. Success clears eight recipient words,
sets the index-minus-one bit, and returns pData. Failure returns
null WITHOUT touching recipient words, correcting the earlier
scalar-contract simplification. Port clears words on failure too
to prevent stale recipient masks; this is explicitly a port guard.

Original engine CVEngineServer RTTI type descriptor 0x20324ca8,
COL 0x202e399c, vtable 0x202c5184, slot18 0x20169e80. Function
uses edict base 0x2079db04 and signed division by 20 bytes
(multiply 0x66666667, shift three, signed adjustment), with
bounds diagnostic. Owner SDK2004 eiface.h confirms IndexOfEdict.
Native adapter requires host-certified edict base/client interval,
exact stride and matching returned index before recipient publication.
Preflights module-owned executable virtual methods via existing
VerifiedMethod, validates relevant readable fields, and catches
C++ exceptions. This is not proof of arbitrary foreign object
validity; host must supply actual compatible entity subobjects and
stable borrowed registry/edict/interface storage.

Original public EHANDLE proxy 0x2227ec60 independently resolves
entry and serial, not player eligibility. Then low ten serial
bits shift left 11 and combine with index, invalid=0x1fffff.
Adapter rejects unrepresentable entry indices, preserving
Phase692 semantic contract. No original global addresses used.

New server-only resolved graph API constructs Phase694's nine
actual native SendTable records with DLL-owned callbacks and
explicit borrowed registry/engine dependencies. Graph extra
data stores its context (port extension); original used globals.
Existing external-callback graph remains available. Reuses
prior EntityHandleView. Pair loader uses actual MSVC x86
vslots 63/72 and engine slot18, original player edict offset,
client 256 recipient boundary, stale serial, null entity,
nonplayer, invalid edict, incorrect index and exception paths.
No engine class registration or retail startup yet.
