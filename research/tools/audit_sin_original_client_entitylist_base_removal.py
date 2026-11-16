#!/usr/bin/env python3
"""Phase247: authenticate original SiN (2006) client entity-list base removal.

Both complete original retail PE32 client/server hashes are mandatory before
whole-original-native-method credit. Only SHA hashes and sparse witnesses are
stored; NEVER commit original DLLs, full proprietary methods or disassemblies.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import struct
import zipfile
from pathlib import Path
from audit_sin_original_dll_interface_abi import ORIGINALS, PEImage, VerificationError

# (name, original VA, entire x86 bytes, whole-native SHA256,
#  RET offset/encoding, trailing INT3 count, sparse opcode witnesses)
METHODS = (
    ("high_slot_free_chain_move_to_tail",0x24127E20,91,
     "a7a4fce2f5f71f02f28ed914f974c0eb6bc10f525b511336c7bfe170c506fcc3",
     ((0x52,"c20400"),(0x58,"c20400")),5,{
         0x00:"8b442404",0x07:"3bd0",0x39:"c7400c00000000",
         0x50:"8901c20400",0x58:"c20400"
     }),
    ("raw_entity_slot_remove",0x24127FC0,176,
     "3dda702afe19b7f523be5d57e8abc1cd947dc94a0feb315e01f7e1df8b284af5",
     ((0xAD,"c20400"),),0,{
         0x00:"538b5c2408",0x0B:"c1e004",
         0x18:"0f848c000000",0x2A:"8b1150ff5204",
         0x4C:"0883c10181e1",0x90:"89760c897608",
         0xA5:"e8b6fdffff",0xAD:"c20400"
     }),
    ("entitylist_base_destructor",0x24128070,48,
     "60ccb6988afda1e4d8aae4497391ef6ec5f27d504aaa0a8526e205fce9fad7cb",
     ((0x2F,"c3"),),0,{
         0x00:"578bf9",0x03:"8b8704000100",
         0x0B:"c707b48e2924",0x13:"568b700c2bc7",
         0x20:"8bcfe829ffffff",0x27:"85f68bc675e7",0x2F:"c3"
     }),
    ("masked_entity_handle_remove_tail_thunk",0x24128180,18,
     "fe23a8e377538a49ae4a22f04ca9d4057a0a356e92151e00597b9f245b17bad5",
     ((0x0D,"e92efeffff"),),14,{
         0x00:"8b442404",0x04:"25ff0f0000",
         0x09:"89442404",0x0D:"e92efeffff"
     }),
)

# Native exact direct E8 CALLs / E9 tail JMPs. The first two calls
# are in recovered methods, the remaining ones link original caller
# methods reviewed previously in Phases245 and 231.
CALLS = (
    (0x24128065,0x24127E20),
    (0x24128092,0x24127FC0),
    (0x24106256,0x24128180),
)
TAIL_JMPS = (
    (0x2412818D,0x24127FC0),
    (0x241074C7,0x24128070),
)
# Already reviewed in stacked Phase246 PR #302. It was omitted from the
# JSON catalog despite authentic SHA review. We verify its whole body
# before adding a catalog bookkeeping entry, but it is NOT a NEW Phase247
# method and earns zero NEW Phase247 code bytes.
CARRY_FORWARD_PHASE246=(0x24105FE0,39,
    "305edbceeb790e6a91f91fa5815f31c11f606f71e1831384350bb5c265186fe2")

def relative(op:int,site:int,target:int)->bytes:
    return bytes([op])+struct.pack("<i",target-site-5)

def verify_native(client:PEImage,*,check_hash:bool=True,
                  authentic_provenance:bool=False)->dict:
    if client.base!=ORIGINALS["client"]["base"]:
        raise VerificationError("Phase247 owner-original client PE32 image-base drift")
    rows=[];witnesses=0
    for name,va,size,sha,exits,padding,anchors in METHODS:
        if client.section_of(va)!=".text" or client.section_of(va+size-1)!=".text":
            raise VerificationError("Phase247 authentic native code outside .text: "+name)
        body=client.at(va,size)
        if check_hash and hashlib.sha256(body).hexdigest()!=sha:
            raise VerificationError("Phase247 entire authentic native method SHA drift: "+name)
        if client.at(va+size,padding)!=b"\xCC"*padding:
            raise VerificationError("Phase247 original method INT3 boundary drift: "+name)
        for off,hx in exits:
            expected=bytes.fromhex(hx)
            if body[off:off+len(expected)]!=expected:
                raise VerificationError(f"Phase247 native return/JMP drift: {name}+{off:#x}")
        for off,hx in anchors.items():
            expected=bytes.fromhex(hx)
            if body[off:off+len(expected)]!=expected:
                raise VerificationError(f"Phase247 native opcode witness drift: {name}+{off:#x}")
            witnesses+=1
        rows.append({
            "name":name,"original_va":f"0x{va:08X}",
            "whole_original_native_bytes":size,
            "whole_original_code_SHA256":sha if check_hash else None,
            "terminal_paths":len(exits),"trailing_original_INT3":padding
        })
    for opcode,edges in ((0xE8,CALLS),(0xE9,TAIL_JMPS)):
        for site,target in edges:
            if client.section_of(site)!=".text" or \
                    client.at(site,5)!=relative(opcode,site,target):
                raise VerificationError(
                    f"Phase247 native {opcode:#x} target drift at {site:#x}")
    va,size,sha=CARRY_FORWARD_PHASE246
    if client.section_of(va)!=".text" or \
            (check_hash and hashlib.sha256(client.at(va,size)).hexdigest()!=sha):
        raise VerificationError("Phase247 predecessor Phase246 original body SHA drift")
    if client.at(va+size,9)!=b"\xCC"*9 or \
       client.at(va+size-2,2)!=b"\x5e\xc3" or \
       client.at(0x24105FF0,5)!=relative(0xE8,0x24105FF0,0x2416F940):
        raise VerificationError("Phase247 Phase246 method return/padding/free edge drift")
    return {
        "phase":247,
        "new_original_complete_SHA_methods":(
            len(rows) if check_hash and authentic_provenance else 0),
        "new_original_native_code_bytes":(
            sum(m[2] for m in METHODS) if check_hash and authentic_provenance else 0),
        "reviewed_original_methods":rows,
        "previous_phase246_method_0x24105FE0_verified_not_new":bool(check_hash),
        "sparse_original_opcode_witnesses":witnesses,
        "original_exact_E8_CALL_edges":len(CALLS),
        "original_exact_E9_tail_JMP_edges":len(TAIL_JMPS),
        "parent_original_slots":4096,
        "handle_slot_index_mask":0xFFF,
        "serial_wrap_mask":0x7FFF,
        "high_free_list_slot_start":2048,
        "base_vtable_original_VA":"0x24298EB4",
        "base_dtor_original_VA":"0x24128070",
        "real_native_gameplay_DLL_boot_confirmed":False,
    }

def audit(server:bytes,client:bytes)->dict:
    for name,blob in (("server",server),("client",client)):
        if hashlib.sha256(blob).hexdigest()!=ORIGINALS[name]["sha256"]:
            raise VerificationError(
                "Phase247 requires BOTH complete owner-original retail DLL SHA256s: "+name)
    result=verify_native(PEImage(client),authentic_provenance=True)
    if (result["new_original_complete_SHA_methods"]!=4 or
            result["new_original_native_code_bytes"]!=333):
        raise VerificationError("Phase247 whole-original verified-method accounting drift")
    return result

def main()->None:
    p=argparse.ArgumentParser(description=__doc__)
    group=p.add_mutually_exclusive_group(required=True)
    group.add_argument("--archive",type=Path)
    group.add_argument("--server",type=Path)
    p.add_argument("--client",type=Path)
    args=p.parse_args()
    if args.archive:
        if args.client:p.error("--archive excludes --client")
        with zipfile.ZipFile(args.archive) as archive:
            if (archive.namelist().count("client.dll")!=1 or
                    archive.namelist().count("server.dll")!=1):
                raise VerificationError("Phase247 expects exactly one root original DLL pair")
            report=audit(archive.read("server.dll"),archive.read("client.dll"))
    else:
        if not args.client:p.error("--server also requires --client")
        report=audit(args.server.read_bytes(),args.client.read_bytes())
    print(json.dumps(report,indent=2))

if __name__=="__main__":
    main()
