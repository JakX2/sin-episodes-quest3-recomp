#!/usr/bin/env python3
"""Original SiN 2006 blend-input and four gameplay-metric consumer x86 audit.

Runs Phase 78 SHA-256 original client/server and named-table math audit first.
Outputs metadata only; no owner-held binary contents are embedded.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path
from audit_sin_metric_math import audit as phase78_audit
from audit_sin_manager_state import PEImage, VerificationError

OPCODES={
    0x222ba399:'8b0d68215922', 0x222ba39f:'8b5128',
    0x222ba3a2:'895608',0x222ba3b0:'8b0d68215922',
    0x222ba3b6:'8b5128',0x222ba3b9:'52',
    0x222ba3bf:'a168215922',0x222ba3c4:'8b4828',0x222ba3c7:'51',
    0x222b915a:'83be6402000000',0x222b9161:'7543',
    0x222b9189:'8b4608',0x222b918c:'50',
    0x222b918d:'c7460c0000003f',
    0x222b919c:'c7866402000001000000',
    0x222d47f0:'6a01',0x222d47f7:'d84c2408',0x222d47fe:'c20400',
    0x222d4830:'6a02',0x222d4837:'59',0x222d4838:'c20400',
    0x222d4cf0:'6a0b',0x222d4cf7:'d8442408',0x222d4cfe:'c20400',
    0x222d5421:'8b742408',0x222d5425:'f7463821400280',
    0x222d542c:'7510',0x222d542e:'6a00',
    0x222d5435:'d84e2c',0x222d543b:'d95e2c',0x222d543f:'c20400'
}
DIRECT_CALLS={
    0x222ba3ba:0x222d3490,0x222ba3c8:0x222c0b90,
    0x222b9194:0x222d3490,0x222bbfda:0x222b9130,
    0x222d47f2:0x222d33e0,0x222d4832:0x222d33e0,
    0x222d4cf2:0x222d33e0,0x222d5430:0x222d33e0,
}
FUNCTION_POINTERS={
    0x2245f844:0x222d47f0,0x2245f880:0x222d4830,
    0x2245f8f8:0x222d4cf0,0x2245f848:0x222d5420
}
REGION_HASHES={
    (0x222d47f0,17):'035a5749ac12b140343abc52dce0ca2f432fa78667fe1e373fab4a99cd0a2310',
    (0x222d4830,11):'ed08d06969aecf41bc72c704b9ed65d93be5ec6a1c08e23a56186a7c5e515cf8',
    (0x222d4cf0,17):'dc2543dc9ab4947a786f803591c80a4da432dd37789200838a37a39ca9fac6c0',
    (0x222d5420,34):'5c48c6832387452dabc568be81434c50394f250dd11346378d9c1dbd6b00a97a',
    (0x222ba399,52):'32df91a4b3bdf8f0c428f7a3a46d972a9c698ebdc9dded04456d4cd567888104',
    (0x222b915a,76):'bfbcc9952794edcf0e24f2918592f1dbfb9b06c6b1334f872d4dea0852ebb533'
}
def check(pe,va,expected):
    if pe.at(va,len(expected))!=expected:
        raise VerificationError("Phase79 original x86 mismatch at %#x"%va)

def verify_consumers(pe,check_hashes=True):
    for va,raw in OPCODES.items():check(pe,va,bytes.fromhex(raw))
    for va,target in DIRECT_CALLS.items():
        check(pe,va,b'\xe8'+struct.pack('<i',target-va-5))
    for va,target in FUNCTION_POINTERS.items():
        check(pe,va,struct.pack('<I',target))
    if check_hashes:
        for (va,length),digest in REGION_HASHES.items():
            if hashlib.sha256(pe.at(va,length)).hexdigest()!=digest:
                raise VerificationError("Phase79 original region SHA-256 mismatch at %#x"%va)
    return {
        'blend_source_global_ptr_va':'0x22592168',
        'blend_float_member_offset':'0x28',
        'manager_cached_blend_member_offset':'0x08',
        'blend_callsite':'0x222BA3BA',
        'restore_gate_member_offset':'0x264',
        'restore_reblend_callsite':'0x222B9194',
        'restore_parent_callsite':'0x222BBFDA',
        'source_object_identity':'UNKNOWN',
        'metric_consumers':[
            {'va':'0x222D5420','index':0,'equation':'if ((type & 0x80024021)==0) damage *= metric[0]'},
            {'va':'0x222D47F0','index':1,'equation':'return argument * metric[1]'},
            {'va':'0x222D4830','index':2,'equation':'return metric[2]'},
            {'va':'0x222D4CF0','index':11,'equation':'return argument + metric[11]'},
        ],
        'opcode_sites':len(OPCODES),'call_edges':len(DIRECT_CALLS),
        'function_pointer_sites':len(FUNCTION_POINTERS),
        'region_hashes':len(REGION_HASHES) if check_hashes else 0,
        'scope':'Finite float reference math, no full source engine runtime or x87 precision parity.'
    }

def audit(client,server):
    previous=phase78_audit(client,server)
    previous['metric_consumers']=verify_consumers(PEImage(server))
    return previous

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--client',required=True,type=Path)
    p.add_argument('--server',required=True,type=Path)
    p.add_argument('--output',required=True,type=Path)
    a=p.parse_args()
    result=audit(a.client.read_bytes(),a.server.read_bytes())
    a.output.write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(result['metric_consumers'],indent=2))
if __name__=='__main__':main()
