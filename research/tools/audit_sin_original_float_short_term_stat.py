#!/usr/bin/env python3
"""Exact owner-binary evidence for SiN 2006 CFloatShortTermStat.

Verifies both PE32 DLL hashes, RTTI, 13 virtual entries, x86 function bodies
and call edges. Results are metadata only; no proprietary DLL bytes exported.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

SHA256 = {
    'server':'568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650',
    'client':'6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d',
}
REGIONS = {
    'float_constructor':(0x222C94C0,63,'e88f67926f1b308b2627794f46635a51488d00e8048d06a03b6a28e87b002d7b'),
    'ring_initialization':(0x222C8EE0,78,'d0c67fe99ca9bc65e3c8f975d49fef5cad27038f9e6a1ad35857051aba891a38'),
    'float_mean':(0x222C9050,52,'9cbbab52ba3fadb137f6fda792559cc95eb133846345af0fbc0a11dccf0f8146'),
    'ring_sum':(0x222C64A0,63,'5a4d4b9c090e88b37576434e2b7d7569fb073d46276784dd2f1137638aca907b'),
    'fill_history':(0x222C9500,37,'05b3374ab164ab8df893c98b9f618f593bee9efb8419e5591ef08521b8dfdc3c'),
    'append_history':(0x222C9530,49,'7cff2102b93d236b86f59f8cc6b641a4b7e8ab0cad6f2e4523ba0bb412d40fcf'),
    'resize_preserve_mean':(0x222C9090,36,'c3f2a5d91e7b0679c57513811cb5ee3cdbf2a07666daac9415a599a0c37bb4a0'),
    'dirty_peek':(0x222C8E40,4,'e0eb8cffec07b89f095aa1e60d89eed1f3338b76216e649ad3aeb3ba92dc7d04'),
    'dirty_consume':(0x222C8E50,8,'bb9a10ca7bb347cc13896787e247cdd9b8149e51f5987cc0155c051d1dd1817f'),
}
# Consecutive primary virtual slots from actual original 2006 MSVC RTTI COL.
VTABLE_START = 0x2245E6E8
VTABLE_TARGETS = (
    0x222C9570,0x222C8DD0,0x222C9050,0x222C9500,
    0x222C9090,0x222C8DE0,0x222C8DF0,0x222C8E00,
    0x222C8E20,0x222C9530,0x222C8E30,0x222C8E40,0x222C8E50,
)
RTTI = {
    0x2245E6E4:0x224CB2B8, # complete-object locator pointer
    0x224CB2C4:0x22522E10, # COL +12 -> type descriptor
}
TYPE_NAME_VA = 0x22522E18
TYPE_NAME = b'.?AVCFloatShortTermStat@@\x00'
# Distinct instruction sites; machine code itself stays exclusively private.
OPCODES = {
    0x222C94C8:'c6460400', 0x222C94CC:'c706e8e64522',
    0x222C94E0:'c70100000000',0x222C94E6:'c7410400000000',
    0x222C94ED:'c7410800000000',0x222C94FC:'c20400',
    0x222C8EEF:'8d0c8500000000',0x222C8EF7:'894604',
    0x222C8F21:'8b4604',0x222C8F27:'894608',
    0x222C64A0:'56',0x222C64B0:'8b4108',
    0x222C64B3:'8d440701',0x222C64B7:'33d2',
    0x222C64B9:'f7f6',0x222C64C0:'d90490',
    0x222C64C3:'d803',0x222C64C5:'d91b',
    0x222C64CA:'3bfe',0x222C64DC:'c20400',
    0x222C9050:'83ec08',0x222C9056:'8d442404',
    0x222C905E:'c744240800000000',0x222C906B:'db460c',
    0x222C907B:'d87c2404',0x222C9083:'c3',
    0x222C9500:'33c0',0x222C9502:'39410c',
    0x222C9513:'d9442404',0x222C9517:'d91c82',
    0x222C951D:'3b410c',0x222C9522:'c20400',
    0x222C9530:'8b4110',0x222C9533:'d9442404',
    0x222C953A:'d91c82',0x222C953D:'8b4110',
    0x222C9540:'85c0',0x222C954A:'894110',
    0x222C954D:'c6410401',0x222C955E:'c20400',
    0x222C9090:'51',0x222C9094:'8b06',
    0x222C9096:'ff5008',0x222C9099:'d95c2404',
    0x222C90B1:'c20400',
    0x222C8E40:'8a4104',0x222C8E43:'c3',
    0x222C8E50:'8a4104',0x222C8E53:'c6410400',
    0x222C8E57:'c3',
    0x222C96D2:'6a14',0x222C96E1:'8bc8',
}
CALLS = {
    0x222C94F4:0x222C8EE0, # constructor -> ring init
    0x222C9066:0x222C64A0, # reader -> ordered ring sum
    0x222C90AA:0x222C8EE0, # resize -> ring reinit
    0x222C96E3:0x222C94C0, # stat source factory -> float ctor
}

class OriginalBinaryMismatch(ValueError):
    pass

class PE32:
    def __init__(self, blob):
        self.blob=blob
        if len(blob)<0x100 or blob[:2]!=b'MZ':
            raise OriginalBinaryMismatch('invalid MZ header')
        pe=struct.unpack_from('<I',blob,0x3c)[0]
        if pe+24>len(blob) or blob[pe:pe+4]!=b'PE\0\0':
            raise OriginalBinaryMismatch('invalid PE signature')
        machine,n=struct.unpack_from('<HH',blob,pe+4)
        opt_size=struct.unpack_from('<H',blob,pe+20)[0]
        opt=pe+24
        if machine!=0x14c or opt_size<96 or opt+opt_size>len(blob):
            raise OriginalBinaryMismatch('not valid x86 PE32')
        if struct.unpack_from('<H',blob,opt)[0]!=0x10b:
            raise OriginalBinaryMismatch('not PE32 optional header')
        self.base=struct.unpack_from('<I',blob,opt+28)[0]
        self.sections=[]
        first=opt+opt_size
        if first+40*n>len(blob) or n>128:
            raise OriginalBinaryMismatch('section table outside image')
        for i in range(n):
            off=first+40*i
            vsize, rva, raw_size, raw_ptr = struct.unpack_from('<IIII',blob,off+8)
            if raw_ptr>len(blob) or raw_size>len(blob)-raw_ptr:
                raise OriginalBinaryMismatch('invalid section raw bounds')
            self.sections.append((self.base+rva,raw_ptr,raw_size))

    def at(self,va,length):
        for start,off,size in self.sections:
            displacement=va-start
            if displacement>=0 and displacement<=size and length<=size-displacement:
                return self.blob[off+displacement:off+displacement+length]
        raise OriginalBinaryMismatch('original VA not mapped: %#x' % va)

def verify(pe,check_hashes=True):
    if pe.base!=0x22000000:
        raise OriginalBinaryMismatch('wrong original server image base')
    for address,value in RTTI.items():
        if pe.at(address,4)!=struct.pack('<I',value):
            raise OriginalBinaryMismatch('CFloatShortTermStat RTTI pointer changed at %#x' % address)
    if pe.at(TYPE_NAME_VA,len(TYPE_NAME))!=TYPE_NAME:
        raise OriginalBinaryMismatch('CFloatShortTermStat class name changed')
    for i,target in enumerate(VTABLE_TARGETS):
        if pe.at(VTABLE_START+i*4,4)!=struct.pack('<I',target):
            raise OriginalBinaryMismatch('CFloatShortTermStat virtual slot %s changed' % i)
    for address,text in OPCODES.items():
        wanted=bytes.fromhex(text)
        if pe.at(address,len(wanted))!=wanted:
            raise OriginalBinaryMismatch('original x86 opcode mismatch at %#x' % address)
    for site,target in CALLS.items():
        expected=b'\xe8'+struct.pack('<i',target-site-5)
        if pe.at(site,5)!=expected:
            raise OriginalBinaryMismatch('original call mismatch at %#x' % site)
    if check_hashes:
        for name,(va,count,expected) in REGIONS.items():
            if hashlib.sha256(pe.at(va,count)).hexdigest()!=expected:
                raise OriginalBinaryMismatch('full SHA256 mismatch for '+name)
    return {
        'original_class':'CFloatShortTermStat',
        'rtti_name_va':hex(TYPE_NAME_VA),
        'vtable_va':hex(VTABLE_START),
        'virtual_slots':len(VTABLE_TARGETS),
        'validated_functions':len(REGIONS) if check_hashes else 0,
        'opcode_witnesses':len(OPCODES),
        'direct_calls':len(CALLS),
        'methods':{'slot2_mean':'0x222C9050','slot3_fill':'0x222C9500',
                   'slot4_resize':'0x222C9090','slot9_append':'0x222C9530',
                   'ring_sum':'0x222C64A0','ring_initialize':'0x222C8EE0'},
        'instance_offsets':{'modified_byte':'+0x04','buffer_pointer':'+0x08',
                            'length':'+0x0C','descending_cursor':'+0x10'},
        'scope':'Stat source behavior only; not original engine/ABI or full game DLL',
    }

def audit(client_bytes,server_bytes):
    for role,blob in (('client',client_bytes),('server',server_bytes)):
        if hashlib.sha256(blob).hexdigest()!=SHA256[role]:
            raise OriginalBinaryMismatch('SHA256 mismatch for original '+role+'.dll')
    result=verify(PE32(server_bytes))
    result['original_dll_sha256']=dict(SHA256)
    return result

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--client',type=Path,required=True)
    parser.add_argument('--server',type=Path,required=True)
    parser.add_argument('--output',type=Path)
    args=parser.parse_args()
    result=json.dumps(audit(args.client.read_bytes(),args.server.read_bytes()),indent=2)+'\n'
    if args.output:
        args.output.parent.mkdir(parents=True,exist_ok=True)
        args.output.write_text(result,encoding='utf-8')
    else:
        print(result,end='')

if __name__=='__main__':main()
