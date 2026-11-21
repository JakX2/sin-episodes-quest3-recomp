#!/usr/bin/env python3
"""Original 2006 SiN client.dll VClient011 Init suffix: SHA-pinned control-flow audit.

Pass the owner's unmodified client.dll/server.dll ZIP with --bin-zip.
No bytes from the original binaries are emitted or committed.
The standalone --self-test runs without any game files.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import struct
import zipfile
from pathlib import Path

# Reuse the already authenticated Phase538 PE32 and original-file identities.
from audit_sin_original_client_init_interface_manifest import (
    CLIENT_SHA, SERVER_SHA, PE32, INIT_VA, INIT_LEN, INIT_SHA,
)

# Original complete functions (VA, exact length, SHA256). No raw bytes.
METHODS = (
    (0x24156840, 23,  'e48841e7e14a9565ac425416c3fc3230615c8246fa603be65d7149500396292e', 'game_system_append_adapter'),
    (0x24156860, 277, 'b4f5ca8aefcb1248cdd7a62edf042a3f7fd5a04ac782fde277d13f335067ccb7', 'game_system_init_validation'),
    (0x24164120, 54,  '645a9002d77b476692fe8688d691e87c4d2ebf8a61976be9eda071e41d7d3b7f', 'registered_interface_resolver'),
    (0x24182C60, 97,  '46e079ea76e0cfee721efe3a421d68920cf980079aea4d76d7542f6f9b5281f2', 'secondary_factory_requirements'),
    (0x240FCDD0, 74,  '8a9a82e9e7a8bbd9eb61c80802270817f00c8053bff3b21ba941e9c46311d289', 'final_client_object_allocation'),
)
FAIL = 0x240FE59E
# Ordered, native rel32 CALLs in original VClient011::Init.
APPEND_SITES = (
    0x240FE3FC, 0x240FE407, 0x240FE420, 0x240FE42B,
    0x240FE436, 0x240FE441, 0x240FE44D, 0x240FE457,
    0x240FE461, 0x240FE46B, 0x240FE476, 0x240FE481,
)
# (call address, native function target, initial NULL/BOOL failure branch)
INIT_GATES = (
    (0x240FE39B,0x24164120,0x240FE3A2),
    (0x240FE3B9,0x2417F680,0x240FE3C0),
    (0x240FE3C7,0x241EDFB0,0x240FE3D1),
    (0x240FE4B3,0x24156860,0x240FE4BA),
    (0x240FE540,0x24182C60,0x240FE54A),
)
# Second factory helper original query site (name pointer, resulting global, literal)
SECOND_FACTORY = (
    (0x24182C65,0x242A2338,0x24374398,'VPhysics030'),
    (0x24182C7A,0x242A2320,0x243743A8,'VPhysicsSurfaceProps001'),
    (0x24182C8F,0x242A2308,0x243743A0,'VPhysicsCollision007'),
)

class Refusal(ValueError):
    pass

def require(value, description):
    if not value:
        raise Refusal(description)

def verify_rel_call(read, site, target):
    b = read(site,5)
    require(len(b)==5 and b[0]==0xE8, f'not a native CALL rel32 at {site:#x}')
    got = site+5+struct.unpack_from('<i',b,1)[0]
    require(got==target, f'wrong call target at {site:#x}: {got:#x}')

def verify_cond_branch(read,site,target):
    b = read(site,6)
    if b[:2]==b'\x0f\x84':
        got=site+6+struct.unpack_from('<i',b,2)[0]
    elif b[0] == 0x74:  # short JE, read first two bytes only
        got=site+2+struct.unpack('<b',b[1:2])[0]
    else:
        raise Refusal(f'not a JE fail branch at {site:#x}')
    require(got==target, f'wrong JE destination at {site:#x}: {got:#x}')

def check_original(server:bytes,client:bytes)->dict:
    require(hashlib.sha256(server).hexdigest()==SERVER_SHA, 'server.dll whole SHA mismatch')
    require(hashlib.sha256(client).hexdigest()==CLIENT_SHA, 'client.dll whole SHA mismatch')
    pe=PE32(client)
    require(pe.base==0x24000000, 'unexpected client.dll image base')
    require(hashlib.sha256(pe.at(INIT_VA,INIT_LEN)).hexdigest()==INIT_SHA,
            'original VClient011 Init full-body digest mismatch')
    for va,size,sha,name in METHODS:
        require(pe.section(va)=='.text',name+' outside executable section')
        require(hashlib.sha256(pe.at(va,size)).hexdigest()==sha,
                name+' original whole-function digest mismatch')
        require(pe.at(va+size,1)==b'\xcc',name+' exact function boundary missing')
    # Ordered twelve original calls to the append adapter.
    for site in APPEND_SITES:
        verify_rel_call(pe.at,site,0x24156840)
    # 12 explicit calls in a 0x3F6..0x486 run; after the last call
    # the original stack is explicitly adjusted by 0x28 bytes.
    require(pe.at(0x240FE48E,3)==b'\x83\xc4\x28','12 appended arguments cleanup changed')
    # Game system append helper: passes pointer to its own stack argument
    # and count to 0x24156640, then returns normally.
    verify_rel_call(pe.at,0x24156851,0x24156640)
    require(pe.at(0x24156856,1)==b'\xc3','game-system append adapter return')
    # The main game-system list gets consumed into the growable array:
    require(pe.at(0x24156860,5)==b'\xa1'+struct.pack('<I',0x24366C8C),
            'game-system linked-list head changed')
    for va,blob in (
        (0x24156886,b'\x8b\x15'+struct.pack('<I',0x242FBFA8)),
        (0x24156950,b'\xa1'+struct.pack('<I',0x242FBFA8)),
        (0x2415695A,b'\xff\x12'), # call first virtual method of each game system
        (0x24156960,b'\x83\xc6\x01'),
        (0x24156935,b'\xc7\x05'+struct.pack('<I',0x24366C8C)+b'\x00'*4),
        (0x2415693F,b'\xc6\x05'+struct.pack('<I',0x24366C84)+b'\x01'),
        (0x2415696C,b'\xb0\x01'),(0x24156971,b'\x32\xc0'),
    ):
        require(pe.at(va,len(blob))==blob, f'game system initialization witness changed at {va:#x}')
    # Separate originally linked-list-based required-interface resolver.
    require(pe.at(0x24164121,6)==b'\x8b\x35'+struct.pack('<I',0x24369374),
            'interface resolver list pointer changed')
    require(pe.at(0x24164130,3)==b'\xff\x56\x04',
            'interface resolver getter callback changed')
    verify_rel_call(pe.at,0x2416414D,0x241640F0)
    require(pe.at(0x24164149,2)==b'\xb0\x01', 'interface resolver success changed')
    require(pe.at(0x24164152,2)==b'\x32\xc0', 'interface resolver failure changed')
    # Distinct boolean gates in the Init suffix. All end at same native fail.
    for site,target,branch in INIT_GATES:
        verify_rel_call(pe.at,site,target)
        verify_cond_branch(pe.at,branch,FAIL)
    # Note the last gate has a short JE, not the same 6-byte opcode.
    for site,nameptr,globalptr,name in SECOND_FACTORY:
        require(pe.at(nameptr,len(name)+1)==name.encode()+b'\x00',
                'second factory interface name changed: '+name)
        require(pe.at(site,2)==b'\x6a\x00',
                f'second factory null return-status push changed at {site:#x}')
        require(pe.at(site+2,5)==b'\x68'+struct.pack('<I',nameptr),
                f'second factory name push changed at {site:#x}')
        call_site=site+7
        require(pe.at(call_site,2)==b'\xff\xd6',
                f'second factory indirect CALL changed at {call_site:#x}')
        # The helper stores each query result in its original global.
        store_site=call_site+7 # call(2) + cleanup(3) + test(2)
        require(pe.at(store_site,5)==b'\xa3'+struct.pack('<I',globalptr),
                f'second factory pointer destination changed at {store_site:#x}')
    for site in (0x24182C78,0x24182C8D,0x24182CA2):
        verify_cond_branch(pe.at,site,0x24182CBD)
    verify_rel_call(pe.at,0x24182CB1,0x241927A0)
    require(pe.at(0x24182CB9,2)==b'\xb0\x01' and pe.at(0x24182CBD,2)==b'\x32\xc0',
            'secondary factory helper success/fail returns changed')
    # Final Init step is NOT a bool-gated failure: object allocation uses
    # 0x4B0 bytes; a null result writes a null global and still returns.
    verify_rel_call(pe.at,0x240FE590,0x240FCDD0)
    require(pe.at(0x240FCDD1,5)==b'\x68\xb0\x04\x00\x00',
            'late client object allocation amount changed')
    verify_rel_call(pe.at,0x240FCDD6,0x240C4FF0)
    require(pe.at(0x240FCE06,6)==b'\x89\x35'+struct.pack('<I',0x2433F3F4),
            'late allocated object global destination changed')
    require(pe.at(0x240FCE0E,10)==b'\xc7\x05'+struct.pack('<I',0x2433F3F4)+b'\x00'*4,
            'late allocation null branch changed')
    # Final method returns bool success independently of allocation result.
    require(pe.at(0x240FE595,5)==b'\xb8\x01\x00\x00\x00' and
            pe.at(0x240FE59E,2)==b'\x33\xc0',
            'original Init success and shared failure return values changed')
    return {
        'status':'ORIGINAL_CLIENT_INIT_TAIL_CONTROL_FLOW_AUTHENTICATED',
        'client_sha256':CLIENT_SHA,
        'original_init_full_length':INIT_LEN,
        'additional_complete_original_methods':len(METHODS),
        'additional_complete_original_bytes':sum(n for _,n,_,_ in METHODS),
        'init_game_system_registration_calls':len(APPEND_SITES),
        'exact_fail_closed_late_gates':len(INIT_GATES),
        'secondary_factory_required_names':[v[3] for v in SECOND_FACTORY],
        'notes':'No source code equivalence, engine/physics implementation or game boot claimed.'
    }

def self_test():
    a=0x24000100;b=0x2400F000
    original=b'\xe8'+struct.pack('<i',b-a-5)
    verify_rel_call(lambda va,n:original[:n],a,b)
    for i in range(5):
        x=bytearray(original);x[i]^=1
        try:verify_rel_call(lambda va,n:bytes(x[:n]),a,b)
        except Refusal:pass
        else:raise AssertionError('mutated CALL accepted '+str(i))
    je=b'\x0f\x84'+struct.pack('<i',b-a-6)
    verify_cond_branch(lambda va,n:je[:n],a,b)
    for i in range(6):
        x=bytearray(je);x[i]^=1
        try:verify_cond_branch(lambda va,n:bytes(x[:n]),a,b)
        except Refusal:pass
        else:raise AssertionError('mutated JE accepted '+str(i))
    short=b'\x74\x52'
    verify_cond_branch(lambda va,n:short[:n],a,a+2+0x52)
    try: check_original(b'not DLL',b'not DLL')
    except Refusal:pass
    else:raise AssertionError('untrusted original accepted')
    print('PASS five synthetic CALL mutations, six synthetic JE mutations, short JE target and wrong DLL hashes refused')

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--bin-zip',type=Path)
    p.add_argument('--self-test',action='store_true')
    args=p.parse_args()
    if not args.self_test and args.bin_zip is None:p.error('choose --self-test or --bin-zip')
    if args.self_test:self_test()
    if args.bin_zip:
        with zipfile.ZipFile(args.bin_zip) as z:
            by_basename={Path(n).name.lower():n for n in z.namelist()}
            require('client.dll' in by_basename and 'server.dll' in by_basename,
                    'ZIP lacks original client.dll/server.dll pair')
            report=check_original(z.read(by_basename['server.dll']),z.read(by_basename['client.dll']))
        print(json.dumps(report,indent=2))

if __name__=='__main__':main()