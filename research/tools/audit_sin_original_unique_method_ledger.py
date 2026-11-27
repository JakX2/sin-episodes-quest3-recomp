#!/usr/bin/env python3
"""Check that each SHA-authenticated original SiN method has one canonical ledger key.

Method identity is (original DLL SHA-defined role, original VA). Two native
exports or two research reports are *aliases*, not additional original code.
No proprietary original game executable is necessary for this check.
"""
from __future__ import annotations
import argparse
import copy
import json
import re
from pathlib import Path

SERVER_ORIGINAL_MINOR_VA=0x2237e8c0
SERVER_MINOR_SHA="e08a5b91b9f7e3411976303123a218fb28e6d3ff2df0690a1e8df6525b714ff7"
class Refused(ValueError):
    pass

def require(condition: bool, why: str) -> None:
    if not condition:raise Refused(why)

def validate(ledger: dict) -> dict:
    require(ledger.get("schema_version")==1, "unexpected ledger schema")
    roles=ledger.get("dll_sha256")
    require(isinstance(roles,dict), "missing original DLL identities")
    for role in ("client","server"):
        require(re.fullmatch(r"[a-fA-F0-9]{64}",str(roles.get(role,""))),
                "invalid original DLL SHA: "+role)
    rows=ledger.get("methods")
    require(isinstance(rows,list) and bool(rows), "missing original method entries")
    seen={}
    for i,rec in enumerate(rows):
        require(isinstance(rec,dict),"method entry must be object: "+str(i))
        role=rec.get("dll")
        require(role in roles,"unidentified DLL role: "+str(i))
        va=rec.get("entry_va")
        require(isinstance(va,str) and re.fullmatch(r"0x[0-9a-fA-F]+",va),
                "invalid original VA: "+str(i))
        addr=int(va,16)
        require(0x10000<=addr<0x100000000,"original VA not PE32: "+str(i))
        digest=rec.get("original_code_sha256")
        require(isinstance(digest,str) and re.fullmatch(r"[a-fA-F0-9]{64}",digest),
                "invalid original function digest: "+str(i))
        n=rec.get("original_code_length")
        require(type(n) is int and n>0,"invalid original function length: "+str(i))
        key=(role,addr)
        require(key not in seen,
                "duplicate authenticated original method "+role+":"+hex(addr)+
                " (entries "+str(seen.get(key))+", "+str(i)+")")
        seen[key]=i
    key=("server",SERVER_ORIGINAL_MINOR_VA)
    require(key in seen,"canonical Minor +0x398 identity missing")
    canonical=rows[seen[key]]
    require(canonical["original_code_sha256"].lower()==SERVER_MINOR_SHA,
            "original Minor +0x398 method digest drift")
    require(canonical["original_code_length"]==56,
            "original Minor +0x398 length drift")
    require(canonical.get("implementation_path")==
            "pc/independent_slot0/native_server_recombinant_minor_damage_gate.cpp",
            "canonical Minor +0x398 implementation path drift")
    require(canonical.get("additional_implementation_path")==
            "pc/independent_slot0/native_server_recombinant_minor.cpp",
            "Minor +0x398 compatibility export path drift")
    return {"status":"ORIGINAL_METHOD_LEDGER_UNIQUE","methods":len(seen),
            "identities":len(roles),"canonical_minor_va":hex(SERVER_ORIGINAL_MINOR_VA)}

def self_test(original: dict) -> None:
    validate(original)
    first=copy.deepcopy(original)
    first["methods"].append(copy.deepcopy(first["methods"][0]))
    try:validate(first)
    except Refused:pass
    else:raise AssertionError("duplicated original function identity accepted")
    second=copy.deepcopy(original)
    match=next(v for v in second["methods"] if v["dll"]=="server" and
               int(v["entry_va"],16)==SERVER_ORIGINAL_MINOR_VA)
    match["original_code_sha256"]="0"*64
    try:validate(second)
    except Refused:pass
    else:raise AssertionError("canonical original SHA drift accepted")
    third=copy.deepcopy(original)
    third["methods"][0]["original_code_length"]=0
    try:validate(third)
    except Refused:pass
    else:raise AssertionError("invalid original code extent accepted")

def main() -> None:
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--ledger",type=Path,
        default=Path(__file__).resolve().parents[1]/"catalog"/"original_dll_verified_methods.json")
    ap.add_argument("--self-test",action="store_true")
    args=ap.parse_args()
    data=json.loads(args.ledger.read_text(encoding="utf-8"))
    result=validate(data)
    if args.self_test:
        self_test(data)
        result["mutated_duplicate_sha_extent_tests"]="passed"
    print(json.dumps(result,sort_keys=True))

if __name__=="__main__":
    try:main()
    except (Refused,OSError,ValueError) as e:
        raise SystemExit("REFUSED: "+str(e))
