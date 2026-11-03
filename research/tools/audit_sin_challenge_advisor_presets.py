#!/usr/bin/env python3
"""Verify original SiN dd_challenge ConVar and six advisor five-float presets.

Original owner-held Windows PE32 DLLs mandatory for audit. No executable
bytes are written to JSON; only names, numeric field values and proofs.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
from pathlib import Path

from audit_sin_metric_consumers import audit as phase79_audit
from audit_sin_manager_state import PEImage, VerificationError

PRESET_BASE=0x22521f70
PRESET_COUNT=6
PRESET_FLOATS=5
PRESET_STRIDE=20
PRESET_BITS=(
    (0x42a00000,0x41a00000,0x41a00000,0x3e99999a,0x3eaaa64c),
    (0x41a00000,0x42700000,0x41200000,0x3eaaa64c,0x3e2aa8eb),
    (0x428c0000,0x41a00000,0x41a00000,0x3f000000,0x3f000000),
    (0x41f00000,0x40a00000,0x41700000,0x3dcccccd,0x3f000000),
    (0x41200000,0x428c0000,0x41f00000,0x3f000000,0x3f000000),
    (0,0,0,0,0),
)
OPCODES={
    0x223c2da0:'680000803f', 0x223c2da5:'6a01',
    0x223c2da7:'6a00', 0x223c2da9:'6a01',
    0x223c2dab:'68f0cd4522', 0x223c2db2:'68e8cd4522',
    0x223c2db7:'68d8cd4522', 0x223c2dbc:'b950215922',
    0x223c2de0:'680000803f', 0x223c2df2:'6818ce4522',
    0x223c2df7:'6808ce4522', 0x223c2dfc:'b998215922',
    0x2225b9c9:'896d18', 0x2225ba4b:'d95528',
    0x222ba399:'8b0d68215922', 0x222ba39f:'8b5128',
    0x222ba3a2:'895608', 0x222ba3a5:'a1b0215922',
    0x222ba3aa:'d86028', 0x222ba3ad:'d95e0c',
    0x222ba3b0:'8b0d68215922', 0x222ba3b6:'8b5128',
    0x222ba3b9:'52', 0x222ba3bf:'a168215922',
    0x222ba3c4:'8b4828', 0x222ba3c7:'51',
    0x222c0ae0:'d94104', 0x222c0ae3:'d901',
    0x222c0ae7:'d85904', 0x222c0aec:'f6c405',
    0x222c0aef:'7a05', 0x222c0afd:'d905f8223d22',
    0x222c0b0b:'d8c9', 0x222c0b0f:'d84c2404',
    0x222c0b13:'dec2', 0x222c0b20:'f6c405',
    0x222c0b27:'7a2a', 0x222c0b3d:'d90514633d22',
    0x222c0b9c:'be701f5222', 0x222c0bde:'83c614',
    0x222c0be1:'83c701', 0x222c0be4:'81fee81f5222',
    0x222b946a:'8b4cbe48',
}
DIRECT_CALLS={
    0x223c2dc1:0x2225bde0,
    0x223c2e01:0x2225bde0,
    0x2225be25:0x2225b9c0,
    0x222ba3ba:0x222d3490,
    0x222ba3c8:0x222c0b90,
    0x222c0bb8:0x222c0ae0,
    0x222c0bd2:0x222bc4a0,
    0x222c0bd9:0x222b9450,
    0x222b9470:0x222c4ac0,
}
REGION_HASHES={
    (0x223c2da0,50):'6cac12892b40d42e79d299e6d314bd42b98553463b250ad30e085280d9518da0',
    (0x2225b9c0,244):'505bfc077d945394d4046075d09b2180854fabd4105623caf08c47d0e5241c37',
    (0x2225bde0,78):'93112d5925c9e1df6ca5f6b99dd897b1e1e29acf8a936502cbec3e9599dee206',
    (0x222c0ae0,157):'6b91da5c016dde846e73ba9afee5cabb980ad04565982115a0614660779ad7dc',
    (0x222c0b90,99):'4c22ee011ea460bd16340f892159929815dc732ee0ebd4d50c9b3a6719854587',
    (PRESET_BASE,120):'b761c0c93fc1df03dfad1fecdfce5d74f438dbddc9dbfeb32ab679eb9d9d1b49',
    (0x222ba360,186):'33bfd10766f781da17ad0e52563ca20394c938da75d6725c0e8715ff18e28bb3',
    (0x222b9450,71):'759e2673b79c55b2b32d7cf50f07c95b1916b71500a4d5d7efe47615f9528eac',
}
STRINGS={
    0x2245cdd8:b'dd_challenge\x00',
    0x2245cde8:b'0.2f\x00',
    0x2245cdf0:b'The challenge level.\x00',
    0x2245ce08:b'dd_forgiveness\x00',
    0x2245ce18:b'0.5f\x00',
    0x2245ce20:b'The assistance level.\x00',
}

def verify_one(pe,va,expected):
    if pe.at(va,len(expected))!=expected:
        raise VerificationError('Phase80 x86 or original data mismatch at %#x'%va)

def verify_challenge_advisors(pe, check_hashes=True):
    for va,raw in OPCODES.items():
        verify_one(pe,va,bytes.fromhex(raw))
    for va,target in DIRECT_CALLS.items():
        verify_one(pe,va,b'\xe8'+struct.pack('<i',target-va-5))
    for va,data in STRINGS.items():
        verify_one(pe,va,data)
    rows=[]
    for i,expected_row in enumerate(PRESET_BITS):
        raw=pe.at(PRESET_BASE+i*PRESET_STRIDE,PRESET_STRIDE)
        if struct.unpack('<5I',raw)!=expected_row:
            raise VerificationError('Phase80 advisor float bits mismatch at %d'%i)
        rows.append(list(struct.unpack('<5f',raw)))
    verify_one(pe,0x223d6314,struct.pack('<f',100.0))
    if check_hashes:
        for (va,length),digest in REGION_HASHES.items():
            if hashlib.sha256(pe.at(va,length)).hexdigest()!=digest:
                raise VerificationError('Phase80 region SHA-256 mismatch at %#x'%va)
    return {
        'challenge_cvar':{'name':'dd_challenge','object_va':'0x22592150',
            'parent_pointer_va':'0x22592168','float_offset':'+0x28',
            'default_text':'0.2f','declared_range':[0,1]},
        'forgiveness_cvar':{'name':'dd_forgiveness','object_va':'0x22592198',
            'parent_pointer_va':'0x225921B0','default_text':'0.5f',
            'initial_manager_formula':'manager+0x0C = 1.0f - dd_forgiveness'},
        'challenge_manager_field':'this+0x08',
        'challenge_metric_interpolation_va':'0x222D3490',
        'challenge_advisor_interpolation_va':'0x222C0B90',
        'advisor_single_preset_evaluation_va':'0x222C0AE0',
        'advisor_preset_table_va':hex(PRESET_BASE),
        'advisor_preset_stride_bytes':20,
        'advisor_preset_float_count':5,
        'advisor_count':6,
        'advisor_presets':rows,
        'original_opcode_site_count':len(OPCODES),
        'original_callsite_count':len(DIRECT_CALLS),
        'original_string_count':len(STRINGS),
        'original_region_hashes':len(REGION_HASHES) if check_hashes else 0,
        'limits':'Original 2006 initialization and six advisor configuration formulas; advisor identities, all runtime update callbacks and byte-identical x87-to-ARM64 results unproven.',
    }

def audit(client,server):
    result=phase79_audit(client,server)  # exact SHA-256 both original game DLLs
    result['challenge_advisor_presets']=verify_challenge_advisors(PEImage(server))
    return result

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--client',type=Path,required=True)
    p.add_argument('--server',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True)
    args=p.parse_args()
    result=audit(args.client.read_bytes(),args.server.read_bytes())
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(result['challenge_advisor_presets'],indent=2))
if __name__=='__main__':main()
