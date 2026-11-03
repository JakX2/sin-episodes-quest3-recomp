#!/usr/bin/env python3
"""Verify original 2006 SiN client/server CreateInterface registrations and vtables.

Read-only PE32 x86 metadata/protocol verifier; original game binaries required
but NEVER copied to the repository. Windows VAs are not Android pointers.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import struct

ORIGINAL={
    'client.dll':dict(sha256='6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d',
        base=0x24000000,export=0x24164180,ctor=0x24164160,head=0x24369378,count=55,
        required=('VClient011','GameClientExports001','VClientEntityList003',
                  'ClientLeafSystem001','GameMovement001','VClientPrediction001')),
    'server.dll':dict(sha256='568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650',
        base=0x22000000,export=0x221fb5f0,ctor=0x221fb5d0,head=0x22573c5c,count=10,
        required=('ServerGameDLL003','ServerGameClients003','ServerGameEnts001',
                  'GameMovement001','PlayerInfoManager001','PlayerInfoManager002',
                  'PluginHelpersCheck001','HLTVDirector001'))
}
VTABLES={
    'client.dll':{
        'VClient011':(0x24296c6c,48,'9d28eccf91315f7a6ecc2394386d7b4077919d8649d34297a951eb7a1cafa664'),
        'GameClientExports001':(0x24296b18,4,'23ced2e618d2df47dff35544c12c237d3edf7d7b5a25c823b02d1f433c65dc82')},
    'server.dll':{
        'ServerGameDLL003':(0x22442efc,31,'97e42f1905b28e6aad0e3de21473d12725c49b25980be4a5db285333774a8299'),
        'ServerGameEnts001':(0x22443030,7,'802c3d045ace1464abe8259852c6654c2c4a2327b56be820085cca8e609038ae'),
        'ServerGameClients003':(0x22443054,15,'5d0631d25abacc8fea804b22e5bf489b2d667ce240417c3ab20cc06787eb535f')}
}
class VerificationError(ValueError):pass

class PE32:
    def __init__(self,data):
        self.data=data
        if len(data)<0x100 or data[:2]!=b'MZ':raise VerificationError('not MZ')
        off=struct.unpack_from('<I',data,0x3c)[0]
        if off+0x100>len(data) or data[off:off+4]!=b'PE\x00\x00':
            raise VerificationError('not PE')
        machine,count=struct.unpack_from('<HH',data,off+4)
        optsize=struct.unpack_from('<H',data,off+20)[0]
        if machine!=0x14c or struct.unpack_from('<H',data,off+24)[0]!=0x10b:
            raise VerificationError('not x86 PE32')
        self.base=struct.unpack_from('<I',data,off+24+28)[0]
        self.export_rva=struct.unpack_from('<I',data,off+24+96)[0]
        sections=off+24+optsize
        self.sections=[]
        for i in range(count):
            pos=sections+i*40
            label=data[pos:pos+8].split(b'\x00')[0].decode('ascii','replace')
            virtual,rva,raw,raw_off=struct.unpack_from('<IIII',data,pos+8)
            if raw_off+raw>len(data):raise VerificationError('invalid section')
            self.sections.append((label,self.base+rva,virtual,raw_off,raw))
    def at(self,va,n):
        for label,start,virtual,offset,size in self.sections:
            if start<=va and va+n<=start+size:
                idx=offset+va-start
                return self.data[idx:idx+n]
        raise VerificationError('unbacked VA %#x'%va)
    def cstring(self,va):
        buf=bytearray()
        for i in range(160):
            ch=self.at(va+i,1)[0]
            if ch==0:return buf.decode('ascii')
            if not 32<=ch<=126:raise VerificationError('bad interface name')
            buf.append(ch)
        raise VerificationError('unterminated interface name')
    def exports(self):
        root=self.base+self.export_rva
        function_count,name_count,eat,names,ordinals=struct.unpack('<5I',self.at(root+20,20))
        if function_count>5000 or name_count>5000:
            raise VerificationError('unreasonable exports')
        result={}
        for i in range(name_count):
            namerva=struct.unpack('<I',self.at(self.base+names+i*4,4))[0]
            ordinal=struct.unpack('<H',self.at(self.base+ordinals+i*2,2))[0]
            if ordinal>=function_count:raise VerificationError('bad export ordinal')
            funcrva=struct.unpack('<I',self.at(self.base+eat+ordinal*4,4))[0]
            result[self.cstring(self.base+namerva)]=self.base+funcrva
        return result
    def code(self):
        label,va,virt,offset,size=next(x for x in self.sections if x[0]=='.text')
        return va,self.data[offset:offset+size]

def verify_vtables(pe,module,registrations):
    start,code=pe.code()
    objects={r['name']:int(r['singleton_return_va'],16)
             for r in registrations if r['singleton_return_va']}
    output={}
    for name,(vt,count,digest) in VTABLES[module].items():
        if name not in objects:raise VerificationError('missing original singleton: '+name)
        singleton=objects[name]
        if struct.unpack('<I',pe.at(singleton,4))[0]!=vt:
            raise VerificationError('vtable pointer differs: '+name)
        raw=pe.at(vt,4*count)
        if hashlib.sha256(raw).hexdigest()!=digest:
            raise VerificationError('vtable slot SHA-256 mismatch: '+name)
        slots=struct.unpack('<%dI'%count,raw)
        if any(not start<=x<start+len(code) for x in slots):
            raise VerificationError('non-executable vtable slot')
        after=struct.unpack('<I',pe.at(vt+count*4,4))[0]
        if start<=after<start+len(code):
            raise VerificationError('vtable contiguous slot count underestimated')
        output[name]=dict(singleton_va=hex(singleton),vtable_va=hex(vt),
                          verified_consecutive_slot_count=count,
                          slot_sha256=digest,slot_x86_vas=[hex(x) for x in slots],
                          original_method_signatures='not recovered by this scan')
    return output

def inspect_module(data,module,*,require_original=True):
    info=ORIGINAL[module]
    digest=hashlib.sha256(data).hexdigest()
    if require_original and digest!=info['sha256']:
        raise VerificationError('original '+module+' SHA-256 mismatch')
    pe=PE32(data)
    if pe.base!=info['base']:raise VerificationError('image base mismatch')
    exports=pe.exports()
    if exports.get('CreateInterface')!=info['export']:
        raise VerificationError('CreateInterface export address mismatch')
    ctor,head,factory=info['ctor'],info['head'],info['export']
    ctor_bytes=(b'\x8b\x54\x24\x04\x8b\xc1\x8b\x4c\x24\x08'
                b'\x89\x48\x04\x89\x10\x8b\x0d'+struct.pack('<I',head)
                +b'\x89\x48\x08\xa3'+struct.pack('<I',head)
                +b'\xc2\x08\x00')
    if pe.at(ctor,len(ctor_bytes))!=ctor_bytes:
        raise VerificationError('registry constructor opcode mismatch')
    for offset,code in ((0x11,b'\x8b\x47\x04'),(0x1a,b'\x3a\x16'),
                        (0x43,b'\x8b\x7f\x08'),(0x52,b'\xc7\x00\x01\x00\x00\x00'),
                        (0x66,b'\xc7\x00\x00\x00\x00\x00'),(0x6c,b'\xff\x17')):
        if pe.at(factory+offset,len(code))!=code:
            raise VerificationError('factory lookup opcode mismatch')
    start,blob=pe.code()
    rows=[]
    # Exact 20-byte MSVC initializer: push name; push create function;
    # mov ecx, registration node; CALL the registry constructor.
    for i in range(len(blob)-19):
        if not (blob[i]==0x68 and blob[i+5]==0x68
                and blob[i+10]==0xb9 and blob[i+15]==0xe8):continue
        site=start+i
        dest=site+20+struct.unpack_from('<i',blob,i+16)[0]
        if dest!=ctor:continue
        nameptr=struct.unpack_from('<I',blob,i+1)[0]
        callback=struct.unpack_from('<I',blob,i+6)[0]
        node=struct.unpack_from('<I',blob,i+11)[0]
        name=pe.cstring(nameptr)
        if not name or ' ' in name:raise VerificationError('bad interface name')
        if not any(label=='.data' and lo<=node and node+12<=lo+vsize
                   for label,lo,vsize,raw_off,raw in pe.sections):
            raise VerificationError('registration node outside .data')
        if not start<=callback<start+len(blob):
            raise VerificationError('callback outside code')
        cb=pe.at(callback,6)
        singleton=struct.unpack('<I',cb[1:5])[0] if cb[0]==0xb8 and cb[5]==0xc3 else None
        rows.append(dict(name=name,initializer_va=hex(site),callback_va=hex(callback),
                         node_va=hex(node),singleton_return_va=hex(singleton) if singleton else None))
    if len(rows)!=info['count']:
        raise VerificationError('unexpected static registration count')
    names=[x['name'] for x in rows]
    if len(names)!=len(set(names)):raise VerificationError('duplicate registered name')
    if not set(info['required']).issubset(names):
        raise VerificationError('missing required original interface')
    return dict(module=module,sha256=digest,export_CreateInterface_va=hex(factory),
                registry_constructor_va=hex(ctor),registry_head_va=hex(head),
                static_registration_count=len(rows),registrations=rows,
                verified_vtables=verify_vtables(pe,module,rows),
                other_exports={k:hex(v) for k,v in exports.items() if k!='CreateInterface'},
                status_missing=1,status_found=0,
                name_match='exact case-sensitive comparison',
                limits=['Windows x86 pointers cannot be used as ARM64 pointers',
                        'Constructor order does not follow lexical name order',
                        'Consecutive executable vtable pointers are not complete method signatures',
                        'No original game module was loaded; SourceVR ABI still unverified'])

def audit(client,server):
    return dict(client=inspect_module(client,'client.dll'),
                server=inspect_module(server,'server.dll'))

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--client',required=True,type=Path)
    parser.add_argument('--server',required=True,type=Path)
    parser.add_argument('--output',required=True,type=Path)
    a=parser.parse_args()
    result=audit(a.client.read_bytes(),a.server.read_bytes())
    a.output.parent.mkdir(parents=True,exist_ok=True)
    a.output.write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    for key in ('client','server'):
        module=result[key]
        print(key,module['static_registration_count'],'registrations',
              'vtables:',[(k,v['verified_consecutive_slot_count'])
                          for k,v in module['verified_vtables'].items()])
if __name__=='__main__':main()
