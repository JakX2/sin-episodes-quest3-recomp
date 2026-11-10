#!/usr/bin/env python3
"""Phase162: original 2006 client.dll seven timer HUD property registrars.

Owner-only original PE32 audit, chained to Phase161's corrected original
DLL SHA256s AND real MSVC RTTI class identities. Each of seven native
96-byte guarded property registration bodies has its OWN exact SHA256.
Validates encoded metadata pointer fields, their actual string contents,
original flag-before-registry execution order and two original CALLs
per function. No executable data or copyrighted assets are output.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
from pathlib import Path
from audit_sin_original_client_timer_constructors import audit as prior_audit
from verify_gameplay_entity_registry import PEImage,VerificationError

# entry, original complete 96-byte code SHA256, 2006 global guard,
# owner string VA, property name VA, C++ member string VA, native type VA,
# original text default VA, resolver VA, original category kind byte
REGISTRATION = (
 (0x24230DE0,"efd0b2d7313e3cc2b6171ced8f771b8688205a6efe5bd4f4a1d903aa7caf64ef",
  0x2437FAA6,0x242C7578,0x242C75E4,0x242C75D0,0x242D3700,0x242A17BC,0x242309F0,0),
 (0x24230E50,"a677055002b590862fb1bae7f3b9803f269f6acee348c7420f9770da01be1419",
  0x2437FAA7,0x242C7578,0x242C7634,0x242C7610,0x242D36E8,0x242C75F8,0x24230A10,1),
 (0x24230EC0,"7cce7da6f28c95bdb469aacb21d0543b14a273b2e0744b1342e63567860a07f4",
  0x2437FAA8,0x242C7578,0x242C7684,0x242C7660,0x242D36E8,0x242C7648,0x24230A30,1),
 (0x24236450,"f580f5a30eb2da9e39f237ce4b82ab4893e6be1d1385a5b465b0685cc32af7a3",
  0x2437FC82,0x242C8E2C,0x242C8E88,0x242C8E74,0x242D3700,0x242C8E64,0x24236030,0),
 (0x242364C0,"391237abf11e211c1c64ef5da5eafc69bdc179e865b7c49926c130875ab0c7a6",
  0x2437FC83,0x242C8E2C,0x242C8ED8,0x242C8EB4,0x242D36E8,0x242C8E9C,0x24236050,1),
 (0x24236530,"dfc9a2b9b13923550539a0b052199dbaf15e7486a8471f44646d846eaff3497c",
  0x2437FC84,0x242C8E2C,0x242C8F2C,0x242C8F08,0x242D36E8,0x242C8EF0,0x24236070,1),
 (0x242365A0,"75881836e013ae46e23c1cb8e375b64c49da116cdc9390441c4184af2581451f",
  0x2437FC85,0x242C8E2C,0x242C8F7C,0x242C8F5C,0x242D36E8,0x242C8F44,0x24236090,1),
)
ORIGINAL_STRINGS={
 0x242C7578:"CHudBreathTimer",0x242C8E2C:"CHudPoisonTimer",
 0x242C75E4:"BreathTimerColor",0x242C75D0:"m_breathTimerColor",
 0x242C7634:"BreathBarForeground",0x242C7610:"m_breathBarForegroundTextureName",
 0x242C7684:"BreathBarBackground",0x242C7660:"m_breathBarBackgroundTextureName",
 0x242C8E88:"PoisonTimerColor",0x242C8E74:"m_poisonTimerColor",
 0x242C8ED8:"MutagenBarForeground",0x242C8EB4:"m_mutagenBarForegroundTextureName",
 0x242C8F2C:"MutagenBarBackground",0x242C8F08:"m_mutagenBarBackgroundTextureName",
 0x242C8F7C:"MutagenBarInactive",0x242C8F5C:"m_mutagenBarInactiveTextureName",
 0x242D3700:"Color",0x242D36E8:"string",
 0x242A17BC:"0 0 0 0",
 0x242C75F8:"Breath_bar_foreground",0x242C7648:"Breath_bar_background",
 0x242C8E64:"255 255 255 0",
 0x242C8E9C:"Mutagen_bar_foreground",0x242C8EF0:"Mutagen_bar_background",
 0x242C8F44:"Mutagen_bar_inactive",
}
# original constructor CALL order, already covered by Phase161 but
# additionally checked in chronological registration sequence
CONSTRUCTOR_CALLS={
 0x24230FC2:0x24230DE0,0x24230FDF:0x24230E50,
 0x24230FE4:0x24230EC0,
 0x242366A2:0x24236450,0x242366BF:0x242364C0,
 0x242366C4:0x24236530,0x242366C9:0x242365A0,
}
# Native registration descriptor store positions inside every 96-byte body
# (x86 MOV DWORD PTR [esp+<offset>], <absolute constant>)
STORES=((0x2A,b"\xc7\x44\x24\x08",4),
        (0x32,b"\xc7\x44\x24\x0c",5),
        (0x3A,b"\xc7\x44\x24\x10",6),
        (0x42,b"\xc7\x44\x24\x14",7),
        (0x4A,b"\xc7\x44\x24\x1c",8))

def _call(site,target):
    return b"\xe8"+struct.pack("<i",target-site-5)

def verify_original_registrars(pe:PEImage,*,check_hashes=True)->dict:
    if pe.base!=0x24000000:
        raise VerificationError("original 2006 client.dll image base mismatch")
    for address,value in ORIGINAL_STRINGS.items():
        literal=value.encode("ascii")+b"\x00"
        if pe.at(address,len(literal))!=literal:
            raise VerificationError("original timer HUD native string mismatch at %#x"%address)
    names=[]
    for record in REGISTRATION:
        (entry,sha,guard,owner,prop,member,typ,default,accessor,kind)=record
        if pe.at(entry+3,7)!=b"\x80\x3d"+struct.pack("<I",guard)+b"\x00":
            raise VerificationError("original property guard CMP changed at %#x"%entry)
        if pe.at(entry+0x0A,2)!=b"\x75\x50":
            raise VerificationError("original one-time guard branch changed at %#x"%entry)
        if pe.at(entry+0x0C,5)!=b"\x68"+struct.pack("<I",owner):
            raise VerificationError("original registration owner class mismatch at %#x"%entry)
        if pe.at(entry+0x11,7)!=b"\xc6\x05"+struct.pack("<I",guard)+b"\x01":
            raise VerificationError("original guard must be set before registry call at %#x"%entry)
        if pe.at(entry+0x18,5)!=_call(entry+0x18,0x24011DC0):
            raise VerificationError("original registry object lookup CALL changed at %#x"%entry)
        for offset,prefix,idx in STORES:
            # store operands occupy +4..+8 relative to opcode
            if pe.at(entry+offset,8)!=prefix+struct.pack("<I",record[idx]):
                raise VerificationError("original property descriptor field changed at %#x"%(entry+offset))
        if pe.at(entry+0x52,5)!=b"\xc6\x44\x24\x18"+bytes((kind,)):
            raise VerificationError("original registration kind byte changed at %#x"%entry)
        if pe.at(entry+0x57,5)!=_call(entry+0x57,0x2400A300):
            raise VerificationError("original native Color/string property dispatch changed at %#x"%entry)
        if pe.at(entry+0x5C,4)!=b"\x83\xc4\x18\xc3":
            raise VerificationError("original 96-byte property registration return changed")
        if check_hashes and hashlib.sha256(pe.at(entry,96)).hexdigest()!=sha:
            raise VerificationError("original full property registrar SHA256 mismatch at %#x"%entry)
        names.append(ORIGINAL_STRINGS[prop])
    for site,target in CONSTRUCTOR_CALLS.items():
        if pe.at(site,5)!=_call(site,target):
            raise VerificationError("original timer constructor property CALL order mismatch at %#x"%site)
    return {
      "verified_original_full_registrar_SHA256s":len(REGISTRATION) if check_hashes else 0,
      "original_constructor_registration_call_edges":len(CONSTRUCTOR_CALLS),
      "original_direct_registry_calls":len(REGISTRATION)*2,
      "verified_original_metadata_string_literals":len(ORIGINAL_STRINGS),
      "original_registration_order":names,
      "named_colors":{
        "breath":{"member":"m_breathTimerColor","default":"0 0 0 0","panel_offset":"0xA3"},
        "poison":{"member":"m_poisonTimerColor","default":"255 255 255 0","panel_offset":"0xA3"}},
      "native_one_time_guard_order":"CMP guard -> if zero SET guard=1 -> lookup -> descriptor dispatch",
      "limitations":[
        "Native registration class object and source-compatible VGUI property parser are NOT reconstructed.",
        "Class ownership verified by original constructor RTTI in prerequisite Phase161 audit.",
        "The original panel +0xA6 Color fourth byte is not a separately declared gameplay state member.",
        "Original scripts/configuration can override the default text after registration.",
      ],
    }

def audit(client_bytes:bytes,server_bytes:bytes)->dict:
    result=prior_audit(client_bytes,server_bytes)
    result["original_timer_property_registrars"]=verify_original_registrars(
        PEImage(client_bytes))
    return result

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--client",type=Path,required=True)
    p.add_argument("--server",type=Path,required=True)
    p.add_argument("--output",type=Path)
    args=p.parse_args()
    data=json.dumps(audit(args.client.read_bytes(),args.server.read_bytes()),indent=2)+"\n"
    if args.output:
        args.output.parent.mkdir(parents=True,exist_ok=True)
        args.output.write_text(data,encoding="utf-8")
    else:
        print(data,end="")
if __name__=="__main__":main()
