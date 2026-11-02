#!/usr/bin/env python3
"""Inspect verified original SiN vtable-override x86 bodies and their xrefs.

Requires GNU objdump and the user's original 2006 DLLs. Instruction boundaries
are decoded by objdump, not guessed from raw byte substrings. Only selected
derived-class virtual function targets are inspected; nonvirtual code remains
future work. Body END is a ret/int3 compiler-padding heuristic, not a CFG.
Do not publish the commercial DLL or raw instruction dumps.
"""
from __future__ import annotations
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess

from verify_gameplay_entity_registry import PEImage, VerificationError
from audit_sin_rtti_vtables import scan

LINE=re.compile(r"^\s*([a-fA-F0-9]{8,}):\s+([a-z][a-z0-9_.]*)\s*(.*)$")
HEX=re.compile(r"(?:^|\s|,)0x([a-fA-F0-9]{6,8})(?=$|\s|,)")
DIRECT=re.compile(r"^0x([a-fA-F0-9]{6,8})(?:\s|$)")
BYTES_WINDOW=8192


def read_string(pe:PEImage,va:int)->str|None:
    if pe.section_of(va)!=".rdata":return None
    try:raw=pe.at(va,170)
    except VerificationError:
        try:raw=pe.at(va,64)
        except VerificationError:return None
    string=raw.split(b"\x00",1)[0]
    if not 3<=len(string)<=160 or any(x<32 or x>126 for x in string):
        return None
    return string.decode("ascii")


def inspect_method(path:Path,pe:PEImage,address:int)->dict:
    if shutil.which("objdump") is None:
        raise ValueError("GNU binutils objdump not installed")
    process=subprocess.run(
        ["objdump","-D","-M","intel","--no-show-raw-insn",
         "--start-address="+hex(address),
         "--stop-address="+hex(address+BYTES_WINDOW),str(path)],
        text=True,capture_output=True,check=True,timeout=12,
    )
    calls=Counter()
    indirect=Counter()
    strings=set()
    decoded=0
    finished=False
    prev_ret=False
    last=address
    for line in process.stdout.splitlines():
        matched=LINE.match(line)
        if not matched:continue
        va=int(matched.group(1),16)
        if va<address:continue
        op=matched.group(2)
        args=matched.group(3).strip()
        if op=="int3" and prev_ret:
            finished=True
            break
        if op=="int3" and decoded==0:continue
        decoded+=1
        last=va
        if op=="call":
            dest=DIRECT.match(args)
            if dest:
                target=int(dest.group(1),16)
                if pe.section_of(target)==".text":
                    calls[f"0x{target:08X}"]+=1
            else:
                indirect[args]+=1
        if op in ("mov","lea","push"):
            for literal in HEX.findall(args):
                source_va=int(literal,16)
                content=read_string(pe,source_va)
                if content is not None:
                    strings.add((f"0x{source_va:08X}",content))
        if op.startswith("ret"):
            prev_ret=True
        elif op not in ("nop","int3"):
            prev_ret=False
    return {
        "original_x86_va":f"0x{address:08X}",
        "decoded_instructions":decoded,
        "last_instruction_va":f"0x{last:08X}",
        "ret_then_int3_end_found":finished,
        "body_may_be_truncated":not finished,
        "direct_calls":[{"callee_va":name,"count":count}
                        for name,count in calls.most_common()],
        "indirect_call_operands":dict(indirect),
        "referenced_strings":[{"string_va":va,"literal":value}
                              for va,value in sorted(strings)[:80]],
        "strings_truncated":len(strings)>80,
    }


def analyze(path:Path,kind:str)->dict:
    binary=path.read_bytes()
    rtti=scan(binary,kind)
    pe=PEImage(binary)
    owners={}
    for comparison in rtti["focus_override_comparisons"]:
        for changed in comparison["changed"]:
            key=changed["child_va"]
            owners.setdefault(key,[]).append({
                "derived_class":comparison["derived_class"],
                "base_class":comparison["base_class"],
                "virtual_slot":changed["slot"],
                "parent_va":changed["parent_va"],
            })
    methods=[]
    for address in sorted(owners,key=lambda x:int(x,16)):
        method=inspect_method(path,pe,int(address,16))
        method["ownership_evidence"]=owners[address]
        methods.append(method)
    return {
        "dll":kind+".dll","sha256":hashlib.sha256(binary).hexdigest(),
        "inspected_changed_virtual_function_targets":len(methods),
        "with_direct_calls":sum(bool(x["direct_calls"]) for x in methods),
        "with_immediate_string_references":sum(bool(x["referenced_strings"]) for x in methods),
        "heuristically_truncated":sum(x["body_may_be_truncated"] for x in methods),
        "methods":methods,
        "limitations":[
            "Only original changed vtable entries in the selected parent/derived comparisons.",
            "Method boundaries are compiler-padding guesses, NOT a validated control-flow graph.",
            "No debug symbols or exact C++ method names recovered from vtable slots.",
            "Unresolved indirect calls, inlined code, all nonvirtual methods and jump tables remain.",
            "References to literal strings alone cannot establish the full function behavior.",
            "None of this original Windows x86 machine code is executable on Quest ARM64.",
        ],
    }


def main()->int:
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--client",required=True,type=Path)
    p.add_argument("--server",required=True,type=Path)
    p.add_argument("--output",required=True,type=Path)
    a=p.parse_args()
    try:
        data={"client":analyze(a.client,"client"),"server":analyze(a.server,"server")}
        a.output.write_text(json.dumps(data,indent=2)+"\n",encoding="utf-8")
    except (OSError,ValueError,subprocess.SubprocessError) as e:
        p.exit(1,"SIN VIRTUAL XREF AUDIT FAILED: "+str(e)+"\n")
    print(json.dumps({side:{
        "changed_vfunc_targets":data[side]["inspected_changed_virtual_function_targets"],
        "with_direct_calls":data[side]["with_direct_calls"],
        "with_string_references":data[side]["with_immediate_string_references"],
        "possibly_truncated":data[side]["heuristically_truncated"],
    } for side in ("client","server")},indent=2))
    return 0


if __name__=="__main__":raise SystemExit(main())
