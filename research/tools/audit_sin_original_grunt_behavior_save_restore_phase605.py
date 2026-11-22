#!/usr/bin/env python3
"""Phase605: authenticate original Grunt AI_Behaviors save/restore source.

Verifies complete 2006 retail method extents and original Grunt vtable pair.
The original private executable is read for hashes and sparse witnesses only:
no commercial code, decompiler text, or game assets are emitted.
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

# Original complete whole-method PE spans, exact source SHA256, following INT3 pad.
NEW_METHODS = (
    ("behavior_save_helper", 0x220CD530, 244,
     "a51be7f7295a12b98ed710325c55aa7b3ac97e938f9c8cedcd84cc0793f50c0f", 12),
    ("behavior_restore_helper", 0x220CD630, 341,
     "737d723103081e9c2474976ee2376d82f151db843c7a1e63988572702a3bd908", 11),
    ("grunt_save_wrapper", 0x2216DDF0, 67,
     "7d6a0a11e435df9b172cc1aa7cba7dd611d38e8f83b8d1a648611706b67b8b1e", 13),
)
GRUNT_VTABLE = 0x2249AAA4
PAIRED_VIRTUALS = {0xA0:0x2216DDF0,0xA4:0x2216DE40}
RESTORE_WRAPPER = (0x2216DE40,104,
    "273450c7062ad7ca28aa34edb7faa41c3d5f5d97641e943b32e387c8b06b3ee7")
DIRECT_CALLS = {
    0x2216DDFA:0x22043820, # original inherited Save
    0x2216DE23:0x220CD530, # original behavior save helper
    0x2216DE4A:0x22043AD0, # original inherited Restore
    0x2216DE6C:0x220CD630, # original behavior restore helper
    0x22375D63:0x220CD530, # separate BlendedNPC specialization
    0x22375DAC:0x220CD630,
}
# Sparse original instruction identity witnesses: address -> exact bytes.
WITNESSES = {
    0x220CD53A:'6830383f22',     # header "AI_Behaviors"
    0x220CD585:'8b3d384b4e22', # CAI_BehaviorBase sentinel pointer
    0x220CD5F7:'ff5038',       # serializer field flag virtual
    0x220CD600:'ff5230',       # per-behavior save callback
    0x220CD652:'ff5018',       # restore stream header virtual
    0x220CD65E:'b90d000000',   # 13 byte "AI_Behaviors" comparison
    0x220CD665:'f3a6',         # original repeat CMPS
    0x220CD67D:'66837c241402', # serialized version word = 2
    0x220CD6F9:'ff92d8000000',# original behavior name virtual +0xD8
    0x220CD72F:'38442413',     # restored active flag comparison
    0x220CD735:'897c2420',     # record index when active
    0x220CD73F:'ff5034',       # per-behavior restore callback
    0x2216DE1B:'8b86180c0000',# save wrapper loads current +0xC18
    0x2216DE8B:'898e180c0000',# restore wrapper stores indexed current
    0x2216DE96:'c786180c000000000000',# restore missing=-1 => NULL
}

def audit(server, client):
    require(hashlib.sha256(server).hexdigest()==SERVER,
            "Not the SHA-authenticated original 2006 server.dll")
    require(hashlib.sha256(client).hexdigest()==CLIENT,
            "Not the SHA-authenticated original 2006 client.dll")
    pe=PE32(server)
    for slot,target in PAIRED_VIRTUALS.items():
        require(pe.u32(GRUNT_VTABLE+slot)==target,
                "Original Grunt save/restore vtable slot differs")
    for name,va,n,sha,pad in NEW_METHODS:
        require(pe.section(va)==".text",name+" not in original .text")
        require(hashlib.sha256(pe.read(va,n)).hexdigest()==sha,
                "Original complete "+name+" source SHA differs")
        require(pe.read(va+n,pad)==b'\xcc'*pad,
                "Original "+name+" method boundary differs")
    va,n,sha=RESTORE_WRAPPER
    require(hashlib.sha256(pe.read(va,n)).hexdigest()==sha,
            "Phase604 restore wrapper whole-method SHA changed")
    require(pe.read(va+n,4)==b'\xcc'*4,
            "Original restore wrapper bound changed")
    require(pe.read(0x223F3830,13)==b'AI_Behaviors\x00',
            "Original serialized AI_Behaviors section marker differs")
    require(pe.read(0x223F380C,len(b'CAI_BehaviorBase\x00'))==
            b'CAI_BehaviorBase\x00',"Original base behavior sentinel changed")
    require(pe.u32(0x224E4B38)==0x223F380C,
            "Original global base behavior sentinel pointer changed")
    for site,target in DIRECT_CALLS.items():
        check_edge(pe.read,site,target,0xE8)
    for site,bytes_hex in WITNESSES.items():
        expected=bytes.fromhex(bytes_hex)
        require(pe.read(site,len(expected))==expected,
                "Original source witness changed at "+hex(site))
    return {
      "status":"PHASE605_ORIGINAL_GRUNT_BEHAVIOR_SAVE_RESTORE_AUTHENTICATED",
      "original_server_sha256":SERVER,"original_client_sha256":CLIENT,
      "new_whole_native_methods":len(NEW_METHODS),
      "new_whole_native_method_bytes":sum(x[2] for x in NEW_METHODS),
      "Grunt_paired_vslots_authenticated":{
        "+0xA0":"0x2216DDF0","+0xA4":"0x2216DE40"},
      "restore_wrapper_reauthenticated":"0x2216DE40",
      "new_method_sha_boundaries":[
        {"role":name,"native_va":"0x%08X"%va,"complete_bytes":n,
         "whole_sha256":sha,"verified_following_int3":pad}
        for name,va,n,sha,pad in NEW_METHODS
      ],
      "original_serialized_section":"AI_Behaviors",
      "original_serialization_format_version":2,
      "original_base_behavior_sentinel":"CAI_BehaviorBase",
      "save_helper_excludes_base_behavior_sentinel":True,
      "save_records_active_pointer_identity":True,
      "restore_matches_serialized_behavior_names":True,
      "restore_uses_per_behavior_virtual_plus_0x34":True,
      "restore_returns_recorded_active_index_or_minus_one":True,
      "restore_wrapper_sets_current_plus_0xC18_from_index_or_null":True,
      "phase604_plus_0xA4_is_normal_runtime_policy_selector":False,
      "normal_runtime_behavior_selection_policy_fully_recovered":False,
      "matched_direct_original_E8_calls":len(DIRECT_CALLS),
      "sparse_original_source_witnesses":len(WITNESSES),
      "proprietary_code_bytes_exported":False,
    }

def self_test():
    class Synthetic:
        def __init__(self,site,data):self.site,self.data=site,data
        def read(self,site,length):
            require(site==self.site and length==5,"Bad synthetic read")
            return self.data
    changes=0
    for site,target in DIRECT_CALLS.items():
        original=b'\xe8'+struct.pack('<i',target-site-5)
        check_edge(Synthetic(site,original).read,site,target,0xE8)
        for index in range(5):
            bad=bytearray(original);bad[index]^=1
            try:check_edge(Synthetic(site,bytes(bad)).read,site,target,0xE8)
            except Refusal:changes+=1
            else:raise AssertionError("Accepted mutated native E8")
    assert changes==30 and sum(x[2] for x in NEW_METHODS)==652
    try:audit(b'invalid',b'invalid')
    except Refusal:pass
    else:raise AssertionError("Wrong original binaries accepted")
    print("PHASE605_ORIGINAL_FREE_GRUNT_SAVE_RESTORE_PASS (30 mutated E8 calls rejected)")

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
        if not (a.bin_zip or a.server):return
    if a.bin_zip is None and not(a.server and a.client):
        p.error("Provide --bin-zip, paired --server/--client, or --self-test")
    try:
        if a.bin_zip:
            with zipfile.ZipFile(a.bin_zip) as archive:
                result=audit(archive.read('server.dll'),archive.read('client.dll'))
        else:result=audit(a.server.read_bytes(),a.client.read_bytes())
    except (OSError,Refusal,struct.error,zipfile.BadZipFile,KeyError) as exc:
        p.exit(2,"PHASE605 ORIGINAL RETAIL AUDIT REFUSED: %s\n"%exc)
    output=json.dumps(result,indent=2)+"\n"
    if a.out:
        a.out.write_text(output,encoding='utf-8')
        print('Original source-authenticated save/restore metadata:',a.out)
    else:print(output)
if __name__=='__main__':main()
