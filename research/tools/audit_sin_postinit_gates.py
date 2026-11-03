#!/usr/bin/env python3
"""SHA-pinned original 2006 SiN client/server post-core Init x86 audit.

Verifies the tail of the two genuine game-module Init functions. This is NOT
an Android SourceVR interface adapter and does not execute either DLL.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
from pathlib import Path
from audit_sin_init_contracts import audit as prior_init_audit
from audit_sin_original_factories import PE32, VerificationError

POST = {
    'client.dll': {
        'start':0x240fe32a,'end':0x240fe595,
        'sha256':'0617a6584da528bb2e3e5a7c9967b305a8fba83b732300fef1db7be2e846dd2d',
        'fail':0x240fe59e,
        'gate_direct':(
            (0x240fe39b,0x24164120,0x240fe3a0,0x240fe3a2,'near'),
            (0x240fe3b9,0x2417f680,0x240fe3be,0x240fe3c0,'near'),
            (0x240fe3c7,0x241edfb0,0x240fe3cf,0x240fe3d1,'near'),
            (0x240fe4b3,0x24156860,0x240fe4b8,0x240fe4ba,'near'),
            (0x240fe540,0x24182c60,0x240fe548,0x240fe54a,'short'),
        ),
        'gate_indirect':(
            (0x240fe367,'ff5020',0x240fe36a,0x240fe371,'near',
             'MaterialSystemHardwareConfig012',0x240fe362),
            (0x240fe3e6,'ff5008',0x240fe3e9,0x240fe3f0,'near',
             'MatSystemSurface003',0x240fe3e1),
        ),
    },
    'server.dll': {
        'start':0x221e77f0,'end':0x221e79a1,
        'sha256':'629d342cd1709e59b3473904650b70a57b32cfd2203e164270e7ffc496f468de',
        'fail':0x221e79a1,
        'gate_direct':(
            (0x221e7927,0x221f5700,0x221e792c,0x221e792e,'short'),
            (0x221e7930,0x221ad8a0,0x221e7935,0x221e7937,'short'),
            (0x221e7939,0x22149b60,0x221e793e,0x221e7940,'short'),
        ),
        'gate_indirect':(),
    },
}

def require(pe,va,raw):
    if pe.at(va,len(raw)) != raw:
        raise VerificationError('SiN post-core Init opcode mismatch at %#x' % va)

def require_push_string(pe,va,text):
    require(pe,va,b'\x68')
    ptr=struct.unpack('<I',pe.at(va+1,4))[0]
    if pe.cstring(ptr)!=text:
        raise VerificationError('SiN post-core Init string mismatch at %#x'%va)
    return ptr

def check_branch(pe,site,kind,fail):
    if kind=='near':
        require(pe,site,b'\x0f\x84'+struct.pack('<i',fail-site-6))
    else:
        offset=fail-site-2
        if not -128<=offset<128:
            raise VerificationError('short branch out of range')
        require(pe,site,b'\x74'+struct.pack('<b',offset))

def audit_post_one(pe,module,verify_region_hash=True):
    spec=POST[module]
    if verify_region_hash and hashlib.sha256(pe.at(spec['start'],spec['end']-spec['start'])).hexdigest()!=spec['sha256']:
        raise VerificationError('SiN post-core original region SHA-256 mismatch: '+module)
    gates=[]
    for call,target,test,branch,kind in spec['gate_direct']:
        require(pe,call,b'\xe8'+struct.pack('<i',target-call-5))
        require(pe,test,b'\x84\xc0')
        check_branch(pe,branch,kind,spec['fail'])
        gates.append({'site_va':hex(call),'kind':'direct_helper_bool','target_va':hex(target),'failure_va':hex(spec['fail'])})
    for call,encoding,test,branch,kind,name,push in spec['gate_indirect']:
        require_push_string(pe,push,name)
        require(pe,call,bytes.fromhex(encoding))
        require(pe,test,b'\x85\xc0')
        check_branch(pe,branch,kind,spec['fail'])
        gates.append({'site_va':hex(call),'kind':'virtual_query_nonnull','interface_name':name,'failure_va':hex(spec['fail'])})
    if module=='client.dll':
        require_push_string(pe,0x240fe349,'VMaterialSystemStub001')
        require(pe,0x240fe34e,b'\xff\xd6')
        require(pe,0x240fe35b,b'\xa3\xa0\xf6\x33\x24')
        require_push_string(pe,0x240fe332,'-textmode')
        require(pe,0x240fe339,b'\xff\x52\x28')
        require(pe,0x240fe33c,b'\x85\xc0\x74\x07')
        require(pe,0x240fe340,b'\xc6\x05\xfc\xf6\x33\x24\x01')
        nonfatal=['VMaterialSystemStub001','-textmode flag']
        resources=[]
        ordered=[g for g in gates if g['kind']=='virtual_query_nonnull' and g['interface_name']=='MaterialSystemHardwareConfig012']
        ordered += [g for g in gates if g['kind']=='direct_helper_bool' and int(g['site_va'],16)<0x240fe3e6]
        ordered += [g for g in gates if g.get('interface_name')=='MatSystemSurface003']
        ordered += [g for g in gates if g['kind']=='direct_helper_bool' and int(g['site_va'],16)>=0x240fe4b3]
    else:
        require_push_string(pe,0x221e783a,'resource/gameevents.res')
        require(pe,0x221e783f,b'\xff\x52\x04')
        require_push_string(pe,0x221e791f,'resource/ModEvents.res')
        require(pe,0x221e7924,b'\xff\x52\x04')
        require_push_string(pe,0x221e7847,'sv_cheats')
        require(pe,0x221e784c,b'\xe8'+struct.pack('<i',0x2225bc00-0x221e784c-5))
        require(pe,0x221e7854,b'\x85\xc0')
        require(pe,0x221e7856,b'\xa3\x68\x5e\x57\x22')
        check_branch(pe,0x221e785b,'near',spec['fail'])
        require_push_string(pe,0x221e7944,'VDebugOverlay003')
        require(pe,0x221e7949,b'\xff\xd6')
        require(pe,0x221e7950,b'\xa3\xcc\x21\x57\x22')
        require(pe,0x221e794b,b'\x68\x10\x05\x00\x00')
        require(pe,0x221e7955,b'\xe8'+struct.pack('<i',0x2226e790-0x221e7955-5))
        require(pe,0x221e795d,b'\x85\xc0\x74\x21')
        require(pe,0x221e7978,b'\x5f\xb0\x01')
        require(pe,0x221e7997,b'\x5f\xb0\x01')
        require(pe,0x221e7988,b'\xc7\x05\x88\xb8\x54\x22\x00\x00\x00\x00')
        nonfatal=['VDebugOverlay003','0x510-byte allocation (null returns success via fallback)']
        resources=['resource/gameevents.res','resource/ModEvents.res']
        ordered=[{'site_va':hex(0x221e784c),'kind':'direct_helper_nonnull','target_va':hex(0x2225bc00),
                  'requested_name':'sv_cheats','failure_va':hex(spec['fail'])}]+sorted(gates,key=lambda g:int(g['site_va'],16))
    return {
      'original_module':module,'original_post_core_va_range':[hex(spec['start']),hex(spec['end'])],
      'original_subregion_sha256':spec['sha256'] if verify_region_hash else 'synthetic_fixture',
      'late_required_checks':ordered,'late_required_check_count':len(ordered),
      'nonfatal_immediate_observations':nonfatal,'resource_registration_paths':resources,
      'shared_failure_va':hex(spec['fail']),
      'limitations':'Demonstrated post-core gates and local nonfatal branches only. No semantic helper names, full game Init implementation, engine ABI or ARM64 game DLL.'
    }

def audit(client,server):
    # Full prior Phase85 -> Phase84 original dual-DLL SHA, vtables and Init gate checks.
    prior_init_audit(client,server)
    return {'client':audit_post_one(PE32(client),'client.dll'),
            'server':audit_post_one(PE32(server),'server.dll')}

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--client',required=True,type=Path)
    p.add_argument('--server',required=True,type=Path)
    p.add_argument('--output',required=True,type=Path)
    a=p.parse_args()
    result=audit(a.client.read_bytes(),a.server.read_bytes())
    a.output.parent.mkdir(parents=True,exist_ok=True)
    a.output.write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    print(json.dumps({'client_post_gates':result['client']['late_required_check_count'],
      'server_post_gates':result['server']['late_required_check_count']},indent=2))
if __name__=='__main__':main()
