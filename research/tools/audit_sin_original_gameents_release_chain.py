#!/usr/bin/env python3
"""Authenticated original ServerGameEnts001 FreeContainingEntity path auditor.

The original retail server.dll is supplied locally by its owner. The auditor
does not emit any proprietary executable content. Synthetic self-tests do not
require commercial bytes or GitHub secrets.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
from pathlib import Path
from audit_sin_original_recombinant_minor_metric_gate import PE32, Refused, require, SERVER_SHA, synthetic_test

VTABLE=0x22443030
VIRTUAL_SLOT=0x0c
THUNK=0x221e82e0
THUNK_BYTES=14
THUNK_SHA='0b0caf62afd15971ebdd8778c492e03519e45a2c93c03b42aceec070630cb0ac'
HELPER=0x22266e60
HELPER_BYTES=65
HELPER_SHA='a502a1f89195150a94d7a02b8e224090d543047fb3a95c384d62260289435090'
# Absolute instruction witnesses, not a disassembly dump.
WITNESSES=(
    (0x22266e69,'8b4f0c','edict unknown at original +0x0c'),
    (0x22266e73,'ff5014','unknown virtual +0x14 returns entity'),
    (0x22266e7d,'c7470c00000000','detach edict unknown first'),
    (0x22266e84,'c70700000000','clear first edict dword second'),
    (0x22266e9b,'83c40c','three 32-bit cdecl arguments removed'),
    (0x22266ea0,'c3','original helper void return'),
    (0x221e82eb,'c20400','original virtual thunk returns one argument'),
)
CALLS=(
    (0x221e82e5,HELPER,'virtual +0x0c thunk calls helper'),
    (0x22266e8a,0x2221ffd0,'remove touched list first'),
    (0x22266e90,0x22220190,'remove ground list second'),
    (0x22266e96,0x22286c10,'remove immediately third'),
)

def witness(pe, addr: int, data: bytes, why: str) -> None:
    require(pe.at(addr,len(data))==data,
            'instruction witness changed '+why+' '+hex(addr))

def direct_call(pe, addr: int, target: int, why: str) -> None:
    ins=pe.at(addr,5)
    require(ins[0]==0xe8 and
            addr+5+struct.unpack_from('<i',ins,1)[0]==target,
            'direct call target changed '+why+' '+hex(addr))

def verify_regions(pe, thunk_sha=THUNK_SHA, helper_sha=HELPER_SHA) -> None:
    require(pe.u32(VTABLE+VIRTUAL_SLOT)==THUNK,
            'original ServerGameEnts001 +0x0c virtual mapping changed')
    require(pe.section(THUNK)=='.text' and pe.section(HELPER)=='.text',
            'original function code sections changed')
    require(hashlib.sha256(pe.at(THUNK,THUNK_BYTES)).hexdigest()==thunk_sha,
            'original virtual thunk SHA mismatch')
    require(hashlib.sha256(pe.at(HELPER,HELPER_BYTES)).hexdigest()==helper_sha,
            'original release helper SHA mismatch')
    witness(pe,THUNK+THUNK_BYTES,b'\xcc','original thunk INT3 boundary')
    witness(pe,HELPER+HELPER_BYTES,b'\xcc','original helper INT3 boundary')
    for addr,bytes_hex,why in WITNESSES:
        witness(pe,addr,bytes.fromhex(bytes_hex),why)
    for addr,target,why in CALLS:direct_call(pe,addr,target,why)

def verify(raw: bytes, check_whole: bool=True) -> dict:
    if check_whole:
        require(hashlib.sha256(raw).hexdigest()==SERVER_SHA,
                'wrong original server.dll SHA-256')
    pe=PE32(raw)
    require(pe.base==0x22000000,'wrong original PE32 image base')
    verify_regions(pe)
    return {
        'status':'ORIGINAL_GAMEENTS_FREECONTAININGENTITY_PATH_AUTHENTICATED',
        'original_server_sha256':SERVER_SHA,
        'factory_vtable':hex(VTABLE),
        'virtual_slot':hex(VIRTUAL_SLOT),
        'thunk':{'va':hex(THUNK),'bytes':THUNK_BYTES,'sha256':THUNK_SHA},
        'helper':{'va':hex(HELPER),'bytes':HELPER_BYTES,'sha256':HELPER_SHA},
        'stage_order':[hex(target) for _,target,_ in CALLS[1:]],
        'callsite_count':len(CALLS),
        'instruction_witness_count':len(WITNESSES),
        'limits':'Original code authenticated. Does not verify retail engine execution.'
    }

def self_test() -> None:
    synthetic_test()
    class Synthetic:
        def __init__(self):
            self.memory={
                0x1000:b'\xe8\x7b\x00\x00\x00', # call target 0x1080
                0x2000:b'\x11\x22\xcc'
            }
        def at(self,addr,n):
            for start,data in self.memory.items():
                if start<=addr and addr+n<=start+len(data):
                    return data[addr-start:addr-start+n]
            raise Refused('unmapped synthetic instruction')
    s=Synthetic()
    direct_call(s,0x1000,0x1080,'synthetic valid rel32')
    witness(s,0x2000,b'\x11\x22','synthetic valid witness')
    for action in ('wrong-target','wrong-witness','tampered-rel32'):
        trial=Synthetic()
        if action=='tampered-rel32':
            trial.memory[0x1000]=b'\xe8\x7c\x00\x00\x00'
        try:
            if action=='wrong-witness':
                witness(trial,0x2000,b'\x11\x23','synthetic corruption')
            else:
                direct_call(trial,0x1000,0x1081 if action=='wrong-target'
                            else 0x1080,'synthetic corruption')
        except Refused:pass
        else:raise AssertionError('tampered synthetic instruction accepted: '+action)
    require(THUNK!=HELPER and len(set(a for a,_,_ in CALLS))==4,
            'distinct virtual helper and callsite proof required')

def original_corruption_test(raw: bytes) -> None:
    pe=PE32(raw)
    for va in (VTABLE+VIRTUAL_SLOT,THUNK,HELPER,0x22266e7d,
               0x22266e84,0x22266e8a,0x22266e90,0x22266e96):
        for _,start,size,off in pe.sections:
            if start<=va<start+size:
                changed=bytearray(raw)
                changed[off+va-start]^=1
                try:verify(bytes(changed),False)
                except Refused:pass
                else:raise AssertionError('tampered original evidence admitted '+hex(va))
                break

def main() -> None:
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--server',type=Path,help='owner-local original server.dll')
    parser.add_argument('--self-test',action='store_true')
    args=parser.parse_args()
    if args.self_test:self_test()
    if not args.server:
        require(args.self_test,'specify --server or --self-test')
        print(json.dumps({'status':'SYNTHETIC_GAMEENTS_RELEASE_AUDIT_PASSED'}))
        return
    raw=args.server.read_bytes()
    result=verify(raw)
    if args.self_test:
        original_corruption_test(raw)
        result['original_corruption_test']='passed'
    print(json.dumps(result,indent=2,sort_keys=True))

if __name__=='__main__':
    try:main()
    except (Refused,OSError,struct.error) as error:
        raise SystemExit('REFUSED: '+str(error))
