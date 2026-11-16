#!/usr/bin/env python3
"""Phase245: original SiN (2006) client entity-list cleanup and PVS wrappers.

Whole native-method credit is permitted ONLY after both complete owner-
original retail PE32 DLL SHA256s and all entire-code-body SHA256s match.
Original commercial DLLs / full native disassembly are never committed.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import struct
import zipfile
from pathlib import Path
from audit_sin_original_dll_interface_abi import ORIGINALS, PEImage, VerificationError

# Native method name, start VA, full original body length, complete SHA256,
# following INT3 bytes, all exits (offset, opcode), sparse byte witnesses.
METHODS = (
    ("entitylist_drain_live_entities",0x241061E0,209,
     "371ab22a53c0c0927ca9098f22df381543b2beecb0c75347f2091009dd61b64d",
     15,((0xD0,"c3"),),{
         0x00:"5356578b",0x05:"8b870400",0x15:"83e804c1",
         0x27:"0bf083fe",0x39:"8b138bcb",0x4C:"22518bc4",
         0x5A:"c074128b",0x6B:"c8ff5210",0x76:"e8251f02",
         0xA3:"758fc787",0xA5:"c7871800",0xBC:"00010000",
         0xC3:"c7872400",0xD0:"c3"}),
    ("entitylist_parent_destructor",0x24107430,156,
     "ba31e5b972b0d2a3242e636970ad553c1df5e7e72ab49f3d0ebfd48aa99b8787",
     4,((0x97,"e9a40b0200"),),{
         0x00:"53568bf1",0x05:"c706b873",0x0B:"c7861400",
         0x15:"e896edff",0x1C:"83be6c80",0x2C:"c3740f50",
         0x45:"be448001",0x4C:"e8dfeeff",0x51:"837f08ff",
         0x6B:"8dbe2880",0x74:"28e9ffff",0x78:"837f08ff",
         0x96:"5be9a40b",0x97:"e9a40b02"}),
    ("aux8_recycle_frontend",0x241082C0,37,
     "122e5732b38e8f7d785aa9bcb1c50954a08ebb2f1d6566f8d582cf323aa9454f",
     11,((0x22,"c20400"),),{
         0x00:"56578b7c",0x04:"240c578b",0x09:"e852e9ff",
         0x0E:"668b5610",0x14:"0fb7c766",0x18:"8954c106",
         0x1C:"66897e10",0x22:"c20400"}),
    ("list16_recycle_frontend",0x241082F0,40,
     "8b3136ca2362a79f47968c2c3d648db881b2c50337bd5835493718d87f56027e",
     8,((0x25,"c20400"),),{
         0x00:"56578b7c",0x04:"240c578b",0x09:"e8b2e9ff",
         0x0E:"668b5610",0x15:"b7c7c1e0",0x17:"c1e00466",
         0x1A:"66895408",0x25:"c20400"}),
    ("aux8_allocate_link_payload_frontend",0x24108320,54,
     "af3290321c9fbb9b378386b7106b59344e6d50c5b958295dc1df2e3e342cace4",
     10,((0x33,"c20800"),),{
         0x00:"56576a00",0x02:"6a008bf1",0x06:"e845e2ff",
         0x11:"240c5750",0x15:"e8a6f2ff",0x1B:"160fb7cf",
         0x25:"088b4c24",0x2B:"11891066",0x33:"c20800"}),
    ("list16_allocate_link_frontend",0x24108360,34,
     "ce40641871fb98845468e99cebde1c872d57f1821150414248c74929ace84163",
     14,((0x1F,"c20400"),),{
         0x00:"56576a00",0x02:"6a008bf1",0x06:"e8a5e2ff",
         0x10:"44240c57",0x15:"e8e6f2ff",0x1A:"668bc75f",
         0x1F:"c20400"}),
    ("map_allocate_attach_frontend",0x24108390,42,
     "2d2aad642b4d04a75ee602b01dd872dc26c005d768dbbb5012b6626b45c97392",
     6,((0x27,"c20800"),),{
         0x00:"56578bf1",0x04:"e867e3ff",0x09:"8b4c240c",
         0x0F:"8b442410",0x16:"8bcee833",0x1B:"ffff6683",
         0x22:"668bc75f",0x27:"c20800"}),
)

# Original exact direct CALL relationships including constructor and scalar
# deleting wrapper callers, never inferred from symbol names.
CALLS = (
    (0x24106256,0x24128180),  # entity-list drain -> original remove handle
    (0x24107445,0x241061E0),  # parent destructor -> drain
    (0x24107460,0x2416F940),  # map storage free
    (0x2410747C,0x24106360),  # distinct 16-byte list reset
    (0x2410748E,0x2416F940),  # 16-byte list storage free
    (0x241074A3,0x24105DD0),  # 8-byte auxiliary reset
    (0x241074B5,0x2416F940),  # 8-byte auxiliary storage free
    (0x241082C9,0x24106C20),  # 8-byte list unlink
    (0x241082F9,0x24106CB0),  # 16-byte list unlink
    (0x24108326,0x24106570),  # 8-byte list allocate index
    (0x24108335,0x241075E0),  # 8-byte list link before
    (0x24108366,0x24106610),  # 16-byte list allocate index
    (0x24108375,0x24107660),  # 16-byte list link before
    (0x24108394,0x24106700),  # map allocate index
    (0x241083A8,0x241076E0),  # map attach and balance
    (0x2410828F,0x241061E0),  # owner constructor calls drain
    (0x241082A3,0x24107430),  # prior reviewed scalar deleting destructor
)
TAIL_JMP=(0x241074C7,0x24128070) # last step delegates base destructor

def rel32(op:int,site:int,target:int)->bytes:
    return bytes([op])+struct.pack("<i",target-site-5)

def verify_native(client:PEImage,*,check_hash:bool=True,
                  authentic_provenance:bool=False)->dict:
    if client.base!=ORIGINALS["client"]["base"]:
        raise VerificationError("Phase245 retail original PE image base drift")
    rows=[];anchors=0
    for name,va,size,sha,padding,returns,witnesses in METHODS:
        if client.section_of(va)!=".text" or client.section_of(va+size-1)!=".text":
            raise VerificationError("Phase245 native method outside .text: "+name)
        raw=client.at(va,size)
        if check_hash and hashlib.sha256(raw).hexdigest()!=sha:
            raise VerificationError("Phase245 complete native SHA drift: "+name)
        if client.at(va+size,padding)!=b"\xCC"*padding:
            raise VerificationError("Phase245 original INT3 boundary drift: "+name)
        for offset,hx in returns:
            val=bytes.fromhex(hx)
            if raw[offset:offset+len(val)]!=val:
                raise VerificationError(f"Phase245 native exit drift: {name}+{offset:#x}")
        for offset,hx in witnesses.items():
            val=bytes.fromhex(hx)
            if raw[offset:offset+len(val)]!=val:
                raise VerificationError(f"Phase245 original witness drift: {name}+{offset:#x}")
            anchors+=1
        rows.append({"name":name,"entry_va":f"0x{va:08X}","whole_native_bytes":size,
                     "entire_original_code_SHA256":sha if check_hash else None,
                     "trailing_INT3":padding,"native_exits":len(returns)})
    for site,target in CALLS:
        if client.section_of(site)!=".text" or \
                client.at(site,5)!=rel32(0xE8,site,target):
            raise VerificationError(f"Phase245 original direct E8 CALL drift: {site:#x}")
    site,target=TAIL_JMP
    if client.at(site,5)!=rel32(0xE9,site,target):
        raise VerificationError("Phase245 parent destructor original tail JMP drift")
    return {
        "phase":245,
        "new_original_complete_SHA_methods":len(rows) if check_hash and authentic_provenance else 0,
        "new_original_native_code_bytes":sum(m[2] for m in METHODS)
            if check_hash and authentic_provenance else 0,
        "reviewed_original_methods":rows,
        "sparse_original_opcode_witnesses":anchors,
        "verified_original_direct_E8_calls":len(CALLS),
        "verified_original_tail_JMPs":1,
        "entity_primary_pointer_offset":0x10004,
        "entity_slot_stride":16,
        "entity_serial_shift":12,
        "entity_state_live_count_offset":0x10018,
        "entity_state_second_count_offset":0x10020,
        "entity_state_high_water_offset":0x10024,
        "owner_embedded_aux8_list_offset":0x18028,
        "owner_embedded_aux16_list_offset":0x18044,
        "owner_embedded_map_offset":0x18060,
        "free_head_WORD_offset":0x10,
        "map_count_WORD_offset":0x12,
        "parent_base_destructor_VA":"0x24128070",
        "scope_limits":[
            "Native entity vtable methods and base parent destructor remain opaque.",
            "The original allocator/free helper 0x2416F940 is not recreated.",
            "Client entity-list state is represented by host-owned safe structs.",
            "Original 16-byte/8-byte list link internals are separately reviewed.",
            "No independent rebuilt retail gameplay client/server DLL boot is demonstrated.",
        ],
    }

def audit(server:bytes,client:bytes)->dict:
    for module,data in (("server",server),("client",client)):
        if hashlib.sha256(data).hexdigest()!=ORIGINALS[module]["sha256"]:
            raise VerificationError("Phase245 requires BOTH entire original retail DLL SHA256s: "+module)
    report=verify_native(PEImage(client),authentic_provenance=True)
    if report["new_original_complete_SHA_methods"]!=7 or \
            report["new_original_native_code_bytes"]!=572:
        raise VerificationError("Phase245 authentic whole method ledger drift")
    return report

def main()->None:
    parser=argparse.ArgumentParser(description=__doc__)
    args_group=parser.add_mutually_exclusive_group(required=True)
    args_group.add_argument("--archive",type=Path)
    args_group.add_argument("--server",type=Path)
    parser.add_argument("--client",type=Path)
    a=parser.parse_args()
    if a.archive:
        if a.client:parser.error("--archive excludes --client")
        with zipfile.ZipFile(a.archive) as z:
            if z.namelist().count("server.dll")!=1 or \
                    z.namelist().count("client.dll")!=1:
                raise VerificationError("Phase245 needs exactly one root original DLL pair")
            result=audit(z.read("server.dll"),z.read("client.dll"))
    else:
        if not a.client:parser.error("--server requires --client")
        result=audit(a.server.read_bytes(),a.client.read_bytes())
    print(json.dumps(result,indent=2))

if __name__=="__main__":
    main()
