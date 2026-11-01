#!/usr/bin/env python3
"""Read-only original 2006 six-category routing/label-table verifier.

Requires the exact privately owned SiN Episodes server.dll. No original
machine code, string tables, models or campaign assets are emitted.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

SHA256="568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650"
LEVEL_TABLE=0x22521d70
CATEGORY_TABLE=0x22521dc0
LEVELS=("Very Easy","Easy","Medium","Hard","Very Hard")
CATEGORIES=("Health","Damage","Accuracy","Headshot","Death","Time")
CODE={
    0x222b9e8b:"8d5d48",       # lea ebx, [ebp+0x48]: six category pointers
    0x222b9e90:"83fe04",       # index 4 bypass
    0x222b9e93:"8b0b",         # current category object
    0x222b9e95:"8b7920",       # category object local +0x20 level
    0x222b9e98:"7433",         # index 4 jumps to dispatch
    0x222b9e9a:"83fe05",       # index 5 bypass
    0x222b9e9f:"8b4540",       # global level at this+0x40
    0x222b9ea2:"83f802",       # compare global to level 2
    0x222b9ea5:"7d08",         # high level comparison branch
    0x222b9ea7:"83ff02",       # compare local to level 2
    0x222b9eac:"83f802",
    0x222b9eb1:"83ff02",
    0x222b9eb6:"83f802",
    0x222b9ebb:"3bc7",         # compare global and local
    0x222b9ebd:"7e09",
    0x222b9ec4:"3bc7",         # second compare
    0x222b9ec8:"bf02000000",   # override selected EDI with 2 (Medium)
    0x222b9ecd:"8b411c",       # category object helper data +0x1c
    0x222b9efa:"57",           # forward selected difficulty level
    0x222b9efb:"e8f0670000",   # helper 0x222c06f0, level->label
    0x222b9f04:"56",           # forward slot index
    0x222b9f05:"e8466a0000",   # helper 0x222c0950, slot->label
    0x222b9f18:"b900215922",   # category update helper object
    0x222b9f1d:"e83e630000",   # calls helper 0x222c0260
    0x222b9f22:"83c601",       # increment category index
    0x222b9f25:"83c304",       # advance this+0x48 pointer array
    0x222b9f28:"83fe06",       # stop at exactly six
    0x222c06f4:"8b0485701d5222", # global five label table
    0x222c0954:"8b0485c01d5222", # six category labels
}

class EvidenceError(ValueError): pass

def read_pe_va(blob, va, count):
    if len(blob)<128 or blob[:2]!=b"MZ":
        raise EvidenceError("invalid DOS header")
    start=struct.unpack_from("<I",blob,0x3c)[0]
    if start+24>len(blob) or blob[start:start+4]!=b"PE\0\0":
        raise EvidenceError("invalid PE signature")
    machine,sections,_,_,_,opt_size,_=struct.unpack_from("<HHIIIHH",blob,start+4)
    opt=start+24
    if machine!=0x14c or sections<1 or sections>96 or opt_size<96 or opt+opt_size>len(blob):
        raise EvidenceError("expected i386 PE32")
    if struct.unpack_from("<H",blob,opt)[0]!=0x10b:
        raise EvidenceError("expected PE32 optional header")
    image_base=struct.unpack_from("<I",blob,opt+28)[0]
    rva=va-image_base
    if rva<0 or count<0:raise EvidenceError("negative PE RVA")
    table=opt+opt_size
    if table+40*sections>len(blob):raise EvidenceError("truncated PE section table")
    for i in range(sections):
        row=table+40*i
        virtual,section,raw_size,offset=struct.unpack_from("<IIII",blob,row+8)
        if section<=rva and rva+count<=section+min(virtual,raw_size):
            at=offset+rva-section
            if at+count>len(blob):raise EvidenceError("truncated PE data")
            return blob[at:at+count]
    raise EvidenceError("VA not backed by PE section")

def verify(original,*,sha=SHA256,signatures=CODE,levels=LEVELS,categories=CATEGORIES):
    if hashlib.sha256(original).hexdigest()!=sha:
        raise EvidenceError("original DLL SHA256 mismatch")
    for va,code in signatures.items():
        expected=bytes.fromhex(code)
        if read_pe_va(original,va,len(expected))!=expected:
            raise EvidenceError("collector instruction mismatch at "+hex(va))
    for table,strings,kind in ((LEVEL_TABLE,levels,"level"),(CATEGORY_TABLE,categories,"category")):
        for i,label in enumerate(strings):
            pointer=struct.unpack("<I",read_pe_va(original,table+4*i,4))[0]
            if read_pe_va(original,pointer,len(label)+1)!=label.encode("ascii")+b"\0":
                raise EvidenceError("original "+kind+" label mismatch at "+str(i))
    return {
        "original_sha256_verified": True,
        "collector_va":"0x222b9e80",
        "code_signatures_verified":len(signatures),
        "global_level_field":"+0x40",
        "category_pointer_array_field":"+0x48",
        "category_local_level_field":"+0x20",
        "category_helper_data_field":"+0x1c",
        "category_count":len(categories),
        "level_count":len(levels),
        "global_override_applies_to_slots":[0,1,2,3],
        "global_override_bypassed_slots":[4,5],
        "override_selected_level":2,
        "original_category_stat_weight_calculations_recovered":False,
        "original_runtime_executed":False,
        "quest_headset_tested":False
    }

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("original_server_dll",type=Path)
    args=parser.parse_args()
    try:
        print(json.dumps(verify(args.original_server_dll.read_bytes()),indent=2,sort_keys=True))
    except (OSError,ValueError,struct.error) as ex:
        parser.exit(1,"Phase96 original feedback routing audit failed: "+str(ex)+"\n")
if __name__=="__main__":
    main()
