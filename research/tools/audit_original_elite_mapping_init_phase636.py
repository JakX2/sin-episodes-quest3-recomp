#!/usr/bin/env python3
"""Phase636: exact 2006 retail SiN Elite mapping constructor/link audit.

Three complete original server.dll functions (58+55+119=232 bytes).
Checks original SHA256s, all original direct call edges, original
registration arguments, opcode witnesses and .data BSS ownership.
Use --self-test for strictly original-free public CI mutation tests.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
from pathlib import Path
from verify_gameplay_entity_registry import (
    PEImage,VerificationError,SERVER_SHA256,CLIENT_SHA256
)
ORIGINAL_METHODS=(
    ("mapping_node_constructor",0x22073610,58,
     "9c393a56c72a4cb224bebc3f12e5e0ffa40c37e249234c6a1a7a473288c2f4bc"),
    ("mapping_node_conditional_link",0x22073650,55,
     "8a0c782739b72f07fa9c673161442cc592b5561e12fda400cbcc40a5e6786d4a"),
    ("three_node_mapping_owner_registration",0x22088F10,119,
     "fee95bd5819a59e5d5641dca6fa2e55daed2e3503a23e01bc142c2c9f253e9c3")
)
ORIGINAL_EDGES=(
    (0x223C6A57,0x22073610,"init_Elite_owner_node_plus4"),
    (0x223C6A63,0x22073610,"init_Elite_owner_node_plus1C"),
    (0x223C6A6F,0x22073610,"init_Elite_owner_node_plus34"),
    (0x223C6A87,0x22073610,"init_neighboring_mapping_head"),
    (0x22088F33,0x22073650,"register_first_embedded_owner_node"),
    (0x22088F4F,0x22073650,"register_second_embedded_owner_node"),
    (0x22088F6B,0x22073650,"register_third_embedded_owner_node"),
    (0x22355AD9,0x22088F10,"register_Elite_mapping_owner")
)
WITNESSES=(
    # Original 24-byte node constructor: mode byte to 0 / 9999.
    (0x22073610,"8bc1"),
    (0x22073612,"8a4c2404"),
    (0x22073618,"f6da"),
    (0x2207361A,"1bd2"),
    (0x22073621,"f6d9"),
    (0x22073627,"81e1f1d8ffff"),
    (0x2207362D,"81c10f270000"),
    (0x22073633,"894804"),
    (0x22073636,"83c9ff"),
    (0x2207363C,"89480c"),
    (0x22073641,"894810"),
    (0x22073644,"894814"),
    (0x22073647,"c20400"),
    # Conditional node/link helper: branch on existing +0xC,
    # optional +4/+8/+C, always +0x10/+0x14, source+4 then +0.
    (0x22073650,"8b442408"),
    (0x22073657,"39510c"),
    (0x22073663,"81e60f270000"),
    (0x22073669,"897104"),
    (0x22073673,"894110"),
    (0x22073676,"8b442404"),
    (0x2207367A,"894114"),
    (0x2207367D,"8b4004"),
    (0x22073680,"8901"),
    (0x22073682,"b001"),
    (0x22073684,"c20800"),
    # Three successive original embedded-owner links and early tests.
    (0x22088F10,"8b442404"),
    (0x22088F16,"8b742414"),
    (0x22088F1F,"8907"),
    (0x22088F23,"8d4604"),
    (0x22088F30,"8d4f04"),
    (0x22088F38,"84c0"),
    (0x22088F40,"8d461c"),
    (0x22088F48,"8d4b08"),
    (0x22088F4C,"8d4f1c"),
    (0x22088F54,"84c0"),
    (0x22088F5C,"8d4634"),
    (0x22088F64,"83c310"),
    (0x22088F68,"8d4f34"),
    (0x22088F70,"84c0"),
    (0x22088F76,"b801000000"),
    (0x22088F7C,"c20c00"),
    (0x22088F84,"c20c00"),
    # Original Elite call's PUSH order is arg3, arg2, arg1.
    (0x22355AC5,"6858c65922"), # third: linked range owner BSS
    (0x22355ACA,"6824765422"), # second: source-record BSS
    (0x22355ACF,"6868da4822"), # first: class name .rdata
    (0x22355AD4,"b9f8c25922"), # ECX: mapping-owner BSS
    (0x223C6A68,"6a00"), # original mode zero
    (0x223C6A6A,"b92cc35922") # embedded third range node BSS
)
ELITE_OWNER=0x2259C2F8
ELITE_FIRST_RANGE=ELITE_OWNER+0x34
REGISTERED_CLASSNAME_VA=0x2248DA68
SOURCE_RECORDS_VA=0x22547624
LINKED_OWNER_VA=0x2259C658
EXPECTED_CLASSNAME=b"CNPC_EliteGrunt\x00"

def verify_method(read,va,size,expected):
    data=read(va,size)
    if len(data)!=size or hashlib.sha256(data).hexdigest()!=expected:
        raise VerificationError(f"complete original method SHA changed at {va:#x}")
def verify_call(read,site,target):
    blob=read(site,5)
    if len(blob)!=5 or blob[0]!=0xe8 or (
        site+5+struct.unpack_from("<i",blob,1)[0]!=target
    ):
        raise VerificationError(f"retail original rel32 call changed at {site:#x}")
def verify_opcode(read,site,hexcode):
    original=bytes.fromhex(hexcode)
    if read(site,len(original))!=original:
        raise VerificationError(f"retail original instruction changed at {site:#x}")
def verify(read):
    for _,va,size,digest in ORIGINAL_METHODS:
        verify_method(read,va,size,digest)
    for site,target,_ in ORIGINAL_EDGES:
        verify_call(read,site,target)
    for site,bytes_hex in WITNESSES:
        verify_opcode(read,site,bytes_hex)
def verify_original(server,client):
    if hashlib.sha256(server).hexdigest()!=SERVER_SHA256 or (
       hashlib.sha256(client).hexdigest()!=CLIENT_SHA256):
        raise VerificationError("both original owner retail DLL SHA256s required")
    pe=PEImage(server)
    if pe.base!=0x22000000 or any(
       pe.section_of(addr)!=".text" for _,addr,_,_ in ORIGINAL_METHODS
    ) or any(pe.section_of(site)!=".text"
             for site,_,_ in ORIGINAL_EDGES):
        raise VerificationError("original 2006 PE32 x86 text layout changed")
    if pe.section_of(REGISTERED_CLASSNAME_VA)!=".rdata":
        raise VerificationError("original NPC classname section changed")
    if pe.at(REGISTERED_CLASSNAME_VA,len(EXPECTED_CLASSNAME))!=EXPECTED_CLASSNAME:
        raise VerificationError("source original Elite classname is not as pinned")
    data=next((s for s in pe.sections if s[0]==".data"),None)
    if data is None:
        raise VerificationError("original .data section absent")
    _,rva,virtual_size,raw_offset,raw_size=data
    for addr in (ELITE_OWNER,ELITE_FIRST_RANGE,SOURCE_RECORDS_VA,LINKED_OWNER_VA):
        relative=addr-pe.base-rva
        if not (raw_size<=relative<virtual_size):
            raise VerificationError(
                f"original mapping address {addr:#x} not in virtual uninitialized .data tail")
    verify(pe.at)
    return {
        "status":"ORIGINAL_2006_ELITE_MAPPING_INITIALIZERS_PHASE636_VERIFIED",
        "original_server_sha256":SERVER_SHA256,
        "original_client_sha256":CLIENT_SHA256,
        "complete_original_methods":[
            {"role":name,"native_va":hex(va),
             "original_bytes":size,"sha256":digest}
            for name,va,size,digest in ORIGINAL_METHODS
        ],
        "distinct_original_methods":len(ORIGINAL_METHODS),
        "new_original_executable_bytes":sum(n for _,_,n,_ in ORIGINAL_METHODS),
        "original_calls":[
            {"site":hex(site),"target":hex(target),"role":role}
            for site,target,role in ORIGINAL_EDGES
        ],
        "original_opcode_witnesses":len(WITNESSES),
        "elite_mapping_owner":hex(ELITE_OWNER),
        "first_embedded_range_head":hex(ELITE_FIRST_RANGE),
        "elite_name":EXPECTED_CLASSNAME.decode("ascii").rstrip("\x00"),
        "source_records_guest_va":hex(SOURCE_RECORDS_VA),
        "linked_owner_guest_va":hex(LINKED_OWNER_VA),
        "mapping_structures_in_uninitialized_virtual_data":True,
        "original_range_content_available_on_disk":False,
        "original_game_process_executed":False,
        "replacement_engine_callback_observed":False
    }
def self_test():
    invented=bytes(((i*13+27)&255) for i in range(238))
    checksum=hashlib.sha256(invented).hexdigest()
    verify_method(lambda _v,n:invented[:n],0x71001000,len(invented),checksum)
    for index in range(len(invented)):
        changed=bytearray(invented);changed[index]^=1
        try:
            verify_method(lambda _v,n:bytes(changed[:n]),
                          0x71001000,len(invented),checksum)
        except VerificationError:pass
        else:raise AssertionError("invented original whole-body mutation accepted")
    src=0x71002000;target=0x71004000
    original_call=b"\xe8"+struct.pack("<i",target-src-5)
    verify_call(lambda _v,n:original_call[:n],src,target)
    for byte_index in range(5):
        mutated=bytearray(original_call);mutated[byte_index]^=1
        try:verify_call(lambda _v,n:bytes(mutated[:n]),src,target)
        except VerificationError:pass
        else:raise AssertionError("invented original CALL mutation accepted")
    verify_opcode(lambda _v,n:b"\x81\xc1\x0f\x27\x00\x00"[:n],
                  src,"81c10f270000")
    try:
        verify_opcode(lambda _v,n:b"\x81\xc1\x10\x27\x00\x00"[:n],
                      src,"81c10f270000")
    except VerificationError:pass
    else:raise AssertionError("changed original 9999 sentinel accepted")
    try:verify_original(b"not retail",b"not retail")
    except VerificationError:pass
    else:raise AssertionError("wrong original DLLs were accepted")
    print("PASS Phase636 synthetic full-method SHA, 8-call edge, opcode and wrong-original identity refusals")
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--server",type=Path)
    p.add_argument("--client",type=Path)
    p.add_argument("--output",type=Path)
    p.add_argument("--self-test",action="store_true")
    args=p.parse_args()
    if args.self_test:
        self_test();return 0
    if args.server is None or args.client is None:
        p.error("provide exact original --server and --client, or --self-test")
    try: report=verify_original(args.server.read_bytes(),args.client.read_bytes())
    except (OSError,VerificationError,ValueError,struct.error) as error:
        p.exit(1,"ORIGINAL PHASE636 MAPPING INITIALIZER AUDIT REFUSED: "+str(error)+"\n")
    payload=json.dumps(report,indent=2)+"\n"
    if args.output:args.output.write_text(payload,encoding="utf-8")
    else:print(payload,end="")
    return 0
if __name__=="__main__":
    raise SystemExit(main())
