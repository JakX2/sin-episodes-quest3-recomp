#!/usr/bin/env python3
"""Phase606: authenticate original 2006 Grunt runtime behavior choice.

SHA-pinned original PE32 audit. No source executable bytes are exported.
Original Grunt behavior registration order is established separately in Phase535.
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
# Exact original complete native PE ranges and post-method INT3 padding.
METHODS=(
 ("priority_selector",0x2234E590,152,
  "726b660c52b649fea86e5fff6018dbf27d67136ad951ffc51328ba22d14ff744",8),
 ("MobGrunt_accept_predicate",0x2216DDE0,5,
  "f4c6d7ae520f88aecb3ea65952e885437fa4a6ce4b5c3439a161d1c5d8e42863",11),
 ("schedule_bridge",0x2234E730,83,
  "38c124a7ea670e7d5277f7175d8f6114934eaf2adeaefe796785bcddddcee52f",13),
)
GRUNT_VTABLE=0x2249AAA4
GRUNT_SLOTS={
  0x434:0x2235E4E0, # Grunt schedule method calls selector at 0x2235E6EE
  0x77C:0x2216DDE0, # original always-allow vfunc
}
E8={
  0x2234E734:0x2234E590,
  0x2234E61E:0x2216E510,
  0x2235E6EE:0x2234E590,
  0x22366137:0x2234E590,
  0x2236E6F5:0x2234E590,
  0x2236E74D:0x2234E590,
}
E9={0x2234E77E:0x2204FD80}
# Compact original-native opcode identity probes. These are not a code dump.
WITNESSES={
  0x2234E596:'39be280c0000',      # registered behavior count
  0x2234E5A0:'8b861c0c0000',      # registered behavior backing array
  0x2234E5AB:'ff5218',            # individual behavior eligibility vfunc
  0x2234E5C0:'ff927c070000',      # owner acceptance virtual
  0x2234E5D5:'8bbe180c0000',      # previous active behavior
  0x2234E5E2:'c786180c000000000000',# no match => current NULL
  0x2234E5F6:'ff5020',            # old behavior cleanup
  0x2234E607:'ff9278070000',      # owner old->null notification
  0x2234E612:'8b8e1c0c0000',      # winner taken from registered vector
  0x2234E624:'b001',              # found => true
  0x2216DDE0:'b001c20400',        # Grunt owner accepts any candidate
  0x2234E739:'8bb7180c0000',      # schedule bridge loads active behavior
  0x2234E741:'c687300c000001',    # flags behavior bridge state
  0x2234E74E:'ff5058',            # active behavior schedule callback
  0x2234E758:'ff92d4000000',      # active behavior output predicate
}
REGISTRATION_ORDER=(
  "CAI_RappelBehavior","CAI_FuncTankBehavior","CAI_TakecoverBehavior",
  "CAI_BeserkBehavior","CAI_ActBusyBehavior","CAI_AssaultBehavior",
  "CGruntStandoffBehavior","CAI_FollowBehavior",
)

def audit(server,client):
    require(hashlib.sha256(server).hexdigest()==SERVER,
            "Wrong original 2006 server.dll")
    require(hashlib.sha256(client).hexdigest()==CLIENT,
            "Wrong original 2006 client.dll")
    pe=PE32(server)
    require(pe.section(GRUNT_VTABLE)=='.rdata',"Original Grunt RTTI vtable missing")
    for offset,target in GRUNT_SLOTS.items():
        require(pe.u32(GRUNT_VTABLE+offset)==target,
                "Original Grunt virtual differs +%x"%offset)
    rows=[]
    for role,va,length,sha,pad in METHODS:
        require(pe.section(va)=='.text',"Original method outside text: "+role)
        require(hashlib.sha256(pe.read(va,length)).hexdigest()==sha,
                "Whole original 2006 method SHA mismatch: "+role)
        require(pe.read(va+length,pad)==b'\xcc'*pad,
                "Original method boundary differs: "+role)
        rows.append({'role':role,'native_va':'0x%08X'%va,
                     'whole_method_bytes':length,'whole_sha256':sha,
                     'post_method_INT3_checked':pad})
    for site,target in E8.items():check_edge(pe.read,site,target,0xE8)
    for site,target in E9.items():check_edge(pe.read,site,target,0xE9)
    for site,hexcode in WITNESSES.items():
        code=bytes.fromhex(hexcode)
        require(pe.read(site,len(code))==code,
                "Original runtime behavior selector instruction changed "+hex(site))
    return {
      'status':'PHASE606_NATIVE_MOBGRUNT_RUNTIME_BEHAVIOR_PRIORITY_AUTHENTICATED',
      'original_server_sha256':SERVER,
      'original_client_sha256':CLIENT,
      'new_original_complete_methods':len(METHODS),
      'new_original_complete_method_bytes':sum(x[2] for x in METHODS),
      'complete_native_methods':rows,
      'original_grunt_schedule_virtual_slot':'0x434',
      'original_grunt_schedule_function':'0x2235E4E0',
      'grunt_schedule_to_selector_native_call':'0x2235E6EE',
      'true_runtime_behavior_selector':'0x2234E590',
      'original_behavior_candidate_acceptance_slot':'0x77C',
      'grunt_acceptance_target':'0x2216DDE0',
      'grunt_acceptance_returns_true_unconditionally':True,
      'registered_original_order':list(REGISTRATION_ORDER),
      'selection_policy':'first registered behavior whose +0x18 eligibility is true, because owner +0x77C accepts',
      'winner_transition_call':'0x2216E510',
      'no_winner_clears_owner_0xC18':True,
      'no_winner_prior_behavior_cleanup_and_owner_notification':True,
      'schedule_bridge_original_entry':'0x2234E730',
      'schedule_bridge_loads_current_behavior_after_selection':True,
      'original_direct_E8_edges_checked':len(E8),
      'original_tail_E9_edges_checked':len(E9),
      'original_instruction_witnesses':len(WITNESSES),
      'all_eligibility_virtual_0x18_implementations_recovered':False,
      'original_engine_runtime_boot_verified':False,
      'proprietary_code_exported':False
    }

def self_test():
    class Fake:
        def __init__(self,site,raw):self.site,self.raw=site,raw
        def read(self,site,n):
            require(site==self.site and n==5,"Bad synthetic rel32 read")
            return self.raw
    changed=0
    for site,target,op in (
        [(site,target,0xE8) for site,target in E8.items()]+
        [(site,target,0xE9) for site,target in E9.items()]
    ):
        raw=bytes([op])+struct.pack('<i',target-site-5)
        check_edge(Fake(site,raw).read,site,target,op)
        for i in range(5):
            altered=bytearray(raw);altered[i]^=1
            try:check_edge(Fake(site,bytes(altered)).read,site,target,op)
            except Refusal:changed+=1
            else:raise AssertionError("Accepted a corrupt native branch")
    assert changed==35 and sum(x[2] for x in METHODS)==240
    assert REGISTRATION_ORDER[0]=='CAI_RappelBehavior'
    assert REGISTRATION_ORDER[-1]=='CAI_FollowBehavior'
    try:audit(b'invalid',b'invalid')
    except Refusal:pass
    else:raise AssertionError("Wrong original DLLs accepted")
    print("PHASE606_ORIGINAL_FREE_BEHAVIOR_PRIORITY_PASS (35 corrupt branches rejected)")

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
    if a.bin_zip is None and not (a.server and a.client):
        p.error('Use --bin-zip, both --server/--client, or --self-test')
    try:
        if a.bin_zip:
            with zipfile.ZipFile(a.bin_zip) as z:
                result=audit(z.read('server.dll'),z.read('client.dll'))
        else:result=audit(a.server.read_bytes(),a.client.read_bytes())
    except (OSError,Refusal,struct.error,KeyError,zipfile.BadZipFile) as err:
        p.exit(2,"PHASE606 ORIGINAL SOURCE REFUSED: %s\n"%err)
    summary=json.dumps(result,indent=2)+'\n'
    if a.out:
        a.out.write_text(summary,encoding='utf-8')
        print('Authenticated original Grunt runtime behavior policy:',a.out)
    else:print(summary)
if __name__=='__main__':main()
