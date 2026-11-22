#!/usr/bin/env python3
"""Phase609: authenticate four original Assault entity handle-guard helpers.

The original 2006 SiN DLLs stay private. This audits whole-method SHA256,
native control transitions and source-specific opcode witnesses only.
"""
import argparse
import hashlib
import json
import struct
import zipfile
from pathlib import Path
from audit_sin_original_grunt_behavior_event54_phase601 import (
    SERVER, CLIENT, PE32, Refusal, require, check_edge
)
PTR_SHA='3e1f38e2f06c1fd7b15f8268e7eecf5e4e21067cfa73e45348e71183740c9fa6'
MISSING_SHA='55c24daf3da453809e6e0692d604c1a7008273d6a08b2ec9f38c4bbc094c8f0a'
# Original compiled routines occur at distinct VAs but duplicate exact native
# byte sequences by common template/inline code generation.
METHODS=(
 ('first_raw_handle_resolve',0x220CE210,42,PTR_SHA,6),
 ('first_handle_missing_or_null',0x220CE240,60,MISSING_SHA,4),
 ('second_raw_handle_resolve',0x220CE2F0,42,PTR_SHA,6),
 ('second_handle_missing_or_null',0x220CE350,60,MISSING_SHA,4),
)
ASSAULT_VTABLE=0x223F5544
ASSAULT_ELIGIBLE=0x220CEAF0
ASSAULT_ELIGIBLE_LEN=255
ASSAULT_ELIGIBLE_SHA='0c5b688341149ae7ed28a49f9f930c87116810c67f1107a444e48742ba570466'
DIRECT={
  0x220CEB3A:0x220CE240,
  0x220CEB4A:0x220CE350,
  0x220CEB73:0x220CE210,
  0x220CEB7E:0x220CE2F0,
}
WITNESSES={
  0x220CE210:'8b01',             # first handle value at pointer+0
  0x220CE212:'83f8ff',           # invalid sentinel
  0x220CE217:'8b1520564f22',     # serial-checked original global entity table
  0x220CE21F:'81e1ff0f0000',    # 0xfff index mask
  0x220CE22C:'c1e80c',          # high bits serial
  0x220CE22F:'394104',          # stored serial comparison
  0x220CE234:'8b01',            # entity pointer returned
  0x220CE237:'33c0',            # invalid returns null
  0x220CE240:'8b01',            # second helper reads handle
  0x220CE242:'83f8ff',
  0x220CE266:'33c9',            # missing bool computed from entity pointer null
  0x220CE26A:'0f94c1',          # sete after TEST eax
  0x220CE270:'33c0',            # invalid handle case
  0x220CE276:'0f94c1',          # true iff null
  0x220CE2F0:'8b01',            # second pointer resolver duplicate
  0x220CE314:'8b01',
  0x220CE350:'8b01',            # second missing-target helper duplicate
  0x220CE378:'85c0',
  0x220CE37A:'0f94c1',
  0x220CEB38:'8bcf',            # caller's first handle at behavior+0x14
  0x220CEB47:'8d4e18',          # caller's second handle at behavior+0x18
  0x220CEB3F:'84c0',            # first missing => skip
  0x220CEB4F:'84c0',            # second missing => skip
  0x220CEBE1:'895e1c',          # reset native state+0x1C to zero
  0x220CEBE4:'885e24',          # reset byte+0x24 to zero
  0x220CEBE7:'885e25',          # reset byte+0x25 to zero
}

