#!/usr/bin/env python3
"""Exact-original SiN 2006 client/server post-Init game-subsystem audit.

Chains Phase86's SHA-pinned owner-held PE32 verifier. No original game code
bytes are saved or committed; the output is metadata and original addresses.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path
from audit_sin_postinit_gates import audit as previous_original_audit
from audit_sin_original_factories import PE32, VerificationError

def pairs(raw):
    return {int(k,16):v for part in raw.split() for k,v in [part.split('=')]}
C="client.dll"
S="server.dll"
REGIONS={
 C:{
 "client_registry_setup":(0x24164120,54,"645a9002d77b476692fe8688d691e87c4d2ebf8a61976be9eda071e41d7d3b7f"),
 "client_registry_cleanup":(0x241640f0,48,"93dbf327403a2963eba2ee6341b0aa5bfc0a7fef581840e57b71b418d7ae78d2"),
 "client_vgui_startup":(0x241edfb0,123,"64cbfa079f093b1e577a59721396d73742eb4543b836e7fb0cc55d724783417d"),
 "client_particle_bucket":(0x2417f680,82,"702b2e6cd174ce3abc939e91d99710a3669a0c23b82aa50b2904064445a3cf5a")
 },
 S:{
 "server_system_registry":(0x221f5700,277,"1e2a6e068b24b061fbd5cba16741f8ca8f470421b7ccbca3fa0239f579a9c530"),
 "server_repository_wrapper_a":(0x221ad8a0,10,"83762183370ab5370d0c099c4350b20768c57043ed99066edb8e1b1804da49e0"),
 "server_repository_impl_a":(0x221ad3e0,576,"7b9db352473a8fc2d5343473b673ac52c89a8c98320847b0b4dd451dcf8e4e69"),
 "server_repository_wrapper_b":(0x22149b60,10,"47793d5818d074eb9ad8526102a131bb60fd50ab5fcaaa52aa5afe6e940852d9"),
 "server_repository_impl_b":(0x221498c0,576,"94b55dbf7e21d7b45362d277505a8a7c02537667c3c4c4afbeeded9c40ec2218")
 }
}
SITES={
 C:pairs("""
24164121=8b3574933624 24164130=ff5604 24164133=85c0 24164135=7406
24164137=8b0e 24164139=8901 2416413d=833e00 24164142=8b760c
24164149=b001 24164152=32c0 241640f1=8b3574933624 24164100=8b06
24164104=7411 24164106=8b00 24164109=ff5608 24164111=c70100000000
24164117=8b760c 241edfb0=6a01 241edfb7=6824c52a24 241edfc4=84c0
241edfc8=32c0 241edfcd=680cc52a24 241edfd2=ff54240c 241edfdb=a3b88a3724
241edfe4=68c0322d24 241edfe9=ff54240c 241edff2=a3bc8a3724 241edff9=803d288b372400
241ee00c=c605288b372401 241ee025=85c0 241ee027=0f95c0 2417f68c=6a30
2417f68e=894640 2417f699=85c0 2417f6a1=68e01b2a24 2417f6a6=6a00
2417f6a9=6a60 2417f6b2=898684000000 2417f6ba=0f95c0 2417f6c3=898684000000
"""),
 S:pairs("""
