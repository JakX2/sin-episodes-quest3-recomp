#!/usr/bin/env python3
"""Phase163 original 2006 client descriptor-vector insertion/growth audit.

Owner-only SHA-pinned audit for complete original 0x2400A300 (141 bytes)
and 0x2400B140 (119 bytes). Chains the complete original client/server
SHA verification and all seven Phase162 property registrars. Enforces
27 partial instruction witnesses and 11 x86 direct CALL edges including
all seven real HUD registrations. Outputs metadata, never executable bytes.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import struct

from audit_sin_original_client_timer_property_registrars import audit as phase162_audit
from verify_gameplay_entity_registry import PEImage,VerificationError

REGIONS={
  "native_24byte_descriptor_insert":(0x2400A300,141,
    "3fc3fcbdc8f5f3515e23f4b2cfd0146af268702aa270d6e892935f9a9ecaccf8"),
  "native_descriptor_capacity_growth":(0x2400B140,119,
    "ca54ced9589a979692188164c701cb963a17196ed6bf5ffe886433c5f0fed60f"),
}
# Deliberately sparse code-only witnesses. Full-region checks are hashes;
# synthetic fixtures cannot recreate complete commercial executable bodies.
OPCODES={
  0x2400A303:"8b460c",  # count at +0x0C
  0x2400A306:"8b4e04",  # capacity at +0x04
  0x2400A31E:"83460c01", # native count++ after reserve
  0x2400A325:"8b7c240c", # first argument: insertion index
  0x2400A332:"894e10", # cache pointer into +0x10
  0x2400A337:"8d0440", # shift record count x3
  0x2400A343:"8d0cd1", # source pointer uses index x3
  0x2400A346:"50", # shift size in bytes
  0x2400A354:"8b16", # load storage pointer +0
  0x2400A360:"8b4c2410", # second arg: six-DWORD record input
  0x2400A374:"8b510c", # copy descriptor DWORD offset +12
  0x2400A380:"8b4914", # copy descriptor DWORD offset +20
  0x2400A38A:"c20800", # two caller arguments, RET 8
  0x2400B140:"56",
  0x2400B143:"8b4e08", # growth amount +0x08
  0x2400B146:"83f9ff", # growth == -1 sentinel
  0x2400B152:"03d0", # requested capacity += current
  0x2400B154:"3bc2", # signed capacity compare
  0x2400B15F:"85c9", # fixed increment versus geometric policy
  0x2400B163:"03c1", # add fixed increment
  0x2400B16A:"03c0", # double existing capacity
  0x2400B171:"c7460402000000", # initial capacity 2
  0x2400B17B:"7cdb", # while cap < requested
  0x2400B186:"8d1440", # capacity times 3 then *8 = 24-byte stride
  0x2400B19F:"8d0440",
  0x2400B1B4:"c20400", # growth function cleans 1 argument
}
INTERNAL_CALLS={
  0x2400A319:0x2400B140, # insertion -> growth
  0x2400A34C:0x24248B50, # shift using memmove
  0x2400B191:0x2416F960, # realloc
  0x2400B1A9:0x2416F8F0, # malloc
}
REGISTRAR_CALLS={
  0x24230E37:0x2400A300,
  0x24230EA7:0x2400A300,
  0x24230F17:0x2400A300,
  0x242364A7:0x2400A300,
  0x24236517:0x2400A300,
  0x24236587:0x2400A300,
  0x242365F7:0x2400A300,
}
CALLS={**INTERNAL_CALLS,**REGISTRAR_CALLS}

def _rel32(site:int,target:int)->bytes:
    return b"\xe8"+struct.pack("<i",target-site-5)

def verify_native_descriptor_vector(pe:PEImage,*,check_hashes=True)->dict:
    if pe.base!=0x24000000:
        raise VerificationError("original client.dll image base changed")
    for va,hexbytes in OPCODES.items():
        expected=bytes.fromhex(hexbytes)
        if pe.at(va,len(expected))!=expected:
            raise VerificationError("original descriptor vector x86 instruction mismatch at %#x"%va)
    for site,target in CALLS.items():
        if pe.at(site,5)!=_rel32(site,target):
            raise VerificationError("original descriptor vector rel32 CALL changed at %#x"%site)
    if check_hashes:
        for name,(va,length,digest) in REGIONS.items():
            if hashlib.sha256(pe.at(va,length)).hexdigest()!=digest:
                raise VerificationError("original descriptor vector whole-function SHA mismatch: "+name)
    return {
      "original_function_entry":{
        "descriptor_insert":"0x2400A300",
        "descriptor_capacity_growth":"0x2400B140",
      },
      "verified_complete_function_SHA256s":len(REGIONS) if check_hashes else 0,
      "verified_x86_instruction_anchors":len(OPCODES),
      "verified_original_CALL_edges":len(CALLS),
      "verified_original_timer_registrar_calls":len(REGISTRAR_CALLS),
      "native_record_size_bytes":24,
      "native_element_words":6,
      "native_object_layout":{
        "storage_pointer":"+0x00","capacity":"+0x04",
        "grow_amount":"+0x08","live_count":"+0x0C",
        "cached_storage_pointer":"+0x10",
      },
      "native_growth_policy":[
        "grow_amount == -1: no allocation",
        "capacity=0: initialize capacity to 2",
        "grow_amount=0 and capacity>0: double until enough",
        "grow_amount>0: increment by grow_amount until enough",
        "reallocate existing pointer or allocate new buffer of capacity*24 bytes",
      ],
      "native_insertion_policy":[
        "if count+1>capacity call growth helper with delta (count+1-capacity)",
        "increment count before moving old descriptors",
        "refresh cached pointer +0x10",
        "memmove (old_count-index)*24 bytes by one element for in-range index",
        "copy six raw 32-bit words into inserted element without parsing them",
        "return caller-specified index; original function does not validate it",
      ],
      "limits":[
        "Native C++ array and allocation ownership/lifecycle are NOT restored.",
        "Portable reference rejects dangerous invalid arguments rather than corrupting memory.",
        "Original property descriptor semantic parsing and downstream native VGUI callbacks remain unresolved.",
      ],
    }
def audit(client_bytes:bytes,server_bytes:bytes)->dict:
    out=phase162_audit(client_bytes,server_bytes)
    out["original_client_descriptor_vector"]=verify_native_descriptor_vector(
        PEImage(client_bytes))
    return out
def main()->None:
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--client",required=True,type=Path)
    ap.add_argument("--server",required=True,type=Path)
    ap.add_argument("--output",type=Path)
    args=ap.parse_args()
    result=json.dumps(audit(args.client.read_bytes(),args.server.read_bytes()),indent=2)+"\n"
    if args.output:
        args.output.parent.mkdir(parents=True,exist_ok=True)
        args.output.write_text(result,encoding="utf-8")
    else:
        print(result,end="")
if __name__=="__main__":
    main()
