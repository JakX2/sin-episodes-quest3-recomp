#!/usr/bin/env python3
"""Authenticate the extended numeric console catalog against owner originals."""
import argparse
import hashlib
import json
import re
import runpy
from pathlib import Path

root=Path(__file__).parent
prior=runpy.run_path(str(root/"Inspect-OwnerOriginalBatchCvars.py"))
PIN="9b47c80ae0393896967f5b1e35fa0779142b13fa58aba97df748fb7c20bbdfbe"
NUMERIC=re.compile(r"[+-]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+-]?\d+)?")

def specs():
    source=(root/"server_extended_cvar_catalog.h").read_text()
    rows=[(n,v,int(c,16),int(o,16)) for n,v,c,o in
          re.findall(r'\{"([^"]+)","([^"]+)",(0x[0-9a-f]+),(0x[0-9a-f]+)\}',source)]
    canonical=json.dumps(rows,separators=(",",":"),ensure_ascii=True).encode("ascii")
    if len(rows)!=181 or hashlib.sha256(canonical).hexdigest()!=PIN:
        raise ValueError("Extended catalog canonical digest mismatch")
    if len({x[0].lower() for x in rows})!=181 or len({x[3] for x in rows})!=181:
        raise ValueError("Extended catalog duplicate")
    if not set(prior["specs"]()).issubset(rows):
        raise ValueError("Retail-verified 24 declaration subset missing")
    return rows

def select(decls,client):
    result=[]
    for r in decls:
        if r["kind"]!="variable" or r["ctor_rva"]!=0x25bd20 or r["flags"]!=0:
            continue
        name,value=r["name"],r["default"]
        if not NUMERIC.fullmatch(value) or name.encode("ascii")+b"\0" in client:
            continue
        result.append((name,value,r["call_rva"],r["object_rva"]))
    return sorted(result,key=lambda r:r[2])

def inspect(server,client):
    if hashlib.sha256(client).hexdigest()!=prior["CLIENT_PIN"]:
        raise ValueError("Original client SHA256 mismatch")
    rows=specs()
    decls=prior["prior"]["inspect"](server)["declarations"]
    actual=select(decls,client)
    if actual!=rows:
        raise ValueError("Extended console catalog does not match authenticated declaration selection")
    return {"status":"candidate metadata; not retail registration proof",
            "candidate_count":len(rows),"retail_subset_count":24,
            "catalog_sha256":PIN}

def self_test():
    rows=specs()
    decls=[dict(name=n,kind="variable",default=v,flags=0,ctor_rva=0x25bd20,
                call_rva=c,object_rva=o) for n,v,c,o in rows]
    assert select(decls[::-1],b"")==rows
    assert select(decls,rows[-1][0].encode()+b"\0")==rows[:-1]
    for change in (dict(flags=4),dict(default="not-numeric"),dict(ctor_rva=0x25bda0),
                   dict(kind="command")):
        mutated=decls[:-1]+[{**decls[-1],**change}]
        assert select(mutated,b"")==rows[:-1]
    try:
        inspect(b"not original",b"not original")
    except ValueError as exc:
        assert "client SHA256 mismatch" in str(exc)
    else:
        raise AssertionError("Unpinned client accepted")
    print("PASS extended 181-declaration catalog digest, exact subset and selection")

if __name__=="__main__":
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--server",type=Path)
    p.add_argument("--client",type=Path)
    p.add_argument("--self-test",action="store_true")
    a=p.parse_args()
    if a.self_test:
        self_test()
    elif a.server and a.client:
        print(json.dumps(inspect(a.server.read_bytes(),a.client.read_bytes()),indent=2))
    else:
        p.error("Supply both owner originals or --self-test")
