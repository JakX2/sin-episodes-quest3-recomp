#!/usr/bin/env python3
"""Original SiN 2006 dual-DLL m_HL2Local/m_poisonTimer property verifier.

Chained to Phase153's exact original client PoisonTimer HUD verifier.
Checks original x86 RecvProp and SendProp initializer code witnesses,
their property-name pointers, verified call targets and owner-only code
region SHA256s. Never exports proprietary original code bytes.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

from audit_sin_original_client_poison_timer_hud import audit as audit_poison_hud
from verify_gameplay_entity_registry import PEImage, VerificationError

# Bounded original instruction sequences, not entire enclosing functions.
REGIONS = {
    "client_parent_m_HL2Local":("client",0x24216409,32,
       "13a824b620334a4d93962764935d5809527cc93d5306c080d5af152c83e84fe0"),
    "client_child_m_poisonTimer":("client",0x2421AD1B,26,
       "3aefc92ca08b291e32ed0b0bef1fc31d3c8a90d6ed21752eff97439eb50089e0"),
    "server_parent_m_HL2Local":("server",0x2239F559,30,
       "0a5a750e0128edc71fe58ce10834fb5f1b50e9a1c4d0b4963ab28b69641d2d10"),
    "server_child_m_poisonTimer":("server",0x223A2905,35,
       "fe02bdc1e9a771d06145116ec6269167ad0cecbe23e63531ca54d45af4741ab2"),
}
# Exact property-site push immediates, each proved to reside in code:
# these offsets are original 2006 x86 object layout, NOT Android.
OPCODES = {
    "client": {
        0x24216415:"68c8110000",       # parent m_HL2Local offset +0x11C8
        0x2421641A:"6804442b24",       # property string m_HL2Local
        0x2421AD24:"6a24",             # child m_poisonTimer offset +0x24
        0x2421AD26:"684c5e2b24",       # child property string
        0x2421AD6D:"68245e2b24",       # DT_HL2Local table
        0x24236333:"8b87ec110000",     # HUD reads player +0x11EC
    },
    "server": {
        0x2239F563:"687c130000",       # parent +0x137C
        0x2239F568:"6880f44a22",       # m_HL2Local property string
        0x223A2917:"6a24",             # child +0x24
        0x223A2919:"68a4f64a22",       # m_poisonTimer property string
    },
}
CALLS = {
    "client":{
        0x24216424:0x24126B50,  # datatable initializer
        0x2421AD30:0x24126D80,  # named property initializer
    },
    "server":{
        0x2239F572:0x2225EDD0,  # datatable initializer
        0x223A2923:0x2225E8C0,  # named property initializer
    },
}
STRINGS = {
    "client":{
        0x242B4404:"m_HL2Local",
        0x242B5E4C:"m_poisonTimer",
        0x242B5E24:"DT_HL2Local",
    },
    "server":{
        0x224AF480:"m_HL2Local",
        0x224AF6A4:"m_poisonTimer",
        0x224AFA28:"DT_HL2Local",
    },
}

def verify_orig_properties(images: dict, *, check_hashes=True) -> dict:
    for side,addresses in OPCODES.items():
        pe=images[side]
        base=0x24000000 if side=="client" else 0x22000000
        if pe.base!=base:
            raise VerificationError("original "+side+" PE base mismatch")
        for site,hexcode in addresses.items():
            expected=bytes.fromhex(hexcode)
            if pe.at(site,len(expected))!=expected:
                raise VerificationError("original "+side+" poison network offset opcode changed at %#x"%site)
    for side,addresses in CALLS.items():
        for site,target in addresses.items():
            expected=b"\xE8"+struct.pack("<i",target-site-5)
            if images[side].at(site,5)!=expected:
                raise VerificationError(side+" poison network constructor CALL changed at %#x"%site)
    for side,strings in STRINGS.items():
        for address,literal in strings.items():
            expected=literal.encode("ascii")+b"\x00"
            if images[side].at(address,len(expected))!=expected:
                raise VerificationError(side+" poison network table/property name changed")
    if check_hashes:
        for name,(side,address,length,sha) in REGIONS.items():
            actual=hashlib.sha256(images[side].at(address,length)).hexdigest()
            if actual!=sha:
                raise VerificationError("original poison network bounded code SHA256 mismatch: "+name)
    return {
        "network_table":"DT_HL2Local",
        "parent_property":"m_HL2Local",
        "nested_property":"m_poisonTimer",
        "client_original_x86_offsets":{
            "parent":0x11C8,"nested":0x24,"absolute":0x11EC,
            "hud_read_va":"0x24236333"},
        "server_original_x86_offsets":{
            "parent":0x137C,"nested":0x24,"absolute":0x13A0},
        "verified_original_code_sequences":len(REGIONS) if check_hashes else 0,
        "verified_property_opcode_sites":sum(map(len,OPCODES.values())),
        "verified_call_edges":sum(map(len,CALLS.values())),
        "limits":[
            "Verified original x86 SendProp/RecvProp data offsets are not the ARM64 object layout.",
            "Original property constructor names and helper call patterns verified; complete network quantization and proxy callbacks unresolved.",
            "No original game DLL bytes or Source engine code distributed.",
        ],
    }

def audit(client_bytes:bytes,server_bytes:bytes)->dict:
    result=audit_poison_hud(client_bytes,server_bytes)
    result["original_poison_timer_network"]=verify_orig_properties({
        "client":PEImage(client_bytes),"server":PEImage(server_bytes)})
    return result

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--client",type=Path,required=True)
    p.add_argument("--server",type=Path,required=True)
    p.add_argument("--output",type=Path)
    args=p.parse_args()
    txt=json.dumps(audit(args.client.read_bytes(),args.server.read_bytes()),indent=2)+"\n"
    if args.output:
        args.output.parent.mkdir(parents=True,exist_ok=True)
        args.output.write_text(txt,encoding="utf-8")
    else:
        print(txt,end="")

if __name__=="__main__":
    main()
