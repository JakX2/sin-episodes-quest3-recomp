#!/usr/bin/env python3
"""Read-only original SiN 2006 client stat definition factory and RTTI.

Requires owner's SHA256-authenticated original client.dll; prints type
metadata only. Does not export method bodies, original DLL or assets.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import pathlib
import struct
from audit_sin_client_engine_vgui_slot1 import OriginalPE, CLIENT_SHA256
DISPATCH_VA=0x24205BC0
DISPATCH_LENGTH=560
DISPATCH_SHA="3ec859fcfc931d3d695ff389347737f63d65334bcddc57e631cc5bfa876f54ac"
JUMP_TABLE=0x24205DF0
# case target, malloc bytes, ctor callsite, ctor target,
# final vptr write site, vtable address, RTTI TypeDescriptor, name.
CASES=(
(0x24205C00,16,0,0,0x24205C1F,0x242AF384,0x2431ED00,"CIntStat"),
(0x24205BD9,16,0,0,0x24205BF8,0x242AF3DC,0x2431ED18,"CFloatStat"),
(0x24205C47,20,0x24205C60,0x242054E0,0x242054F1,0x242AF434,0x2431ED34,"CIntOutOfStat"),
(0x24205C27,20,0x24205C40,0x24205590,0x242055A1,0x242AF48C,0x2431ED50,"CFloatOutOfStat"),
(0x24205C67,20,0x24205C80,0x24205620,0x24205634,0x242AF4E4,0x2431ED70,"CPercentageStat"),
(0x24205C87,20,0x24205CA0,0x24205690,0x242056A4,0x242AF53C,0x2431ED90,"CAccuracyStat"),
(0x24205CA7,20,0x24205CC0,0x242056C0,0x242056D4,0x242AF594,0x2431EDAC,"CAverageStat"),
(0x24205CC7,20,0x24205CE0,0x24205720,0x2420572E,0x242AF5EC,0x2431EDC8,"CTimeStat"),
(0x24205CE7,28,0x24205D05,0x242057A0,0x242057B9,0x242AF644,0x2431EDE0,"CPerTimeStat"),
(0x24205D0C,28,0x24205D2A,0x242057A0,0x242057B9,0x242AF644,0x2431EDE0,"CPerTimeStat"),
(0x24205D31,28,0x24205D4F,0x242057A0,0x242057B9,0x242AF644,0x2431EDE0,"CPerTimeStat"),
(0x24205D56,32,0x24205D74,0x24205840,0x24205863,0x242AF69C,0x2431EDFC,"CAveragePerTimeStat"),
(0x24205D97,16,0x24205DAC,0x242058D0,0x242058E2,0x242AF6F4,0x2431EE20,"CMaxStat"),
(0x24205D7B,16,0x24205D90,0x24205980,0x24205992,0x242AF74C,0x2431EE38,"CMinStat"),
(0x24205DCF,20,0x24205DE4,0x24205A30,0x24205A58,0x242AF7A4,0x2431EE50,"CMaxAccumulatorStat"),
(0x24205DB3,20,0x24205DC8,0x24205AE0,0x24205B08,0x242AF7FC,0x2431EE74,"CMinAccumulatorStat")
)
RATES={
 0x24205CFD:0x3F800000,
 0x24205D22:0x3C888889,
 0x24205D47:0x3991A2B4,
 0x24205D6C:0x3F800000
}
class AuditError(ValueError):pass
def dword(pe,va):
    return struct.unpack("<I",pe.read(va,4))[0]
def verify(pe,expected_sha=DISPATCH_SHA):
    if pe.base!=0x24000000:
        raise AuditError("Original image base mismatch")
    if hashlib.sha256(pe.read(DISPATCH_VA,DISPATCH_LENGTH)).hexdigest()!=expected_sha:
        raise AuditError("Original type switch body SHA mismatch")
    seen=set()
    for index,(entry,size,call,ctor,write,vt,td,name) in enumerate(CASES):
        if dword(pe,JUMP_TABLE+index*4)!=entry:
            raise AuditError("Original stat factory jump case mismatch: "+str(index))
        if pe.read(entry,2)!=bytes((0x6A,size)):
            raise AuditError("Original stat factory allocation size mismatch: "+str(index))
        if call:
            expected=b"\xe8"+struct.pack("<i",ctor-call-5)
            if pe.read(call,5)!=expected:
                raise AuditError("Original stat ctor call mismatch: "+str(index))
        ptr=pe.read(write,6)
        if ptr[:2] not in (b"\xc7\x00",b"\xc7\x06") or ptr[2:]!=struct.pack("<I",vt):
            raise AuditError("Original stat constructor vptr mismatch: "+str(index))
        # CompleteObjectLocator at vtable[-1] is signed metadata, not
        # an executable engine object. TypeDescriptor+8 holds .?AV...@@.
        locator=dword(pe,vt-4)
        if dword(pe,locator+12)!=td:
            raise AuditError("Original stat constructor RTTI locator mismatch: "+str(index))
        decorated=(".?AV"+name+"@@").encode("ascii")+b"\0"
        if pe.read(td+8,len(decorated))!=decorated:
            raise AuditError("Original stat constructor RTTI class mismatch: "+str(index))
        seen.add(name)
    for site,bits in RATES.items():
        if pe.read(site,5)!=b"\x68"+struct.pack("<I",bits):
            raise AuditError("Original stat rate argument mismatch: %#x"%site)
    return {
        "original_client_sha256":CLIENT_SHA256,
        "original_factory_va":hex(DISPATCH_VA),
        "complete_dispatch_sha256":DISPATCH_SHA,
        "cases":[{"index":i,"class":name,"native_allocation_bytes":size,
                  "original_case_entry":hex(entry),"original_constructor":hex(ctor),
                  "native_vtable":hex(vt)}
                 for i,(entry,size,call,ctor,write,vt,td,name) in enumerate(CASES)],
        "unique_native_rtti_classes":len(seen),
        "rate_argument_float_bits":{hex(site):hex(bits) for site,bits in RATES.items()},
        "original_type_ids_after_15":"null",
        "limitations":["Type-specific constructors and vtables not ported",
                       "No original game-defined stat object instantiated",
                       "Original retail VClient011 Init not completed"]
    }
def audit_original(path):
    raw=pathlib.Path(path).read_bytes()
    if hashlib.sha256(raw).hexdigest()!=CLIENT_SHA256:
        raise AuditError("Original client.dll full file SHA mismatch")
    return verify(OriginalPE(raw))
def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument("client")
    args=ap.parse_args()
    print(json.dumps(audit_original(args.client),indent=2,sort_keys=True))
if __name__=="__main__":main()
