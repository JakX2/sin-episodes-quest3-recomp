#!/usr/bin/env python3
"""Phase181: verify owner-original 2006 client.dll surface-props manifest.

SHA-pins complete x86 functions and sparse behavioral witnesses; chains prior
Phase180 original client/server full-file and Init verification. The original
executables, full instruction bodies, and proprietary assets are not copied.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
from pathlib import Path
from audit_sin_original_client_postinit import audit as phase180_audit
from audit_sin_original_dll_interface_abi import PEImage, VerificationError

CLIENT_SHA='6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d'
SERVER_SHA='568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650'
REGIONS={
 'surface_manifest_loader':(0x241927A0,247,'7357bdfb9934f175e0d67f4a81236d81087a07bb6145918083c44fd255ca6905'),
 'kv_first_child':(0x24164B90,4,'0b78473b3a56c63ff2a64431fb600078066cbe407d3dee7ea8fd7575bd7f82d5'),
 'kv_next_peer':(0x24164BA0,4,'38058506e8e859d5be73a33baa621352c08ea01f2224ee170173253757193941'),
 'kv_find_child_by_symbol':(0x24164B20,32,'5264003f4859e6bc4704ba42c9ba251e689725135f8d3c4c6b4a4b3815ee90da'),
 'kv_name_vtable_bridge':(0x24164770,21,'e6fedc91b795bc4c5e8120f9744bfc75b5ca12785e94bc50411881423cf5710f'),
 'kv_allocator_bridge':(0x24164C60,19,'33a5a0dbfb95c16aeb48f7a64b929b8ccf3742e823518c3b01e20a076ac6dd11'),
 'kv_release':(0x24166050,28,'7907cfaa8ae3f25951a87879d7050db0d8491e6b835008976883899a004814f2'),
}
# Disjoint native opcode fragments only; never publish whole function bytes.
OPCODES={
 0x241927A0:'51',0x241927A4:'6a28',0x241927AB:'83c404',
 0x241927AE:'85c0',0x241927B0:'7416',0x241927B2:'8b0d8cd12f24',
 0x241927C2:'897c240c',0x241927C8:'c744240c00000000',
 0x241927D4:'8b158cd12f24',0x241927DE:'68583b2924',
 0x241927EC:'84c0',0x241927EE:'0f8481000000',
 0x241927FB:'8bf0',0x241927FD:'85f6',
 0x24192805:'8b7c2414',0x2419280A:'8b2de0612724',
 0x24192817:'68003d2924',0x24192825:'85c0',
 0x24192829:'7515',0x2419282B:'53',0x2419282D:'680e332d24',
 0x24192846:'a18cd12f24',0x2419284C:'68082d2a24',
 0x24192851:'ffd5',0x24192853:'83c40c',
 0x2419285F:'85f6',0x24192861:'75ad',
 0x24192863:'8b7c2410',0x24192868:'8bcf',
 0x24192875:'8b0d8cd12f24',0x2419287C:'68803c2924',
 0x24192881:'ff15fc612724',0x24192887:'83c408',
 0x24164B90:'8b411cc3',0x24164BA0:'8b4118c3',
 0x24164B20:'8b411c',0x24164B23:'85c0',
 0x24164B30:'3908',0x24164B34:'8b4018',
 0x24164B3B:'33c0',0x24164B3D:'c20400',
 0x24164770:'568bf1',0x24164773:'ff1530622724',
 0x24164779:'8b0e',0x24164780:'ff5210',
 0x24164C60:'ff1530622724',0x24164C6A:'8b10',
 0x24164C6F:'ff5204',0x24164C72:'c3',
 0x24166050:'568bf1',0x24166053:'85f6',
 0x24166055:'7413',0x2416605C:'ff1530622724',
 0x24166062:'8b10',0x24166067:'ff5208',0x2416606B:'c3'
}
DIRECT_CALLS={
 0x241927A6:0x24164C60,
 0x241927BB:0x24165010,
 0x241927E7:0x24164A50,
 0x241927F6:0x24164B90,
 0x24192812:0x24164770,
 0x2419281D:0x2425F81B,
 0x24192833:0x241667C0,
 0x24192839:0x24191EC0,
 0x24192840:0x24164770,
 0x24192858:0x24164BA0,
 0x24166057:0x24166620,
}
TAIL_JMPS={0x24192870:0x24166050, 0x24192892:0x24166050}
INCOMING_CALLS={0x24182CB1:0x241927A0}
STRINGS={
 0x242A2C5C:b'scripts/surfaceproperties_manifest.txt\0',
 0x24293B58:b'GAME\0',
 0x24293D00:b'file\0',
 0x24293C80:b"Unable to load manifest file '%s'\n\0",
 0x242A2D08:b"surfaceprops::Init:  Manifest '%s' with bogus file type '%s', expecting 'file'\n\0",
}


def verify_native(pe:PEImage, *,check_hash=True)->dict:
    if pe.base!=0x24000000:raise VerificationError('unexpected original client image base')
    for va,rawhex in OPCODES.items():
        raw=bytes.fromhex(rawhex)
        if pe.at(va,len(raw))!=raw:
            raise VerificationError('Phase181 original client x86 opcode mismatch at %#x'%va)
    for src,target in {**DIRECT_CALLS,**INCOMING_CALLS}.items():
        expected=b'\xe8'+struct.pack('<i',target-src-5)
        if pe.at(src,5)!=expected:
            raise VerificationError('Phase181 direct CALL relationship mismatch at %#x'%src)
    for src,target in TAIL_JMPS.items():
        expected=b'\xe9'+struct.pack('<i',target-src-5)
        if pe.at(src,5)!=expected:
            raise VerificationError('Phase181 tail JMP relationship mismatch at %#x'%src)
    for va,string in STRINGS.items():
        if pe.section_of(va)!='.rdata' or pe.at(va,len(string))!=string:
            raise VerificationError('Phase181 exact original surface manifest string mismatch at %#x'%va)
    # Compiler data field contains native pointer to the in-module manifest path.
    if pe.at(0x242FD18C,4)!=struct.pack('<I',0x242A2C5C):
        raise VerificationError('original surface manifest filename pointer changed')
    if check_hash:
        for name,(va,n,digest) in REGIONS.items():
            if hashlib.sha256(pe.at(va,n)).hexdigest()!=digest:
                raise VerificationError('Phase181 complete original native body digest mismatch: '+name)
    return {
      'complete_original_function_hashes':len(REGIONS) if check_hash else 0,
      'native_bytes_covered_by_region_hashes':sum(n for _,n,_ in REGIONS.values()) if check_hash else 0,
      'sparse_native_opcode_witnesses':len(OPCODES),
      'direct_original_call_edges':len(DIRECT_CALLS),
      'original_incoming_call_edges':len(INCOMING_CALLS),
      'native_tail_jump_edges':len(TAIL_JMPS),
      'original_exact_strings':len(STRINGS),
      'surface_manifest':'scripts/surfaceproperties_manifest.txt',
      'manifest_search_path':'GAME',
      'manifest_accepted_type':'file',
      'native_functions':{name:{'va':hex(va),'bytes':n} for name,(va,n,_) in REGIONS.items()},
      'limits':['No complete native engine allocations, KeyValues parsing, or physics surface file loading',
                'Host-neutral model adds invalid pointer/cycle guards absent from native machine code',
                'No original Windows game DLL startup or level load demonstrated'],
    }


def audit(client_bytes:bytes,server_bytes:bytes)->dict:
    if hashlib.sha256(client_bytes).hexdigest()!=CLIENT_SHA or hashlib.sha256(server_bytes).hexdigest()!=SERVER_SHA:
        raise VerificationError('original full client/server SHA256 mismatch')
    result=phase180_audit(client_bytes,server_bytes)
    result['original_client_surface_manifest']=verify_native(PEImage(client_bytes))
    return result


def main()->None:
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--client',type=Path,required=True)
    parser.add_argument('--server',type=Path,required=True)
    parser.add_argument('--output',type=Path)
    args=parser.parse_args()
    output=json.dumps(audit(args.client.read_bytes(),args.server.read_bytes()),indent=2)+'\n'
    if args.output:
        args.output.parent.mkdir(parents=True,exist_ok=True)
        args.output.write_text(output,encoding='utf-8')
    else: print(output,end='')
if __name__=='__main__':main()