#!/usr/bin/env python3
"""Phase161: original 2006 client breath/poison HUD constructor witnesses.

Owner-only PE32 verifier checks SHA256 of both original DLLs, two complete
native constructor code regions, eight tiny member-offset helper regions,
native vtable writes, exact four-byte breath initialization, poison direct
field clears, original visibility-mask values and verified call edges.
No original executable code is copied to output or GitHub.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import struct
from verify_gameplay_entity_registry import PEImage,VerificationError,CLIENT_SHA256,SERVER_SHA256

# (original client VA, full method length, exact original code SHA256)
REGIONS={
 "breath_timer_constructor":(0x24230F30,230,"38c93563155918495201f0bbb99a75c339e8a16533033cb91917ef58424cb3b1"),
 "poison_timer_constructor":(0x24235E80,201,"7dc7ea0b731d13de89fcd70f5ff34c5b64aa07ed62a38a1a1d097542817bcafb"),
 "breath_panel_field_address":(0x242309F0,16,"7b425799d9562e26157a8051624ce73a5a995f1e1c2a0fa98f43f3eb7cf7762b"),
 "breath_complete_field_offset":(0x24230A00,6,"bd36350ae3bfe08df6fb572ebee9f406ed4ea953003deb777e245d2f5a156e11"),
 "poison_panel_field_address":(0x24236030,16,"7b425799d9562e26157a8051624ce73a5a995f1e1c2a0fa98f43f3eb7cf7762b"),
 "poison_complete_field_offset":(0x24236040,6,"bd36350ae3bfe08df6fb572ebee9f406ed4ea953003deb777e245d2f5a156e11"),
 "breath_resource_field_address":(0x24230A10,16,"5eb92598979c5339d1371426334129d2144e713d68e18894b79a2474b90cdae9"),
 "breath_resource_complete_offset":(0x24230A20,6,"bcc59c532702d05a8d70becbe0c2599d8af785c9b1cba14bc44b166f2c7b5501"),
 "poison_resource_field_address":(0x24236050,16,"5eb92598979c5339d1371426334129d2144e713d68e18894b79a2474b90cdae9"),
 "poison_resource_complete_offset":(0x24236060,6,"bcc59c532702d05a8d70becbe0c2599d8af785c9b1cba14bc44b166f2c7b5501"),
}
# Partial original instructions, never full proprietary function bodies.
OPCODES={
 0x24230F30:"8b442404",
 0x24230F52:"c706cc782c24", # breath primary vtable at original this+0x0
 0x24230F58:"c7079c762c24", # breath panel vtable at this+0x14
 0x24230FC7:"889eb7000000", # breath stores zero at complete+0xB7
 0x24230FCD:"889eb8000000",
 0x24230FD3:"889eb9000000",
 0x24230FD9:"889eba000000", # panel-relative +0xA6
 0x24230FFC:"6a10",         # breath CHudElement mask
 0x24231013:"c20400",
 0x24235E80:"8b442404",
 0x24235E9F:"c706ec8d2c24", # poison primary vtable
 0x24235EA5:"c707248b2c24", # poison panel vtable
 0x24235F27:"6a18",         # poison CHudElement mask
 0x24235F2B:"8986fc020000", # poison clears +0x2FC
 0x24235F31:"898600030000", # poison clears +0x300
 0x24235F37:"898604030000", # poison clears +0x304
 0x24235F46:"c20400",
 0x242309F0:"8b442404",0x242309F8:"05a3000000",
 0x24230A00:"b8b7000000",
 0x242309F4:"85c0",
 0x24236030:"8b442404",0x24236038:"05a3000000",
 0x24236040:"b8b7000000",
 0x24230A10:"8b442404",0x24230A18:"05a8000000",
 0x24230A20:"b8bc000000",
 0x24236050:"8b442404",0x24236058:"05a8000000",
 0x24236060:"b8bc000000",
}
CALLS={
 0x24230F3B:0x24142220, # original breath -> CHudElement ctor
 0x24230F4D:0x24003910, # original breath -> panel ctor
 0x24230FDF:0x24230E50, # original breath VGUI property registration
 0x24230FE4:0x24230EC0, # original breath VGUI property registration
 0x24231000:0x24141850, # breath -> CHudElement mask setter
 0x24235E89:0x24142220, # poison -> CHudElement ctor
 0x24235E9A:0x24153320, # poison -> panel ctor
 0x24235F3D:0x24141850, # poison -> CHudElement mask setter
}
RTTI_NAMES={
 0x24320D3C:b".?AVCHudBreathTimer@@\x00",
 0x24320EB0:b".?AVCHudPoisonTimer@@\x00",
}
def verify_original_client(pe:PEImage,*,check_hashes=True)->dict:
    if pe.base!=0x24000000:
        raise VerificationError("expected original 2006 client PE32 image base")
    for addr,raw in RTTI_NAMES.items():
        if pe.at(addr,len(raw))!=raw:
            raise VerificationError("original timer HUD RTTI identity mismatch at %#x"%addr)
    for addr,hexcode in OPCODES.items():
        expected=bytes.fromhex(hexcode)
        if pe.at(addr,len(expected))!=expected:
            raise VerificationError("original timer ctor or accessor x86 opcode mismatch at %#x"%addr)
    for site,destination in CALLS.items():
        encoded=b"\xe8"+struct.pack("<i",destination-site-5)
        if pe.at(site,5)!=encoded:
            raise VerificationError("original timer constructor CALL mismatch at %#x"%site)
    if check_hashes:
        for name,(addr,size,digest) in REGIONS.items():
            if hashlib.sha256(pe.at(addr,size)).hexdigest()!=digest:
                raise VerificationError("original timer HUD complete code SHA mismatch: "+name)
    return {
        "verified_complete_code_region_hashes":len(REGIONS) if check_hashes else 0,
        "verified_constructor_hashes":2 if check_hashes else 0,
        "verified_small_offset_accessor_hashes":8 if check_hashes else 0,
        "original_opcode_witnesses":len(OPCODES),
        "original_direct_call_edges":len(CALLS),
        "original_vtable_writes":{
            "breath":{"primary":"0x242C78CC","panel":"0x242C769C"},
            "poison":{"primary":"0x242C8DEC","panel":"0x242C8B24"},
        },
        "original_visibility_masks":{"breath":16,"poison":24},
        "original_panel_offset":20,
        "panel_relative_four_byte_field":[163,164,165,166],
        "complete_object_four_byte_field":[183,184,185,186],
        "explicit_zero_initialization":{"breath":"+0xB7..+0xBA"},
        "poison_explicit_zero_dwords":["+0x2FC","+0x300","+0x304"],
        "scope":[
            "Only explicit constructor fields and small offset accessors are modeled.",
            "Constructor registration, native Panel setup and VGUI surface are NOT reimplemented.",
            "Original byte +0xA6 appears as fourth of four consecutive panel bytes; source-level alpha type is unproven.",
            "Do not infer that poison constructor explicitly zeroes the same color bytes.",
        ],
    }
def audit(client_bytes:bytes,server_bytes:bytes)->dict:
    if hashlib.sha256(client_bytes).hexdigest()!=CLIENT_SHA256:
        raise VerificationError("wrong original client.dll SHA256")
    if hashlib.sha256(server_bytes).hexdigest()!=SERVER_SHA256:
        raise VerificationError("wrong original server.dll SHA256")
    result=verify_original_client(PEImage(client_bytes))
    result["sha256"]={"client.dll":CLIENT_SHA256,"server.dll":SERVER_SHA256}
    return result
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--client",required=True,type=Path)
    p.add_argument("--server",required=True,type=Path)
    p.add_argument("--output",type=Path)
    args=p.parse_args()
    out=json.dumps(audit(args.client.read_bytes(),args.server.read_bytes()),indent=2)+"\n"
    if args.output:
        args.output.parent.mkdir(parents=True,exist_ok=True)
        args.output.write_text(out,encoding="utf-8")
    else:
        print(out,end="")
if __name__=="__main__":main()
