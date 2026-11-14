#!/usr/bin/env python3
"""Phase229: SHA-review selected original virtual methods in embedded interfaces.

Audits *complete* original 2006 client/server.dll method hashes only after
both owner-original whole DLL SHA256s match the project provenance. Sparse
opcode witnesses, never original whole instruction bodies, are published.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import struct
import zipfile
from pathlib import Path

from audit_sin_original_dll_interface_abi import ORIGINALS, PEImage, VerificationError
from audit_sin_original_core_factory_thunks import pe_highlow_relocations
from audit_sin_original_embedded_interface_initializers import CHAINS

# module: (descriptive behavior, code VA, full native code length, whole
# original SHA256, following INT3 padding, original embedded vtable slot,
# native return opcode + pop byte count, SPARSE opcode witnesses).
METHODS = {
    "server": (
        ("hltv_get_state_dword_04",0x2225F920,4,
         "f7e0208f5e988ae39c7a45dfddbd12921d02f9c3deb5695d4c534c8ab0ccdb0f",
         12,3,0, {0:"8b41"}),
        ("hltv_state_nonzero",0x2225F930,9,
         "f862b03f571a3d453c5ef9b7e990a48ac6059bd0f34bac7da7740d0d8fc81cd9",
         7,1,0,{0:"33c0",5:"0f95"}),
        ("hltv_get_float_08_x87",0x2225F940,4,
         "a4b7d4539fa03abbe69a398ca110f35831e031556ffd14e0089956276d803f49",
         12,7,0,{0:"d941"}),
        ("hltv_get_dword_0c",0x2225F950,4,
         "2e339cdc5de837ec151dbde90057caeabba18ac3ea32e7c2cffc8f95df41a6ff",
         12,4,0,{0:"8b41"}),
        ("hltv_get_dword_10",0x2225F960,4,
         "74032ad0e931f3c4730df99da43479f9f5315cc898c7c8614278631cb2c16a62",
         12,5,0,{0:"8b41"}),
        ("hltv_copy_three_dwords_from_14",0x2225F970,30,
         "3bb7057383057b467573a706b63c99a8a0e5a96a9458c6b6295a720eef8e79f7",
         2,6,4,{0:"8b442404",4:"83c114",7:"56",27:"c20400"}),
        ("hltv_get_global_pointer_token",0x2225F990,6,
         "bbfc0932da48683f3210f699f5e5a3892cecaa5c8fd48ae3a2eb4a5bcfb5dbcc",
         10,8,0,{0:"b8"}),
    ),
    "client": (
        ("entitylist_indexed_dword_stride8",0x241056F0,11,
         "c6dd5f622ddbfbf71a8fc6775e469a11093e9667280a57175103822991205917",
         5,0,4,{0:"8b442404",8:"c20400"}),
        ("entitylist_get_base_plus_optional_offset",0x24105700,22,
         "574b3832d44e07ecd636cbdc983c184df58eee49e690b4532e8b4aa5fafda8a8",
         10,5,4,{0:"807c240401",5:"7509",19:"c20400"}),
        ("entitylist_set_dword_08",0x24105720,10,
         "b35caac991eb0d06657c3fbba49c85de5f83e8cf4de0b07b70fa4b90cb5770fd",
         6,7,4,{0:"8b442404",7:"c20400"}),
        ("entitylist_get_dword_08",0x24105730,4,
         "916ae10aa3fd76a4d5b25b2fb66e9df0fd04957c7ab10930268d1ed7808be689",
         12,8,0,{0:"8b41"}),
        ("entitylist_get_dword_10",0x24105740,4,
         "74032ad0e931f3c4730df99da43479f9f5315cc898c7c8614278631cb2c16a62",
         12,6,0,{0:"8b41"}),
    ),
}
# Native slot order, not method address order: verified through original
# final embedded-interface vtables. Every pointer is PE32 HIGHLOW relocated.
SERVER_VTABLE_VA=0x22453DA0
CLIENT_VTABLE_VA=0x24297390
SERVER_GLOBAL_ABSOLUTE_VA=0x2251B054
SERVER_GLOBAL_IMM_SITE=0x2225F991
CLIENT_STRIDE=8
CLIENT_INDEXED_BASE_OFFSET=0x14

def check_module(pe:PEImage, module:str, *,
                 check_hash:bool=True,
                 original_provenance_verified:bool=False,
                 relocated_sites:set[int]|None=None)->dict:
    if module not in METHODS or pe.base!=ORIGINALS[module]["base"]:
        raise VerificationError("Phase229 original PE32 base/module mismatch")
    if relocated_sites is None:
        relocated_sites=pe_highlow_relocations(pe)
    parent=CHAINS[module]
    if module=="server":
        expected_interface="HLTVDirector001"
        expected_vtable=SERVER_VTABLE_VA
    else:
        expected_interface="VClientEntityList003"
        expected_vtable=CLIENT_VTABLE_VA
    if parent[0]!=expected_interface or parent[10]!=expected_vtable:
        raise VerificationError("Phase229 original Phase228 embedded parent/vtable mismatch")
    if pe.section_of(expected_vtable)!=".rdata":
        raise VerificationError("Phase229 original final embedded vtable outside .rdata")
    evidence=[]
    for label,va,size,sha,pad,slot,pop,anchors in METHODS[module]:
        if pe.section_of(va)!=".text" or pe.section_of(va+size-1)!=".text":
            raise VerificationError("Phase229 original virtual method outside .text: "+label)
        code=pe.at(va,size)
        if check_hash and hashlib.sha256(code).hexdigest()!=sha:
            raise VerificationError("Phase229 original entire virtual method SHA256 drift: "+label)
        if pop:
            if code[-3:]!=b"\xc2"+struct.pack("<H",pop):
                raise VerificationError("Phase229 original callee-cleanup RET drift: "+label)
        elif code[-1:]!=b"\xc3":
            raise VerificationError("Phase229 original C3 RET drift: "+label)
        if pe.at(va+size,pad)!=b"\xcc"*pad:
            raise VerificationError("Phase229 original next-function INT3 boundary drift: "+label)
        for off,hexbytes in anchors.items():
            b=bytes.fromhex(hexbytes)
            if code[off:off+len(b)]!=b:
                raise VerificationError("Phase229 original sparse opcode witness drift: "+label+" "+hex(off))
        slot_va=expected_vtable+slot*4
        if (pe.section_of(slot_va)!=".rdata" or
            pe.at(slot_va,4)!=struct.pack("<I",va) or
            slot_va not in relocated_sites):
            raise VerificationError("Phase229 original relocated vtable method slot drift: "+label)
        evidence.append({
            "method":label,"entry":f"0x{va:08X}",
            "original_full_code_length":size,
            "whole_original_native_sha256":sha if check_hash else None,
            "vtable_slot_index":slot,
            "slot_pointer_va":f"0x{slot_va:08X}",
            "ret_stack_pop_bytes":pop,
            "original_INT3_padding":pad,
        })
    if module=="server":
        if (pe.at(SERVER_GLOBAL_IMM_SITE,4)!=struct.pack("<I",SERVER_GLOBAL_ABSOLUTE_VA)
            or SERVER_GLOBAL_IMM_SITE not in relocated_sites):
            raise VerificationError("Phase229 original absolute server global address/relocation drift")
    return {
        "module":module,
        "interface":expected_interface,
        "original_embedded_vtable":f"0x{expected_vtable:08X}",
        "complete_native_method_sha_evidence":evidence,
        "checked_original_vtable_slots":len(evidence),
        "new_whole_SHA_verified_original_methods":len(evidence)
            if check_hash and original_provenance_verified else 0,
        "new_original_x86_code_bytes":sum(row[2] for row in METHODS[module])
            if check_hash and original_provenance_verified else 0,
        "server_global_pointer_relocation_verified":module=="server",
    }

def audit(server:bytes,client:bytes)->dict:
    for module,buf in (("server",server),("client",client)):
        if hashlib.sha256(buf).hexdigest()!=ORIGINALS[module]["sha256"]:
            raise VerificationError("Phase229 original complete retail PE32 SHA256 mismatch: "+module)
    sv=check_module(PEImage(server),"server",original_provenance_verified=True)
    cl=check_module(PEImage(client),"client",original_provenance_verified=True)
    return {
        "phase":229,
        "original_native_methods":sv["new_whole_SHA_verified_original_methods"]+
                                  cl["new_whole_SHA_verified_original_methods"],
        "original_x86_instruction_bytes":sv["new_original_x86_code_bytes"]+
                                         cl["new_original_x86_code_bytes"],
        "server":sv,"client":cl,
        "limits":[
            "These twelve are small selected original virtual methods, not a complete interface vtable or game object.",
            "Host implementations use explicit byte-buffer views and safe guards, not real engine C++ objects.",
            "Original x87 FLD returns float in ST(0); source has IEEE binary32 but host does not emulate exception/trap flags.",
            "The original server global pointer is an address token, not a portable valid pointer.",
            "Original embedded interface pointer and containing-object pointer are different.",
            "Real Windows Source engine loading rebuilt gameplay DLLs remains unproven.",
        ],
    }

def main()->None:
    p=argparse.ArgumentParser(description=__doc__)
    source=p.add_mutually_exclusive_group(required=True)
    source.add_argument("--archive",type=Path)
    source.add_argument("--server",type=Path)
    p.add_argument("--client",type=Path)
    args=p.parse_args()
    if args.archive:
        if args.client:p.error("--archive cannot combine with --client")
        with zipfile.ZipFile(args.archive) as z:
            if z.namelist().count("server.dll")!=1 or z.namelist().count("client.dll")!=1:
                raise VerificationError("Phase229 expected original root client/server DLL pair")
            report=audit(z.read("server.dll"),z.read("client.dll"))
    else:
        if not args.client:p.error("--server needs --client")
        report=audit(args.server.read_bytes(),args.client.read_bytes())
    print(json.dumps(report,indent=2))

if __name__=="__main__":
    main()
