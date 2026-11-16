"""Inspect PE32 x86 delay imports and forwarded exports without executable bytes."""
import argparse
import json
import struct
from pathlib import Path

class PEError(ValueError):
    pass

def audit(data):
    def u16(p):
        if p < 0 or p + 2 > len(data): raise PEError("truncated word")
        return struct.unpack_from("<H", data, p)[0]
    def u32(p):
        if p < 0 or p + 4 > len(data): raise PEError("truncated dword")
        return struct.unpack_from("<I", data, p)[0]
    if data[:2] != b"MZ": raise PEError("not MZ")
    pe = u32(0x3c)
    if pe > len(data)-24 or data[pe:pe+4] != b"PE\0\0": raise PEError("not PE")
    coff = pe+4
    if u16(coff) != 0x14c: raise PEError("not x86")
    count, optsize = u16(coff+2), u16(coff+16)
    opt = coff+20
    if optsize < 112 or opt+optsize > len(data) or u16(opt) != 0x10b:
        raise PEError("not PE32")
    if not u16(coff+18)&0x2000: raise PEError("not DLL")
    base, headers, dirs = u32(opt+28), u32(opt+60), u32(opt+92)
    sections=[]
    for i in range(count):
        p=opt+optsize+40*i
        if p+40>len(data): raise PEError("section table truncated")
        sections.append((u32(p+12),max(u32(p+8),u32(p+16)),u32(p+20),u32(p+16)))
    def rva(v,size=1):
        if v < 0 or size < 1 or v+size>0x100000000: raise PEError("invalid RVA")
        if v+size<=min(headers,len(data)): return v
        for va,span,raw,rawsize in sections:
            off=v-va
            if off>=0 and off+size<=span and off+size<=rawsize and raw+off+size<=len(data):
                return raw+off
        raise PEError("unbacked RVA")
    def name(v):
        p=rva(v)
        end=data.find(b"\0",p,min(len(data),p+512))
        if end<0: raise PEError("unterminated string")
        try: return data[p:end].decode("ascii")
        except UnicodeDecodeError as e: raise PEError("non-ASCII string") from e
    def directory(i):
        if dirs<=i: return (0,0)
        if 96+(i+1)*8>optsize: raise PEError("directory exceeds optional header")
        return u32(opt+96+i*8),u32(opt+100+i*8)
    delays=[]
    delay,size=directory(13)
    if delay:
        if size<32: raise PEError("short delay directory")
        for i in range(min(size//32,4096)):
            p=rva(delay+32*i,32)
            desc=struct.unpack_from("<8I",data,p)
            if not any(desc): break
            attrs,nm=desc[:2]
            if attrs & ~1: raise PEError("unsupported delay attributes")
            if not attrs&1:
                if nm<base: raise PEError("invalid VA delay name")
                nm-=base
            delays.append(name(nm))
        else: raise PEError("unterminated delay descriptors")
    forwards=[]
    exp,exp_size=directory(0)
    if exp:
        if exp_size<40 or exp+exp_size>0x100000000: raise PEError("invalid export directory")
        p=rva(exp,40)
        n=u32(p+20)
        funcs=u32(p+28)
        if n>100000: raise PEError("excessive export count")
        for i in range(n):
            target=u32(rva(funcs+i*4,4))
            if target and exp<=target<exp+exp_size:
                forwards.append(name(target))
    return {"delay_imports":sorted(set(delays),key=str.casefold),"export_forwarders":sorted(set(forwards),key=str.casefold)}

def forbidden(result,names):
    deny={x.casefold() for x in names}
    return {
        "delay_imports":[x for x in result["delay_imports"] if x.casefold() in deny],
        "export_forwarders":[x for x in result["export_forwarders"] if (x.split(".",1)[0]+".dll").casefold() in deny],
    }

if __name__=="__main__":
    parser=argparse.ArgumentParser()
    parser.add_argument("dll",type=Path)
    parser.add_argument("--forbid",action="append",default=[])
    args=parser.parse_args()
    result=audit(args.dll.read_bytes())
    result["forbidden"]=forbidden(result,args.forbid)
    print(json.dumps(result,indent=2))
    if any(result["forbidden"].values()): raise SystemExit(1)
