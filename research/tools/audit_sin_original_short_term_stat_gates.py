#!/usr/bin/env python3
"""Original SiN 2006 CShortTermStatGate/Conjugate: exact owner-DLL audit.

Requires owner-held original SHA-pinned PE32 client.dll and server.dll.
No original instructions, copyrighted DLLs, assets or generated machine
disassembly are committed; synthetic tests exercise this auditor in CI.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path

from audit_sin_original_stat_advisor_sample_pipeline import audit as phase149_audit
from audit_sin_manager_state import PEImage, VerificationError

# Original compiler method boundaries end before aligned INT3 padding.
REGIONS = {
    "ordinary_ctor": (0x222C6600, 104,
        "0eed18121db5af0bdb4d71133be223da9a4af07c1191216d1ac34967c45c2449"),
    "conjugate_ctor": (0x222C66A0, 46,
        "1ee9d538fe20c62e12d711747c23e2ce070dd8e2aef451f310fde9b80ce01a73"),
    "slot1_cached": (0x222C6670, 4,
        "933a6c85af8213a87a347402fdaa62905c7b53039e4c483855521c3da6fd9509"),
    "slot2_normalize_forward": (0x222C6370, 34,
        "1124fe36fec84be51a5879424064d5ee1330ab355932395d24d3f639566ba11d"),
    "ordinary_slot3_refresh": (0x222C63A0, 76,
        "34d384c82ff5819c9ef5d021e4e6fd31b575a53ca619e88c511e6fba0d608b6e"),
    "conjugate_slot3_refresh": (0x222C6420, 94,
        "367cfdf63e773c461edc8a3e87b6bc9390bf03d935f6d32bcf0a80bcfb169c54"),
}
TYPE_INFO = {
    "CShortTermStatGate": (0x2245E4A4, 0x224CAFA0,
                           0x22522C98, 0x22522CA0),
    "CShortTermStatGateConjugate": (0x2245E4C4, 0x224CAFEC,
                                    0x22522CBC, 0x22522CC4),
}
VFTABLES = {
    "CShortTermStatGate": (0x2245E4A8, (
        0x222C6680, 0x222C6670, 0x222C6370, 0x222C63A0,
        0x222C63F0, 0x222C6400, 0x222C6410)),
    "CShortTermStatGateConjugate": (0x2245E4C8, (
        0x222C66D0, 0x222C6670, 0x222C6370, 0x222C6420,
        0x222C63F0, 0x222C6400, 0x222C6410)),
}
OPCODES = {
    0x222C6370:"8bc1",    0x222C6372:"d94010",
    0x222C6375:"8b4c2404", 0x222C6379:"d84804",
    0x222C637C:"89480c", 0x222C637F:"8b4808",
    0x222C6382:"8b11",    0x222C6384:"d87c2404",
    0x222C638C:"ff520c",  0x222C638F:"c20400",
    0x222C63A3:"8b4e08",  0x222C63A8:"ff5008",
    0x222C63AB:"d84e10",  0x222C63B4:"dfe0",
    0x222C63B6:"f6c441",  0x222C63C6:"d95e0c",
    0x222C63D5:"f6c405",  0x222C63E2:"d84e04",
    0x222C63E5:"d95e0c",  0x222C6423:"8b4e08",
    0x222C6428:"ff5008",  0x222C642B:"d84e10",
    0x222C6436:"f6c441",  0x222C6449:"d8e1",
    0x222C644B:"d84e04",  0x222C644E:"d95e0c",
    0x222C645F:"f6c405",  0x222C6472:"d8e1",
    0x222C6474:"d84e04",  0x222C6477:"d95e0c",
    0x222C660F:"894604",  0x222C6612:"c706a8e44522",
    0x222C6618:"894e08",  0x222C661B:"895610",
    0x222C6620:"ff5008",  0x222C6623:"d84e10",
    0x222C663D:"d84e04",  0x222C6640:"d95e0c",
    0x222C665C:"d84e04",  0x222C6661:"d95e0c",
    0x222C6665:"c20c00",  0x222C6670:"d9410c",
    0x222C6673:"c3",
    0x222C66B4:"e847ffffff",
    0x222C66BD:"c706c8e44522",
    0x222C66C3:"e858fdffff",
    0x222C66CB:"c20c00",
}
CALLS = {
    0x222BC163: 0x222C6600,  # factory chooses normal listener
    0x222BC1C3: 0x222C66A0,  # factory chooses conjugate listener
    0x222BC602: 0x222C6600,
    0x222BC6B6: 0x222C6600,
    0x222BC7AE: 0x222C6600,
    0x222BC951: 0x222C6600,
    0x222C66B4: 0x222C6600,  # derived constructor chains normal
    0x222C66C3: 0x222C6420,  # then recomputes conjugate cache
}

def verify_original_short_term_gates(pe, check_hashes=True):
    for name,(col_cell,col,type_va,name_va) in TYPE_INFO.items():
        for site,value in ((col_cell,col),(col+12,type_va)):
            if pe.at(site,4)!=struct.pack('<I',value):
                raise VerificationError("short-term stat gate RTTI mismatch: %s at %#x" % (name,site))
        expected=(".?AV"+name+"@@").encode("ascii")+b"\x00"
        if pe.at(name_va,len(expected))!=expected:
            raise VerificationError("short-term stat gate RTTI name mismatch: "+name)
    for name,(start,entries) in VFTABLES.items():
        for index,target in enumerate(entries):
            if pe.at(start+index*4,4)!=struct.pack('<I',target):
                raise VerificationError("%s vtable slot %d changed" % (name,index))
    for va,hexcode in OPCODES.items():
        wanted=bytes.fromhex(hexcode)
        if pe.at(va,len(wanted))!=wanted:
            raise VerificationError("short-term stat gate opcode mismatch at %#x" % va)
    for va,target in CALLS.items():
        wanted=b"\xE8"+struct.pack("<i",target-va-5)
        if pe.at(va,5)!=wanted:
            raise VerificationError("short-term stat gate CALL changed at %#x" % va)
    if check_hashes:
        for name,(va,length,digest) in REGIONS.items():
            if hashlib.sha256(pe.at(va,length)).hexdigest()!=digest:
                raise VerificationError("short-term stat gate SHA-256 mismatch: "+name)
    return {
        "original_classes":list(TYPE_INFO),
        "vtable_slots":{
            name: ["0x%08X"%t for t in targets]
            for name,(_,targets) in VFTABLES.items()},
        "original_code_regions": {
            name: {"va":"0x%08X"%va,"size":length,"sha256":sha}
            for name,(va,length,sha) in REGIONS.items()},
        "verified_full_function_hashes":len(REGIONS) if check_hashes else 0,
        "verified_opcode_anchors":len(OPCODES),
        "verified_original_calls":len(CALLS),
        "member_offsets": {"scale":"+0x04","source":"+0x08",
                           "cached":"+0x0C","gain":"+0x10"},
        "source_vtable_calls": {"value_read_slot":2,"normalized_submit_slot":3},
        "gate_slots": {"cached_read":1,"normalized_forward":2,
                       "recompute":3},
        "scope":"Behavioral reconstruction; not binary-compatible x86 class or native engine module."
    }

def audit(client_bytes, server_bytes):
    out=phase149_audit(client_bytes,server_bytes)
    out["original_short_term_stat_gates"]=verify_original_short_term_gates(
        PEImage(server_bytes))
    return out

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--client",type=Path,required=True)
    parser.add_argument("--server",type=Path,required=True)
    parser.add_argument("--output",type=Path)
    args=parser.parse_args()
    out=audit(args.client.read_bytes(),args.server.read_bytes())
    output=json.dumps(out,indent=2)+"\n"
    if args.output:
        args.output.parent.mkdir(parents=True,exist_ok=True)
        args.output.write_text(output,encoding="utf-8")
    else:
        print(output,end="")

if __name__=="__main__":
    main()