221f5700=a1c02c5722 221f5707=8b0d640a5122 221f5718=8b1d28213d22 221f5726=8b15580a5122
221f5730=393c82 221f5743=a15c0a5122 221f5779=890d640a5122 221f577f=8915680a5122
221f57af=8938 221f57b3=68383e4422 221f57c5=8b7f04 221f57d5=c705c02c572200000000
221f57df=c605b82c572201 221f57f0=a1580a5122 221f57f5=8b0cb0 221f57f8=8b11
221f57fa=ff12 221f57fc=84c0 221f57fe=7410 221f580c=b001
221f5811=32c0 221ad8a0=b9e03e5022 22149b60=b988034f22 221ad3e9=807e3d00
221ad401=c6463d01 221ad3f9=8b4634 221ad5fd=b001 221ad616=32c0
221498c9=807e3d00 221498e1=c6463d01 221498d9=8b4634 22149add=b001
22149af6=32c0
""")
}
CALLS={
 C:{0x240fe39b:0x24164120,0x240fe3b9:0x2417f680,0x240fe3c7:0x241edfb0,
    0x2416414d:0x241640f0,0x241edfbc:0x240010d0,0x241ee013:0x24009340,
    0x241ee01b:0x2422ecd0,0x241ee020:0x2422ecf0,0x2417f683:0x2417f3a0,
    0x2417f691:0x2416fcf0,0x2417f6ad:0x241704c0},
 S:{0x221e7927:0x221f5700,0x221e7930:0x221ad8a0,0x221e7939:0x22149b60,
    0x221f575c:0x221f51a0,0x221f5794:0x223a5d10}
}
TAIL_JUMPS={C:{},S:{0x221ad8a5:0x221ad3e0,0x22149b65:0x221498c0}}
STRINGS={
 C:{0x242ac524:b'CLIENT\0',0x242ac50c:b'VGUI_InputInternal001\0',
    0x242d32c0:b'VGUI_System010\0',
    0x242a1be0:b'CParticleMgr::m_pParticleBucket\0'},
 S:{0x22443e38:b'AutoGameSystem already added to game system list!!!\n\0'}
}
def verify_subsystem(pe,module,hash_check=True):
    sections={}
    for name,(va,size,digest) in REGIONS[module].items():
        if hash_check and hashlib.sha256(pe.at(va,size)).hexdigest()!=digest:
            raise VerificationError('Phase87 original code SHA-256 mismatch: '+name)
        sections[name]={'virtual_address':hex(va),'byte_length':size,
                        'sha256':digest if hash_check else None}
    for va,raw in SITES[module].items():
        if pe.at(va,len(bytes.fromhex(raw))) != bytes.fromhex(raw):
            raise VerificationError('Phase87 original opcode mismatch at %#x'%va)
    for va,target in CALLS[module].items():
        raw=b'\xe8'+struct.pack('<i',target-va-5)
        if pe.at(va,5)!=raw:
            raise VerificationError('Phase87 original call mismatch at %#x'%va)
    for va,target in TAIL_JUMPS[module].items():
        raw=b'\xe9'+struct.pack('<i',target-va-5)
        if pe.at(va,5)!=raw:
            raise VerificationError('Phase87 original tail jump mismatch at %#x'%va)
    for va,raw in STRINGS[module].items():
        if pe.at(va,len(raw)) != raw:
            raise VerificationError('Phase87 original string mismatch at %#x'%va)
    report={'module':module,'function_regions':sections,'opcode_sites':len(SITES[module]),
            'call_edges':len(CALLS[module]),'tail_jumps':len(TAIL_JUMPS[module]),
            'strings':len(STRINGS[module]),'limitations':'Reference behavior only; no native SiN Android client or server module.'}
    if module==C:
        report.update({
            'registry_head_va':'0x24369374','registry_setup_va':'0x24164120',
            'registry_cleanup_va':'0x241640F0',
            'registry_x86_fields':{'slot':'+0x00','setup':'+0x04','teardown':'+0x08','next':'+0x0C'},
            'rollback_on_required_failure':True,
            'vgui_module':'CLIENT','vgui_interface_names':['VGUI_InputInternal001','VGUI_System010'],
            'vgui_saved_global_vas':['0x24378AB8','0x24378ABC'],
            'particle_bucket_member_offset':'0x84',
            'particle_bucket_allocation_bytes':48,'particle_bucket_ctor_argument':96,
        })
    else:
        report.update({
            'registered_vector_va':'0x22510A58','registered_count_va':'0x22510A64',
            'registered_capacity_va':'0x22510A5C','pending_head_va':'0x22572CC0',
            'initialized_flag_va':'0x22572CB8',
            'pending_next_offset':'+0x04','per_system_init_virtual_offset':'+0x00',
            'initialized_flag_set_before_callbacks':True,'deduplicates_system_pointers':True,
            'repository_wrappers':[
                {'entry':'0x221AD8A0','singleton':'0x22503EE0','implementation':'0x221AD3E0'},
                {'entry':'0x22149B60','singleton':'0x224F0388','implementation':'0x221498C0'}],
            'repository_state_offsets':{'filename':'+0x34','started':'+0x3D','other_state':'+0x3C'},
            'repository_semantics':'Two file-backed initialization routines: complete parsing not yet recovered'
        })
    return report
def audit(client,server):
    previous_original_audit(client,server)
    return {'client':verify_subsystem(PE32(client),C),
            'server':verify_subsystem(PE32(server),S)}
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--client',required=True,type=Path)
    p.add_argument('--server',required=True,type=Path)
    p.add_argument('--output',required=True,type=Path)
    a=p.parse_args()
    report=audit(a.client.read_bytes(),a.server.read_bytes())
    a.output.parent.mkdir(parents=True,exist_ok=True)
    a.output.write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    for name in ('client','server'):
        r=report[name]
        print(name,len(r['function_regions']),'regions,',r['opcode_sites'],
              'opcode sites,',r['call_edges'],'CALL edges')
if __name__=='__main__':main()
