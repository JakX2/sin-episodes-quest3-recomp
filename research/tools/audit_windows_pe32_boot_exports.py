#!/usr/bin/env python3
"""Offline PE32 game-DLL export gate. No original binary content is emitted."""
import argparse
import json
import struct
from pathlib import Path

class PEError(ValueError):
    pass

def inspect(path):
    data = Path(path).read_bytes()
    def u16(o):
        if o < 0 or o + 2 > len(data): raise PEError("truncated PE field")
        return struct.unpack_from("<H", data, o)[0]
    def u32(o):
        if o < 0 or o + 4 > len(data): raise PEError("truncated PE field")
        return struct.unpack_from("<I", data, o)[0]
    if len(data) < 64 or data[:2] != b"MZ": raise PEError("missing DOS header")
    pe = u32(0x3c)
    if pe + 24 > len(data) or data[pe:pe+4] != b"PE\\0\\0": raise PEError("missing NT header")
    machine = u16(pe+4)
    count = u16(pe+6)
    opt_size = u16(pe+20)
    opt = pe+24
    if count == 0 or count > 96 or opt+opt_size > len(data): raise PEError("invalid section table")
    if u16(opt) != 0x10b or machine != 0x14c: raise PEError("not Windows x86 PE32")
    if opt_size < 104 or u32(opt+92) < 1: raise PEError("missing export data directory")
    export_rva, export_size = u32(opt+96), u32(opt+100)
    sections = []
    for i in range(count):
        s = opt+opt_size+i*40
        if s+40 > len(data): raise PEError("truncated section header")
        sections.append((u32(s+12), max(u32(s+8),u32(s+16)),u32(s+20),u32(s+16)))
    def offset(rva, size=1):
        for base, extent, raw, raw_size in sections:
            if base <= rva and rva+size <= base+extent and rva+size <= base+raw_size:
                o=raw+rva-base
                if o+size <= len(data): return o
        raise PEError("unmapped or truncated RVA")
    if not export_rva: raise PEError("no export directory")
    ed=offset(export_rva,40)
    funcs, names = u32(ed+20),u32(ed+24)
    if funcs > 100000 or names > funcs: raise PEError("invalid export counts")
    name_table=u32(ed+32)
    exports=[]
    for i in range(names):
        rva=u32(offset(name_table+i*4,4))
        start=offset(rva)
        end=data.find(b"\\0",start,min(len(data),start+512))
        if end < 0: raise PEError("unterminated export name")
        try: name=data[start:end].decode("ascii")
        except UnicodeDecodeError: raise PEError("non-ASCII export name")
        exports.append(name)
    return {"pe32_x86": True, "create_interface_export": "CreateInterface" in exports,
            "export_count": len(exports), "exports": sorted(exports)}

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument("client",type=Path)
    ap.add_argument("server",type=Path)
    ap.add_argument("--show-exports",action="store_true",help="Show export names, never binary bytes")
    args=ap.parse_args()
    result={}
    for label,path in (("client",args.client),("server",args.server)):
        try:
            item=inspect(path)
            if not args.show_exports: item.pop("exports")
            result[label]={"status":"pass" if item["create_interface_export"] else "fail",**item}
        except (OSError,PEError) as exc:
            result[label]={"status":"fail","error":str(exc)}
    result["overall"]="pass" if all(result[k]["status"]=="pass" for k in ("client","server")) else "fail"
    print(json.dumps(result,indent=2,sort_keys=True))
    return 0 if result["overall"]=="pass" else 1

if __name__=="__main__":
    raise SystemExit(main())
