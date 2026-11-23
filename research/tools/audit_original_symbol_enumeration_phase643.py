#!/usr/bin/env python3
"""Original 2006 SiN server.dll complete 255-byte symbol enumeration audit.

Owner-original server.dll+client.dll are required for authenticated verification.
Public --self-test checks only invented bytes and never leaks retail DLL data.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
from pathlib import Path
from audit_original_symbol_slot_allocation_phase639 import (
    PEImage, VerificationError, SERVER_SHA256, CLIENT_SHA256,
    check_full, check_edge, check_pin)

METHOD_NAME='original_symbol_tree_parent_and_side_enumeration'
METHOD_VA=0x22280fb0
METHOD_BYTES=255
METHOD_SHA256='3dcd2d8a66908958c0df0ccb5be7cdc3130a06b526ec4df58b9ee18f52e0560f'
CALLS=(
    (0x2228228b,0x22280fb0,'original_registrar_uses_search'),
)
# Eleven source-exact conditional/unconditional branches in the entire 255B method.
EDGES=(
    (0x22280fcf,0x222810a9,'0f84','empty_tree_skip_loop'),
    (0x22280fd6,0x22280fe0,'eb','start_search_loop'),
    (0x22281002,0x22281054,'74','al_zero_walk_right'),
    (0x22281012,0x2228103a,'75','left_sentinel_initialized'),
    (0x2228103d,0x2228104a,'74','left_sentinel_fallback'),
    (0x22281048,0x2228109f,'eb','left_walk_join'),
    (0x22281052,0x2228109f,'eb','left_sentinel_join'),
    (0x22281062,0x2228108a,'75','right_sentinel_initialized'),
    (0x2228108d,0x22281096,'74','right_sentinel_fallback'),
    (0x22281094,0x2228109b,'eb','right_walk_join'),
    (0x222810a2,0x22280fe0,'0f85','iterate_until_sentinel'),
)
# Exact native x86 instruction witnesses, keyed by original retail VA.
# The full SHA256 still guards every original method byte not pinned here.
PIN_TEXT='''
22280fb0:8b442408 22280fb8:8b4c2414 22280fbd:668b7d10
22280fc1:bbffff0000 22280fc9:668918 22280fcc:c60100
22280fcf:0f84d4000000 22280fd5:56 22280fd6:eb08
22280fe0:8b542418 22280fe4:8b4c2414 22280fe8:0fb7f7
22280feb:66893a 22280fee:8b4504 22280ff1:c1e604
22280ff4:8d440608 22280ff8:50 22280ff9:51
22280ffa:ff5500 22280ffd:83c408 22281000:84c0
22281002:7450 22281004:8b54241c 22281008:c60201
2228100b:f6054091582201 22281012:7526 22281019:090540915822
2228101f:66891d38915822 22281026:66891d3a915822
2228102d:66891d3c915822 22281034:66a33e915822
2228103d:740b 2228103f:8b4504 22281042:668b3c30
22281048:eb55 22281054:8b44241c 22281058:c60000
2228105b:f6054091582201 22281062:7526 22281069:090540915822
2228106f:66891d38915822 22281076:66891d3a915822
2228107d:66891d3c915822 22281084:66a33e915822
2228108d:7407 2228108f:8b4504 22281092:03c6
2228109b:668b7802 2228109f:663bfb 222810a2:0f8538ffffff
222810ac:c21000
'''
PINS=tuple((int(part.split(':',1)[0],16),part.split(':',1)[1])
           for part in PIN_TEXT.split())
assert len(PINS)==51

def check_branch(read,site,target,opcode_hex):
    opcode=bytes.fromhex(opcode_hex)
    size=len(opcode)+(4 if opcode[0]==0x0f else 1)
    data=read(site,size)
    if len(data)!=size or not data.startswith(opcode):
        raise VerificationError(f'original branch opcode changed at {site:#x}')
    if size-len(opcode)==4:
        relative=struct.unpack_from('<i',data,len(opcode))[0]
    else:
        relative=struct.unpack_from('<b',data,len(opcode))[0]
    if site+size+relative!=target:
        raise VerificationError(f'original branch target changed at {site:#x}')

def authenticate(server:bytes,client:bytes)->dict:
    if (hashlib.sha256(server).hexdigest()!=SERVER_SHA256 or
        hashlib.sha256(client).hexdigest()!=CLIENT_SHA256):
        raise VerificationError('both original 2006 whole-DLL SHA256s required')
    image=PEImage(server)
    if image.base!=0x22000000 or image.section(METHOD_VA)!='.text':
        raise VerificationError('unexpected PE32 image base or executable section')
    check_full(image.at,METHOD_VA,METHOD_BYTES,METHOD_SHA256)
    if image.at(METHOD_VA+METHOD_BYTES,1)!=b'\xcc':
        raise VerificationError('expected INT3 after complete original method')
    for src,dst,_ in CALLS:
        if image.section(src)!='.text' or image.section(dst)!='.text':
            raise VerificationError('registration call outside original .text')
        check_edge(image.at,src,dst)
    for src,dst,opcode,_ in EDGES:
        check_branch(image.at,src,dst,opcode)
    for va,hexbytes in PINS:
        check_pin(image.at,va,hexbytes)
    return {
        'status':'PHASE643_RETAIL_ORIGINAL_SYMBOL_ENUMERATION_VERIFIED',
        'server_sha256':SERVER_SHA256,'client_sha256':CLIENT_SHA256,
        'method_va':hex(METHOD_VA),'method_name':METHOD_NAME,
        'complete_original_method_bytes':METHOD_BYTES,
        'complete_original_method_sha256':METHOD_SHA256,
        'original_external_calls':len(CALLS),
        'original_control_flow_edges':len(EDGES),
        'address_pinned_instruction_witnesses':len(PINS),
        'original_comparator_indirect_call_va':'0x22280ffa',
        'caller_parent_write_width_bits':16,
        'caller_side_write_width_bits':8,
        'record_key_offset':8,
        'slot_stride':16,
        'original_sentinel_flag_va':'0x22589140',
        'original_sentinel_node_va':'0x22589138',
        'actual_original_gameplay_executed':False,
        'original_comparator_internals_recovered':False,
    }

def self_test():
    invented=bytes(((i*19+7)&255) for i in range(METHOD_BYTES))
    sha=hashlib.sha256(invented).hexdigest()
    check_full(lambda _va,n:invented[:n],0x71000000,METHOD_BYTES,sha)
    for i in range(METHOD_BYTES):
        altered=bytearray(invented);altered[i]^=1
        try:check_full(lambda _va,n:bytes(altered[:n]),0x71000000,METHOD_BYTES,sha)
        except VerificationError:pass
        else:raise AssertionError('changed complete native method byte accepted')
    site,target=0x71000000,0x71001000
    call=b'\xe8'+struct.pack('<i',target-site-5)
    check_edge(lambda _va,n:call[:n],site,target)
    for i in range(len(call)):
        changed=bytearray(call);changed[i]^=1
        try:check_edge(lambda _va,n:bytes(changed[:n]),site,target)
        except VerificationError:pass
        else:raise AssertionError('changed direct call edge accepted')
    for opcode in ('0f84','0f85','75','eb'):
        op=bytes.fromhex(opcode)
        size=len(op)+(4 if opcode.startswith('0f') else 1)
        to=site+size+10 if size-len(op)==1 else target
        code=op+(struct.pack('<b',10) if size-len(op)==1
                 else struct.pack('<i',to-site-size))
        check_branch(lambda _va,n:code[:n],site,to,opcode)
        for i in range(len(code)):
            bad=bytearray(code);bad[i]^=1
            try:check_branch(lambda _va,n:bytes(bad[:n]),site,to,opcode)
            except VerificationError:pass
            else:raise AssertionError('changed conditional branch accepted')
    check_pin(lambda _va,n:b'\x66\x89\x18'[:n],site,'668918')
    try:check_pin(lambda _va,n:b'\x89\x18'[:n],site,'668918')
    except VerificationError:pass
    else:raise AssertionError('accepted DWORD instead of WORD parent store')
    check_pin(lambda _va,n:b'\xc6\x01\x00'[:n],site,'c60100')
    try:check_pin(lambda _va,n:b'\xc7\x01\x00'[:n],site,'c60100')
    except VerificationError:pass
    else:raise AssertionError('accepted wrong BYTE side store')
    try:authenticate(b'unrelated',b'not sin')
    except VerificationError:pass
    else:raise AssertionError('unrelated DLL accepted')
    print('PASS Phase643 invented SHA256/rel32/branch/16-bit parent/8-bit side mutations and retail identity refusal')

def main():
    a=argparse.ArgumentParser(description=__doc__)
    a.add_argument('--server',type=Path)
    a.add_argument('--client',type=Path)
    a.add_argument('--output',type=Path)
    a.add_argument('--self-test',action='store_true')
    options=a.parse_args()
    if options.self_test:self_test();return
    if not options.server or not options.client:
        a.error('provide original --server and --client, or --self-test')
    try:report=authenticate(options.server.read_bytes(),options.client.read_bytes())
    except (OSError,VerificationError,ValueError,struct.error) as err:
        a.exit(1,'PHASE643 ORIGINAL RETAIL AUDIT REFUSED: '+str(err)+'\n')
    content=json.dumps(report,indent=2)+'\n'
    if options.output:options.output.write_text(content,encoding='utf-8')
    else:print(content,end='')
if __name__=='__main__':main()
