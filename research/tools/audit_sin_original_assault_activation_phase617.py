#!/usr/bin/env python3
"""Phase617: original 2006 Assault lifecycle +0x1C/+0x24/+0x28 audit.

SHA-locks entire original PE32 DLL pair and all three exact tiny native
method bodies, method alignments, and actual eight-behavior vtable slots.
Exports addresses/hashes/semantics, not copyrighted executable bytes.
"""
import argparse,hashlib,json,struct,zipfile
from pathlib import Path
from audit_sin_original_grunt_behavior_event54_phase601 import (
    SERVER,CLIENT,PE32,Refusal,require
)
ASSAULT=0x223F5544
SLOTS=(0x1c,0x20,0x24,0x28)
METHODS=(
 ('Assault_new_1c_ret',0x1c,0x220CDB40,1,
  'ae3f4619b0413d70d3004b9131c3752153074e45725be13b9a148978895e359e',
  15,0x220CDB50,'837c240430'),
 ('Assault_new_24_bridge_vfunc0',0x24,0x220CD290,7,
  '5250ce3c1823f4699bcf08fcd456150282d40a8864891740e49ecbb6c1492db3',
  9,0x220CD2A0,'c3'),
 ('Assault_slot_28_ret',0x28,0x220CD900,1,
  'ae3f4619b0413d70d3004b9131c3752153074e45725be13b9a148978895e359e',
  15,0x220CD910,'c3'),
)
# The other entries are ONLY original authenticated vtable-pointer inventory.
# Do not count their method implementations as recovered in this phase.
REGISTERED=(
 ('CAI_RappelBehavior',0x223F956C,(0x220CD8E0,0x220CD8F0,0x220DDB30,0x220CD900)),
 ('CAI_FuncTankBehavior',0x224ABEC4,(0x223926F0,0x22393720,0x223930D0,0x220CD900)),
 ('CAI_TakecoverBehavior',0x224AE624,(0x223982C0,0x22398990,0x223983D0,0x22398390)),
 ('CAI_BeserkBehavior',0x224AB88C,(0x220CD8E0,0x220CD8F0,0x220CD290,0x220CD900)),
 ('CAI_ActBusyBehavior',0x224AB494,(0x220CD8E0,0x2238E8E0,0x2238E820,0x220CD900)),
 ('CAI_AssaultBehavior',0x223F5544,(0x220CDB40,0x220CEBF0,0x220CD290,0x220CD900)),
 ('CGruntStandoffBehavior',0x22495344,(0x220DF430,0x220E0C20,0x220DFDB0,0x220DF520)),
 ('CAI_FollowBehavior',0x223F6B7C,(0x220D2260,0x220D0F90,0x220D4740,0x220CD900)),
)
def audit(server,client):
    require(hashlib.sha256(server).hexdigest()==SERVER,
            'Wrong 2006 retail server.dll SHA256')
    require(hashlib.sha256(client).hexdigest()==CLIENT,
            'Wrong 2006 retail client.dll SHA256')
    pe=PE32(server)
    rows=[]
    for name,vt,slots in REGISTERED:
        require(pe.section(vt)=='.rdata','Original RTTI vtable in wrong section')
        observed=tuple(pe.u32(vt+off) for off in SLOTS)
        require(observed==slots,'Original lifecycle slots differ for '+name)
        require(all(pe.section(v)=='.text' for v in slots),
                'Original lifecycle target outside .text: '+name)
        rows.append({'original_type':name,'original_vtable':'0x%08X'%vt,
                     'virtual_1c_20_24_28':['0x%08X'%v for v in observed]})
    methods=[]
    for role,slot,va,n,sha,pad,next_va,next_hex in METHODS:
        require(pe.u32(ASSAULT+slot)==va,
                'Original Assault lifecycle method slot differs '+role)
        data=pe.read(va,n)
        require(hashlib.sha256(data).hexdigest()==sha,
                'Complete original 2006 lifecycle body SHA changed: '+role)
        require(pe.read(va+n,pad)==b'\xcc'*pad and va+n+pad==next_va,
                'Original INT3 alignment or function boundary differs: '+role)
        next_raw=bytes.fromhex(next_hex)
        require(pe.read(next_va,len(next_raw))==next_raw,
                'Distinct following original method start changed: '+role)
        methods.append({'role':role,'vtable_byte_offset':'0x%02X'%slot,
                        'native_entry':'0x%08X'%va,'whole_native_bytes':n,
                        'whole_native_SHA256':sha,'following_INT3_bytes':pad})
    require(pe.read(0x220CD290,7)==b'\x8b\x49\x0c\x8b\x01\xff\x20',
            'Original 7-byte ECX+0x0C vtable0 tail dispatch changed')
    require(pe.u32(0x224AB88C+0x24)==0x220CD290,
            'Beserk must share same exact native +0x24 implementation')
    return {
       'status':'PHASE617_ORIGINAL_ASSAULT_ACTIVATION_SLOTS_AUTHENTICATED',
       'retail_server_sha256':SERVER,'retail_client_sha256':CLIENT,
       'source_authenticated_native_method_entries':methods,
       'new_native_function_addresses':3,
       'new_complete_native_body_bytes':9,
       'original_registered_behavior_lifecycle_slot_map':rows,
       'original_assault_plus_1c_is_exact_RET_no_host_effect':True,
       'original_assault_plus_24_dispatches_pointer_at_behavior_plus_0c_vfunc0':True,
       'original_beserk_plus_24_shares_same_native_method':True,
       'original_assault_plus_28_is_exact_RET':True,
       'original_six_classes_share_plus_28_RET_address':True,
       'other_classes_lifecycle_body_semantics_fully_recovered':False,
       'original_2006_vptrs_portable_as_live_host_pointers':False,
       'game_process_or_NPC_spawn_observed':False,
       'commercial_executable_bytes_exported':False,
    }

