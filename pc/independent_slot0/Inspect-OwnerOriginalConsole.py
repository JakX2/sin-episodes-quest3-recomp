#!/usr/bin/env python3
"""Recover original static console declaration metadata (owner-local only)."""
import argparse
import collections
import hashlib
import json
import re
import runpy
import struct
from pathlib import Path

prior=runpy.run_path(str(Path(__file__).with_name("Inspect-OwnerOriginalServerPE.py")))
PE32=prior["PE32"]
PIN=prior["PIN"]
# Verified wrapper RVAs and argument counts; command callback and optional
# completion arguments are addresses, not reconstructed implementations.
CTORS={0x25bd20:3,0x25bda0:4,0x25bde0:8,0x25be30:5,0x25be80:9,0x25bcc0:5}
COMMAND=0x25bcc0
EXPECTED={0x25bd20:335,0x25bda0:85,0x25bde0:5,0x25be30:4,0x25bcc0:196}
INVENTORY_PIN="0b222f8e2da05644491e8cfc99112420ef74e68eae775a149b82ae0ae7eaecaa"


def push_arguments(code,n):
    values=[]
    p=0
    while p<len(code):
        if code[p]==0x68 and p+5<=len(code):
            values.append(struct.unpack_from("<I",code,p+1)[0])
            p+=5
        elif code[p]==0x6a and p+2<=len(code):
            values.append(struct.unpack_from("<b",code,p+1)[0]&0xffffffff)
            p+=2
        else:
            return None
    return values[::-1] if len(values)==n else None


def recover(code,begin,base,ctors,resolve,mapped,executable):
    rows=[]
    calls=collections.Counter()
    # Find every E8 byte, including overlaps, validating its relative
    # destination and the entire observed immediate constructor pattern.
    for offset in range(len(code)-4):
        if code[offset]!=0xe8:
            continue
        call=begin+offset
        target=call+5+struct.unpack_from("<i",code,offset+1)[0]
        if target not in ctors:
            continue
        calls[target]+=1
        if offset<5 or code[offset-5]!=0xb9:
            raise ValueError("Constructor lacks immediate receiver")
        obj=struct.unpack_from("<I",code,offset-4)[0]-base
        size=36 if target==COMMAND else 68
        if not mapped(obj,size):
            raise ValueError("Constructor receiver outside data storage")
        n=ctors[target]
        candidates=[]
        for start in range(max(0,offset-5-5*n),offset-5-2*n+1):
            args=push_arguments(code[start:offset-5],n)
            if args is None:
                continue
            try:
                name=resolve(args[0]-base)
                if not re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*",name):
                    continue
                if target==COMMAND:
                    if not executable(args[1]-base):
                        continue
                    default=None
                    flags=args[3]
                else:
                    default=resolve(args[1]-base)
                    flags=args[2]
            except (ValueError,UnicodeError,KeyError):
                continue
            candidates.append({"name":name,"kind":"command" if target==COMMAND else "variable",
                               "default":default,"flags":flags,"object_rva":obj,
                               "call_rva":call,"ctor_rva":target,"args":args})
        if len(candidates)!=1:
            raise ValueError("Missing or ambiguous declaration at "+hex(call))
        rows.append(candidates[0])
    for key in (lambda r:r["name"].lower(),lambda r:r["object_rva"]):
        if len({key(r) for r in rows})!=len(rows):
            raise ValueError("Duplicate console declaration")
    rows.sort(key=lambda r:r["name"].lower())
    return rows,calls


def inspect(data):
    if hashlib.sha256(data).hexdigest()!=PIN:
        raise ValueError("Original server.dll SHA256 mismatch")
    pe=PE32(data)
    if pe.base!=0x22000000:
        raise ValueError("Original image base mismatch")
    _,begin,size,raw,_=next(s for s in pe.sections if s[0]==".text")
    _,dstart,dsize,draw,_=next(s for s in pe.sections if s[0]==".data")
    _,rstart,rsize,rraw,roff=next(s for s in pe.sections if s[0]==".rdata")
    def resolve(rva):
        if not rstart<=rva<rstart+min(rsize,rraw):
            raise ValueError("String outside original rdata")
        off=roff+rva-rstart
        end=data.find(b"\0",off,min(off+513,roff+min(rsize,rraw)))
        if end<0:
            raise ValueError("Invalid declaration string")
        return data[off:end].decode("latin1")
    rows,calls=recover(pe.raw(begin,min(size,raw)),begin,pe.base,CTORS,resolve,
                       lambda r,n:dstart<=r and r+n<=dstart+max(dsize,draw),
                       lambda r:begin<=r<begin+size)
    encoded=json.dumps(rows,sort_keys=True,separators=(",",":"),ensure_ascii=True).encode("ascii")
    if dict(calls)!=EXPECTED or len(rows)!=625 or hashlib.sha256(encoded).hexdigest()!=INVENTORY_PIN:
        raise ValueError("Original console declaration inventory mismatch")
    return {"status":"static declaration metadata, not a complete live command registry",
            "variables":sum(r["kind"]=="variable" for r in rows),
            "commands":sum(r["kind"]=="command" for r in rows),
            "replicated":sum(bool(r["flags"]&0x2000) for r in rows),
            "unregistered":sum(bool(r["flags"]&1) for r in rows),
            "declarations":rows}


def self_test():
    base,begin=0x22000000,0x1000
    target=0x25bd20
    def record(at,name,obj,flags=0):
        prefix=(b"\x6a"+struct.pack("b",flags)+b"\x68"+struct.pack("<I",base+3)+
                b"\x68"+struct.pack("<I",base+name)+b"\xb9"+struct.pack("<I",base+obj)+b"\xe8")
        return prefix+struct.pack("<i",target-at-len(prefix)-4)
    first=record(begin,1,0x5000)
    second=record(begin+len(first),2,0x5100)
    names={1:"Zulu",2:"alpha",3:"0"}
    rows,calls=recover(first+second,begin,base,{target:3},names.__getitem__,
                       lambda r,n:r>=0x5000,lambda r:True)
    assert [r["name"] for r in rows]==["alpha","Zulu"] and calls[target]==2
    assert push_arguments(b"\x6a\xff",1)==[0xffffffff]
    for malformed in (first+record(begin+len(first),1,0x5100),
                      first+record(begin+len(first),2,0x5000),
                      record(begin,1,0x100)):
        try:
            recover(malformed,begin,base,{target:3},names.__getitem__,
                    lambda r,n:r>=0x5000,lambda r:True)
        except ValueError:
            pass
        else:
            raise AssertionError("Invalid console declaration accepted")
    try:
        inspect(b"not the owner original")
    except ValueError as exc:
        assert "SHA256 mismatch" in str(exc)
    else:
        raise AssertionError("Unpinned original accepted")
    print("PASS bounded original console constructor metadata regression")


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
        p.error("Provide private original server.dll or --self-test")