def audit(server,client):
    require(hashlib.sha256(server).hexdigest()==SERVER,'Wrong original server.dll')
    require(hashlib.sha256(client).hexdigest()==CLIENT,'Wrong original client.dll')
    pe=PE32(server)
    require(pe.u32(ASSAULT_VTABLE+0x18)==ASSAULT_ELIGIBLE,
            'Original Assault eligibility vtable+0x18 differs')
    require(hashlib.sha256(pe.read(ASSAULT_ELIGIBLE,ASSAULT_ELIGIBLE_LEN)).hexdigest()==ASSAULT_ELIGIBLE_SHA,
            'Existing Phase607 Assault predicate differs')
    rows=[]
    for name,entry,size,sha,pad in METHODS:
        require(pe.section(entry)=='.text','Assault handle method not in .text')
        require(hashlib.sha256(pe.read(entry,size)).hexdigest()==sha,
                'Whole original handle-helper SHA mismatch '+name)
        require(pe.read(entry+size,pad)==b'\xcc'*pad,
                'Original handle-helper function boundary differs '+name)
        rows.append({'role':name,'original_entry':'0x%08X'%entry,
                     'whole_method_bytes':size,'whole_method_sha256':sha,
                     'native_post_method_int3':pad})
    require(pe.read(METHODS[0][1],42)==pe.read(METHODS[2][1],42),
            'Two native original pointer resolver bodies no longer identical')
    require(pe.read(METHODS[1][1],60)==pe.read(METHODS[3][1],60),
            'Two native original missing-target predicate bodies differ')
    for site,target in DIRECT.items():check_edge(pe.read,site,target,0xE8)
    for va,hexcode in WITNESSES.items():
        b=bytes.fromhex(hexcode)
        require(pe.read(va,len(b))==b,
                'Original Assault handle guard opcode witness differs '+hex(va))
    return {
      'status':'PHASE609_ORIGINAL_ASSAULT_HANDLE_GUARDS_AUTHENTICATED',
      'original_server_sha256':SERVER,'original_client_sha256':CLIENT,
      'new_whole_original_methods':len(METHODS),
      'new_whole_native_method_bytes':sum(x[2] for x in METHODS),
      'whole_original_methods':rows,
      'identical_ptr_resolver_pairs':True,
      'identical_missing_target_predicate_pairs':True,
      'assault_original_eligibility_function':'0x220CEAF0',
      'assault_first_target_encoded_handle_field':'0x14',
      'assault_second_target_encoded_handle_field':'0x18',
      'original_global_entity_serial_table':'0x224F5620',
      'original_handle_index_mask':'0xFFF',
      'original_handle_serial_shift':12,
      'missing_target_predicate_true_for_invalid_stale_or_null':True,
      'both_targets_must_resolve_to_reach_later_assault_eligibility_logic':True,
      'on_either_missing_resets_fields':['0x1C','0x24','0x25'],
      'original_direct_E8_helper_edges':len(DIRECT),
      'original_sparse_opcode_witnesses':len(WITNESSES),
      'whole_assault_eligibility_semantics_reconstructed':False,
      'original_engine_runtime_verified':False,
      'proprietary_executable_bytes_exported':False,
    }

def self_test():
    class Fake:
        def __init__(self,site,data):self.site,self.data=site,data
        def read(self,site,n):
            require(site==self.site and n==5,'Synthetic E8 read mismatch')
            return self.data
    mutation_count=0
    for site,target in DIRECT.items():
        b=b'\xe8'+struct.pack('<i',target-site-5)
        check_edge(Fake(site,b).read,site,target,0xE8)
        for index in range(5):
            bad=bytearray(b);bad[index]^=1
            try:check_edge(Fake(site,bytes(bad)).read,site,target,0xE8)
            except Refusal:mutation_count+=1
            else:raise AssertionError('Accepted corrupt original Assault handle call')
    assert mutation_count==20
    assert sum(x[2] for x in METHODS)==204
    for valid_a,valid_b in ((0,0),(0,1),(1,0),(1,1)):
        missing=not valid_a or not valid_b
        assert missing==(not (valid_a and valid_b))
    try:audit(b'invalid',b'invalid')
    except Refusal:pass
    else:raise AssertionError('Accepted wrong original game files')
    print('PHASE609_ORIGINAL_FREE_ASSAULT_HANDLE_GUARD_PASS (20 mutations rejected)')

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
        p.error('Pass --bin-zip, paired DLL paths or --self-test')
    try:
        if a.bin_zip:
            with zipfile.ZipFile(a.bin_zip) as z:
                result=audit(z.read('server.dll'),z.read('client.dll'))
        else:result=audit(a.server.read_bytes(),a.client.read_bytes())
    except (OSError,KeyError,Refusal,struct.error,zipfile.BadZipFile) as ex:
        p.exit(2,'PHASE609 ORIGINAL SOURCE REFUSED: %s\n'%ex)
    output=json.dumps(result,indent=2)+'\n'
    if a.out:
        a.out.write_text(output,encoding='utf-8')
        print('Verified original Assault handle helpers:',a.out)
    else:print(output)

if __name__=='__main__':main()
