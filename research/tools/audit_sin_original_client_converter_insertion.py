#!/usr/bin/env python3
"""Authenticate the original 2006 SiN client converter red-black tree insertion.

Owner-private original DLLs are SHA-gated and NEVER emitted or embedded in
this output. Complete-method hashes distinguish whole functions from the
0x2400B230 shared-tail entry block.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
import zipfile
from pathlib import Path
from audit_sin_original_client_init_interface_manifest import PE32, CLIENT_SHA, SERVER_SHA

# Exact original PE32 virtual addresses, exclusive lengths and full SHA256.
METHODS = (
    (0x2400AB30,  103, 'ebe740c4da64fa6d47de6a8b28ac58f1db8c8be9d587cc8f2bcce41b45fd6da4', 'acquire_node_index'),
    (0x2400ABA0,  230, '8b2efe3b45eec69c08f1d8f75a3b6c170313d0edde71925108401fb8f9f4be27', 'find_insertion_parent_and_side'),
    (0x2400B920,  106, '375dff84a7a4a8c27ebdd19eeda1425e080ada46ced58647a00b9bdba8bddd7c', 'grow_24_byte_node_array'),
    (0x2400B4B0, 1112, 'af5123bc8f0d2f047038c6a32f74d03d878790e701568c1ace27239fb4eb9090', 'red_black_insertion_fixup'),
    (0x2400BF00,  150, '3253db893036184ec1cda9a2b7e95e2effde85390047c069c3563ee5b6a1d2f6', 'is_smaller_child'),
    (0x2400BFA0,  152, '33a21a59958be43bfdd5fecd4b78b1e80896e054331d2f7cad62e09891f61bd3', 'is_larger_child'),
    (0x2400C120,  337, 'ea91794a6ef125d786b4cc71ffe94b683f974be4ee3740c1d782b75298aac37e', 'rotate_left'),
    (0x2400C280,  338, 'eee663603b4c43da589506e883ee9303863e87b75357e166156e28ebf877645e', 'rotate_right'),
)
# 0x2400B230 jumps to 0x2400B4B0; it is NOT a whole function here.
ENTRY_BLOCK=(0x2400B230, 89, 'aaa9b7489aaa150e325eee96ac4ed6cbdc6cae16a90a8eaf62b3d500ebe452ea')
PREVIOUS_ORIGINAL_INSERT=(0x24009F40,128,'c6a10d23962af7f91b8debedb98e7ae661cf044d54fe55d4dd812a58a945f41c')
# All direct x86 CALL rel32 edges within the 8 methods, plus the caller of
# the newly recovered routines. Indirect CALL [descriptor+0] is an opcode.
CALLS = (
 (0x24009F74,0x2400ABA0),(0x24009F83,0x2400AB30),
 (0x2400AB49,0x2400B920),(0x2400AB5C,0x2400AAE0),(0x2400AB88,0x2400B230),
 (0x2400B96A,0x2416F960),(0x2400B97F,0x2416F8F0),
 (0x2400B5D8,0x2400BF00),(0x2400B6AD,0x2400BFA0),
 (0x2400B6C2,0x2400C120),(0x2400B773,0x2400C280),
 (0x2400B811,0x2400BF00),(0x2400B826,0x2400C280),
 (0x2400B8D7,0x2400C120),
 (0x2400C1FC,0x2400AA90),(0x2400C213,0x2400BEB0),
 (0x2400C227,0x2400BF00),(0x2400C233,0x2400BEB0),
 (0x2400C243,0x2400BEB0),
 (0x2400C35C,0x2400AAE0),(0x2400C373,0x2400BEB0),
 (0x2400C387,0x2400BFA0),(0x2400C393,0x2400BEB0),
 (0x2400C3A4,0x2400BEB0),
)
TAIL_JUMPS=(0x2400B26B,0x2400B278,0x2400B284)
# Explicit instruction witnesses for layout, flow and numeric constants.
ANCHORS=(
 (0x2400AB30,'8b471883f8ff','free-node head at +0x18 and -1 check'),
 (0x2400AB39,'8b47083b471c','allocated node count compared with limit'),
 (0x2400AB51,'8d4e01894f1c','increment high-watermark +0x1C'),
 (0x2400AB67,'8d14768d44d010','24-byte stride and node token +0x10'),
 (0x2400AB72,'66c700ffff','fresh/reused token sentinel 0xffff'),
 (0x2400AB8D,'83471401','increment active node count +0x14'),
 (0x2400ABB2,'83cbff3bf38918c60100','parent=-1 and side=0 output on entry'),
 (0x2400ABDF,'ff5500','indirect unsigned-token comparator'),
 (0x2400ABE2,'83c40884c07449','compare result controls which child'),
 (0x2400AC83,'c21000','insertion slot search pops 4 arguments'),
 (0x2400B920,'8b4e0883f9ff7461','growth mode +0x08 and reserve guard'),
 (0x2400B93C,'03c1894604','capacity += increment'),
 (0x2400B943,'03c0894604','capacity doubles when increment zero'),
 (0x2400B94A,'c7460402000000','initial array capacity is two'),
 (0x2400B230,'83faff','node parent -1 check'),
 (0x2400B23D,'895108c74104ffffffffc701ffffffff','node parent+8, children +0,+4 initialized to -1'),
 (0x2400B247,'c701ffffffff','smaller-child -1'),
 (0x2400B24D,'c7410c00000000','new node color code zero'),
 (0x2400B264,'8934d1','parent smaller-child is new index'),
 (0x2400B270,'8974d104','parent larger-child is new index'),
 (0x2400B27D,'897010','root index updated on empty tree'),
 (0x2400B553,'83780c00','parent color branch'),
 (0x2400B6C2,'e8590a0000','left rotate in fixup'),
 (0x2400B773,'e8080b0000','right rotate in fixup'),
 (0x2400B900,'895cc10c','force root color to one'),
 (0x2400BF7D,'33c939300f94c18ac1','is smaller child by comparing parent +0'),
 (0x2400C01D,'33c93970040f94c1','is larger child by comparing parent +4'),
 (0x2400C1B6,'894c2a04','rotate-left rewires right child'),
 (0x2400C316,'890c2a','rotate-right rewires left child'),
)
class AuditError(ValueError):
    pass

def need(ok,message):
    if not ok:
        raise AuditError(message)

def check_rel32(read,at,target,opcode=b'\xe8'):
    code=read(at,5)
    need(len(code)==5 and code[:1]==opcode, 'expected direct call/jmp at '+hex(at))
    actual=at+5+struct.unpack_from('<i',code,1)[0]
    need(actual==target, 'original direct edge changed: '+hex(at))

def verify(server,client):
    need(hashlib.sha256(server).hexdigest()==SERVER_SHA, 'not original server.dll')
    need(hashlib.sha256(client).hexdigest()==CLIENT_SHA, 'not original client.dll')
    pe=PE32(client)
    need(pe.base==0x24000000, 'unrecognized original client base')
    for va,size,sha,role in METHODS:
        need(pe.section(va)=='.text','method outside .text: '+role)
        need(hashlib.sha256(pe.at(va,size)).hexdigest()==sha, 'complete method changed: '+role)
        need(pe.at(va+size,1)==b'\xcc','original function boundary changed: '+role)
    va,size,sha=ENTRY_BLOCK
    need(hashlib.sha256(pe.at(va,size)).hexdigest()==sha,'insertion entry block changed')
    need(pe.at(va+size,1)==b'\xcc','insertion entry block boundary changed')
    va,size,sha=PREVIOUS_ORIGINAL_INSERT
    need(hashlib.sha256(pe.at(va,size)).hexdigest()==sha,'Phase542 caller changed')
    for site,target in CALLS:
        check_rel32(pe.at,site,target)
    for site in TAIL_JUMPS:
        check_rel32(pe.at,site,0x2400B4B0,opcode=b'\xe9')
    for at,pattern,reason in ANCHORS:
        b=bytes.fromhex(pattern)
        need(pe.at(at,len(b))==b,'opcode witness failed: '+reason)
    return {
        'status':'ORIGINAL_CLIENT_CONVERTER_REDBLACK_INSERTION_AUTHENTICATED',
        'source_sha256':CLIENT_SHA,
        'new_complete_methods':len(METHODS),
        'new_complete_method_bytes':sum(m[1] for m in METHODS),
        'separate_shared_tail_entry_block_bytes':ENTRY_BLOCK[1],
        'authenticated_direct_calls':len(CALLS),
        'entry_to_fixup_jumps':len(TAIL_JUMPS),
        'layout':{'record_stride':24,'smaller_child_offset':0,'larger_child_offset':4,
                  'parent_offset':8,'color_offset':12,'token_offset':16,
                  'converter_offset':20,'root_index_offset':16},
        'color':{'new_node':0,'root_on_completion':1},
        'rotation_methods':['0x2400C120','0x2400C280'],
        'limitations':'Original source names unavailable. No actual commercial DLL disassembly or code emitted. No native port boot.'
    }

def self_test():
    assert len(METHODS)==8 and sum(x[1] for x in METHODS)==2528
    assert len(CALLS)==24 and len(TAIL_JUMPS)==3
    at,target=0x24000880,0x24006120
    for opcode in [b'\xe8',b'\xe9']:
        valid=opcode+struct.pack('<i',target-at-5)
        check_rel32(lambda a,n:valid[:n],at,target,opcode)
        for i in range(5):
            corrupt=bytearray(valid); corrupt[i]^=1
            try:check_rel32(lambda a,n:bytes(corrupt[:n]),at,target,opcode)
            except AuditError:pass
            else:raise AssertionError('mutation accepted at byte '+str(i))
    try:verify(b'not original',b'not original')
    except AuditError:pass
    else:raise AssertionError('rejected sha gate did not reject')
    print('PASS 8 method metadata, 24 calls, 3 fixup jumps, 10 synthetic corruptions, sha refusal')

def main():
    a=argparse.ArgumentParser(description=__doc__)
    a.add_argument('--self-test',action='store_true')
    a.add_argument('--bin-zip',type=Path)
    opts=a.parse_args()
    if not opts.self_test and not opts.bin_zip:a.error('use --self-test or --bin-zip')
    if opts.self_test:self_test()
    if opts.bin_zip:
        with zipfile.ZipFile(opts.bin_zip) as z:
            names={Path(n).name.lower():n for n in z.namelist()}
            need('client.dll' in names and 'server.dll' in names,'missing original pair')
            result=verify(z.read(names['server.dll']),z.read(names['client.dll']))
        print(json.dumps(result,indent=2))

if __name__=='__main__':main()