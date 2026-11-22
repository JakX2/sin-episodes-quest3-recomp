#!/usr/bin/env python3
"""Phase603: authenticate original SiN 2006 Grunt behavior-host constructor RTTI.

Reads only the owner's SHA-pinned original PE32 files. No proprietary code
bytes, entire disassembly or game asset payloads are exported or committed.
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
HOST=(0x2216EDE0,86,'2d403029787e1848e3804b4cdcfddd20a8eae329bb262171c2f0859a3bda48e1')
GRUNT=(0x22360D80,338,'20ba9979f88e567a350b3378178dd03eef942667b891a83d4510fd539059356b')
# EXACT original RTTI TypeDescriptors via original MSVC vtable[-4] COL.
RTTI=(
 (0x224190A0,'.?AVIBehaviorBackBridge@@'),
 (0x224188B8,'.?AV?$CAI_BehaviorHost@VCAI_BaseNPC@@@@'),
 (0x2249543C,'.?AVCNPC_GruntBase@@'),
)
EDGES={0x22360D86:0x2216EDE0,0x2216EDE3:0x22043DE0}
# Source-specific instructions for host null+collection initialization, actual
# vtable writes, and the three previously ambiguous unrelated immediate stores.
WITNESSES={
 0x2216EDE8:'c786140c0000a0904122',
 0x2216EDF2:'33c0',0x2216EDF4:'8986180c0000',
 0x2216EE0A:'c786140c0000b8884122',
 0x2216EE14:'89861c0c0000',
 0x2216EE1A:'8986200c0000',
 0x2216EE20:'8986240c0000',
 0x2216EE26:'8986280c0000',
 0x2216EE2C:'89862c0c0000',
 0x22360D9B:'c786140c00003c544922',
  0x223527AC:'c786180c000001000000',
  0x22353C3C:'c786180c000003000000',
  0x22353D4C:'c786180c000002000000',
}
def verify_rtti(pe,vtable,expected):
    require(pe.section(vtable)=='.rdata','Original vtable outside .rdata')
    col=pe.u32(vtable-4)
    require(pe.section(col)=='.rdata','Original RTTI COL outside .rdata')
    td=pe.u32(col+0xC)
    require(pe.section(td+8) in ('.rdata','.data'),
            'Original RTTI TypeDescriptor not in data')
    require(pe.read(td+8,len(expected)+1)==expected.encode('ascii')+b'\0',
            'Original native RTTI TypeDescriptor mismatch '+hex(vtable))

def audit(server,client):
    require(hashlib.sha256(server).hexdigest()==SERVER,
            'Unsupported original server.dll')
    require(hashlib.sha256(client).hexdigest()==CLIENT,
            'Unsupported original client.dll')
    pe=PE32(server)
    for start,n,sha in (HOST,GRUNT):
        require(hashlib.sha256(pe.read(start,n)).hexdigest()==sha,
                'Complete original constructor differs '+hex(start))
        require(pe.read(start+n,4)==b'\xcc'*4,
                'Original constructor boundary/padding differs '+hex(start))
    for vtable,name in RTTI:verify_rtti(pe,vtable,name)
    for site,target in EDGES.items():check_edge(pe.read,site,target,0xE8)
    for site,bytes_hex in WITNESSES.items():
        raw=bytes.fromhex(bytes_hex)
        require(pe.read(site,len(raw))==raw,
                'Original ctor/false-positive witness changed '+hex(site))
    require(pe.read(0x2216EE36,10)==b'\xcc'*10,
            'Original host constructor alignment differs')
    require(pe.read(0x2216EE40,2)==bytes.fromhex('568b'),
            'Original next method begins elsewhere')
    return {
        'status':'PHASE603_ORIGINAL_BEHAVIOR_HOST_RTTI_AND_CTOR_AUTHENTICATED',
        'original_server_sha256':SERVER,'original_client_sha256':CLIENT,
        'new_original_complete_methods':1,
        'new_original_whole_method_bytes':HOST[1],
        'previous_grunt_constructor_reauthenticated_bytes':GRUNT[1],
        'original_host_constructor':'0x%08X'%HOST[0],
        'original_host_constructor_sha256':HOST[2],
        'RTTI_2006_behavior_host_class':'CAI_BehaviorHost<CAI_BaseNPC>',
        'RTTI_2006_secondary_interface':'IBehaviorBackBridge',
        'RTTI_2006_grunt_parent_class':'CNPC_GruntBase',
        'original_grunt_calls_host_constructor_at':'0x22360D86',
        'owner_0xC14_secondary_behavior_host_vtable_installed':True,
        'owner_0xC18_initially_null':True,
        'owner_behavior_collection_zeroed':['0xC1C','0xC20','0xC24','0xC28','0xC2C'],
        'phase595_false_pointer_assignment_sites':[
            {'native_va':'0x223527AC','literal_stored':1},
            {'native_va':'0x22353C3C','literal_stored':3},
            {'native_va':'0x22353D4C','literal_stored':2},
        ],
        'native_original_direct_constructor_E8_edges':len(EDGES),
        'native_sparse_opcode_field_witnesses':len(WITNESSES),
        'original_current_behavior_field_declaration_name_proven':False,
        'original_runtime_behavior_selection_proven':False,
        'proprietary_machine_code_bytes_exported':False,
    }

def selftest():
    class Synthetic:
        def __init__(self,site,raw):self.site,self.raw=site,raw
        def read(self,site,n):
            require(site==self.site and n==5,'Synthetic read mismatch')
            return self.raw
    changed_count=0
    for site,target in EDGES.items():
        encoded=b'\xe8'+struct.pack('<i',target-site-5)
        check_edge(Synthetic(site,encoded).read,site,target,0xE8)
        for index in range(5):
            bad=bytearray(encoded);bad[index]^=1
            try:check_edge(Synthetic(site,bytes(bad)).read,site,target,0xE8)
            except Refusal:changed_count+=1
            else:raise AssertionError('Accepted corrupted original constructor call')
    assert changed_count==10
    try:audit(b'invalid',b'invalid')
    except Refusal:pass
    else:raise AssertionError('Accepted wrong original DLLs')
    print('PHASE603_ORIGINAL_FREE_BEHAVIOR_HOST_CTOR_SYNTHETIC_PASS (10 corrupted E8 edges refused)')

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
        p.error('Use --self-test, --bin-zip or paired original DLLs')
    try:
        if a.bin_zip:
            with zipfile.ZipFile(a.bin_zip) as z:
                result=audit(z.read('server.dll'),z.read('client.dll'))
        else:result=audit(a.server.read_bytes(),a.client.read_bytes())
    except (Refusal,OSError,struct.error,KeyError,zipfile.BadZipFile) as exc:
        p.exit(2,'PHASE603 ORIGINAL SOURCE REFUSED: %s\n'%exc)
    output=json.dumps(result,indent=2)+'\n'
    if a.out:
        a.out.write_text(output,encoding='utf-8')
        print('Original constructor/RTTI metadata:',a.out)
    else:print(output)

if __name__=='__main__':main()
