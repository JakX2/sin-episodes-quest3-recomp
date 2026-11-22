#!/usr/bin/env python3
"""Phase602: independently verify five original 2006 Grunt behavior ACT methods.

Uses Phase601's original SHA-checked PE32 mapper. Original retail DLLs stay
private. No original machine bytes or disassembly dumped or committed.
"""
import argparse
import hashlib
import json
import struct
import zipfile
from pathlib import Path
from audit_sin_original_grunt_behavior_event54_phase601 import (
    SERVER,CLIENT,PE32,Refusal,require,check_edge
)
# 2006 whole-method bytes/SHA256, followed by exact original INT3 padding.
# Important: Standoff ends after just 142 bytes, with 2 INT3 then another function.
ACT_METHODS=(
 ('Common behavior bridge',0x220CD400,8,'b7bd9eccad64400217c35647d73def30cb590070fdf477ff01b2d56b7eb0ccf4',8),
 ('CAI_FuncTankBehavior',0x22392700,19,'7dc9cc2cb138916a0ffa128954a400c587c0e07c6a3c6d16c371b08d18e846c9',4),
 ('CAI_TakecoverBehavior',0x22398670,457,'286e96bf50d40a21365ba40578c3154317bcd646cbe4bbcd3827491a2ed44851',4),
 ('CGruntStandoffBehavior',0x220E1EE0,142,'28d47fabade3850134945b8e988f988e42370608920efd8b709e0955c3ad4133',2),
 ('CAI_FollowBehavior',0x220D33B0,166,'415e92b3d358ef558a9bfcc0dd61acd4877eded4df38a457b48b1db5f98946c2',4),
)
# Eight real original Grunt behavior RTTI-vtables each have virtual slot +0x7C.
ORIGINAL_8=(
 ('CAI_RappelBehavior',0x223F956C,0x220CD400),
 ('CAI_FuncTankBehavior',0x224ABEC4,0x22392700),
 ('CAI_TakecoverBehavior',0x224AE624,0x22398670),
 ('CAI_BeserkBehavior',0x224AB88C,0x220CD400),
 ('CAI_ActBusyBehavior',0x224AB494,0x220CD400),
 ('CAI_AssaultBehavior',0x223F5544,0x220CD400),
 ('CGruntStandoffBehavior',0x22495344,0x220E1EE0),
 ('CAI_FollowBehavior',0x223F6B7C,0x220D33B0),
)
# Exact direct native E9 or E8 branches to common original activity delegate.
DELEGATES={
 0x2239270E:0x220CD400,
 0x2239882E:0x220CD400,
 0x220E1F64:0x220CD400,
 0x220D344B:0x220CD400,
}
# Sparse, original method-size SHA-constrained opcode witnesses only.
WITNESSES={
 0x220CD400:'8b490c8b01ff600c',
 0x22392700:'80791800',0x22392706:'b871010000',
 0x22398675:'83fb10',0x223987A4:'83fb42',0x223987E5:'83fb01',0x223987EA:'83fb48',
 0x223986E6:'b812000000',0x2239874A:'b8e8000000',
 0x2239879B:'b8e9000000',0x223987DC:'b843000000',0x22398822:'b82d000000',
 0x220E1F28:'83f8ff',0x220E1F38:'837e4001',0x220E1F3E:'83f805',
 0x220E1F43:'c7464002000000',0x220E1F5C:'83f8ff',
 0x220D33B7:'83ff01',0x220D33C2:'e829f0ffff',
}
def audit(server,client):
    require(hashlib.sha256(server).hexdigest()==SERVER,'Wrong original server.dll SHA256')
    require(hashlib.sha256(client).hexdigest()==CLIENT,'Wrong original client.dll SHA256')
    pe=PE32(server)
    for name,vt,target in ORIGINAL_8:
        require(pe.section(vt)=='.rdata' and pe.u32(vt+0x7C)==target,
                'Original behavior activity vslot changed '+name)
        require(pe.section(target)=='.text','Activity method outside .text '+name)
    rows=[]
    for name,entry,n,sha,pad in ACT_METHODS:
        require(hashlib.sha256(pe.read(entry,n)).hexdigest()==sha,
                'Complete original ACT method hash differs '+name)
        require(pe.read(entry+n,pad)==b'\xcc'*pad,
                'Original method end/alignment changed '+name)
        rows.append({'class_or_helper':name,'original_method':'0x%08X'%entry,
                     'whole_method_size':n,'whole_method_sha256':sha,
                     'post_method_int3_bytes_verified':pad})
    for site,target in DELEGATES.items():
        check_edge(pe.read,site,target,0xE9 if site==0x2239270E else 0xE8)
    for site,hexbytes in WITNESSES.items():
        raw=bytes.fromhex(hexbytes)
        require(pe.read(site,len(raw))==raw,
                'Original activity opcode witness differs '+hex(site))
    require(pe.read(0x220E1F6E,2)==b'\xcc\xcc' and
            pe.read(0x220E1F70,3)==bytes.fromhex('83ec48'),
            'Original Standoff next function boundary differs')
    return {
      'status':'PHASE602_ORIGINAL_GRUNT_BEHAVIOR_ACTIVITY_METHODS_AUTHENTICATED',
      'original_server_sha256':SERVER,'original_client_sha256':CLIENT,
      'new_complete_original_methods':len(ACT_METHODS),
      'new_original_native_method_bytes':sum(x[2] for x in ACT_METHODS),
      'verified_original_activity_vtable_entries':len(ORIGINAL_8),
      'direct_original_delegate_edges':len(DELEGATES),
      'sparse_opcode_field_witnesses':len(WITNESSES),
      'common_original_host_bridge':'0x220CD400',
      'func_tank_byte_plus_0x18_nonzero_returns_activity_0x171':True,
      'takecover_raw_input_0x10_conditional_outputs':['0x12','0xE8','0xE9'],
      'takecover_raw_input_0x42_conditional_output':'0x43',
      'takecover_raw_inputs_0x01_or_0x48_conditional_output':'0x2D',
      'takecover_uses_owner_0xA18_serial_checked_target':True,
      'standoff_raw_state_plus_0x40_transition_1_to_2_on_code_5':True,
      'follow_activity_0x01_has_conditional_target_0xA18_route':True,
      'standoff_following_0x220E1F70_distinct_function':True,
      'original_method_boundaries':rows,
      'all_activity_ID_symbolic_names_proven':False,
      'native_code_bytes_exported':False,
    }

