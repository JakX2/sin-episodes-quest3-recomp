#!/usr/bin/env python3
"""Phase1662: exact native C_BasePlayer orientation getter/setter evidence.

This source-free SHA-gated audit proves the four ORIGINAL C_BasePlayer
primary-vtable targets used by authenticated CPrediction slots9-12.
It never executes original retail code; --self-test uses invented,
non-executable sparse instruction windows with mutation checks.
No original complete native function body is copied to the repository.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import struct

from audit_sin_original_client_prediction23_abi import (
    CLIENT_BASE,verify_original,check,section_read,
)
from audit_sin_original_client_prediction_view_readers import (
    PLAYER_PRIMARY_VTABLE_VA,PLAYER_PRIMARY_RTTI_PTR_VA,PLAYER_RTTI_COL_VA,
)
from audit_sin_original_dll_interface_abi import PEImage
from audit_sin_original_core_factory_thunks import pe_highlow_relocations

# Original C_BasePlayer primary table slots (offsets +0xE4..+0xF0).
# (slot, original method VA, extent, complete native SHA256, player field)
GETTERS=(
    (57,0x240C5190,7,
     "58beed9442fa84e2898a6468a1096bece7c950612ede928be4c9c05d280b198e",0x220),
    (59,0x240C5260,7,
     "6a8d91e1da08718432a2b232c0bd73c760a9437e0182a6a3a39529cbdb21f11b",0x25C),
)
# Original native setter compares all three floats with x87 FUCOMPP;
# equal three floats (including +0/-0) -> no dirty signal/no writes.
# NaN yields an unordered compare, causing the changed path.
# When changed, helper 0x24096290 receives 1/2 via push imm8,
# BEFORE native copies three DWORDs into the player object.
SETTERS=(
    (58,0x240C51B0,101,
     "3f4bb4faf68496fd5f46ccde0790bcb116909152c50e74c7b124dd3b451fd5b3",
     0x220,1),
    (60,0x240C5280,101,
     "a0de64a8e7a4a04f9f85f423e922323163a13f73b36c010eacfe1a4942db953e",
     0x25C,2),
)
DIRTY_HELPER_VA=0x24096290
GETTER_POP=0
SETTER_POP=4

def u32(blob: bytes,off: int=0)->int:
    return struct.unpack_from("<I",blob,off)[0]

def pack(value: int)->bytes:
    return struct.pack("<I",value)

def verify_getter(blob: bytes,field: int)->None:
    check(len(blob)==7 and blob[:2]==b"\x8D\x81" and
          u32(blob,2)==field and blob[6]==0xC3,
          "C_BasePlayer primary getter must LEA this+field, RET0")

def verify_setter(blob: bytes,field: int,flag: int,
                  address: int,dirty_helper: int)->None:
    check(len(blob)==101,"original player setter must be 101 bytes")
    checks=(
       (0,bytes.fromhex("56578b7c240c8bf1")),
       (8,b"\xD9\x86"+pack(field)),
       (20,bytes.fromhex("f6c4447a24")),
       (25,b"\xD9\x86"+pack(field+4)),
       (38,bytes.fromhex("f6c4447a12")),
       (43,b"\xD9\x86"+pack(field+8)),
       (56,bytes.fromhex("f6c4447b23")),
       (61,b"\x6A"+bytes((flag,))+bytes.fromhex("8bcee8")),
       (72,b"\x89\x86"+pack(field)),
       (81,b"\x89\x8E"+pack(field+4)),
       (90,b"\x89\x96"+pack(field+8)),
       (96,bytes.fromhex("5f5ec20400")),
    )
    for pos,evidence in checks:
        check(blob[pos:pos+len(evidence)]==evidence,
              "native three-float unordered/equality, dirty-first, "
              "three DWORD mirror, or RET4 witness changed at "+hex(pos))
    delta=struct.unpack_from("<i",blob,66)[0]
    check(address+70+delta==dirty_helper,
          "player setter dirty helper native rel32 target is not pinned")
    # Explicitly document reason for the strange x87 parity branch:
    # comparing C3|C2 (AH bits 6|2) then JP branches on
    # unequal or unordered; all three ordered equal -> no-op.
    # This behavior cannot be modeled by raw 32-bit inequality
    # because +/-0 compare equal and NaNs compare unordered.

def ieee_equal(a: int,b: int)->bool:
    """Reference for original x87 FUCOMPP equality without host FP modes."""
    amag=a & 0x7FFFFFFF
    bmag=b & 0x7FFFFFFF
    if amag>0x7F800000 or bmag>0x7F800000:
        return False
    if amag==0 and bmag==0:
        return True
    return a==b

def vector_changed(before: tuple[int,int,int],
                   after: tuple[int,int,int])->bool:
    return any(not ieee_equal(a,b) for a,b in zip(before,after))

def audit(client_bytes: bytes,engine_bytes: bytes)->dict:
    parent=verify_original(client_bytes,engine_bytes)
    check(parent["original_virtual_slots"]==23,
          "prior original prediction vtable provenance missing")
    pe=PEImage(client_bytes)
    check(pe.base==CLIENT_BASE,
          "unexpected original 2006 PE32 client base")
    reloc=pe_highlow_relocations(pe)
    check(section_read(pe,PLAYER_PRIMARY_RTTI_PTR_VA,4,".rdata")==
          pack(PLAYER_RTTI_COL_VA),
          "wrong original primary C_BasePlayer vtable COL")
    verified=[]
    for slot,va,n,digest,field in GETTERS:
        tablecell=PLAYER_PRIMARY_VTABLE_VA+slot*4
        check(tablecell in reloc and
              section_read(pe,tablecell,4,".rdata")==pack(va),
              "C_BasePlayer getter original primary vtable slot mismatch")
        native=section_read(pe,va,n,".text")
        check(hashlib.sha256(native).hexdigest()==digest,
              "entire native player getter SHA mismatch: "+str(slot))
        verify_getter(native,field)
        check(section_read(pe,va+n,2,".text")==b"\xCC\xCC",
              "native getter function boundary not INT3 padded")
        verified.append({"player_slot":slot,
                         "virtual_offset":hex(slot*4),
                         "method_va":hex(va),
                         "method_bytes":n,
                         "whole_sha256":digest,
                         "getter_field":hex(field),
                         "stack_pop":GETTER_POP})
    for slot,va,n,digest,field,dirty_flag in SETTERS:
        tablecell=PLAYER_PRIMARY_VTABLE_VA+slot*4
        check(tablecell in reloc and
              section_read(pe,tablecell,4,".rdata")==pack(va),
              "C_BasePlayer setter original primary vtable slot mismatch")
        native=section_read(pe,va,n,".text")
        check(hashlib.sha256(native).hexdigest()==digest,
              "entire native player setter SHA mismatch: "+str(slot))
        verify_setter(native,field,dirty_flag,va,DIRTY_HELPER_VA)
        check(section_read(pe,va+n,2,".text")==b"\xCC\xCC",
              "native setter function boundary not INT3 padded")
        verified.append({"player_slot":slot,
                         "virtual_offset":hex(slot*4),
                         "method_va":hex(va),
                         "method_bytes":n,
                         "whole_sha256":digest,
                         "setter_field":hex(field),
                         "dirty_helper":hex(DIRTY_HELPER_VA),
                         "dirty_flag":dirty_flag,
                         "stack_pop":SETTER_POP})
    return {
        "phase":1662,
        "retail_original_SHA_gate":"unchanged full client.dll + engine.dll",
        "original_player_class":"C_BasePlayer",
        "original_primary_vtable_va":hex(PLAYER_PRIMARY_VTABLE_VA),
        "four_player_virtual_method_windows":verified,
        "additional_authenticated_entire_method_bytes":216,
        "getter_return_address_this_plus_offset":True,
        "setter_three_ieee754_numeric_equal_noop":True,
        "setter_nan_unordered_is_changed":True,
        "setter_signed_zero_equal_noop":True,
        "dirty_called_before_any_three_dword_writes":True,
        "no_original_native_methods_executed":True,
        "gameplay_startup_verified":False,
    }

def reject(action,what: str)->None:
    try:action()
    except ValueError:return
    raise AssertionError("corruption incorrectly accepted: "+what)

def mutation_windows() -> None:
    fake_va=0x49110000
    fake_helper=0x49113000
    for field in (0x220,0x25C):
        fake_get=b"\x8D\x81"+pack(field)+b"\xC3"
        verify_getter(fake_get,field)
        for i in range(len(fake_get)):
            corrupt=bytearray(fake_get);corrupt[i]^=1
            reject(lambda data=bytes(corrupt):verify_getter(data,field),
                   "synthetic player getter byte "+str(i))
    for field,flag in ((0x220,1),(0x25C,2)):
        # Non-runnable invented method: 101-byte INT3 buffer populated
        # only with sparse independent architectural witnesses.
        fake=bytearray(b"\xCC"*101)
        chunks=(
            (0,bytes.fromhex("56578b7c240c8bf1")),
            (8,b"\xD9\x86"+pack(field)),
            (20,bytes.fromhex("f6c4447a24")),
            (25,b"\xD9\x86"+pack(field+4)),
            (38,bytes.fromhex("f6c4447a12")),
            (43,b"\xD9\x86"+pack(field+8)),
            (56,bytes.fromhex("f6c4447b23")),
            (61,b"\x6A"+bytes((flag,))+bytes.fromhex("8bcee8")+
                 struct.pack("<i",fake_helper-fake_va-70)),
            (72,b"\x89\x86"+pack(field)),
            (81,b"\x89\x8E"+pack(field+4)),
            (90,b"\x89\x96"+pack(field+8)),
            (96,bytes.fromhex("5f5ec20400")),
        )
        touched=set()
        for pos,src in chunks:
            fake[pos:pos+len(src)]=src
            touched.update(range(pos,pos+len(src)))
        verify_setter(bytes(fake),field,flag,fake_va,fake_helper)
        for i in sorted(touched):
            corrupt=bytearray(fake);corrupt[i]^=1
            reject(lambda data=bytes(corrupt):verify_setter(
                data,field,flag,fake_va,fake_helper),
                "synthetic player setter witness byte "+str(i))
        reject(lambda:verify_setter(bytes(fake),field,flag^3,
                                    fake_va,fake_helper),
               "original dirty flag changed")
        reject(lambda:verify_setter(bytes(fake),field,flag,
                                    fake_va,fake_helper+1),
               "original dirty helper rel32 target changed")
        reject(lambda:verify_setter(bytes(fake[:-1]),field,flag,
                                    fake_va,fake_helper),
               "short fake original method")

def self_test()->None:
    mutation_windows()
    vzero=(0x00000000,0x00000000,0x00000000)
    check(not vector_changed(vzero,(0x80000000,0,0)),
          "+0.0 vs -0.0 must be equal in original x87 comparisons")
    check(vector_changed(vzero,(0x7FC01234,0,0)) and
          vector_changed((0x7FC01234,0,0),(0x7FC01234,0,0)),
          "any NaN input must be treated as changed, even identical bits")
    check(not vector_changed((0x7F800000,0xFF800000,0x1),
                             (0x7F800000,0xFF800000,0x1)) and
          vector_changed((0x7F800000,0,0),(0xFF800000,0,0)),
          "infinite +/- and finite equality x87 reference mismatch")
    reject(lambda:audit(b"fake client",b"fake engine"),
           "owner original complete-SHA bypass")
    print("PASS Phase1662 original-free: C_BasePlayer primary slots"
          "57/58/59/60, +0x220/+0x25C address getters RET0, "
          "three-float x87 equality/+zero/-zero/NaN changed semantics, "
          "dirty helper 0x24096290 flag1/2 BEFORE three stores "
          "RET4, sparse native mutation tests and fake SHA refusal")

def main()->int:
    a=argparse.ArgumentParser(description=__doc__)
    a.add_argument("--self-test",action="store_true")
    a.add_argument("--client",type=Path)
    a.add_argument("--engine",type=Path)
    args=a.parse_args()
    if args.self_test:
        if args.client is not None or args.engine is not None:
            a.error("--self-test cannot combine with owner binary paths")
        self_test()
    else:
        if args.client is None or args.engine is None:
            a.error("original audit needs BOTH --client and --engine")
        print(json.dumps(audit(args.client.read_bytes(),
                               args.engine.read_bytes()),indent=2,
                         sort_keys=True))
    return 0
if __name__=="__main__":
    raise SystemExit(main())