def selftest():
    # Synthetic exact full-body SHA witness refusal, with all 9 single-byte
    # mutations rejected. This does not pretend to include original DLLs.
    bodies=(bytes.fromhex('c3'),
            bytes.fromhex('8b490c8b01ff20'),
            bytes.fromhex('c3'))
    total=0
    for original,row in zip(bodies,METHODS):
        assert len(original)==row[3]
        assert hashlib.sha256(original).hexdigest()==row[4]
        for i in range(len(original)):
            corrupt=bytearray(original);corrupt[i]^=1
            assert hashlib.sha256(corrupt).hexdigest()!=row[4]
            total+=1
    assert total==9 and len(REGISTERED)==8
    assert sum(row[3] for row in METHODS)==9
    assert sum(x[2][3]==0x220CD900 for x in REGISTERED)==6
    assert all(len(x[2])==4 for x in REGISTERED)
    try:audit(b'not original',b'not original')
    except Refusal:pass
    else:raise AssertionError('Incorrect commercial DLL SHA accepted')
    print('PHASE617_ORIGINAL_FREE_ASSAULT_ACTIVATION_PASS (9 corrupt native-body bytes refused)')

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
        p.error('Use --self-test, --bin-zip or paired --server/--client')
    try:
        if a.bin_zip:
            with zipfile.ZipFile(a.bin_zip) as z:
                result=audit(z.read('server.dll'),z.read('client.dll'))
        else:result=audit(a.server.read_bytes(),a.client.read_bytes())
    except (Refusal,OSError,KeyError,struct.error,zipfile.BadZipFile) as ex:
        p.exit(2,'PHASE617 ORIGINAL PE32 SOURCE REFUSED: %s\n'%ex)
    serialized=json.dumps(result,indent=2)+'\n'
    if a.out:
        a.out.write_text(serialized,encoding='utf-8')
        print('Authenticated original Assault activation slot metadata:',a.out)
    else:print(serialized)
if __name__=='__main__':main()
