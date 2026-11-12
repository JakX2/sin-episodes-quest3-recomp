#!/usr/bin/env python3
"""Phase182 original 2006 client surface file/KeyValues GetString verification.

Reads owner-local PE32 client.dll and server.dll; checks *both full file hashes*,
five complete native function region SHA256s, one separate jump-table digest,
scattered x86 instruction/call witnesses and original game strings.
Chained Phase181 audits when used through audit(). No original DLL/code-body
bytes, extracted assets, or disassembly are committed to this research tool.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
from pathlib import Path

CLIENT_SHA='6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d'
SERVER_SHA='568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650'

class VerificationError(ValueError):pass
class PEImage:
    """Minimal section-aware read-only 32-bit PE mapper, standard library only."""
    def __init__(self,b:bytes):
        self.b=b
        if b[:2]!=b'MZ' or len(b)<0x100:raise VerificationError('not DOS PE')
        pe=struct.unpack_from('<I',b,0x3c)[0]
        if pe+24>len(b) or b[pe:pe+4]!=b'PE\0\0':raise VerificationError('PE signature mismatch')
        if struct.unpack_from('<H',b,pe+4)[0]!=0x14c:raise VerificationError('PE machine is not i386')
        n=struct.unpack_from('<H',b,pe+6)[0]
        opt=pe+24
        opt_size=struct.unpack_from('<H',b,pe+20)[0]
        if struct.unpack_from('<H',b,opt)[0]!=0x10b or not 1<=n<=96:
            raise VerificationError('not valid PE32')
        self.base=struct.unpack_from('<I',b,opt+28)[0]
        self.sections=[]
        start=opt+opt_size
        for i in range(n):
            off=start+i*40
            if off+40>len(b):raise VerificationError('section header overflow')
            name=b[off:off+8].split(b'\0')[0].decode('ascii','replace')
            vsz,rva,sz,ptr=struct.unpack_from('<IIII',b,off+8)
            self.sections.append((name,rva,vsz,sz,ptr))
    def section_of(self,va:int)->str:
        rva=va-self.base
        for name,start,vsz,sz,ptr in self.sections:
            if start<=rva<start+max(vsz,sz):return name
        return ''
    def at(self,va:int,n:int)->bytes:
        if n<0:raise VerificationError('negative read')
        rva=va-self.base
        for name,start,vsz,sz,ptr in self.sections:
            if start<=rva and rva+n<=start+sz:
                off=ptr+rva-start
                if off+n<=len(self.b):return self.b[off:off+n]
        raise VerificationError('native VA inaccessible: %#x'%va)

# Native code stops before the separate five-DWORD jump table at 0x241668A8.
REGIONS={
 'keyvalues_get_string_typed':(0x241667C0,232,'ff0eb63ceb387d73abca9cd91928809eac5a902ee04fc3eea2098c22325a5124'),
 'surface_file_open_read_parse':(0x24191EC0,136,'f5e1bbb19b7d8305690aa65992653d244ee0e8aaaf105e057fb760a889c218b5'),
 'conditional_surface_dispatch_slot4':(0x24192050,32,'b558e0cbc0c489b26369d3ea1e38d16e5668952877b892e698051b5979a742e8'),
 'conditional_surface_dispatch_slot8':(0x24192070,32,'4123fed65cbc55dbc746751ac69c5ed77b6fe95e597eb7f37971373b37dc1f4e'),
 'unconditional_surface_dispatch_slot12':(0x24192090,22,'00ecc6870ea5a88df35279c6bba7ed2c9b626b3600bec486303451b64ceede86'),
}
VALUE_TAG_TABLE_VA=0x241668A8
VALUE_TAG_TARGETS=(0x2416688F,0x2416682B,0x241667F4,0x2416682B,0x2416685D)
VALUE_TAG_TABLE_SHA='dbcd6e68c213b9cc929530b59a5a5104e681584c1a2f8c4c95fcab84702291c4'
OPCODES={
 0x241667C0:'83ec40', 0x241667CD:'8bf9', 0x241667D6:'85f6',
 0x241667E1:'83c0ff', 0x241667ED:'ff2485a8681624',
 0x241667FA:'dd1c24',0x24166806:'6a40',0x24166812:'8d4c240c',
 0x2416681F:'8b4604',0x2416682B:'8b560c',0x24166838:'6a40',
 0x24166844:'8d4c240c',0x24166851:'8b4604',0x2416685D:'8b5608',
 0x24166864:'6800020000',0x24166871:'6a00',0x24166873:'68e9fd0000',
 0x2416687E:'85c0',0x24166887:'8bcf',0x24166895:'83c440',
 0x241668A2:'83c440',
 0x24191EC1:'8bec',0x24191ECB:'8b06',0x24191ED6:'8bce',
 0x24191EDD:'85ff',0x24191EE4:'8bce',0x24191EEB:'8d4301',
 0x24191EF3:'e8e8740b00',0x24191EFF:'8bce',0x24191F06:'8b06',
 0x24191F0E:'8b4510',0x24191F18:'8b11',0x24191F22:'8d65f4',
 0x24191F2A:'8b0d8cd12f24',0x24191F37:'ff15fc612724',
 0x24192050:'8b542404',0x24192056:'7417',0x2419205C:'85c0',
 0x24192060:'8b0dac433724',0x2419206B:'ff5604',
 0x24192070:'8b542404',0x24192076:'7417',0x2419207C:'85c0',
 0x24192080:'8b0dac433724',0x2419208B:'ff5608',
 0x24192090:'8b542408',0x2419209A:'8b01',0x241920A2:'ff500c',
}
DIRECT_CALLS={
 0x241667CF:0x241656E0,
 0x2416681A:0x24165AE0,
 0x2416684C:0x24165AE0,
 0x2416688A:0x24165AE0,
 0x24191EF3:0x242493E0,
}
INCOMING_CALLS={0x24192833:0x241667C0,0x24192839:0x24191EC0}
VTABLE_CALLS={
 0x24191ED8:'ff5008',0x24191EE6:'ff521c',
 0x24191F04:'ff12',0x24191F0B:'ff500c',
 0x24191F1F:'ff5204',0x2419206B:'ff5604',
 0x2419208B:'ff5608',0x241920A2:'ff500c',
}
STRINGS={
 0x24289C0C:b'rb\0',
 0x242A2C90:b"Unable to load surface prop file '%s' (referenced by manifest file '%s')\n\0",
 0x242850E8:b'%f\0',
 0x242D49F0:b'%d\0',
}

def verify_native(pe:PEImage,*,check_hash=True)->dict:
    if pe.base!=0x24000000:raise VerificationError('original client base mismatch')
    for site,rawhex in {**OPCODES,**VTABLE_CALLS}.items():
        if pe.at(site,len(bytes.fromhex(rawhex)))!=bytes.fromhex(rawhex):
            raise VerificationError('Phase182 opcode/vtable mismatch at %#x'%site)
    for site,target in {**DIRECT_CALLS,**INCOMING_CALLS}.items():
        opcode=b'\xe8'+struct.pack('<i',target-site-5)
        if pe.at(site,5)!=opcode:
            raise VerificationError('Phase182 CALL mismatch at %#x'%site)
    for site,raw in STRINGS.items():
        if pe.section_of(site)!='.rdata' or pe.at(site,len(raw))!=raw:
            raise VerificationError('Phase182 original string mismatch at %#x'%site)
    rawtable=pe.at(VALUE_TAG_TABLE_VA,20)
    if struct.unpack('<5I',rawtable)!=VALUE_TAG_TARGETS or hashlib.sha256(rawtable).hexdigest()!=VALUE_TAG_TABLE_SHA:
        raise VerificationError('Phase182 KeyValues type jump table mismatch')
    # Caches the manifest name pointer for warning on open failure.
    if pe.at(0x24191F2A,6)!=b'\x8b\x0d'+struct.pack('<I',0x242FD18C):
        raise VerificationError('Phase182 native manifest global reference mismatch')
    if check_hash:
        for name,(va,n,digest) in REGIONS.items():
            if hashlib.sha256(pe.at(va,n)).hexdigest()!=digest:
                raise VerificationError('Phase182 complete original function SHA mismatch '+name)
    return {
       'complete_original_function_hashes':len(REGIONS) if check_hash else 0,
       'original_function_bytes':sum(n for _,n,_ in REGIONS.values()) if check_hash else 0,
       'original_separate_jump_table_bytes':20,
       'sparse_opcode_witnesses':len(OPCODES),
       'original_vtable_dispatch_witnesses':len(VTABLE_CALLS),
       'internal_rel32_calls':len(DIRECT_CALLS),
       'incoming_rel32_calls':len(INCOMING_CALLS),
       'original_static_strings':len(STRINGS),
       'original_value_type_mapping':{'1':'inline string','2':'decimal','3':'float','4':'decimal','5':'UTF8 wide conversion'},
       'limitations':['Original KeyValues setters/find-key and Source filesystem/physics vtables remain host callbacks',
         'Host uses bounded heap memory instead of original unchecked stack allocation',
         'No running original Windows game or functional reconstructed client/server DLLs'],
    }

def audit(client_bytes:bytes,server_bytes:bytes)->dict:
    if hashlib.sha256(client_bytes).hexdigest()!=CLIENT_SHA or hashlib.sha256(server_bytes).hexdigest()!=SERVER_SHA:
        raise VerificationError('original 2006 full client/server DLL SHA256 mismatch')
    # Import lazily so the Phase182 native verifier can also be independently
    # exercised with isolated original-free test fixtures.
    from audit_sin_original_client_surface_manifest import audit as phase181_audit
    report=phase181_audit(client_bytes,server_bytes)
    report['original_client_surface_file']=verify_native(PEImage(client_bytes))
    return report

def main()->None:
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--client',required=True,type=Path)
    parser.add_argument('--server',required=True,type=Path)
    parser.add_argument('--output',type=Path)
    args=parser.parse_args()
    data=json.dumps(audit(args.client.read_bytes(),args.server.read_bytes()),indent=2)+'\n'
    if args.output:args.output.write_text(data,encoding='utf-8')
    else:print(data,end='')
if __name__=='__main__':main()