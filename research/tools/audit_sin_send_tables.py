#!/usr/bin/env python3
"""Read original 2006 SiN server send-table constructor sites, compare to client.

Strictly recognizes a verified x86 MSVC initialization pattern. Entries not
matching it are explicitly left unresolved; no Source SDK C++ ABI is guessed.
"""
from __future__ import annotations
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import re
import struct

from verify_gameplay_entity_registry import SERVER_SHA256, CLIENT_SHA256, PEImage, VerificationError
from audit_sin_recv_tables import (
    section_data, strings_by_va, NAME_RE, PROPERTY_RE,
    scan_client_tables, scan_client_properties,
)

SERVER_BASE = 0x22000000
SEND_TABLE_CTOR = 0x2225E850
SEND_PROP_STRIDE = 0x4C


def scan_server_tables(server: bytes, *, check_hash: bool = True) -> list[dict]:
    if check_hash and hashlib.sha256(server).hexdigest() != SERVER_SHA256:
        raise VerificationError("original SiN server.dll SHA-256 mismatch")
    pe = PEImage(server)
    if pe.base != SERVER_BASE:
        raise VerificationError("unexpected original SiN server PE image base")
    names = strings_by_va(pe, ".rdata", NAME_RE)
    codeva, code = section_data(pe, ".text")
    out = []
    for match in re.finditer(rb"\xb9", code):
        i = match.start()
        if i < 12 or i+10 > len(code) or code[i+5] != 0xE8:
            continue
        ctor = codeva+i+10+struct.unpack_from("<i",code,i+6)[0]
        if ctor != SEND_TABLE_CTOR:
            continue
        sendtable = struct.unpack_from("<I", code, i+1)[0]
        if pe.section_of(sendtable) != ".data" or code[i-5] != 0x68:
            continue
        props = struct.unpack_from("<I",code,i-4)[0]
        if pe.section_of(props) != ".data":
            continue
        j = i - 5
        if code[j-2] == 0x6A:
            count, k = code[j-1], j-2
        elif code[j-5] == 0x68:
            count, k = struct.unpack_from("<I",code,j-4)[0], j-5
        else:
            continue
        if count > 256 or k < 6:
            continue
        # The server uses a global pointer to a string rather than the
        # client's immediate push. Confirm mov eax,[name_global];push eax.
        if code[k-6] != 0xA1 or code[k-1] != 0x50:
            continue
        name_global = struct.unpack_from("<I",code,k-5)[0]
        if pe.section_of(name_global) != ".data":
            continue
        name_va = struct.unpack("<I",pe.at(name_global,4))[0]
        name = names.get(name_va)
        if name is None:
            continue
        if count and pe.section_of(props+(count-1)*SEND_PROP_STRIDE) != ".data":
            continue
        out.append({
            "table": name, "registration_va": f"0x{codeva+i:08x}",
            "property_count": count,
            "properties_va": f"0x{props:08x}",
            "send_table_va": f"0x{sendtable:08x}",
            "global_name_pointer_va": f"0x{name_global:08x}",
            "constructor_va": f"0x{ctor:08x}",
            "_props": props,
        })
    if len(set(row["registration_va"] for row in out)) != len(out):
        raise VerificationError("duplicate send table constructor location")
    if check_hash:
        anchors = {
            "DT_LocalWeaponData": (5, 0x22591474),
            "DT_BaseEntity": (24, 0x22558E4C),
            "DT_BaseViewModel": (16, 0x22575974),
        }
        for name,(count,props) in anchors.items():
            if not any(x["table"]==name and x["property_count"]==count
                       and x["_props"]==props for x in out):
                raise VerificationError(f"original SiN server sendtable anchor mismatch: {name}")
    return out


