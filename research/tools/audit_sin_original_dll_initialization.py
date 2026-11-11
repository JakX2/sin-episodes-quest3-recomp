#!/usr/bin/env python3
"""Phase179 original SiN x86 server/client Init interface-negotiation audit.

Chains Phase178 SHA-pinned original-binary evidence, then verifies two entire
native initializer function hashes and all 37 mandatory/4 late query sites.
Never commits original DLLs or whole disassembly bodies.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
from pathlib import Path
from audit_sin_original_engine_interface_objects import audit as phase178_audit
from audit_sin_original_dll_interface_abi import PEImage, VerificationError

MANIFEST_FILE = Path(__file__).resolve().parents[1] / 'catalog' / 'original_dll_initialization_requests.json'
MANIFEST_SHA256 = 'fcd4df72892e18013d8167e62f65ede456f454a05ee4ee9b0cc85c9077a80bef'
EXPECTED_PREFIX = {'server':16,'client':21}
EXPECTED_LATE = {'server':1,'client':3}
EXPECTED_ORIGINAL_LENGTHS={'server':875,'client':1220}
EXPECTED_FAILURE={'server':0x221e79a1,'client':0x240fe59e}
EXPECTED_SHA256={
 'server':'abb6eb2fc523fc0cb6301630e7ba9f0b61e70e48f15d5809145171250898b680',
 'client':'14d81449866220aaef1f5f906b577179ea107df78a424a364c715261adbb35db',
}


def _u32(va: str)->int:
    return int(va,16)


def _expect(pe,at:int,payload:bytes,label:str)->None:
    if pe.at(at,len(payload)) != payload:
        raise VerificationError(f'Phase179 {label} original x86 instruction mismatch at {at:#x}')


def _branch(pe,site:int,gate:int,target:int)->None:
    _expect(pe,gate,b'\x0f\x84'+struct.pack('<i',target-(gate+6)),
            f'mandatory failure branch for {site:#x}')


def original_manifest()->dict:
    raw=MANIFEST_FILE.read_bytes()
    if hashlib.sha256(raw).hexdigest()!=MANIFEST_SHA256:
        raise VerificationError('Phase179 initialization request manifest SHA256 mismatch')
    return json.loads(raw)


def verify_initializer(pe:PEImage, dll:str, info:dict, *, check_hash=True)->dict:
    if dll not in EXPECTED_PREFIX:
        raise VerificationError('unsupported original DLL for initialization audit')
    start=_u32(info['function_va'])
    length=info['function_len']
    if length!=EXPECTED_ORIGINAL_LENGTHS[dll] or info['original_sha256']!=EXPECTED_SHA256[dll]:
        raise VerificationError('original initialization function metadata mismatch')
    if check_hash and hashlib.sha256(pe.at(start,length)).hexdigest()!=EXPECTED_SHA256[dll]:
        raise VerificationError('original complete Init function SHA256 mismatch')
    prefix=info['prefix_requests'];later=info['later_requests']
    if len(prefix)!=EXPECTED_PREFIX[dll] or len(later)!=EXPECTED_LATE[dll]:
        raise VerificationError('original Init interface request count mismatch')
    if _u32(info['failure_return_va'])!=EXPECTED_FAILURE[dll]:
        raise VerificationError('original Init failure return metadata mismatch')
    observed=[]
    for i,rec in enumerate(prefix+later):
        name=rec['name'];name_va=_u32(rec['name_va'])
        site=_u32(rec['request_va']);store=_u32(rec['storage_va'])
        if not (start<=site<start+length):
            raise VerificationError('native Init request site outside function')
        if pe.section_of(name_va)!='.rdata' or pe.section_of(store)!='.data':
            raise VerificationError('original Init interface name/store section mismatch')
        _expect(pe,name_va,name.encode('ascii')+b'\x00',f'interface version {name}')
        if i<len(prefix):
            if rec['phase']!='mandatory_prefix' or rec['required'] is not True:
                raise VerificationError('incorrect original mandatory prefix metadata')
            if rec['factory']=='derived_virtual':
                raise VerificationError('early prefix unexpectedly has derived factory')
        if rec.get('special_initial_store'):
            if dll!='client' or site!=0x240fe109 or i!=0:
                raise VerificationError('unexpected special engine-factory pre-store')
            _expect(pe,site,b'\x6a\x00\x68'+struct.pack('<I',name_va),'initial interface PUSH')
            _expect(pe,site+7,b'\xa3'+struct.pack('<I',0x242fa458),'initial globals snapshot')
            _expect(pe,site+12,b'\xff\xd6\x83\xc4\x28\x85\xc0','initial factory call')
            _expect(pe,site+19,b'\xa3'+struct.pack('<I',store),'initial return pointer storage')
            _branch(pe,site,site+24,EXPECTED_FAILURE[dll])
        elif rec['factory'] in ('engine','filesystem'):
            _expect(pe,site,b'\x6a\x00\x68'+struct.pack('<I',name_va),f'{name} query arguments')
            call=b'\xff\xd7' if rec['factory']=='filesystem' else b'\xff\xd6'
            _expect(pe,site+7,call,f'{name} engine/filesystem factory origin')
            if rec['phase']=='mandatory_prefix':
                _expect(pe,site+9,b'\x83\xc4\x08\x85\xc0\xa3'+struct.pack('<I',store),
                        f'{name} pointer store after null test')
                _branch(pe,site,site+19,EXPECTED_FAILURE[dll])
            elif dll=='server' and name=='VDebugOverlay003':
                _expect(pe,site+9,b'\x68\x10\x05\x00\x00\xa3'+struct.pack('<I',store),
                        'server optional late debug overlay no-branch store')
            elif dll=='client' and name=='VMaterialSystemStub001':
                _expect(pe,site+9,b'\x8b\x0d'+struct.pack('<I',0x2433f69c)+
                        b'\x83\xc4\x08\x6a\x00\xa3'+struct.pack('<I',store),
                        'client optional material stub store')
            else:
                raise VerificationError('unexpected native optional Init query')
        elif rec['factory']=='derived_virtual':
            if dll!='client' or rec['phase']!='later_conditional' or not rec['required']:
                raise VerificationError('unknown later original indirect engine interface request')
            _expect(pe,site,b'\x68'+struct.pack('<I',name_va),f'{name} derived interface push')
            expected_opcode={
                'MaterialSystemHardwareConfig012':b'\xff\x50\x20',
                'MatSystemSurface003':b'\xff\x50\x08',
            }.get(name)
            if not expected_opcode:
                raise VerificationError('unexpected derived query')
            _expect(pe,site+5,expected_opcode+b'\x85\xc0\xa3'+struct.pack('<I',store),
                    f'{name} required derived interface lookup')
            _branch(pe,site,site+15,EXPECTED_FAILURE[dll])
        else:
            raise VerificationError('unknown factory origin')
        observed.append(name)
    if len(set(_u32(r['request_va']) for r in prefix+later))!=len(prefix)+len(later):
        raise VerificationError('duplicate native interface Init request sites')
    if dll=='server':
        _expect(pe,EXPECTED_FAILURE[dll],bytes.fromhex('5f32c05e83c40cc21000'),'server return false')
        _expect(pe,0x221e7978,bytes.fromhex('5fb0015e83c40cc21000'),'server first success')
        # The failure path after the three helper checks and optional overlay
        # shares the same return address. The optional overlay is NOT checked.
    else:
        _expect(pe,EXPECTED_FAILURE[dll],bytes.fromhex('33c05ec20c00'),'client return false')
        _expect(pe,0x240fe595,bytes.fromhex('b8010000005ec20c00'),'client return true')
    return {
       'complete_original_Init_function_hashes':int(check_hash),
       'original_native_Init_bytes_hashed':length if check_hash else 0,
       'mandatory_early_interface_queries':len(prefix),
       'later_interface_queries':len(later),
       'later_optional_without_immediate_null_gate':sum(not r['required'] for r in later),
       'failure_return_va':info['failure_return_va'],
       'all_requested_interface_versions_verified':observed,
       'limitations':['This covers initial interface lookups; intervening engine callbacks and full original Init implementation are not reconstructed',
                      'No real engine-owned initialization or playable client/server DLL execution demonstrated'],
    }


def audit(client_bytes:bytes,server_bytes:bytes)->dict:
    report=phase178_audit(client_bytes,server_bytes)
    manifest=original_manifest()
    report['phase']=179
    for dll,data in [('server',server_bytes),('client',client_bytes)]:
        report[dll]['initializer_interface_negotiation']=verify_initializer(PEImage(data),dll,manifest['dlls'][dll])
    report['original_Initialization_manifest_sha256']=MANIFEST_SHA256
    return report


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--server',type=Path,required=True)
    p.add_argument('--client',type=Path,required=True)
    p.add_argument('--output',type=Path)
    args=p.parse_args()
    result=json.dumps(audit(args.client.read_bytes(),args.server.read_bytes()),indent=2)+'\n'
    if args.output:
        args.output.parent.mkdir(parents=True,exist_ok=True)
        args.output.write_text(result)
    else:print(result,end='')
if __name__=='__main__': main()