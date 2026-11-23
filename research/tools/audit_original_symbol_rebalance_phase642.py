#!/usr/bin/env python3
"""Authenticates the complete original 2006 SiN symbol-tree rebalancer.

Owner's *original* server.dll and client.dll are REQUIRED for original audit.
The CI-friendly --self-test uses invented method bytes only. No retail assets
or proprietary executable instructions are included beyond sparse opcode pins.
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

METHOD_NAME="original_symbol_tree_red_black_insert_fixup"
METHOD_VA=0x22281840
METHOD_LENGTH=1482
METHOD_SHA256="4ef6465536a6a442a842531f09f0d8f0a37f274be6403d540528b24059527ede"
ROTATIONS=(
 (0x22281B47,0x22281180,"left_triangle_fixup"),
 (0x22281BFF,0x22281270,"right_straight_fixup"),
 (0x22281D23,0x22281270,"right_triangle_fixup"),
 (0x22281DDB,0x22281180,"left_straight_fixup"),
)
CONTROL_EDGES=(
 (0x22281851,0x22281DF6,"0f84","initial_root_short_circuit"),
 (0x222818EA,0x22281DF4,"0f85","parent_black_stop"),
 (0x222819EF,0x22281C09,"0f85","mirror_side_selection"),
 (0x22281A76,0x22281AB2,"75","left_uncle_black"),
 (0x22281AAD,0x22281DE0,"e9","left_uncle_red_repeat"),
 (0x22281B3A,0x22281BDC,"0f85","left_zigzag_or_straight"),
 (0x22281C04,0x22281DE0,"e9","left_rotation_join"),
 (0x22281C89,0x22281A78,"0f84","mirror_uncle_red_common_recolor"),
 (0x22281D16,0x22281DB8,"0f85","right_zigzag_or_straight"),
 (0x22281DEE,0x22281860,"0f85","red_parent_fixup_loop"),
)
PIN_BYTES="""
22281840:56
22281842:8b7c240c
22281848:663b7e10
22281851:0f849f050000
22281857:53
22281859:bbffff0000
22281864:a140915822
22281869:a801
2228186d:0bc2
2228186f:a340915822
22281874:66891d38915822
22281889:6689153e915822
22281890:663bfb
22281893:740b
22281895:0fb7cf
22281898:c1e104
2228189b:034e04
222818a0:b938915822
222818a7:668b4904
222818d0:663bcb
222818e5:6683790600
222818ea:0f8504050000
222818f0:a801
2228192c:a801
22281932:7523
2228196c:a801
2228196e:668b6904
22281997:663bfb
222819ac:a801
222819d7:663bcb
222819ec:663939
222819ef:0f8514020000
22281a1c:663beb
22281a31:a801
22281a5c:663bcb
22281a71:66837a0600
22281a76:753a
22281a78:0fb7d7
22281a7b:8b7e04
22281a81:b801000000
22281a86:6689443a06
22281a8b:8b5604
22281a8e:0fb7c9
22281a94:6689441106
22281a99:8b4e04
22281aa2:66c74408060000
22281aa9:896c2414
22281aad:e92e030000
22281ab2:a801
22281adb:8b542414
22281af4:a801
22281b21:663bcb
22281b36:66395002
22281b3a:0f859c000000
22281b40:57
22281b43:897c2418
22281b47:e834f6ffff
22281b4c:8b0d40915822
22281b93:f6c101
22281bd8:668b6804
22281bdc:8b4604
22281be5:0fb7cd
22281bef:8b5604
22281bf5:66c74411060000
22281bfc:55
22281bff:e86cf6ffff
22281c04:e9d7010000
22281c09:a801
22281c30:663beb
22281c45:a801
22281c6f:663bcb
22281c84:66837a0600
22281c89:0f84e9fdffff
22281cb8:8b542414
22281cfe:663bcb
22281d13:663910
22281d16:0f859c000000
22281d1c:57
22281d1f:897c2418
22281d23:e848f5ffff
22281d28:8b0d40915822
22281d6f:f6c101
22281db4:668b6804
22281db8:8b4604
22281dc4:66c74402060100
22281dd1:66c74411060000
22281dd8:55
22281ddb:e8a0f3ffff
22281de0:668b442414
22281de5:663b4610
22281dee:0f856cfaffff
22281df4:5d
22281df6:0fb74e10
22281dfa:8b4604
22281dfd:c1e104
22281e00:5f
22281e01:6689540106
22281e06:5e
22281e07:c20400
"""
PINS=tuple((int(pair.split(':')[0],16),pair.split(':')[1])
           for pair in PIN_BYTES.split())
assert len(PINS)==99

def verify_control_edge(read,site,target,opcode_hex):
    op=bytes.fromhex(opcode_hex)
    n=len(op)+ (1 if len(op)==1 and op[0] in (0x74,0x75) else 4)
    bits=read(site,n)
    if len(bits)!=n or not bits.startswith(op):
        raise VerificationError(f"wrong original conditional opcode at {site:#x}")
    if n-len(op)==1:
        rel=struct.unpack_from('<b',bits,len(op))[0]
    else:
        rel=struct.unpack_from('<i',bits,len(op))[0]
    if site+n+rel!=target:
        raise VerificationError(f"changed original control-flow target at {site:#x}")

def authenticate(server:bytes,client:bytes)->dict:
    if hashlib.sha256(server).hexdigest()!=SERVER_SHA256 or (
            hashlib.sha256(client).hexdigest()!=CLIENT_SHA256):
        raise VerificationError("whole original 2006 SiN server/client DLL identities mismatch")
    pe=PEImage(server)
    if pe.base!=0x22000000:
        raise VerificationError("unexpected PE32 original server image base")
    if pe.section(METHOD_VA)!=".text":
        raise VerificationError("original rebalancer is not in .text")
    check_full(pe.at,METHOD_VA,METHOD_LENGTH,METHOD_SHA256)
    if pe.at(METHOD_VA+METHOD_LENGTH,2)!=b'\xcc\xcc':
        raise VerificationError("rebalancer complete-method boundary changed")
    for site,target,_ in ROTATIONS:
        if pe.section(site)!=".text" or pe.section(target)!=".text":
            raise VerificationError("rotation edge outside original .text")
        check_edge(pe.at,site,target)
    for site,target,op,_ in CONTROL_EDGES:
        verify_control_edge(pe.at,site,target,op)
    for va,data in PINS:
        check_pin(pe.at,va,data)
    return {
        "status":"PHASE642_RETAIL_RED_BLACK_INSERT_FIXUP_VERIFIED",
        "retail_server_sha256":SERVER_SHA256,
        "retail_client_sha256":CLIENT_SHA256,
        "function_name":METHOD_NAME,
        "complete_original_method_va":hex(METHOD_VA),
        "complete_original_method_bytes":METHOD_LENGTH,
        "complete_original_method_sha256":METHOD_SHA256,
        "original_rotation_edges":[dict(source=hex(a),target=hex(b),role=c)
                                   for a,b,c in ROTATIONS],
        "verified_original_branch_edges":len(CONTROL_EDGES),
        "address_pinned_instruction_witnesses":len(PINS),
        "sentinel_init_global_va":"0x22589140",
        "sentinel_node_va":"0x22589138",
        "uses_16bit_color_fields":True,
        "uses_16bit_index_and_FFFF_sentinel":True,
        "standalone_game_engine_executed":False,
        "quest_gameplay_executed":False,
    }

def self_test():
    invented=bytes((i*29+31)&255 for i in range(METHOD_LENGTH))
    digest=hashlib.sha256(invented).hexdigest()
    check_full(lambda _va,n:invented[:n],0x70000000,METHOD_LENGTH,digest)
    for i in range(METHOD_LENGTH):
        changed=bytearray(invented);changed[i]^=1
        try:check_full(lambda _va,n:bytes(changed[:n]),0x70000000,METHOD_LENGTH,digest)
        except VerificationError:pass
        else:raise AssertionError("whole-method hash accepted mutated byte")
    site,target=0x70001000,0x70002000
    call=b'\xe8'+struct.pack('<i',target-site-5)
    check_edge(lambda _va,n:call[:n],site,target)
    for i in range(5):
        changed=bytearray(call);changed[i]^=1
        try:check_edge(lambda _va,n:bytes(changed[:n]),site,target)
        except VerificationError:pass
        else:raise AssertionError("rotation rel32 mutation accepted")
    for op in ('0f84','0f85','e9','75'):
        n=len(bytes.fromhex(op));rel_size=1 if op=='75' else 4
        pc=site+n+rel_size
        branch=bytes.fromhex(op)+(struct.pack('<b',8) if rel_size==1
                                   else struct.pack('<i',target-pc))
        expect=(pc+8 if rel_size==1 else target)
        verify_control_edge(lambda _va,m:branch[:m],site,expect,op)
        for i in range(len(branch)):
            changed=bytearray(branch);changed[i]^=1
            try:verify_control_edge(lambda _va,m:bytes(changed[:m]),site,expect,op)
            except VerificationError:pass
            else:raise AssertionError("branch opcode/relative displacement mutation accepted")
    check_pin(lambda _va,n:b'\x66\xc7\x44\x02\x06\x01\x00'[:n],site,
              '66c74402060100')
    try:check_pin(lambda _va,n:b'\xc7\x44\x02\x06\x01\x00'[:n],site,
                  '66c74402060100')
    except VerificationError:pass
    else:raise AssertionError("bad native 16bit color-store width accepted")
    try:authenticate(b'not original',b'not original')
    except VerificationError:pass
    else:raise AssertionError("unrelated DLL identities accepted")
    print('PASS Phase642 invented 1482-byte SHA, 4 rotation calls, jcc/jmp/loop, 16-bit opcode, DLL refusal')

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--server',type=Path)
    parser.add_argument('--client',type=Path)
    parser.add_argument('--output',type=Path)
    parser.add_argument('--self-test',action='store_true')
    args=parser.parse_args()
    if args.self_test:self_test();return
    if not args.server or not args.client:
        parser.error('specify exact original --server and --client, or --self-test')
    try:report=authenticate(args.server.read_bytes(),args.client.read_bytes())
    except (OSError,VerificationError,ValueError,struct.error) as exc:
        parser.exit(1,'PHASE642 ORIGINAL RETAIL AUDIT REFUSED: '+str(exc)+'\n')
    out=json.dumps(report,indent=2)+'\n'
    if args.output:args.output.write_text(out,encoding='utf-8')
    else:print(out,end='')
if __name__=='__main__':main()
