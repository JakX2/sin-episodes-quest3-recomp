#!/usr/bin/env python3
"""Phase604: original 2006 Grunt behavior selection and transition source proof.

Checks owner-private retail DLL pair, complete method SHA and padding, native
Grunt virtual table entries, original E8 callsites and sparse operation sites.
No original game code/retail DLL exported or committed.
"""
import argparse
import hashlib
import json
import struct
import zipfile
from pathlib import Path
from audit_sin_original_grunt_behavior_event54_phase601 import (
    SERVER,CLIENT,PE32,Refusal,require,check_edge,
)

# Native method start, complete original byte length, SHA256, INT3 padding.
METHODS=(
 ('behavior-index-selection',0x2216DE40,104,'273450c7062ad7ca28aa34edb7faa41c3d5f5d97641e943b32e387c8b06b3ee7',8),
 ('reset-before-base-action',0x2216E4E0,36,'5dfa361d61a3eb70d088e81886e2378b987609cc4ccf74b62255e49521354b31',12),
 ('active-behavior-transition',0x2216E510,104,'3fd8b4e4642526aa8171927faf9be753b3d7761ec7e2c48a49e73786eafdd216',8),
 ('clear-behavior-then-delegate',0x2216E580,87,'ff4469eecb4800a67e882c186eaee30a0cfbe69f00e9eec8775f0a4bb0d6d568',9),
)
VTABLE=0x2249AAA4
GRUNT_VIRTUALS={0xA4:0x2216DE40,0x660:0x2216E4E0,0x664:0x2216E580}
DIRECT={
 0x2216DE4A:0x22043AD0,0x2216DE6C:0x220CD630,
 0x2216E4E5:0x2216E510,0x2216E4FB:0x2204CAE0,
 0x2216E561:0x22054CA0,0x2216E5AA:0x22054CA0,
 0x2216E5CD:0x2204CDD0,
}
# Sparse original opcode checks: no bulk disassembly distribution.
WITNESSES={
 0x2216DE76:'83fbff',0x2216DE85:'8b0c98',
 0x2216DE8B:'898e180c0000',0x2216DE96:'c786180c000000000000',
 0x2216E4E1:'6a00',0x2216E514:'8b86180c0000',
 0x2216E51F:'3bc7',0x2216E528:'89be180c0000',
 0x2216E538:'ff501c',0x2216E53B:'c605a0c0542201',
 0x2216E542:'8b8e180c0000',0x2216E54A:'ff5224',
 0x2216E54D:'c605a0c0542200',0x2216E55C:'ff5020',
 0x2216E56C:'ff9278070000',0x2216E584:'8bbe180c0000',
 0x2216E591:'c786180c000000000000',
  0x2216E5A5:'ff5020',0x2216E5B6:'ff9278070000',
}
def audit(server,client):
    require(hashlib.sha256(server).hexdigest()==SERVER,'Wrong 2006 server.dll SHA')
    require(hashlib.sha256(client).hexdigest()==CLIENT,'Wrong 2006 client.dll SHA')
    pe=PE32(server)
    require(pe.u32(VTABLE+0x444)==0x2235BFD0,'Grunt animation virtual changed')
    require(pe.u32(VTABLE+0x338)==0x2235A250,'Grunt activity virtual changed')
    for offset,expected in GRUNT_VIRTUALS.items():
        require(pe.u32(VTABLE+offset)==expected,
                'Grunt behavior virtual slot altered: +%x'%offset)
    verified=[]
    for role,va,n,digest,pad in METHODS:
        require(pe.section(va)=='.text','Method target not in executable .text')
        require(hashlib.sha256(pe.read(va,n)).hexdigest()==digest,
                'Whole native body mismatch: '+role)
        require(pe.read(va+n,pad)==b'\xcc'*pad,
                'Original method end/padding mismatch: '+role)
        verified.append({'role':role,'entry':'0x%08X'%va,'bytes':n,'sha256':digest,
                         'original_int3_padding_verified':pad})
    for site,target in DIRECT.items():check_edge(pe.read,site,target,0xE8)
    for va,hexbytes in WITNESSES.items():
        op=bytes.fromhex(hexbytes)
        require(pe.read(va,len(op))==op,'Original Grunt field/branch witness changed '+hex(va))
    return {
       'status':'PHASE604_ORIGINAL_GRUNT_BEHAVIOR_SELECTION_AUTHENTICATED',
       'server_sha256':SERVER,'client_sha256':CLIENT,
       'new_complete_original_methods':len(METHODS),
       'new_original_complete_method_bytes':sum(x[2] for x in METHODS),
       'verified_original_grunt_vtable_slots':len(GRUNT_VIRTUALS),
       'verified_original_E8_calls':len(DIRECT),
       'verified_native_instruction_witnesses':len(WITNESSES),
       'original_selection_entry':'0x2216DE40',
       'original_pointer_transition_entry':'0x2216E510',
       'new_behavior_index_method':'0x220CD630',
       'owner_member_current_behavior':'0xC18',
       'index_selection_reads_registration_array_then_stores_current_pointer':True,
       'unmatched_index_sets_pointer_null':True,
       'transition_new_accepted_pointer_stored':True,
       'transition_changed_flag_derived_from_old_vs_new_pointer':True,
       'new_behavior_virtual_calls':['0x1C','0x24'],
       'old_behavior_virtual_calls':['0x20'],
       'owner_transition_notification_virtual':'0x778',
       'transition_guard_global':'0x2254C0A0',
       'newly_authenticated_methods':verified,
       'selection_helper_full_semantics_proven':False,
       'all_runtime_behaviors_are_one_of_eight_proven':False,
       'proprietary_machine_code_exported':False,
    }

