#!/usr/bin/env python3
"""Owner-private proof: original 2006 SiN comparator + initialization.

Both retail DLLs required for a real audit; --self-test mutates invented bytes.
Never commit/upload either original proprietary game DLL.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
from pathlib import Path
from audit_original_symbol_slot_allocation_phase639 import (
    PEImage, VerificationError, SERVER_SHA256, CLIENT_SHA256,
    check_full,check_edge,check_pin)

METHODS=(
 ('symbol_word_unsigned_lt_comparator',0x22280b50,19,
  'b0521eacdb9687665838d9a4df7374f3b2665f842939aea4d2d4c550dc97d413'),
 ('symbol_table_direct_constructor',0x22282130,63,
  '93ae1a19f1ec9f63c8a36dd04b08c748d163715e5ab71639a7669c6108fa2d6f'),
 ('symbol_table_heap_factory',0x22282210,90,
  '75ce836b6fffec4fd84da9ccd39ff82187bc3a767bade60c10fd610d2efe22aa'),
 ('table_factory_original_allocate_wrapper',0x2226e790,79,
  '1ba57fe89ebae2110c154315e1371c8481cc0bd1cf8363a4c83689219ff9714f'),
)
EDGES=(
 (0x2226e7a3,0x2226e290,'factory_allocate_tag_discovery'),
 (0x22282166,0x2228f360,'direct_constructor_nested_initialization'),
 (0x22282216,0x2226e790,'factory_0x50_bytes_allocate'),
 (0x22282257,0x2228f360,'factory_nested_initialization'),
 (0x2228228b,0x22280fb0,'registrar_original_tree_enumerator'),
)
BRANCHES=(
 (0x2226e79b,0x2226e7af,'75','allocator_already_cached'),
 (0x2226e7ad,0x2226e7b4,'eb','allocator_cache_join'),
 (0x2226e7b6,0x2226e7ca,'75','allocator_tagged_path'),
 (0x22282224,0x22282263,'74','factory_alloc_returned_null'),
)
POINTER_WRITES=(
 (0x22282135,0x22280b50,'direct_table_comparator_assignment'),
 (0x22282226,0x22280b50,'factory_table_comparator_assignment'),
)
PIN_TEXT='''
22280b50:8b442404 22280b54:668b08 22280b57:8b542408
22280b5b:663b0a 22280b5e:1bc0 22280b60:f7d8 22280b62:c3
22282130:33c0 22282132:56 22282133:8bf1
22282135:c706500b2822 2228213b:894604 2228213e:894608
22282141:89460c 22282144:b9ffff0000 22282149:66894e10
2228214d:66894e14 22282151:8b4e04 22282154:6a01
22282156:50 22282157:894e18 2228215a:50
2228215b:8d4e1c 2228215e:66894612 22282162:66894616
22282166:e8f5d10000 2228216b:8bc6 2228216d:5e 2228216e:c3
22282210:56 22282211:57 22282212:6a50 22282214:8bf9
22282216:e875c5feff 2228221b:8bf0 2228221d:33c0
2228221f:83c404 22282222:3bf0 22282224:743d
22282226:c706500b2822 2228222c:894604 2228222f:894608
22282232:89460c 22282235:b9ffff0000
2228223a:66894e10 2228223e:66894e14 22282242:8b4e04
22282245:6a01 22282247:50 22282248:894e18 2228224b:50
2228224c:8d4e1c 2228224f:66894612 22282253:66894616
22282257:e804d10000 2228225c:8937 2228225e:8bc7
22282260:5f 22282261:5e 22282262:c3
22282263:8907 22282265:8bc7 22282267:5f 22282268:5e
22282269:c3
2226e790:b801000000 2226e795:8405e0855822
2226e79b:7512 2226e79d:0905e0855822 2226e7a3:e8e8faffff
2226e7a8:a3dc855822 2226e7ad:eb05 2226e7af:a1dc855822
2226e7b4:85c0 2226e7b6:7512 2226e7b8:a124213d22
2226e7bd:8b08 2226e7bf:8b442404 2226e7c3:8b11
2226e7c5:50 2226e7c6:ff5204 2226e7c9:c3
2226e7ca:8b0d24213d22 2226e7d0:8b09 2226e7d2:8b11
2226e7d4:6a00 2226e7d6:50 2226e7d7:8b44240c
2226e7db:50 2226e7dc:ff12 2226e7de:c3
22280ffa:ff5500 22281000:84c0
'''
PINS=tuple((int(piece.split(':')[0],16),piece.split(':')[1])
           for piece in PIN_TEXT.split())

def check_conditional(read,site,target,opcode):
    prefix=bytes.fromhex(opcode)
    raw=read(site,len(prefix)+1)
    if len(raw)!=len(prefix)+1 or raw[:len(prefix)]!=prefix:
        raise VerificationError(f'wrong branch instruction {site:#x}')
    if site+len(raw)+struct.unpack_from('<b',raw,len(prefix))[0]!=target:
        raise VerificationError(f'changed native branch target {site:#x}')

def check_constructor_pointer(read,site,func):
    raw=read(site,6)
    if raw[:2]!=b'\xc7\x06' or len(raw)!=6 or (
            struct.unpack_from('<I',raw,2)[0]!=func):
        raise VerificationError(f'changed original comparator function pointer {site:#x}')

def authenticate(server:bytes,client:bytes)->dict:
    if hashlib.sha256(server).hexdigest()!=SERVER_SHA256 or (
            hashlib.sha256(client).hexdigest()!=CLIENT_SHA256):
        raise VerificationError('original complete 2006 server/client DLL SHA256 mismatch')
    image=PEImage(server)
    if image.base!=0x22000000:raise VerificationError('wrong PE32 server image base')
    for name,va,length,digest in METHODS:
        if image.section(va)!='.text':raise VerificationError(f'{name} not original .text')
        check_full(image.at,va,length,digest)
        if image.at(va+length,1)!=b'\xcc':
            raise VerificationError(f'{name} original INT3 boundary missing')
    for site,target,_ in EDGES:
        if image.section(site)!='.text' or image.section(target)!='.text':
            raise VerificationError('native call site/target not in .text')
        check_edge(image.at,site,target)
    for site,target,op,_ in BRANCHES:check_conditional(image.at,site,target,op)
    for site,target,_ in POINTER_WRITES:check_constructor_pointer(image.at,site,target)
    for va,opcode in PINS:check_pin(image.at,va,opcode)
    return {
      'status':'PHASE646_ORIGINAL_2006_TABLE_COMPARATOR_PROVEN',
      'retail_server_sha256':SERVER_SHA256,
      'retail_client_sha256':CLIENT_SHA256,
      'whole_method_provenance':[
        {'name':name,'va':hex(va),'bytes':length,'sha256':digest}
        for name,va,length,digest in METHODS],
      'total_authenticated_complete_original_bytes':sum(x[2] for x in METHODS),
      'original_direct_rel32_edges':len(EDGES),
      'original_conditional_branches':len(BRANCHES),
      'verified_constructor_pointer_assignments':len(POINTER_WRITES),
      'address_pinned_instruction_witnesses':len(PINS),
      'original_comparator_va':'0x22280b50',
      'original_unsigned_key_width_bits':16,
      'native_unsigned_less_than_result_eax':1,
      'native_equal_or_greater_result_eax':0,
      'factory_object_allocation_bytes':80,
      'original_nested_map_initializer_va':'0x2228f360',
      'nested_initializer_decompiled':False,
      'original_engine_execution':False,
      'android_quest_gameplay':False,
    }

def self_test():
    for _,va,n,_ in METHODS:
        invented=bytes((i*13+n+71)&255 for i in range(n))
        digest=hashlib.sha256(invented).hexdigest()
        check_full(lambda _site,size:invented[:size],va,n,digest)
        for i in range(n):
            changed=bytearray(invented);changed[i]^=1
            try:check_full(lambda _site,size:bytes(changed[:size]),va,n,digest)
            except VerificationError:pass
            else:raise AssertionError('mutated complete original-free method accepted')
    site,target=0x71000000,0x71001000
    code=b'\xe8'+struct.pack('<i',target-site-5)
    check_edge(lambda _site,n:code[:n],site,target)
    for i in range(len(code)):
        changed=bytearray(code);changed[i]^=1
        try:check_edge(lambda _site,n:bytes(changed[:n]),site,target)
        except VerificationError:pass
        else:raise AssertionError('mutated original-free direct call accepted')
    for op in ('74','75','eb'):
        code=bytes.fromhex(op)+struct.pack('<b',7)
        check_conditional(lambda _site,n:code[:n],site,site+len(code)+7,op)
        for i in range(len(code)):
            changed=bytearray(code);changed[i]^=1
            try:check_conditional(lambda _site,n:bytes(changed[:n]),site,
                                  site+len(code)+7,op)
            except VerificationError:pass
            else:raise AssertionError('mutated native branch accepted')
    original=b'\xc7\x06'+struct.pack('<I',0x22280b50)
    check_constructor_pointer(lambda _site,n:original[:n],site,0x22280b50)
    for i in range(len(original)):
        bad=bytearray(original);bad[i]^=1
        try:check_constructor_pointer(lambda _site,n:bytes(bad[:n]),site,0x22280b50)
        except VerificationError:pass
        else:raise AssertionError('mutated original comparator field assignment accepted')
    for opcode in ('663b0a','1bc0','f7d8'):
        check_pin(lambda _site,n:bytes.fromhex(opcode)[:n],site,opcode)
    try:authenticate(b'wrong game',b'wrong game')
    except VerificationError:pass
    else:raise AssertionError('unrelated DLL accepted')
    print('PASS Phase646 invented 251-byte complete-method hashes, comparator install, branches, original retail identity refusal')

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--server',type=Path)
    p.add_argument('--client',type=Path)
    p.add_argument('--self-test',action='store_true')
    p.add_argument('--output',type=Path)
    a=p.parse_args()
    if a.self_test:self_test();return
    if not a.server or not a.client:p.error('specify both original --server and --client, or --self-test')
    try:report=authenticate(a.server.read_bytes(),a.client.read_bytes())
    except (OSError,VerificationError,ValueError,struct.error) as exc:
        p.exit(1,'PHASE646 OWNER RETAIL AUDIT REFUSED: '+str(exc)+'\n')
    serialized=json.dumps(report,indent=2)+'\n'
    if a.output:a.output.write_text(serialized,encoding='utf-8')
    else:print(serialized,end='')
if __name__=='__main__':main()
