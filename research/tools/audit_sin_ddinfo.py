#!/usr/bin/env python3
"""Verify original 2006 SiN DDInfo typed operations and client state writes.

Uses the earlier exact SHA-256-pinned dual-DLL protocol auditor. All VAs are
Windows x86 original addresses, not Android ARM64 or rebuilt source code.
"""
import argparse
import json
import struct
from pathlib import Path

from audit_sin_difficulty_protocol import audit as prior_protocol_audit
from verify_gameplay_entity_registry import PEImage, VerificationError

# Exact original machine instructions independently checked on both DLLs.
S = {
    0x222BA5FD:'6810cb4522', 0x222BA608:'8a5604', 0x222BA611:'8a4605',
    0x222BA61A:'d94660', 0x222BA61D:'d864243c',
    0x222BA628:'d81d90253d22', 0x223D2590:'00000000',
    0x222BA630:'f6c405', 0x222BA633:'7a08',
    0x222BA647:'8b5640', 0x222BA650:'a0e4275922',
    0x222BA65E:'83c648', 0x222BA661:'bf06000000',
    0x222BA66A:'ff5218', 0x222BA673:'83c404',
    0x222BA676:'83c604', 0x222BA679:'83ef01', 0x222BA67C:'75e8',
    0x221E96B0:'8d7101', 0x221E96DF:'081e', 0x221E96E8:'f6d3',
    0x2225A164:'6a10', 0x22258F00:'6a20',
}
C = {
    0x240A5600:'6a10', 0x240A5631:'6a20',
    0x241F99ED:'85c0', 0x241F99F2:'884d04', 0x241F9A35:'884d05',
    0x241F9A3F:'d95d08', 0x241F9A49:'89450c', 0x241F9AA9:'884d06',
    0x241F9AAC:'8d5529', 0x241F9B4B:'83c203',
    0x241F9B50:'83f806', 0x241F9B53:'0f8c5affffff',
    0x241F9AD2:'83f802', 0x241F9B04:'83f802', 0x241F9B34:'83f802',
    0x241F9ADA:'83f803', 0x241F9B0B:'83f803', 0x241F9B3C:'83f803',
    0x241F9ADF:'8a4d06', 0x241F9B10:'8a4d06', 0x241F9B41:'8a4d06',
    0x241F9B5B:'c685b800000000',
}
SCALL = {
    0x222BA603:0x221E86D0, 0x222BA60C:0x221E9690,
    0x222BA615:0x221E9690, 0x222BA642:0x221E8830,
    0x222BA64B:0x221E87A0, 0x222BA656:0x221E9690,
    0x222BA66E:0x221E9690, 0x222BA67E:0x221E8720,
}
CCALL = {0x241F9A3A:0x240A5630,0x241F9A44:0x240A5600}

def check(pe,site,raw):
    if pe.at(site,len(raw))!=raw:
        raise VerificationError('DDInfo original byte evidence mismatch at %#x' % site)

def verify_ddinfo(client_pe, server_pe):
    for site,raw in S.items(): check(server_pe,site,bytes.fromhex(raw))
    for site,raw in C.items(): check(client_pe,site,bytes.fromhex(raw))
    check(server_pe,0x2245CB10,b'DDInfo\0')
    for pe,calls in ((server_pe,SCALL),(client_pe,CCALL)):
        for site,target in calls.items():
            check(pe,site,b'\xe8'+struct.pack('<i',target-site-5))
    return {
        'message':'DDInfo',
        'server_function_va':'0x222BA5B0',
        'client_function_va':'0x241F99A0',
        'wire_fields':['bit','bit','f32','s16','bit','bit[6]'],
        'verified_field_bits':57,
        'client_six_flag_state_offsets':['0x28','0x29','0x2A','0x2B','0x2C','0x2D'],
        'client_indices_overridden_by_shared_flag':[2,3],
        'sender_advisor_flag_dispatch':'six vtable +0x18 calls',
        'notes':['integer is signed 16-bit on wire but client stores result in a 32-bit member',
                 'server computes a float difference and clamps negative values to zero',
                 'client flag positions 2 and 3 are overwritten after bit-read',
                 'full message framing, scheduling and gameplay formulas still unknown'],
    }

def audit(client,server):
    verified=prior_protocol_audit(client,server) # verifies exact original SHA-256 hashes
    verified['DDInfo_field_evidence']=verify_ddinfo(PEImage(client),PEImage(server))
    return verified

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--client',required=True,type=Path)
    parser.add_argument('--server',required=True,type=Path)
    parser.add_argument('--output',required=True,type=Path)
    args=parser.parse_args()
    result=audit(args.client.read_bytes(),args.server.read_bytes())
    args.output.write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(result['DDInfo_field_evidence'],indent=2))

if __name__=='__main__': main()
