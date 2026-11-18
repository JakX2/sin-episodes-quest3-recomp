#!/usr/bin/env python3
"""Recover original client static console-declaration metadata locally."""
import argparse
import collections
import hashlib
import json
import re
import runpy
import struct
from pathlib import Path

root=Path(__file__).parent
prior=runpy.run_path(str(root/"Inspect-OwnerOriginalServerPE.py"))
PE32=prior["PE32"]
args_parser=runpy.run_path(str(root/"Inspect-OwnerOriginalConsole.py"))["push_arguments"]
PIN="6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d"
SERVER_PIN="568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650"
CTORS={0x121640:3,0x1216c0:4,0x121700:8,0x121750:5,0x1217a0:9,0x1215e0:5}
EXPECTED={0x121640:240,0x1216c0:129,0x121700:9,0x121750:4,0x1217a0:2,0x1215e0:149}
COMMAND=0x1215e0
INVENTORY_PIN="ec58b826b784b5a296fdb5465ccd594aa456b1fd57d028f9833c85656fb75c4b"
NUMERIC=re.compile(r"[+-]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+-]?\d+)?")


def recover(code,begin,base,resolve,mapped,executable,ctors=CTORS):
    rows=[]
    counts=collections.Counter()
    for offset in range(len(code)-4):
        if code[offset]!=0xe8:
            continue
        call=begin+offset
        target=call+5+struct.unpack_from("<i",code,offset+1)[0]
        if target not in ctors:
            continue
        counts[target]+=1
        if offset<5 or code[offset-5]!=0xb9:
            raise ValueError("Constructor without immediate receiver")
        obj=struct.unpack_from("<I",code,offset-4)[0]-base
        if not mapped(obj,36 if target==COMMAND else 68):
            raise ValueError("Receiver outside client data")
        n=ctors[target]
        candidates=[]
        for start in range(max(0,offset-5-5*n),offset-5-2*n+1):
            args=args_parser(code[start:offset-5],n)
            if args is None:
                continue
            try:
                name=resolve(args[0]-base)
                pattern=r"[+-]?[A-Za-z_][A-Za-z0-9_]*" if target==COMMAND else r"[A-Za-z_][A-Za-z0-9_]*"
                if not re.fullmatch(pattern,name):
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
            candidates.append(dict(name=name,kind="command" if target==COMMAND else "variable",
                default=default,flags=flags,object_rva=obj,call_rva=call,ctor_rva=target,args=args))
        if len(candidates)!=1:
            raise ValueError("Ambiguous client constructor at "+hex(call))
        rows.extend(candidates)
    if len({r["object_rva"] for r in rows})!=len(rows):
        raise ValueError("Duplicate object receiver")
    rows.sort(key=lambda r:(r["name"].lower(),r["call_rva"]))
    return rows,counts


def verify_probe_specs(candidates):
    header=(root/"client_cvar_probe.h").read_text()
    selected=[(n,v,int(c,16),int(o,16)) for n,v,c,o in
        re.findall(r'\{"([^"]+)","([^"]+)",(0x[0-9a-f]+),(0x[0-9a-f]+)\}',header)]
    expected=[("cl_starfield_distance","256.0",0x26f031,0x37b108),
        ("cl_starfield_diameter","128.0",0x26f001,0x37b0c0),
        ("cl_show_bloodspray","1",0x268e01,0x361978)]
    available={(r["name"],r["default"],r["call_rva"],r["object_rva"]) for r in candidates}
    if selected!=expected or not set(selected).issubset(available):
        raise ValueError("Client selected probe declaration mismatch")
    return selected


