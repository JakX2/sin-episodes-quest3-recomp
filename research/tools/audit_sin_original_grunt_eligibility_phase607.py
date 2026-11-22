#!/usr/bin/env python3
"""Phase607: authenticate all eight original Grunt behavior +0x18 predicates.

Exact original SiN 2006 PE32 SHA256s, vtable pointers, real function bounds.
Does not export original proprietary game code or raw disassembly.
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
# Class RTTI is previously independently established by Phase535.
# Original vtable, +0x18 entry, exact whole-function size/SHA, INT3 bytes.
PREDICATES=(
 ("CAI_RappelBehavior",0x223F956C,0x220DD770,39,
  "da145e5611e0ae6b7652baf1c8feb2c8860deb74d4ce6a2ec8d99de99da5b7f7",9),
 ("CAI_FuncTankBehavior",0x224ABEC4,0x22392830,81,
  "383cef6c407791c89da83a3b218c1a763865fcf5a5c2acc097f4a989ecf9df13",15),
 ("CAI_TakecoverBehavior",0x224AE624,0x22398250,103,
  "74868593fcb6cb8efc228c9b555a8596e6128a80d8e8470b980e8568cd19b6d9",9),
 ("CAI_BeserkBehavior",0x224AB88C,0x22391D50,25,
  "99990331374c0ef4afd6108e86e138ae77cf66c8304f64bd51a10fd73d64731b",7),
 ("CAI_ActBusyBehavior",0x224AB494,0x2238D8C0,73,
  "7d436aaae82633a72e6c6d87c81a221a7ef675954e128b792b66dd46d48c5ad1",7),
 ("CAI_AssaultBehavior",0x223F5544,0x220CEAF0,255,
  "0c5b688341149ae7ed28a49f9f930c87116810c67f1107a444e48742ba570466",1),
 ("CGruntStandoffBehavior",0x22495344,0x220DF3F0,52,
  "57f2f114d160db1b46c8c3a36809487f138e508df8b72f4ea9f2ef7fb49b8ff6",12),
 ("CAI_FollowBehavior",0x223F6B7C,0x220D15D0,31,
  "c60968717e032c273c8f7a7c5d32edb08f3f8d8d44aa3781d3f68d11579fd542",1),
)
# Source verified real E8 callsites inside those native predicate bodies.
DIRECT={
 0x22392875:0x220397C0,
 0x223982A3:0x222AA640,
 0x2238D8DB:0x2238D340,
 0x220CEB07:0x220397C0,
 0x220CEB3A:0x220CE240,
 0x220CEB4A:0x220CE350,
 0x220DF412:0x222AA640,
}
# Individual opcode identity checks: no original code dump.
WITNESSES={
 0x220DD778:'ff9050040000',
 0x220DD784:'807e1100',
 0x220DD78E:'807e1200',
 0x22392833:'8b4614',
 0x22392873:'6a3b',
 0x2239827C:'ff92c0050000',
 0x22398282:'a900000008',
 0x2239829A:'83b95008000003',
 0x22391D58:'ff9050040000',
 0x22391D64:'8a4611',
 0x2238D8C3:'807e2000',
 0x2238D8C9:'807e1100',
 0x2238D8CF:'807e2200',
 0x2238D8D5:'807e1000',
 0x220CEAF8:'ff9050040000',
 0x220CEB05:'6a3b',
 0x220CEB2A:'395e1c',
 0x220DF3F0:'80b9cd00000000',
 0x220DF3FD:'80793100',
 0x220DF409:'83b95008000003',
 0x220D15D8:'ff9050040000',
 0x220D15E9:'ffa2e4000000',
}
GRUNT_VTABLE=0x2249AAA4
GRUNT_SCHEDULE_VA=0x2235E4E0
SELECTOR_VA=0x2234E590

def audit(server,client):
    require(hashlib.sha256(server).hexdigest()==SERVER,"Wrong retail server.dll SHA")
    require(hashlib.sha256(client).hexdigest()==CLIENT,"Wrong retail client.dll SHA")
    pe=PE32(server)
    require(pe.u32(GRUNT_VTABLE+0x434)==GRUNT_SCHEDULE_VA,
            "Original MobGrunt schedule virtual changed")
    require(pe.read(0x2235E6EE,1)==b'\xe8',
            "Original Grunt selector E8 call not present")
    check_edge(pe.read,0x2235E6EE,SELECTOR_VA,0xE8)
    rec=[]
    for name,vt,entry,length,digest,pad in PREDICATES:
        require(pe.section(vt)=='.rdata' and pe.u32(vt+0x18)==entry,
                "Original registered behavior +0x18 vtable changed: "+name)
        require(pe.section(entry)=='.text',
                "Original eligibility method not in executable section")
        require(hashlib.sha256(pe.read(entry,length)).hexdigest()==digest,
                "Original complete predicate SHA differs: "+name)
        require(pe.read(entry+length,pad)==b'\xcc'*pad,
                "Original predicate boundary differs: "+name)
        rec.append({'original_class':name,'original_entry':'0x%08X'%entry,
                    'whole_body_bytes':length,'whole_sha256':digest,
                    'following_native_int3_bytes':pad})
    # Special one-INT3 boundaries: next code is a DIFFERENT native method.
    require(pe.read(0x220CEBF0,4)==bytes.fromhex('c6412400'),
            "Assault next native method at 0x220CEBF0 changed")
    require(pe.read(0x220D15F0,4)==bytes.fromhex('83ec1856'),
            "Follow next native method at 0x220D15F0 changed")
    for site,target in DIRECT.items():
        check_edge(pe.read,site,target,0xE8)
    for addr,pattern in WITNESSES.items():
        code=bytes.fromhex(pattern)
        require(pe.read(addr,len(code))==code,
                "Original eligibility field/control witness changed "+hex(addr))
    return {
       'status':'PHASE607_ORIGINAL_EIGHT_GRUNT_ELIGIBILITY_PREDICATES_AUTHENTICATED',
       'original_server_sha256':SERVER,'original_client_sha256':CLIENT,
       'eight_registered_2006_behavior_rtti_virtuals_matched':True,
       'new_complete_native_original_methods':len(PREDICATES),
       'new_complete_original_native_bytes':sum(x[3] for x in PREDICATES),
       'original_eligibility_virtual_byte_offset':'0x18',
       'source_authenticated_original_methods':rec,
       'assault_eligibility_end_exclusive':'0x220CEBEF',
       'follow_eligibility_end_exclusive':'0x220D15EF',
       'one_INT3_between_Assault_and_next_method':True,
       'one_INT3_between_Follow_and_next_method':True,
       'original_2006_grunt_schedule_to_live_selector_confirmed':True,
       'native_direct_E8_edges_checked':len(DIRECT)+1,
       'sparse_opcode_field_witnesses':len(WITNESSES),
       'rappel_requires_owner_gate_and_internal_boolean_fields':True,
       'beserk_requires_owner_virtual_0x450_then_returns_byte_0x11':True,
       'functank_validates_serial_checked_entity_handle_0x14':True,
       'takecover_requires_time_and_owner_state_gates':True,
       'actbusy_uses_multiple_internal_flag_and_time_gates':True,
       'assault_uses_multi_step_target_and_schedule_guards':True,
       'standoff_requires_internal_flag_and_owner_state_3_and_predicate':True,
       'follow_delegates_to_virtual_0xE4_after_owner_gate':True,
       'full_auxiliary_predicate_methods_reconstructed':False,
       'original_2006_engine_runtime_exercised':False,
       'proprietary_code_bytes_exported':False
    }

def self_test():
    class Fake:
        def __init__(self,site,data):self.site,self.data=site,data
        def read(self,site,n):
            require(site==self.site and n==5,"Wrong synthetic rel32 input")
            return self.data
    corrupt=0
    for site,target in DIRECT.items():
        raw=b'\xe8'+struct.pack('<i',target-site-5)
        check_edge(Fake(site,raw).read,site,target,0xE8)
        for i in range(5):
            broken=bytearray(raw);broken[i]^=1
            try:check_edge(Fake(site,bytes(broken)).read,site,target,0xE8)
            except Refusal:corrupt+=1
            else:raise AssertionError("Accepted corrupt original E8 target")
    assert corrupt==35 and len(PREDICATES)==8
    assert sum(x[3] for x in PREDICATES)==659
    assert [x[0] for x in PREDICATES][0]=='CAI_RappelBehavior'
    assert [x[0] for x in PREDICATES][-1]=='CAI_FollowBehavior'
    try:audit(b'wrong',b'wrong')
    except Refusal:pass
    else:raise AssertionError("Accepted wrong original retail pair")
    print('PHASE607_ORIGINAL_FREE_ELIGIBILITY_SYNTHETIC_PASS (35 mutated E8 edges refused)')

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--self-test',action='store_true')
    p.add_argument('--bin-zip',type=Path)
    p.add_argument('--server',type=Path)
    p.add_argument('--client',type=Path)
    p.add_argument('--out',type=Path)
    a=p.parse_args()
    if a.self_test:
        self_test()
        if not(a.bin_zip or a.server):return
    if a.bin_zip is None and not(a.server and a.client):
        p.error("Supply --bin-zip, paired --server/--client, or --self-test")
    try:
        if a.bin_zip:
            with zipfile.ZipFile(a.bin_zip) as archive:
                result=audit(archive.read('server.dll'),archive.read('client.dll'))
        else:result=audit(a.server.read_bytes(),a.client.read_bytes())
    except (OSError,Refusal,KeyError,struct.error,zipfile.BadZipFile) as e:
        p.exit(2,'PHASE607 ORIGINAL SOURCE REFUSED: %s\n'%e)
    output=json.dumps(result,indent=2)+'\n'
    if a.out:
        a.out.write_text(output,encoding='utf-8')
        print('Verified native Grunt eligibility predicate report:',a.out)
    else:print(output)
if __name__=='__main__':main()
