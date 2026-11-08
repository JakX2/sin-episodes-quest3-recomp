#!/usr/bin/env python3
"""Verify the two original SiN 2006 entity factories now backed by C++.

Consumes only a user-owned SHA256-pinned original server.dll. Checks the
actual i386 entity-registry call sequence, literal classname, distinct
factory objects and original ELF-independent pointers. Prints metadata,
never original code/data bytes. No SourceVR ARM64 ABI is inferred.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path

from verify_gameplay_entity_registry import (
    PEImage, VerificationError, SERVER_SHA256, REGISTRY_GETTER
)

ORIGINAL_FACTORIES = (
    ("weapon_assault_rifle",0x223c4f60,0x22477c30,0x2252d4fc),
    ("grenade_ar",0x223c4fc0,0x22477230,0x2252d500),
)

def verify_factory_pair(server: bytes, check_hash=True):
    if check_hash and hashlib.sha256(server).hexdigest()!=SERVER_SHA256:
        raise VerificationError("original 2006 server.dll SHA-256 mismatch")
    pe=PEImage(server)
    if pe.base != 0x22000000:
        raise VerificationError("unexpected original image base")
    rows=[]
    for name,register_va,name_va,factory_va in ORIGINAL_FACTORIES:
        if (pe.section_of(register_va)!=".text" or
            pe.section_of(name_va)!=".rdata" or
            pe.section_of(factory_va)!=".data"):
            raise VerificationError(f"{name}: unexpected PE32 section")
        if pe.at(name_va,len(name)+1)!=name.encode("ascii")+b"\0":
            raise VerificationError(f"{name}: original classname changed")
        code=pe.at(register_va,22)
        if not (code[0]==0xe8 and code[5:8]==b"\x8b\x10\x68" and
                code[12]==0x68 and code[17:]==b"\x8b\xc8\xff\x12\xc3"):
            raise VerificationError(f"{name}: original registry call shape changed")
        getter=register_va+5+struct.unpack_from("<i",code,1)[0]
        pushed_name=struct.unpack_from("<I",code,8)[0]
        pushed_factory=struct.unpack_from("<I",code,13)[0]
        if (getter,pushed_name,pushed_factory)!=(
                REGISTRY_GETTER,name_va,factory_va):
            raise VerificationError(f"{name}: original registry operands changed")
        rows.append({
            "classname":name,
            "registry_getter_va":hex(getter),
            "registration_va":hex(register_va),
            "classname_string_va":hex(name_va),
            "factory_object_va":hex(factory_va),
            "currently_behaviour_backed_in_portable_cpp":True,
            "native_sourcevr_engine_registration_proven":False,
        })
    return {
        "original_server_sha256":SERVER_SHA256,
        "verified_original_registry_entries":rows,
        "note":"Original x86 registration does not create an Android SourceVR engine entity.",
    }

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--server",required=True,type=Path)
    args=p.parse_args()
    try: print(json.dumps(verify_factory_pair(args.server.read_bytes()),indent=2))
    except (OSError,VerificationError,struct.error) as exc:
        p.exit(2,f"Original SiN factory audit FAILED: {exc}\n")

if __name__=="__main__":
    main()