def selftest():
    class Fake:
        def __init__(self,site,raw):self.site,self.raw=site,raw
        def read(self,site,n):
            require(site==self.site and n==5,'Synthetic E8 read mismatch')
            return self.raw
    mutations=0
    for site,target in DIRECT.items():
        raw=b'\xe8'+struct.pack('<i',target-site-5)
        check_edge(Fake(site,raw).read,site,target,0xE8)
        for i in range(5):
            bad=bytearray(raw);bad[i]^=1
            try:check_edge(Fake(site,bytes(bad)).read,site,target,0xE8)
            except Refusal:mutations+=1
            else:raise AssertionError('Corrupted original E8 accepted')
    assert mutations==35 and sum(x[2] for x in METHODS)==331
    try:audit(b'invalid',b'invalid')
    except Refusal:pass
    else:raise AssertionError('Wrong original SHA accepted')
    print('PHASE604_ORIGINAL_FREE_GRUNT_BEHAVIOR_SELECTION_PASS (35 mutations rejected)')

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--self-test',action='store_true')
    p.add_argument('--bin-zip',type=Path)
    p.add_argument('--server',type=Path)
    p.add_argument('--client',type=Path)
    p.add_argument('--out',type=Path)
    a=p.parse_args()
    if a.self_test:
        selftest()
        if not(a.bin_zip or a.server):return
    if a.bin_zip is None and not(a.server and a.client):
        p.error('Use --self-test, --bin-zip or paired DLL paths')
    try:
        if a.bin_zip:
            with zipfile.ZipFile(a.bin_zip) as z:
                result=audit(z.read('server.dll'),z.read('client.dll'))
        else:result=audit(a.server.read_bytes(),a.client.read_bytes())
    except (OSError,Refusal,struct.error,KeyError,zipfile.BadZipFile) as ex:
        p.exit(2,'PHASE604 ORIGINAL RETAIL AUDIT REFUSED: %s\n'%ex)
    output=json.dumps(result,indent=2)+'\n'
    if a.out:
        a.out.write_text(output,encoding='utf-8')
        print('Authenticated native selection metadata:',a.out)
    else:print(output)
if __name__=='__main__':main()
