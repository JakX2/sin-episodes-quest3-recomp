#!/usr/bin/env python3
"""Phase1690: source-authenticate original 2006 PE32 VCENTERPRINT002
factory, object constructor, all eight complete original methods,
cold nullable backing layout and original engine x86 callsites.

Owner original mode requires complete retail engine.dll+client.dll
SHA256, and never executes game code. Public --self-test uses ONLY
invented little-endian vtable slots + synthetic source byte windows.
No proprietary original instruction dump is emitted into Git.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
import zipfile
from pathlib import Path
from audit_sin_original_dll_interface_abi import (
    ORIGINALS,PEImage,VerificationError,
)
from audit_sin_original_core_factory_thunks import (
    pe_highlow_relocations,
)

ENGINE_SHA="f3add5b41300e6d709fd0b492bfcdd37b9b3d20d9744f874c0b860bf021cec81"
CLIENT_SHA=ORIGINALS["client"]["sha256"]
K_NAME="VCENTERPRINT002"
K_FACTORY=0x241EC540
K_GLOBAL=0x2431DBF0
K_CTOR=0x241EC5E0
K_VTABLE=0x242ABC4C
K_VTABLE_SHA="35451d0a78e637ba3a333d8f942768cf1a938954277b92ab226ab98fff92edcb"
K_CTOR_SHA="2d512dbd13741df5f0c12d36879081b847815f0768b58eefca9df1bfe9da2d06"
K_FACTORY_SHA="81267ad71e6a917e5251cb606e3180b7d36d219e6be716ed03bc97674e9c0aca"
# (original vtable index, entry VA, COMPLETE original function length,
# full original whole-body SHA, native RET callee stack bytes,
# location of final RET within the method)
SLOTS=(
  (0,0x241EC430,20,
   "834a49a193899c74d545c9a146f09f05e8c15d0d1fc6bccc21b6718d58b539ea",16,17),
  (1,0x241EC480,45,
   "e7454b96e206680f1c1d327702d7444d30f30f0cdb6fc925db580294a99ea71d",4,42),
  (2,0x241EC450,45,
   "0dd19752b1de31efc2d868b2d7587f0854d36288e3d6d7fa692486bc4e8b6d15",4,42),
  (3,0x241EC4D0,20,
   "be035df1ddc71e71c4e2ffd110025df459375c4e0ec3ff99ccab8972ce1e2f52",20,17),
  (4,0x241EC4B0,20,
   "8c7b951c398ba5f0fa36964c2227a7bbc1ebe39fa410b0af6b7c4521075d4b22",20,17),
  (5,0x241EC4F0,18,
   "60bcedfa9a47449dedc44c9466556cfe805437dfbc94be3e2b6cce854218b783",0,17),
  (6,0x241EC760,59,
   "06c40b8e3ae115ea0a0ac197deb2638e828f73ddc0da5e16547f3136da5e16a4",4,56),
  (7,0x241EC510,43,
   "f0d8ceffc860e87d455d2b6781db940593e046b7ad67f7339d5ee4e7857c2b0b",0,42),
)
# Original engine callsites already located by source .text xrefs.
ENGINE_XREFS=(
 (0x200AA539,15,
  "a6b23915c43362bff5232308e260ed3e3782a2b41c809a514dbc2557821751b5",5),
 (0x200AA600,34,
  "36e10b728d6b051ddc80f6d46dc0187cf811e33ed7cb32a0c1033da1ceeead0c",4),
 (0x200AA630,16,
  "ccc47598cef68fa6fc3dbf9d72ebbe56bf0dee595f1bf6573f9aa00137e9fd37",5),
 (0x20150D56,19,
  "b26177313caa60a0cf814a2e1db85ec864759755faf610b6ea5ad50c7bf96cbb",5),
)
ENGINE_CENTERPRINT_GLOBAL=0x203E401C
ENGINE_PRECEDING_FACTORY=0x20023D4A
# Original slot6 uses 0xF8-byte native backing allocation and then
# constructor 0x241EC5F0. We do not recreate or infer this service.
K_BACKING_ALLOCATION=0xF8
K_BACKING_CTOR=0x241EC5F0

def check(ok:bool,what:str)->None:
    if not ok: raise VerificationError("Phase1690 original VCENTERPRINT002: "+what)

def u32(v:int)->bytes:return struct.pack("<I",v)

def check_vtable_shape(raw:bytes)->None:
    check(len(raw)==32,"eight DWORD vtable span missing")
    check(tuple(struct.unpack("<8I",raw))==tuple(s[1] for s in SLOTS),
          "eight source virtual entries order drift")

def check_method_abi(slot:int,raw:bytes)->None:
    index,address,length,digest,pop,ret_offset=SLOTS[slot]
    check(len(raw)==length,"source method length drift")
    if pop==0:
        check(raw[ret_offset:ret_offset+1]==b"\xc3",
              "native zero-stack RET mismatch")
    else:
        check(raw[ret_offset:ret_offset+3]==b"\xc2"+struct.pack("<H",pop),
              "native RET cleanup differs from requested byte count")
    if slot in (0,1,2,3,4,5):
        check(raw[0:4]==b"\x83\x79\x04\x00",
              "null backing pointer guard removed")
    if slot==6:
        check(raw[0:3]==b"\x56\x8b\xf1" and
              b"\x68\xf8\x00\x00\x00" in raw,
              "native service allocation guard or 0xF8 backend changed")
        call_site=0x241EC786
        delta=K_BACKING_CTOR-(call_site+5)
        check(raw[call_site-address:call_site-address+5]==
              b"\xe8"+struct.pack("<i",delta),
              "slot6 must invoke original backend constructor")
    if slot==7:
        check(raw[0:3]==b"\x56\x8b\xf1" and
              b"\xc7\x46\x04\x00\x00\x00\x00" in raw,
              "slot7 must clear backing pointer")

def verify_original(client_bytes:bytes,engine_bytes:bytes)->dict:
    check(hashlib.sha256(client_bytes).hexdigest()==CLIENT_SHA,
          "entire retail client.dll provenance mismatch")
    check(hashlib.sha256(engine_bytes).hexdigest()==ENGINE_SHA,
          "entire retail engine.dll provenance mismatch")
    c=PEImage(client_bytes);e=PEImage(engine_bytes)
    check(c.base==0x24000000 and e.base==0x20000000,
          "expected original two 2006 PE32 image bases")
    reloc=pe_highlow_relocations(c)
    factory=c.at(K_FACTORY,6)
    check(hashlib.sha256(factory).hexdigest()==K_FACTORY_SHA and
          factory==b"\xb8"+u32(K_GLOBAL)+b"\xc3",
          "original factory not an exact static global thunk")
    check(K_FACTORY+1 in reloc,"original factory pointer HIGHLOW absent")
    ctor=c.at(K_CTOR,16)
    check(hashlib.sha256(ctor).hexdigest()==K_CTOR_SHA and
          ctor[2:8]==b"\xc7\x00"+u32(K_VTABLE) and
          ctor[8:15]==b"\xc7\x40\x04\x00\x00\x00\x00" and
          ctor[15:]==b"\xc3",
          "source virtual constructor does not init 8-vtable+nullable backing")
    check(K_CTOR+4 in reloc,"native vtable constructor relocation missing")
    check(c.at(K_GLOBAL,8)==u32(K_VTABLE)+u32(0),
          "original static global not constructed with null backing")
    check(K_GLOBAL in reloc,"original writable vptr relocation absent")
    table=c.at(K_VTABLE,32)
    check(hashlib.sha256(table).hexdigest()==K_VTABLE_SHA,
          "whole original eight-slot vtable SHA changed")
    check_vtable_shape(table)
    for index,address,length,digest,pop,ret_offset in SLOTS:
        check(K_VTABLE+index*4 in reloc,
              "missing original relocated virtual slot "+str(index))
        raw=c.at(address,length)
        check(hashlib.sha256(raw).hexdigest()==digest,
              "full original client function SHA changed for slot "+str(index))
        check_method_abi(index,raw)
    # The source engine stores the interface in this exact global,
    # then dispatches cold-state clear and colored print methods.
    for va,length,digest,index in ENGINE_XREFS:
        raw=e.at(va,length)
        check(hashlib.sha256(raw).hexdigest()==digest,
              "engine usage entire x86 source window changed at "+hex(va))
        check(raw[0:2]==b"\x8b\x0d" or raw[0:2]==b"\xa1\xd4",
              "engine xref source no longer accesses interface state")
        check(u32(ENGINE_CENTERPRINT_GLOBAL) in raw,
              "engine no longer reads original centerprint interface")
    return {
       "phase":1690,"retail_original_SHA_authenticated":True,
       "original_client_factory":hex(K_FACTORY),
       "original_centerprint_object":hex(K_GLOBAL),
       "original_client_virtual_table":hex(K_VTABLE),
       "original_8_virtual_methods":len(SLOTS),
       "original_native_cleanup_bytes":[s[4] for s in SLOTS],
       "original_nullable_service_pointer_offset":4,
       "original_slot6_backend_allocation_bytes":K_BACKING_ALLOCATION,
       "source_engine_xrefs":[hex(x[0]) for x in ENGINE_XREFS],
       "scope":"read-only original ABI authentication; no gameplay or UI backend",
    }

def make_synthetic_method(index:int)->bytes:
    _,address,length,_,pop,offset=SLOTS[index]
    raw=bytearray(length)
    if index<6:raw[:4]=b"\x83\x79\x04\x00"
    if index==6:
        raw[:3]=b"\x56\x8b\xf1"
        raw[6:11]=b"\x68\xf8\x00\x00\x00"
        place=0x241EC786-address
        raw[place:place+5]=b"\xe8"+struct.pack("<i",K_BACKING_CTOR-0x241EC78B)
    if index==7:
        raw[:3]=b"\x56\x8b\xf1"
        raw[14:21]=b"\xc7\x46\x04\x00\x00\x00\x00"
    raw[offset:offset+(1 if pop==0 else 3)]=(
      b"\xc3" if pop==0 else b"\xc2"+struct.pack("<H",pop))
    return bytes(raw)

def self_test()->dict:
    table=b"".join(u32(x[1]) for x in SLOTS)
    check_vtable_shape(table)
    changed=0
    for i in range(len(table)):
        v=bytearray(table);v[i]^=1
        try:check_vtable_shape(bytes(v))
        except VerificationError:changed+=1
        else:raise VerificationError("mutated synthetic vtable accepted")
    for slot in range(len(SLOTS)):
        original=make_synthetic_method(slot)
        check_method_abi(slot,original)
        # Sparse native critical operands are always checked.
        probes=(
          [0,1,2,3] if slot<6 else
          [0,1,2,6,7,8,9,10,0x241EC786-SLOTS[slot][1]]
            if slot==6 else [0,1,2,14,15,16,17,18,19,20]
        )
        probes+=list(range(SLOTS[slot][5],
                           SLOTS[slot][5]+(1 if SLOTS[slot][4]==0 else 3)))
        for at in sorted(set(probes)):
            if at>=len(original):continue
            bad=bytearray(original);bad[at]^=1
            try:check_method_abi(slot,bytes(bad))
            except VerificationError:changed+=1
            else:
                raise VerificationError(
                    "mutated native ABI witness accepted: slot "+str(slot)+
                    " byte "+str(at))
    check(changed>=32+8*4,"insufficient original-free mutation probes")
    return {"phase":1690,"public_synthetic_mutation_cases_rejected":changed,
            "original_game_bytes_embedded":False,
            "original_retail_module_executed":False,
            "native_source8_methods_authenticated":False}

def main()->None:
    p=argparse.ArgumentParser(description=__doc__)
    source=p.add_mutually_exclusive_group(required=True)
    source.add_argument("--self-test",action="store_true")
    source.add_argument("--archive",type=Path)
    source.add_argument("--client",type=Path)
    p.add_argument("--engine",type=Path)
    a=p.parse_args()
    if a.self_test:
        if a.engine:p.error("--self-test does not take --engine")
        result=self_test()
    elif a.archive:
        if not a.engine:p.error("--archive requires --engine")
        with zipfile.ZipFile(a.archive) as z:
            check(z.namelist().count("client.dll")==1,
                  "archive missing original client.dll")
            result=verify_original(z.read("client.dll"),a.engine.read_bytes())
    else:
        if not a.engine:p.error("--client requires --engine")
        result=verify_original(a.client.read_bytes(),a.engine.read_bytes())
    print(json.dumps(result,indent=2))

if __name__=="__main__":
    main()