def inspect(data,server):
    if hashlib.sha256(data).hexdigest()!=PIN:
        raise ValueError("Original client SHA256 mismatch")
    if hashlib.sha256(server).hexdigest()!=SERVER_PIN:
        raise ValueError("Original server SHA256 mismatch")
    pe=PE32(data)
    if pe.base!=0x24000000:
        raise ValueError("Original client image base mismatch")
    _,begin,size,raw,_=next(s for s in pe.sections if s[0]==".text")
    _,dstart,dsize,draw,_=next(s for s in pe.sections if s[0]==".data")
    _,rstart,rsize,rraw,roff=next(s for s in pe.sections if s[0]==".rdata")
    def resolve(rva):
        if not rstart<=rva<rstart+min(rsize,rraw):
            raise ValueError("String outside client rdata")
        off=roff+rva-rstart
        end=data.find(b"\0",off,min(off+513,roff+min(rsize,rraw)))
        if end<0:
            raise ValueError("Invalid declaration string")
        return data[off:end].decode("latin1")
    rows,counts=recover(pe.raw(begin,min(size,raw)),begin,pe.base,resolve,
        lambda r,n:dstart<=r and r+n<=dstart+max(dsize,draw),
        lambda r:begin<=r<begin+size)
    encoded=json.dumps(rows,sort_keys=True,separators=(",",":"),ensure_ascii=True).encode("ascii")
    if dict(counts)!=EXPECTED or len(rows)!=533 or hashlib.sha256(encoded).hexdigest()!=INVENTORY_PIN:
        raise ValueError("Original client declaration digest mismatch")
    duplicates={n:c for n,c in collections.Counter(r["name"].lower() for r in rows).items() if c>1}
    if duplicates!={"mat_fullbright":2}:
        raise ValueError("Original duplicate-name set mismatch")
    # Check the original console vtable spans, including client-specific
    # RVAs. The inherited shape matches the recovered server ABI.
    tables={
        "base":(0x2981a4,(0x1214a0,0x120cd0,0x120dd0,0x120de0,0x120d80,
            0x120e70,0x120e80,0x120ce0,0x120d30)),
        "variable":(0x2981cc,(0x121680,0x121010,0x120fc0,0x120fe0,0x121000,
            0x120fd0,0x120ff0,0x120ce0,0x121020,0x121400,0x1213f0,0x1213e0,
            0x121040,0x1211d0,0x121250,0x121180,0x1210d0,0x1212e0)),
        "command":(0x298218,(0x121620,0x120ea0,0x120dd0,0x120de0,0x120d80,
            0x120e70,0x120e80,0x120ce0,0x120d30,0x120f40,0x120f60,0x120eb0,0x120ed0)),
    }
    for role,(rva,slots) in tables.items():
        actual=tuple(pe.rva(pe.u32(rva+4*i)) for i in range(len(slots)))
        if actual!=slots:
            raise ValueError("Client "+role+" vtable mismatch")
        next_word=pe.u32(rva+4*len(slots))
        if pe.base+begin<=next_word<pe.base+begin+size:
            raise ValueError("Unexpected client console vtable continuation")
    def edge(source,target):
        raw=pe.raw(source,5)
        if raw[0]!=0xe8 or source+5+struct.unpack_from("<i",raw,1)[0]!=target:
            raise ValueError("Client registration call edge mismatch")
    edge(0xfe393,0x120c70)
    edge(0xfe012,0x120e00)
    if pe.rva(pe.u32(0x2fa4ac))!=0x2968d4 or pe.rva(pe.u32(0x2968d4))!=0xfe000:
        raise ValueError("Original client accessor identity mismatch")
    if pe.raw(0xfe007,7)!=bytes.fromhex("6a088bceff500c") or pe.raw(0xfe020,2)!=bytes.fromhex("ff12"):
        raise ValueError("Original client accessor flag or register slot mismatch")
    candidates=[r for r in rows if r["ctor_rva"]==0x121640 and r["flags"]==0 and
        NUMERIC.fullmatch(r["default"]) and r["name"].encode("ascii")+b"\0" not in server]
    if len(candidates)!=81:
        raise ValueError("Client candidate selection mismatch")
    selected=verify_probe_specs(candidates)
    return {"status":"static metadata only","declarations":rows,
        "probe_selection_count":len(selected),
        "variables":sum(r["kind"]=="variable" for r in rows),
        "commands":sum(r["kind"]=="command" for r in rows),
        "input_commands":sum(r["name"].startswith(("+","-")) for r in rows),
        "replicated":sum(bool(r["flags"]&0x2000) for r in rows),
        "unregistered":sum(bool(r["flags"]&1) for r in rows),
        "duplicate_names":duplicates,"numeric_candidates":candidates,
        "vtable_slots":{role:[hex(x) for x in slots] for role,(_,slots) in tables.items()}}


def self_test():
    base,begin=0x24000000,0x1000
    def command(at,name,obj):
        p=b"\x6a\x00"*3+b"\x68"+struct.pack("<I",base+0x1200)+b"\x68"+struct.pack("<I",base+name)+b"\xb9"+struct.pack("<I",base+obj)+b"\xe8"
        return p+struct.pack("<i",COMMAND-at-len(p)-4)
    first=command(begin,1,0x5000)
    second=command(begin+len(first),2,0x5100)
    strings={1:"+attack",2:"-attack"}
    rows,c=recover(first+second,begin,base,strings.__getitem__,
        lambda r,n:r>=0x5000,lambda r:r==0x1200)
    assert [x["name"] for x in rows]==["+attack","-attack"] and c[COMMAND]==2
    # Duplicate names are real original metadata, but duplicate storage is not.
    same=command(begin+len(first),1,0x5100)
    assert len(recover(first+same,begin,base,strings.__getitem__,
        lambda r,n:r>=0x5000,lambda r:r==0x1200)[0])==2
    try:
        recover(first+command(begin+len(first),2,0x5000),begin,base,strings.__getitem__,
            lambda r,n:r>=0x5000,lambda r:r==0x1200)
    except ValueError:
        pass
    else:
        raise AssertionError("Duplicate receiver accepted")
    selected=[dict(name=n,default=v,call_rva=c,object_rva=o) for n,v,c,o in (
        ("cl_starfield_distance","256.0",0x26f031,0x37b108),
        ("cl_starfield_diameter","128.0",0x26f001,0x37b0c0),
        ("cl_show_bloodspray","1",0x268e01,0x361978))]
    assert len(verify_probe_specs(selected))==3
    try:
        verify_probe_specs(selected[:-1])
    except ValueError:
        pass
    else:
        raise AssertionError("Unverified client probe subset accepted")
    print("PASS client console metadata, input commands, duplicate names and selected declarations")


if __name__=="__main__":
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--client",type=Path)
    p.add_argument("--server",type=Path)
    p.add_argument("--self-test",action="store_true")
    a=p.parse_args()
    if a.self_test:
        self_test()
    elif a.client and a.server:
        print(json.dumps(inspect(a.client.read_bytes(),a.server.read_bytes()),indent=2))
    else:
        p.error("Supply both owner originals or --self-test")
