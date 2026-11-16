#!/usr/bin/env python3
"""Phase244: SHA-review original 2006 SiN client PVS node/list primitives.

Grants original review credit only for BOTH exact complete retail PE32 DLLs,
eight entire authentic method SHA256s, native return/padding, and direct calls.
Never commit original DLLs, full proprietary native bodies or disassemblies.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import struct
import zipfile
from pathlib import Path

from audit_sin_original_dll_interface_abi import ORIGINALS, PEImage, VerificationError

# (name, original client VA, native method size, full-body SHA256,
#  trailing INT3 count, (native RET offset, native return bytes), witnesses)
METHODS = (
    ("map_16_record_payload_address",0x24105D10,18,
     "c1940e69b4f114920d06d56700083f2cd9edaf9d6145fdd2020485eb2ad84188",
     14,((0x0F,"c20400"),),{0x00:"0fb7442404",0x05:"8b4904",0x08:"c1e004",0x0B:"8d440808"}),
    ("aux_8_record_free_chain_reset",0x24105DD0,81,
     "be605e319bc1bc6c2ac86feea8b724e3b9d051d4f59dc4752deb527031d106a9",
     15,((0x50,"c3"),),{0x00:"668b4114",0x04:"6685c0",0x07:"7447",0x10:"beffff0000",0x18:"8b29",0x2A:"6689443204",0x37:"66c74110",0x50:"c3"}),
    ("list_16_record_is_active",0x24105E80,46,
     "89b9061da55ddeadc3179e6ee9c9235b1e75ba17b5c773920d0472f810078763",
     2,((0x26,"c20400"),(0x2B,"c20400")), {0x00:"668b442404",0x09:"731e",0x10:"c1e204",0x13:"03ca",0x26:"c20400",0x2B:"c20400"}),
    ("map_16_record_set_parent",0x24105EE0,24,
     "b1c6c757afdf5106f71ba3452c69c46ee0713d822bcc6ebb238132c6e7eafbfb",
     8,((0x15,"c20800"),),{0x00:"0fb7442404",0x05:"8b4904",0x08:"668b542408",0x10:"6689540804"}),
    ("map_16_record_set_left",0x24105F00,23,
     "49174d895e56856ee7877c8eab60c667cf613ea4254052d7f20067fd7bb510f7",
     9,((0x14,"c20800"),),{0x00:"0fb7442404",0x05:"8b4904",0x08:"668b542408",0x10:"66891408"}),
    ("map_16_record_set_right",0x24105F20,24,
     "ed3757cb13ea83780882d9e45819bb069f2c58ae86a7750d2371fcb85765403b",
     8,((0x15,"c20800"),),{0x00:"0fb7442404",0x05:"8b4904",0x08:"668b542408",0x10:"6689540802"}),
    ("map_16_record_read_color_via_resolver",0x24105FA0,17,
     "d7b08a570e5450bdaf0fc9aaadf9ebaf09d07d7701b79e5cadf1e4be2ed21c75",
     15,((0x0E,"c20400"),),{0x00:"8b442404",0x04:"50",0x05:"e896ffffff",0x0A:"0fb74006"}),
    ("map_16_record_set_color",0x24105FC0,24,
     "ed5b194512b3e405f7b96a020481c590fc1a29f87c59fb1b1be4ae80ddf135ef",
     8,((0x15,"c20800"),),{0x00:"0fb7442404",0x05:"8b4904",0x08:"668b542408",0x10:"6689540806"}),
)
# Owner-original direct E8 edges independently verified from retail code.
CALLS = (
    (0x24105FA5,0x24105F40), # read color -> Phase238 sentinel/node resolver
    (0x241069B3,0x24105DD0), # auxiliary manager initialization
    (0x241074A3,0x24105DD0), # containing-object reset/destructor
)

def e8(site:int,target:int)->bytes:
    return b"\xE8"+struct.pack("<i",target-site-5)

def verify_native(client:PEImage,*,check_hash:bool=True,
                  authentic_provenance:bool=False)->dict:
    if client.base!=ORIGINALS["client"]["base"]:
        raise VerificationError("Phase244 original client PE32 base mismatch")
    methods=[];witnesses=0
    for name,va,size,sha,pad,rets,anchors in METHODS:
        if client.section_of(va)!=".text" or client.section_of(va+size-1)!=".text":
            raise VerificationError("Phase244 original method outside .text: "+name)
        code=client.at(va,size)
        if check_hash and hashlib.sha256(code).hexdigest()!=sha:
            raise VerificationError("Phase244 original whole-method SHA drift: "+name)
        if client.at(va+size,pad)!=b"\xCC"*pad:
            raise VerificationError("Phase244 original trailing INT3 drift: "+name)
        for off,hexbytes in rets:
            b=bytes.fromhex(hexbytes)
            if code[off:off+len(b)]!=b:
                raise VerificationError(f"Phase244 original return drift: {name}+{off:#x}")
        for off,hexbytes in anchors.items():
            b=bytes.fromhex(hexbytes)
            if code[off:off+len(b)]!=b:
                raise VerificationError(f"Phase244 native opcode drift: {name}+{off:#x}")
            witnesses+=1
        methods.append({"name":name,"native_entry":f"0x{va:08X}",
                        "whole_original_code_bytes":size,
                        "whole_original_sha256":sha if check_hash else None,
                        "return_paths":len(rets),"trailing_INT3":pad})
    for site,target in CALLS:
        if client.section_of(site)!=".text" or client.at(site,5)!=e8(site,target):
            raise VerificationError(f"Phase244 original E8 target drift: {site:#x}")
    return {
        "phase":244,
        "new_original_complete_SHA_methods":len(methods) if check_hash and authentic_provenance else 0,
        "new_original_native_code_bytes":sum(x[2] for x in METHODS)
            if check_hash and authentic_provenance else 0,
        "reviewed_original_methods":methods,
        "sparse_native_opcode_witnesses":witnesses,
        "exact_original_direct_E8_edges":len(CALLS),
        "node_16_stride":16,
        "node_16_parent_WORD_offset":4,
        "node_16_color_WORD_offset":6,
        "node_16_payload_offset":8,
        "list_16_prev_WORD_offset":12,
        "list_16_next_WORD_offset":14,
        "aux_8_prev_WORD_offset":4,
        "aux_8_next_WORD_offset":6,
        "aux_8_free_head_WORD_offset":16,
        "aux_8_capacity_WORD_offset":20,
        "absent_index":0xFFFF,
        "limitations":[
            "List8 reset and List16 active predicate apply to distinct native record managers.",
            "Native unchecked pointer arithmetic is represented as bounded host offsets.",
            "The original 0x24105FA0 color accessor resolves nil through 0x24105F40.",
            "Native root entity-list object lifetime and retail Windows gameplay DLL boot remain unresolved."
        ],
    }

def audit(server:bytes,client:bytes)->dict:
    for name,data in (("server",server),("client",client)):
        if hashlib.sha256(data).hexdigest()!=ORIGINALS[name]["sha256"]:
            raise VerificationError("Phase244 requires BOTH entire original retail DLL SHA256s: "+name)
    evidence=verify_native(PEImage(client),authentic_provenance=True)
    if evidence["new_original_complete_SHA_methods"]!=8 or \
       evidence["new_original_native_code_bytes"]!=257:
        raise VerificationError("Phase244 original complete-method ledger drift")
    return evidence

def main()->None:
    p=argparse.ArgumentParser(description=__doc__)
    src=p.add_mutually_exclusive_group(required=True)
    src.add_argument("--archive",type=Path)
    src.add_argument("--server",type=Path)
    p.add_argument("--client",type=Path)
    args=p.parse_args()
    if args.archive:
        if args.client:p.error("--archive excludes --client")
        with zipfile.ZipFile(args.archive) as z:
            if z.namelist().count("client.dll")!=1 or z.namelist().count("server.dll")!=1:
                raise VerificationError("Phase244 original DLLs must appear once at ZIP root")
            result=audit(z.read("server.dll"),z.read("client.dll"))
    else:
        if not args.client:p.error("--server requires --client")
        result=audit(args.server.read_bytes(),args.client.read_bytes())
    print(json.dumps(result,indent=2))

if __name__=="__main__":
    main()
