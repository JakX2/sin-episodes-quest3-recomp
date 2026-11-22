#!/usr/bin/env python3
"""Phase611: original 2006 Assault +0x20 deactivation and unlink helper.

Source evidence: original retail PE32 whole-body SHA256, genuine method
boundaries, class vtable virtual+0x20 and direct native call edges.
Commercial executable bytes and disassembly are never exported.
"""
import argparse,hashlib,json,struct,zipfile
from pathlib import Path
from audit_sin_original_grunt_behavior_event54_phase601 import (
    SERVER,CLIENT,PE32,Refusal,require,check_edge
)
ASSAULT_VTABLE=0x223F5544
METHODS=(
 ("Assault_behavior_deactivate",0x220CEBF0,105,
  "4c1d6122cb572112702dd69d8775b32d6535e272d024578c0376f4e0225c57cf",7),
 ("conditionally_unlink_entity_owner",0x220CE4A0,115,
  "afebea980deb52065acaf4d1c29e667257f02b4f4307dafa2e1f2ccaa3841082",13),
)
EDGES={0x220CEC48:0x220CE4A0,0x220CEC52:0x220CE4A0}
WITNESSES={
 0x220CEBF0:'c6412400',0x220CEBF4:'c6412500',
 0x220CEBF8:'8b4118',0x220CEBFB:'83f8ff',
 0x220CEC02:'81e2ff0f0000',0x220CEC09:'8b3520564f22',
 0x220CEC16:'c1e80c',0x220CEC19:'394204',
 0x220CEC23:'8b4118',0x220CEC26:'83f8ff',
 0x220CEC29:'8b5104',0x220CEC30:'81e1ff0f0000',
 0x220CEC3D:'c1e80c',0x220CEC40:'394104',
 0x220CEC45:'8b09',0x220CEC4F:'33c9',
 0x220CE4A0:'8b81bc030000',0x220CE4A6:'83f8ff',
 0x220CE4AC:'8b3520564f22',0x220CE4B4:'81e2ff0f0000',
 0x220CE4C1:'c1e80c',0x220CE4C4:'394204',
 0x220CE4CF:'8b81bc030000',0x220CE4D5:'83f8ff',
 0x220CE4E9:'c1e80c',0x220CE4EC:'394204',
 0x220CE4F7:'3b542408',0x220CE4FD:'32c0',
 0x220CE503:'c781bc030000ffffffff',
 0x220CE50D:'b001',
}
def audit(server,client):
    require(hashlib.sha256(server).hexdigest()==SERVER,
            'Wrong original 2006 server.dll')
    require(hashlib.sha256(client).hexdigest()==CLIENT,
            'Wrong original 2006 client.dll')
    pe=PE32(server)
    require(pe.section(ASSAULT_VTABLE)=='.rdata' and
            pe.u32(ASSAULT_VTABLE+0x20)==METHODS[0][1],
            'Original Assault shutdown virtual+0x20 differs')
    rows=[]
    for role,va,n,digest,pad in METHODS:
        require(pe.section(va)=='.text',
                'Original method not in executable code section')
        require(hashlib.sha256(pe.read(va,n)).hexdigest()==digest,
                'Original whole-method SHA mismatch: '+role)
        require(pe.read(va+n,pad)==b'\xcc'*pad,
                'Original INT3 method alignment differs: '+role)
        rows.append({'original_role':role,'native_entry':'0x%08X'%va,
                     'native_complete_bytes':n,
                     'whole_method_sha256':digest,
                     'following_int3_bytes_verified':pad})
    require(pe.read(0x220CEC60,2)==bytes.fromhex('558b'),
            'Distinct original next function after deactivation changed')
    require(pe.read(0x220CE520,3)==bytes.fromhex('8b4104'),
            'Distinct original next function after unlink helper changed')
    for site,target in EDGES.items():
        check_edge(pe.read,site,target,0xE8)
    for site,rawhex in WITNESSES.items():
        expected=bytes.fromhex(rawhex)
        require(pe.read(site,len(expected))==expected,
                'Original deactivation/unlink source witness differs '+hex(site))
    return {
      'status':'PHASE611_ORIGINAL_ASSAULT_DEACTIVATION_UNLINK_AUTHENTICATED',
      'server_sha256':SERVER,'client_sha256':CLIENT,
      'new_complete_original_native_methods':len(METHODS),
      'new_complete_original_native_bytes':sum(x[2] for x in METHODS),
      'whole_original_method_hashes':rows,
      'Assault_original_behavior_vtable_plus_0x20':'0x220CEBF0',
      'original_cleanup_called_by_behavior_transition_phase604':True,
      'original_deactivation_always_clears_flags_plus_0x24_plus_0x25':True,
      'original_target_handle_member_plus_0x18_revalidated_twice':True,
      'original_target_relationship_handle_offset':'0x3BC',
      'original_link_helper_clears_when_invalid_or_stale':True,
      'original_link_helper_clears_when_linked_owner_equals_owner_argument':True,
      'original_link_helper_preserves_valid_other_owner_relationship':True,
      'original_link_helper_rechecks_encoded_relationship_handle_twice':True,
      'native_second_target_resolution_failure_can_call_null_helper':True,
      'portable_host_fails_closed_on_second_resolution_failure':True,
      'verified_original_E8_calls_to_unlink_helper':len(EDGES),
      'verified_short_instruction_witnesses':len(WITNESSES),
      'actual_engine_lifetime_or_concurrency_guarantees_proven':False,
      'retail_executable_bytes_exported':False,
    }
def selftest():
    class Fake:
        def __init__(self,site,data):self.site,self.data=site,data
        def read(self,site,n):
            require(site==self.site and n==5,'Incorrect synthetic branch input')
            return self.data
    flips=0
    for site,target in EDGES.items():
        raw=b'\xe8'+struct.pack('<i',target-site-5)
        check_edge(Fake(site,raw).read,site,target,0xE8)
        for i in range(5):
            bad=bytearray(raw);bad[i]^=1
            try:check_edge(Fake(site,bytes(bad)).read,site,target,0xE8)
            except Refusal:flips+=1
            else:raise AssertionError('Accepted corrupted native original call')
    assert flips==10 and sum(x[2] for x in METHODS)==220
    try:audit(b'invalid',b'invalid')
    except Refusal:pass
    else:raise AssertionError('Unsupported original SHA accepted')
    print('PHASE611_ORIGINAL_FREE_ASSAULT_DEACTIVATION_PASS (10 corrupt calls refused)')
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
    if not(a.bin_zip or(a.server and a.client)):
        p.error('Provide original --bin-zip or paired --server/--client')
    try:
        if a.bin_zip:
            with zipfile.ZipFile(a.bin_zip) as z:
                result=audit(z.read('server.dll'),z.read('client.dll'))
        else:result=audit(a.server.read_bytes(),a.client.read_bytes())
    except (Refusal,OSError,KeyError,struct.error,zipfile.BadZipFile) as ex:
        p.exit(2,'PHASE611 ORIGINAL SOURCE REFUSED: %s\n'%ex)
    result_text=json.dumps(result,indent=2)+'\n'
    if a.out:
        a.out.write_text(result_text,encoding='utf-8')
        print('Original Assault deactivate/unlink audit saved:',a.out)
    else:print(result_text)
if __name__=='__main__':main()
