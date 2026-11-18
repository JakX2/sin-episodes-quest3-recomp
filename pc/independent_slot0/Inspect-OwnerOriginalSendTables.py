#!/usr/bin/env python3
"""Read-only original SendTable construction metadata, never executable schemas."""
import argparse
import hashlib
import json
import re
import runpy
import struct
from pathlib import Path

prior = runpy.run_path(str(Path(__file__).with_name("Inspect-OwnerOriginalServerClasses.py")))
PE32 = prior["PE32"]
PIN = prior["PIN"]
INVENTORY_PIN = "559bf3f5688297023414863ca5893ef8f7b38f4451def120d09bc03b8e9215c8"
# Three observed register forms of loading the table-name pointer; the
# signed immediate property count excludes the SDK dummy first property.
PAT = (rb"(?:\x8b\x0d(.{4})\x51|\xa1(.{4})\x50|\x8b\x15(.{4})\x52)"
       rb"(?:\x6a(.)|\x68(.{4}))\x68(.{4})\xb9(.{4})\xe8(.{4})")


def u32(value):
    return struct.unpack("<I", value)[0]


def inventory(code, begin, base, ctor, class_names, resolve, mapped):
    rows = []
    for hit in re.finditer(PAT, code, re.DOTALL):
        rva = begin + hit.start()
        if rva + len(hit[0]) + struct.unpack("<i", hit[8])[0] != ctor:
            continue
        pointer = u32(hit[1] or hit[2] or hit[3])-base
        count = struct.unpack("b" if hit[4] else "<i", hit[4] or hit[5])[0]
        props, table = u32(hit[6])-base, u32(hit[7])-base
        if not 0 <= count <= 4096:
            raise ValueError("Invalid property count")
        # An empty table still points one past its dummy property.
        if not mapped(table,20) or not mapped(props,max(1,count)*76):
            raise ValueError("Invalid table or property storage")
        name = resolve(pointer)
        if not re.fullmatch(r"DT[A-Za-z0-9_]+", name):
            raise ValueError("Invalid data-table name")
        rows.append((class_names.get(table,""),name,count,props,table,rva))
    for key in (lambda x:x[1].lower(),lambda x:x[3],lambda x:x[4]):
        if len({key(x) for x in rows}) != len(rows):
            raise ValueError("Duplicate send-table name or storage")
    if not set(class_names).issubset({x[4] for x in rows}):
        raise ValueError("Registered class missing send-table constructor")
    rows.sort(key=lambda x:x[1].lower())
    return rows


def inspect(data):
    classes=prior["inspect"](data)
    pe=PE32(data)
    class_names={int(x["send_table_storage_rva"],16):x["name"] for x in classes["classes"]}
    _, start, size, raw, _ = next(s for s in pe.sections if s[0]==".text")
    def mapped(rva, length):
        return any(s<=rva and rva+length<=s+max(v,n) for _,s,v,n,_ in pe.sections)
    rows=inventory(pe.raw(start,min(size,raw)),start,pe.base,0x25e850,
                   class_names,lambda p:pe.string(pe.rva(pe.u32(p))),mapped)
    encoded="".join(f"{c}\t{n}\t{s}\t{p:08x}\t{t:08x}\t{r:08x}\n"
                    for c,n,s,p,t,r in rows).encode("ascii")
    if len(rows)!=184 or sum(r[2] for r in rows)!=1028 or \
       hashlib.sha256(encoded).hexdigest()!=INVENTORY_PIN:
        raise ValueError("Original SendTable inventory mismatch")
    return {"registered_class_count":len(class_names),"table_count":len(rows),
            "direct_property_slots":sum(r[2] for r in rows),
            "status":"table metadata; not reconstructed individual SendProps",
            "tables":[{"class":c or None,"name":n,"count":s,
                       "props_storage_rva":hex(p),"table_storage_rva":hex(t),
                       "construct_call_region_rva":hex(r)} for c,n,s,p,t,r in rows]}


def self_test():
    base,start,ctor=0x22000000,0x1000,0x1800
    def record(at,form,name,props,table,count,target=ctor):
        ptr=struct.pack("<I",base+name)
        lead=[b"\x8b\x0d"+ptr+b"\x51",b"\xa1"+ptr+b"\x50",
              b"\x8b\x15"+ptr+b"\x52"][form]
        push=(b"\x6a"+struct.pack("b",count)) if count<=127 else b"\x68"+struct.pack("<i",count)
        prefix=lead+push+b"\x68"+struct.pack("<I",base+props)+b"\xb9"+struct.pack("<I",base+table)+b"\xe8"
        return prefix+struct.pack("<i",target-at-len(prefix)-4)
    names={1:"DT_Z",2:"DT_Alpha",3:"DT_Third"}
    one=record(start,0,1,0x3000,0x5000,0)
    two=record(start+len(one),1,2,0x4000,0x5020,3)
    three=record(start+len(one)+len(two),2,3,0x6000,0x5040,128)
    code=one+two+three
    resolve=names.__getitem__
    rows=inventory(code,start,base,ctor,{0x5000:"ClassZ"},resolve,lambda p,n:True)
    assert [(x[0],x[1],x[2]) for x in rows]==[
        ("","DT_Alpha",3),("","DT_Third",128),("ClassZ","DT_Z",0)]
    # All malformed cases are genuinely at their encoded positions.
    duplicate=one+record(start+len(one),2,1,0x4400,0x5060,1)
    for bad, classes, mapped in (
        (duplicate,{},lambda p,n:True),
        (one,{0x5020:"Missing"},lambda p,n:True),
        (one,{},lambda p,n:False),
        (record(start,0,1,0x3000,0x5000,-1),{},lambda p,n:True),
    ):
        try:
            inventory(bad,start,base,ctor,classes,resolve,mapped)
        except ValueError:
            pass
        else:
            raise AssertionError("Invalid SendTable fixture accepted")
    try:
        inspect(b"not an original")
    except ValueError as exc:
        assert "SHA256 mismatch" in str(exc)
    else:
        raise AssertionError("Invented original accepted")
    print("PASS all three original SendTable metadata forms and bounded inventory guards")


if __name__=="__main__":
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("server",nargs="?",type=Path)
    p.add_argument("--self-test",action="store_true")
    args=p.parse_args()
    if args.self_test:
        self_test()
    elif args.server:
        print(json.dumps(inspect(args.server.read_bytes()),indent=2))
    else:
        p.error("Supply private original server.dll or --self-test")