def selftest():
    class Fake:
        def __init__(self,site,raw):self.site,self.raw=site,raw
        def read(self,site,n):
            require(site==self.site and n==5,'Synthetic five-byte branch read')
            return self.raw
    mutations=0
    for site,target in DELEGATES.items():
        opcode=0xE9 if site==0x2239270E else 0xE8
        raw=bytes([opcode])+struct.pack('<i',target-site-5)
        check_edge(Fake(site,raw).read,site,target,opcode)
        for i in range(5):
            bad=bytearray(raw);bad[i]^=1
            try:check_edge(Fake(site,bytes(bad)).read,site,target,opcode)
            except Refusal:mutations+=1
            else:raise AssertionError('Corrupted native ACT delegate accepted')
    assert mutations==20
    assert sum(x[2] for x in ACT_METHODS)==792
    try:audit(b'invalid original',b'invalid original')
    except Refusal:pass
    else:raise AssertionError('Wrong original DLL accepted')
    print('PHASE602_ORIGINAL_FREE_ACTIVITY_SYNTHETIC_PASS (20 corrupted branches rejected)')

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--self-test',action='store_true')
    ap.add_argument('--bin-zip',type=Path)
    ap.add_argument('--server',type=Path)
    ap.add_argument('--client',type=Path)
    ap.add_argument('--out',type=Path)
    a=ap.parse_args()
    if a.self_test:
        selftest()
        if a.bin_zip is None and a.server is None:return
    if a.bin_zip is None and not (a.server and a.client):
        ap.error('Use --self-test, --bin-zip or both --server and --client')
    try:
        if a.bin_zip:
            with zipfile.ZipFile(a.bin_zip) as z:
                result=audit(z.read('server.dll'),z.read('client.dll'))
        else:result=audit(a.server.read_bytes(),a.client.read_bytes())
    except (OSError,Refusal,struct.error,zipfile.BadZipFile,KeyError) as exc:
        ap.exit(2,'PHASE602 ORIGINAL SOURCE REFUSED: %s\n'%exc)
    output=json.dumps(result,indent=2)+'\n'
    if a.out:
        a.out.write_text(output,encoding='utf-8')
        print('Authenticated original ACT method metadata:',a.out)
    else:print(output)

if __name__=='__main__':main()