def scan_server_properties(server: bytes, tables: list[dict]) -> tuple[list[dict], list[dict]]:
    pe=PEImage(server)
    codeva,code=section_data(pe,".text")
    names=strings_by_va(pe,".rdata",PROPERTY_RE)
    slots={}
    for tbl in tables:
        for index in range(tbl["property_count"]):
            slots[tbl["_props"]+index*SEND_PROP_STRIDE] = (tbl["table"],index,tbl["registration_va"])
    found={}
    for match in re.finditer(rb"\x68",code):
        i=match.start()
        if i+15>len(code) or code[i+5]!=0x68 or code[i+10]!=0xE8:
            continue
        nva=struct.unpack_from("<I",code,i+1)[0]
        slot=struct.unpack_from("<I",code,i+6)[0]
        if slot not in slots or nva not in names:
            continue
        helper=codeva+i+15+struct.unpack_from("<i",code,i+11)[0]
        if pe.section_of(helper)!=".text":
            continue
        off=None
        if i>=5 and code[i-5]==0x68:
            off=struct.unpack_from("<I",code,i-4)[0]
        elif i>=2 and code[i-2]==0x6a:
            off=code[i-1]
        if off is not None and off>0x10000:
            off=None
        t,index,reg=slots[slot]
        entry={
            "table":t,"table_registration_va":reg,"slot":index,
            "name":names[nva],"server_object_offset":off,
            "property_constructor_va":f"0x{codeva+i:08x}",
            "property_helper_va":f"0x{helper:08x}",
        }
        key=(t,index,reg)
        if key in found and found[key]!=entry:
            raise VerificationError(f"multiple server sendprop initializers for {key}")
        found[key]=entry
    missing=[{"table":t,"slot":index,"table_registration_va":reg}
             for t,index,reg in slots.values() if (t,index,reg) not in found]
    return sorted(found.values(),key=lambda x:(x["table"],x["slot"])), missing


def analyze(client: bytes,server: bytes, *, check_hash: bool=True) -> dict:
    if check_hash and hashlib.sha256(client).hexdigest()!=CLIENT_SHA256:
        raise VerificationError("original SiN client.dll SHA-256 mismatch")
    ct=scan_client_tables(client,check_hash=check_hash)
    cp,cm=scan_client_properties(client,ct)
    st=scan_server_tables(server,check_hash=check_hash)
    sp,sm=scan_server_properties(server,st)
    client_by={(x["table"],x["slot"],x["name"]):x for x in cp}
    pairs=[{"table":x["table"],"slot":x["slot"],"name":x["name"],
            "client_offset":client_by[(x["table"],x["slot"],x["name"])]["client_object_offset"],
            "server_offset":x["server_object_offset"],
            "same_numeric_offset":client_by[(x["table"],x["slot"],x["name"])]["client_object_offset"]==x["server_object_offset"]}
            for x in sp if (x["table"],x["slot"],x["name"]) in client_by]
    for t in ct+st:
        del t["_props"]
    return {
        "scope":"Strict original x86 client RecvTable and PARTIAL server SendTable constructor patterns",
        "sha256":{"client":hashlib.sha256(client).hexdigest(),"server":hashlib.sha256(server).hexdigest()},
        "client":{"constructor_count":len(ct),"named_props":len(cp),"unresolved_prop_slots":cm},
        "server":{"constructor_count_recognized":len(st),
                  "named_props_recognized":len(sp),
                  "unresolved_prop_slots":sm,
                  "tables":st,"props":sp},
        "matching_client_server_slots":pairs,
        "recognized_matching_slot_count":len(pairs),
        "focus_comparison":[x for x in pairs if x["table"] in
            ("DT_LocalWeaponData","DT_BaseEntity","DT_BaseViewModel","DT_BaseAnimating")],
        "limits":[
            "Server scanner recognizes only one code-generation idiom; do not treat uncovered DT names as missing game features.",
            "Slot/property matches compare original client and server names/indices, not full wire encoding or prediction semantics.",
            "Original x86 property offsets cannot be transplanted into the new ARM64 client/server modules.",
            "No playable game code or verified modern SourceVR ABI is produced.",
        ],
    }


def main() -> int:
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--client",type=Path,required=True)
    p.add_argument("--server",type=Path,required=True)
    p.add_argument("--output",type=Path)
    args=p.parse_args()
    try:
        data=analyze(args.client.read_bytes(),args.server.read_bytes())
    except (OSError,ValueError,struct.error) as exc:
        p.exit(1,f"SIN SEND/RECV AUDIT FAILED: {exc}\n")
    txt=json.dumps(data,indent=2)+"\n"
    if args.output:args.output.write_text(txt,encoding="utf-8")
    else:print(txt,end="")
    return 0


if __name__=="__main__":
    raise SystemExit(main())
