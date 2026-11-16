"""Read-only PE32 x86 static import audit for independently built DLLs.

Does not prove absence of dynamic LoadLibrary/GetProcAddress, delay imports,
forwarded exports, or runtime forwarding. Never treats imports as verified
original function bodies.
"""
import argparse
import json
import struct
from pathlib import Path

class PEError(ValueError):
    pass

def audit(data: bytes):
    def u16(o):
        if o < 0 or o + 2 > len(data): raise PEError("truncated PE")
        return struct.unpack_from("<H", data, o)[0]
    def u32(o):
        if o < 0 or o + 4 > len(data): raise PEError("truncated PE")
        return struct.unpack_from("<I", data, o)[0]
    if data[:2] != b"MZ": raise PEError("missing MZ")
    pe = u32(0x3c)
    if pe > len(data)-24 or data[pe:pe+4] != b"PE\0\0": raise PEError("missing PE")
    coff = pe+4
    if u16(coff) != 0x14c: raise PEError("not x86")
    nsec, optsize = u16(coff+2), u16(coff+16)
    opt = coff+20
    if optsize < 112 or opt+optsize > len(data) or u16(opt) != 0x10b:
        raise PEError("not valid PE32 optional header")
    if not (u16(coff+18) & 0x2000): raise PEError("not a DLL")
    if u32(opt+92) < 2: raise PEError("missing import directory entry")
    sections=[]
    for i in range(nsec):
        s=opt+optsize+i*40
        if s+40>len(data): raise PEError("truncated section table")
        sections.append((u32(s+12), max(u32(s+8),u32(s+16)),u32(s+20),u32(s+16)))
    headers=u32(opt+60)
    def rva(r, size=1):
        if r < headers and r+size <= min(headers,len(data)): return r
        for va,span,raw,rawsize in sections:
            if va<=r and r+size<=va+span and r-va+size<=rawsize and raw+r-va+size<=len(data):
                return raw+r-va
        raise PEError("RVA outside backed sections")
    def string(r):
        o=rva(r)
        end=data.find(b"\0",o,min(len(data),o+512))
        if end<0: raise PEError("unterminated import name")
        try: return data[o:end].decode("ascii")
        except UnicodeDecodeError as e: raise PEError("non-ASCII import name") from e
    imp_rva,imp_size=u32(opt+104),u32(opt+108)
    names=[]
    if imp_rva:
        if imp_size < 20: raise PEError("invalid import directory")
        for i in range(min(imp_size//20,4096)):
            d=rva(imp_rva+i*20,20)
            if data[d:d+20]==bytes(20): break
            name_rva=u32(d+12)
            if not name_rva: raise PEError("missing import descriptor name")
            names.append(string(name_rva))
        else: raise PEError("unterminated import descriptors")
    return {"machine":"x86","format":"PE32","imports":sorted(set(names),key=str.casefold)}

def main():
    p=argparse.ArgumentParser()
    p.add_argument("dll",type=Path)
    p.add_argument("--forbid",action="append",default=[],help="case-insensitive forbidden DLL name")
    a=p.parse_args()
    result=audit(a.dll.read_bytes())
    forbidden={n.casefold() for n in a.forbid}
    bad=[n for n in result["imports"] if n.casefold() in forbidden]
    result["forbidden_imports"]=bad
    print(json.dumps(result,indent=2))
    if bad: raise SystemExit(1)

if __name__=="__main__": main()
