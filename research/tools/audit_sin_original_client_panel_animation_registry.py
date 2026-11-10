#!/usr/bin/env python3
"""Phase164: owner-only original 2006 SiN client VGUI class registry audit.

Checks SHA256 of the complete ORIGINAL client/server DLLs through Phase163,
the four complete native VGUI registry function byte regions, 29 deliberately
partial x86 opcode anchors, 20 exact rel32 CALL edges, the native "vgui::"
prefix and the engine diagnostic mentioning "Panel Animation class".
Never publishes, copies or prints full proprietary original function bytes.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
from pathlib import Path

from audit_sin_original_client_property_descriptor_array import audit as phase163_audit
from verify_gameplay_entity_registry import PEImage,VerificationError

REGIONS={
 "lazy_panel_registry_singleton":(0x24011D30,129,
  "03bb9a680ba41e011f0a4181986be0ccaa1c0bdd2d07c7047ae4b8578c105d97"),
 "class_name_to_owner_wrapper":(0x24011DC0,17,
  "65352515a9960692d154459a17610080cf15935918816e34129b64f387297e6e"),
 "find_existing_panel_class":(0x24011B80,101,
  "4a16915de0aa14d2133e1125a2dc4804ec1adef39edf20d8895670dfb9f260fd"),
 "find_or_create_panel_class":(0x24011BF0,107,
  "ee5d099c12c1689f9934a681bdde25f685469454b9ae4960d25a89c6fc66bdcb"),
}
OPCODES={
 0x24011D30:"f605c0d9382401", # check singleton once flag bit0
 0x24011D39:"830dc0d9382401", # set once flag bit0 BEFORE init
 0x24011D4A:"b938d93824", # init original global registry object
 0x24011D60:"c70568d93824c02c0124", # class-registry map virtual dispatch
 0x24011DAB:"b838d93824", # return singleton address
 0x24011DC6:"8b5c2408", # caller class-name argument (EBX)
 0x24011B80:"83ec0c", # existing-entry finder
 0x24011B84:"6a06", # compare exactly six bytes
 0x24011B88:"6870362d24", # prefix literal "vgui::"
 0x24011B8E:"ff1518622724", # IAT engine Q_strnicmp
 0x24011B97:"85c0", # prefix comparison zero
 0x24011B99:"8d4606", # strip six bytes on match
 0x24011B9C:"7402",
 0x24011B9E:"8bc6", # otherwise keep entire name
 0x24011BA5:"8d7730", # registry map lives at +0x30
 0x24011BD1:"8b4f34", # registered records at +0x34
 0x24011BD7:"8b44c114", # owner pointer at record +0x14
 0x24011BDF:"33c0", # no class -> NULL owner
 0x24011BF0:"515557", # find-or-create prologue
 0x24011BF3:"8bf8", # stash registry owner in EDI
 0x24011BF5:"8bc3", # class name comes from EBX
 0x24011BFC:"33ed", # 0 for new owner DWORD initialization
 0x24011C00:"7555", # if owner found, return without creation
 0x24011C15:"892e", # owner allocation +0x00 = 0
 0x24011C23:"896e14", # owner allocation +0x14 = 0
 0x24011C29:"6a06", # second namespace prefix comparison
 0x24011C40:"8d4306", # strip prefix from new class symbol
 0x24011C45:"8bc3", # no prefix, original name pointer
 0x24011C54:"8bc6", # return allocated owner
}
CALLS={
 0x24011DC1:0x24011D30, # get native singleton first
 0x24011DCA:0x24011BF0, # then find-or-create owner
 0x24011BF7:0x24011B80, # existing class lookup
 0x24011C03:0x240093B0, # initialize original symbol support
 0x24011C0A:0x24170580, # allocate a 28-byte owner object
 0x24011C4F:0x240124B0, # insert into owner symbol map
 0x24011BB3:0x241D4A80, # intern native class name
 0x24011BC6:0x24012DE0, # original tree find
 0x24011D4F:0x241704C0, # singleton member initial setup
 0x24011D98:0x241D4640, # singleton symbol setup
 0x24011DA2:0x24248788, # register native destructor
 0x24011DE1:0x24011D30, # secondary wrapper uses singleton
 0x24011DEC:0x24011B80, # secondary wrapper existing-only find
  0x24230DF8:0x24011DC0, # seven Phase162 VGUI properties
  0x24230E68:0x24011DC0,
  0x24230ED8:0x24011DC0,
  0x24236468:0x24011DC0,
  0x242364D8:0x24011DC0,
  0x24236548:0x24011DC0,
  0x242365B8:0x24011DC0,
}
NATIVE_LITERALS={
 0x242D3670:b"vgui::\x00",
 0x242D4318:b"No such Panel Animation class %s\n\x00",
}
def verify_original_registry(pe:PEImage,*,check_hashes=True)->dict:
    if pe.base!=0x24000000:
        raise VerificationError("original 2006 client.dll base mismatch")
    for va,raw in NATIVE_LITERALS.items():
        if pe.at(va,len(raw))!=raw:
            raise VerificationError("native Panel Animation registry literal mismatch at %#x"%va)
    for va,raw_hex in OPCODES.items():
        original=bytes.fromhex(raw_hex)
        if pe.at(va,len(original))!=original:
            raise VerificationError("native panel registry opcode mismatch at %#x"%va)
    for va,target in CALLS.items():
        expected=b"\xe8"+struct.pack("<i",target-va-5)
        if pe.at(va,5)!=expected:
            raise VerificationError("native panel registry call mismatch at %#x"%va)
    if check_hashes:
        for label,(va,size,expected) in REGIONS.items():
            if hashlib.sha256(pe.at(va,size)).hexdigest()!=expected:
                raise VerificationError("native panel registry complete SHA256 mismatch: "+label)
    return {
      "complete_original_method_hashes":len(REGIONS) if check_hashes else 0,
      "verified_original_opcode_anchors":len(OPCODES),
      "verified_native_call_edges":len(CALLS),
      "original_seven_hud_registrar_owner_calls":7,
      "class_name_prefix":"vgui::",
      "native_prefix_compare":"Q_strnicmp (engine import at IAT 0x24276218), exactly 6 bytes",
      "original_singleton":{"method":"0x24011D30","global":"0x2438D938","flag":"0x2438D9C0"},
      "owner_lookup":{"find":"0x24011B80","get_or_create":"0x24011BF0",
                      "wrapper":"0x24011DC0"},
      "native_record_layout":{"class_object_zeroed_bytes":28,
                              "owner_pointer_inside_registry_record":"+0x14",
                              "registry_record_size":24,
                              "symbol_map":"+0x30","record_storage":"+0x34"},
      "native_class_creation":[
        "existing symbol owner -> return without setup/allocation",
        "missing owner -> setup symbol support; acquire 28-byte owner",
        "if allocated -> initialize 7 original dwords to zero",
        "strip case-insensitive vgui:: prefix before insertion",
        "attempt native key insertion even when allocator returns NULL",
      ],
      "limits":[
        "Underlying symbol intern/hash/tree and allocator not recreated.",
        "Initialization of native singleton internals/destructor not ported.",
        "Unknown case rules beyond prefix are NOT inferred.",
        "No actual Windows VGUI engine registration or game execution.",
      ],
    }
def audit(client_bytes:bytes,server_bytes:bytes)->dict:
    report=phase163_audit(client_bytes,server_bytes)
    report["original_client_panel_registry_owner"]=verify_original_registry(
        PEImage(client_bytes))
    return report

def main()->None:
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--client",required=True,type=Path)
    parser.add_argument("--server",required=True,type=Path)
    parser.add_argument("--output",type=Path)
    args=parser.parse_args()
    value=json.dumps(audit(args.client.read_bytes(),args.server.read_bytes()),indent=2)+"\n"
    if args.output:
        args.output.parent.mkdir(parents=True,exist_ok=True)
        args.output.write_text(value,encoding="utf-8")
    else:
        print(value,end="")
if __name__=="__main__":
    main()
