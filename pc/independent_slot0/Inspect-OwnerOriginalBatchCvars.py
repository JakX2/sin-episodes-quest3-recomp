#!/usr/bin/env python3
"""Verify selected batch declarations against both owner-original DLLs."""
import argparse
import hashlib
import json
import re
import runpy
from pathlib import Path

root=Path(__file__).parent
prior=runpy.run_path(str(root/"Inspect-OwnerOriginalConsole.py"))
CLIENT_PIN="6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d"

def specs():
    source=(root/"server_batch_cvar_probe.h").read_text()
    rows=re.findall(r'\{"([^"]+)","([^"]+)",(0x[0-9a-f]+),(0x[0-9a-f]+)\}',source)
    assert len(rows)==24
    rows=[(n,v,int(c,16),int(o,16)) for n,v,c,o in rows]
    if len({r[0].lower() for r in rows})!=len(rows) or \
       len({r[3] for r in rows})!=len(rows) or \
       sorted(rows,key=lambda r:r[2])!=rows:
        raise ValueError("Invalid selected batch order or duplicate")
    return rows

def check(rows,decls,client):
    by_name={r["name"]:r for r in decls}
    for n,v,c,o in rows:
        r=by_name.get(n)
        if not r or (r["kind"],r["default"],r["flags"],r["ctor_rva"],
                     r["call_rva"],r["object_rva"]) != ("variable",v,0,0x25bd20,c,o) or \
           n.encode("ascii")+b"\0" in client:
            raise ValueError("Original declaration mismatch or client name collision: "+n)
    return len(rows)

def inspect(server,client):
    if hashlib.sha256(client).hexdigest()!=CLIENT_PIN:
        raise ValueError("Original client SHA256 mismatch")
    decls=prior["inspect"](server)["declarations"]
    rows=specs()
    count=check(rows,decls,client)
    return {"status":"selected declaration metadata only",
            "verified_count":count,"variables":[{"name":n,"default":v,
            "call_rva":hex(c),"object_rva":hex(o)} for n,v,c,o in rows]}

def self_test():
    rows=specs()
    decls=[dict(name=n,kind="variable",default=v,flags=0,ctor_rva=0x25bd20,
                call_rva=c,object_rva=o) for n,v,c,o in rows]
    assert check(rows,decls,b"fake client")==24
    for bad,client in ((decls, rows[-1][0].encode()+b"\0"),
                       (decls[:-1],b""),
                       (decls[:-1]+[{**decls[-1],"flags":1}],b"")):
        try:
            check(rows,bad,client)
        except ValueError:
            pass
        else:
            raise AssertionError("Unverified batch declaration accepted")
    print("PASS original batch subset metadata validation")

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
        p.error("Supply both owner-private originals or --self-test")
