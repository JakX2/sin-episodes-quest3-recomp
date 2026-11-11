#!/usr/bin/env python3
"""Phase180: original 2006 SiN client.dll post-Init app/physics checks.

Strictly read-only. Verifies the entire source PE32 client.dll/server.dll,
three complete original client x86 functions, native opcode/branch anchors,
exact original CALL rel32 edges and three immutable physics interface names.
Chains Phase179's dual-DLL and Init audit only when invoked with both DLLs.
Never publish private original binaries or full native function bodies.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
from pathlib import Path

CLIENT_SHA='6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d'
SERVER_SHA='568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650'
REGIONS={
 'client_app_list_cleanup':(0x241640F0,48,'93dbf327403a2963eba2ee6341b0aa5bfc0a7fef581840e57b71b418d7ae78d2'),
 'client_app_list_initialize':(0x24164120,54,'645a9002d77b476692fe8688d691e87c4d2ebf8a61976be9eda071e41d7d3b7f'),
 'client_mandatory_physics_triple':(0x24182C60,97,'46e079ea76e0cfee721efe3a421d68920cf980079aea4d76d7542f6f9b5281f2'),
}
OPCODES={
0x241640F0:'56',0x241640F1:'8b3574933624',0x241640F7:'85f6',
0x241640F9:'7423',0x24164100:'8b06',0x24164104:'7411',
0x24164109:'ff5608',0x24164111:'c70100000000',
0x24164117:'8b760c',0x2416411F:'c3',
0x24164120:'56',0x24164121:'8b3574933624',0x24164129:'741e',
0x24164130:'ff5604',0x24164133:'85c0',0x24164135:'7406',
0x24164137:'8b0e',0x24164139:'8901',0x2416413D:'833e00',
0x24164140:'750b',0x24164142:'8b760c',0x24164149:'b001',
0x24164152:'32c0',0x24164155:'c3',
0x24182C60:'56',0x24182C61:'8b742408',0x24182C65:'6a00',
0x24182C67:'6838232a24',0x24182C6C:'ffd6',0x24182C71:'85c0',
0x24182C73:'a398433724',0x24182C78:'7443',0x24182C7A:'6a00',
0x24182C7C:'6820232a24',0x24182C81:'ffd6',0x24182C86:'85c0',
0x24182C88:'a3a8433724',0x24182C8D:'742e',0x24182C8F:'6a00',
0x24182C91:'6808232a24',0x24182C96:'ffd6',0x24182C9B:'85c0',
0x24182C9D:'a3a0433724',0x24182CA2:'7419',
0x24182CA4:'a1bcf63324',0x24182CA9:'8b0da8433724',
0x24182CAF:'50',0x24182CB0:'51',0x24182CB9:'b001',
0x24182CBD:'32c0',0x24182CC0:'c3',
}
# Internal and incoming genuine x86 E8 rel32 CALLs (not guesses).
CALLS={0x2416414D:0x241640F0,0x24182CB1:0x241927A0}
INCOMING={0x240FE39B:0x24164120,0x240FE540:0x24182C60}
PHYSICS=(
 ('VPhysics030',0x242A2338,0x24374398),
 ('VPhysicsSurfaceProps001',0x242A2320,0x243743A8),
 ('VPhysicsCollision007',0x242A2308,0x243743A0),
)

class VerificationError(ValueError):pass

class PEImage:
    """Read original owner-supplied PE32 mapping, not an injected runtime image."""
    def __init__(self, data: bytes):
        self.data=data
        if len(data)<0x100 or data[:2]!=b'MZ': raise VerificationError('invalid DOS file')
        pe=struct.unpack_from('<I',data,0x3c)[0]
        if pe+24>len(data) or data[pe:pe+4]!=b'PE\x00\x00':
            raise VerificationError('invalid PE header')
        machine,count=struct.unpack_from('<HH',data,pe+4)
        optsize=struct.unpack_from('<H',data,pe+20)[0]
        opt=pe+24
        if machine!=0x14c or not 1<=count<=96 or opt+optsize+40*count>len(data):
            raise VerificationError('expected original PE32 x86 DLL')
        if struct.unpack_from('<H',data,opt)[0]!=0x10b:
            raise VerificationError('expected original PE32 optional header')
        self.base=struct.unpack_from('<I',data,opt+28)[0]
        self.sections=[]
        for i in range(count):
            p=opt+optsize+40*i
            name=data[p:p+8].split(b'\x00')[0].decode('ascii','replace')
            virtual,rva,size,off=struct.unpack_from('<IIII',data,p+8)
            self.sections.append((name,rva,virtual,size,off))
    def at(self,va:int,n:int)->bytes:
        rva=va-self.base
        for _,start,virtual,size,off in self.sections:
            if start<=rva and rva-start+n<=size and off+rva-start+n<=len(self.data):
                return self.data[off+rva-start:off+rva-start+n]
        raise VerificationError('original VA not in raw DLL section: %#x'%va)
    def section_of(self,va:int)->str:
        rva=va-self.base
        for name,start,virtual,size,off in self.sections:
            if start<=rva<start+max(virtual,size):return name
        return ''

def verify_native(pe,*,check_hash=True)->dict:
    if pe.base!=0x24000000:raise VerificationError('wrong original client image base')
    for va,h in OPCODES.items():
        b=bytes.fromhex(h)
        if pe.at(va,len(b))!=b:raise VerificationError('original client postinit opcode mismatch at %#x'%va)
    for va,target in {**CALLS,**INCOMING}.items():
        wanted=b'\xe8'+struct.pack('<i',target-va-5)
        if pe.at(va,5)!=wanted:raise VerificationError('original client postinit rel32 CALL mismatch at %#x'%va)
    for name,va,global_va in PHYSICS:
        if pe.section_of(va)!='.rdata' or pe.section_of(global_va)!='.data':
            raise VerificationError('original physics name/global section mismatch')
        if pe.at(va,len(name)+1)!=name.encode('ascii')+b'\x00':
            raise VerificationError('original client physics interface version mismatch '+name)
    if check_hash:
        for label,(va,n,digest) in REGIONS.items():
            if hashlib.sha256(pe.at(va,n)).hexdigest()!=digest:
                raise VerificationError('original client complete function SHA mismatch '+label)
    return {
     'verified_complete_original_functions':len(REGIONS) if check_hash else 0,
     'verified_complete_original_native_bytes':sum(n for _,n,_ in REGIONS.values()) if check_hash else 0,
     'verified_native_opcode_witnesses':len(OPCODES),
     'verified_native_internal_CALL_edges':len(CALLS),
     'verified_original_incoming_CALL_edges':len(INCOMING),
     'native_app_head':'0x24369374',
     'app_system_record_stride_bytes':16,
     'physics_interfaces':[name for name,_,_ in PHYSICS],
     'limitations':['callbacks have unknown engine-owned implementations',
                    'host models add safe handling of native invalid success+null-output precondition',
                    'no original Windows game engine loading or playable Windows DLLs'],
    }

def audit(client_bytes:bytes,server_bytes:bytes)->dict:
    if hashlib.sha256(client_bytes).hexdigest()!=CLIENT_SHA or hashlib.sha256(server_bytes).hexdigest()!=SERVER_SHA:
        raise VerificationError('original 2006 full client/server DLL SHA mismatch')
    # Preserve all ancestor original-DLL and Init evidence checks in repo.
    from audit_sin_original_dll_initialization import audit as phase179_audit
    result=phase179_audit(client_bytes,server_bytes)
    result['original_client_post_init']=verify_native(PEImage(client_bytes))
    return result

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--client',type=Path,required=True)
    p.add_argument('--server',type=Path,required=True)
    p.add_argument('--output',type=Path)
    a=p.parse_args()
    result=audit(a.client.read_bytes(),a.server.read_bytes())
    report=json.dumps(result,indent=2)+'\n'
    if a.output:
        a.output.parent.mkdir(parents=True,exist_ok=True)
        a.output.write_text(report,encoding='utf-8')
    else:print(report,end='')
if __name__=='__main__':main()