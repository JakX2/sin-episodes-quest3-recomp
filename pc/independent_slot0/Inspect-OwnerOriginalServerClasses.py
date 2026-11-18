#!/usr/bin/env python3
"""Read-only class-registration inventory; not executable network schemas."""
import argparse
import hashlib
import json
import re
import runpy
import struct
from pathlib import Path

PE32 = runpy.run_path(str(Path(__file__).with_name(
    "Inspect-OwnerOriginalServerPE.py")))["PE32"]
PIN = "568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650"
INVENTORY_PIN = "5f23f4f707c8c024fb2cc489b92b97d217e3227d793d192f2f1e407d3d9e6d5e"
REG = rb"\x68(.{4})\x68(.{4})\xb9(.{4})\xe8(.{4})"


def registrations(code, begin, base, constructor, resolve, mapped):
    rows = []
    for match in re.finditer(REG, code, re.DOTALL):
        rva = begin + match.start()
        if rva + 20 + struct.unpack("<i", match[4])[0] != constructor:
            continue
        table, name_rva, obj = [struct.unpack("<I", match[i])[0] - base
                               for i in (1, 2, 3)]
        if not mapped(table, 20) or not mapped(obj, 20):
            raise ValueError("Registration storage outside mapped image")
        name = resolve(name_rva)
        if not re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", name):
            raise ValueError("Invalid class name")
        rows.append((name, table, obj, rva))
    rows.sort(key=lambda row: row[0].lower())
    for key in (lambda row: row[0].lower(), lambda row: row[1], lambda row: row[2]):
        if len({key(row) for row in rows}) != len(rows):
            raise ValueError("Duplicate registration name or storage")
    return rows


def inspect(data):
    if hashlib.sha256(data).hexdigest() != PIN:
        raise ValueError("Original server SHA256 mismatch")
    pe = PE32(data)
    if pe.base != 0x22000000:
        raise ValueError("Original server image base mismatch")
    if pe.raw(0x1e7c30, 6) != b"\xa1" + struct.pack("<I", 0x22589050) + b"\xc3":
        raise ValueError("Original GetAllServerClasses getter mismatch")
    # Class records/tables are initialized in mapped zero-fill data, not
    # necessarily backed by bytes in the file. Validate virtual extents.
    def mapped(rva, length):
        return any(start <= rva and rva + length <= start+max(size,raw)
                   for _,start,size,raw,_ in pe.sections)
    text = next(s for s in pe.sections if s[0]==".text")
    _, start, size, raw, _ = text
    rows = registrations(pe.raw(start, min(size,raw)), start, pe.base,
                         0x1290, pe.string, mapped)
    encoded = "".join(f"{n}\t{t:08x}\t{o:08x}\t{r:08x}\n" for n,t,o,r in rows)
    if len(rows)!=164 or hashlib.sha256(encoded.encode("ascii")).hexdigest()!=INVENTORY_PIN:
        raise ValueError("Original registration inventory mismatch")
    return {
        "status":"registration metadata only; no SendProp reconstruction or engine export",
        "class_count":len(rows),
        "head_global_rva":"0x589050",
        "constructor_rva":"0x1290",
        "classes":[{"name":n,"send_table_storage_rva":hex(t),
                    "class_storage_rva":hex(o),"registration_rva":hex(r)}
                   for n,t,o,r in rows],
    }


def self_test():
    base, start, ctor = 0x22000000, 0x1000, 0x1290
    def record(at, name, table, obj, target):
        return (b"\x68"+struct.pack("<I",base+table)+b"\x68"+
                struct.pack("<I",base+name)+b"\xb9"+struct.pack("<I",base+obj)+
                b"\xe8"+struct.pack("<i",target-at-20))
    names={1:"Zulu",2:"alpha",3:"ALPHA"}
    first=record(start,1,0x5000,0x6000,ctor)
    second=record(start+20,2,0x5020,0x6020,ctor)
    ignored=record(start+40,3,0x5040,0x6040,ctor+1)
    rows=registrations(first+second+ignored,start,base,ctor,
                       names.__getitem__,lambda r,n: r>=0x5000)
    assert [row[0] for row in rows]==["alpha","Zulu"]
    for bad in (first+record(start+20,3,0x5000,0x6040,ctor),
                record(start,2,0x5020,0x6020,ctor)+record(start+20,3,0x5040,0x6040,ctor)):
        try:
            registrations(bad,start,base,ctor,names.__getitem__,lambda r,n: True)
        except ValueError:
            pass
        else:
            raise AssertionError("duplicate registration accepted")
    try:
        inspect(b"not the owner original")
    except ValueError as exc:
        assert "SHA256 mismatch" in str(exc)
    else:
        raise AssertionError("invented original accepted")
    print("PASS synthetic server class-registration inventory parser")


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
        p.error("Provide owner-private original DLL or --self-test")
