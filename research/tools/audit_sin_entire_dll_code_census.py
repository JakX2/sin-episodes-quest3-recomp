#!/usr/bin/env python3
"""Original SiN client/server PE32 code-entry census.

Counts original VFTABLE targets and directly decoded x86 CALL rel32 targets,
plus diagnostic data references. THIS IS NOT a function decompiler: linear
disassembly can decode data as code, and CALL targets need not be C++ methods.
Original user-owned binaries remain private and are SHA-pinned.
"""
from __future__ import annotations
import argparse
from collections import Counter, defaultdict
import csv
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess

HASHES = {
 "client":"6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d",
 "server":"568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650",
}
BASES={"client":0x24000000,"server":0x22000000}
LINE=re.compile(r"^\s*([0-9a-f]{8,}):\s+([a-zA-Z_.][\w.]*)\s*(.*)$")
DIRECT=re.compile(r"^(?:0x)?([0-9a-fA-F]{7,8})(?:\s|$|<)")
IMMED=re.compile(r"(?<![0-9a-zA-Z_])0x([0-9a-fA-F]{7,8})(?![0-9a-zA-Z_])")
KEYWORDS=("npc_","weapon_","grenade","sintek","quadralex","poison",
          "antigen","difficulty","phonebook","grun","clientmode",
          "viewmodel","sound_danger","squad","hud","arnade",
          "screen","worldspawn","createentity","prediction","detonate",
          "anim","vehicle","decap")
KNOWN={
 "server":{
  0x22287CB0:"UTIL_PrecacheOther resource helper",
  0x22266B00:"CreateEntityByName entity factory",
  0x22149B70:"model precache",
  0x221B6170:"script sound precache",
  0x22313C60:"Assault Rifle live grenade launch",
  0x22314570:"CGrenadeAR Detonate",
  0x22314360:"CGrenadeAR flight think and AI warning",
  0x223142F0:"CGrenadeAR touch callback",
  0x22314B10:"CGrenadeAR initialization",
  0x221B8FA0:"NPC AI sound event dispatcher",
  0x222A5E10:"blast radius damage and AI combat event",
  0x22288A90:"screen-shake helper",
  0x223027C0:"CPropPhone initialization and precache",
  0x2235DE00:"CNPC_GruntBase resources",
  0x223746B0:"CNPC_Quadralex resources",
  0x22313780:"Assault Rifle grenade precache",
  0x22146C40:"live grenade entity spawn helper",
 },
 "client":{0x24164180:"exported CreateInterface registry resolver"},
}
class CensusError(ValueError):pass

class PE:
    def __init__(self,b:bytes):
        self.b=b
        if len(b)<512 or b[:2]!=b"MZ":raise CensusError("expected PE32 MZ")
        nt,=struct.unpack_from("<I",b,0x3c)
        if nt+256>len(b) or b[nt:nt+4]!=b"PE\0\0":
            raise CensusError("invalid PE header")
        count,=struct.unpack_from("<H",b,nt+6)
        options,=struct.unpack_from("<H",b,nt+20)
        op=nt+24
        if struct.unpack_from("<H",b,op)[0]!=0x10b or not 1<=count<=32:
            raise CensusError("expected x86 PE32")
        self.base,=struct.unpack_from("<I",b,op+28)
        self.sections={}
        for i in range(count):
            p=op+options+i*40
            if p+40>len(b):raise CensusError("section header truncated")
            name=b[p:p+8].split(b"\0")[0].decode("ascii","replace")
            _,rva,size,off=struct.unpack_from("<IIII",b,p+8)
            if off+size>len(b):raise CensusError("section raw data truncated")
            self.sections[name]=(self.base+rva,off,size)
    def section(self,name:str)->tuple[int,bytes]:
        if name not in self.sections:raise CensusError("missing "+name)
        start,off,size=self.sections[name]
        return start,self.b[off:off+size]
    def in_section(self,va:int,name:str)->bool:
        start,off,size=self.sections[name]
        return start<=va<start+size

def direct(operand:str)->int|None:
    match=DIRECT.match(operand)
    return int(match[1],16) if match else None

def rdata_string(pe:PE,va:int)->str|None:
    if not pe.in_section(va,".rdata"):return None
    start,data=pe.section(".rdata")
    off=va-start
    end=data.find(b"\0",off,min(off+180,len(data)))
    if end<0 or not 4<=end-off<=160:return None
    payload=data[off:end]
    if not all(32<=x<127 for x in payload):return None
    return payload.decode("ascii")

def inspect_lines(lines:list[str],text_start:int,text_length:int,
                  vtable_owners:dict[int,list[str]],pe:PE)->tuple[dict,list[dict]]:
    called=defaultdict(list)
    jumped=defaultdict(list)
    nearby=defaultdict(set)
    decoded=direct_count=indirect_count=branches=direct_jumps=0
    for line in lines:
        match=LINE.match(line)
        if not match:continue
        va=int(match[1],16)
        if not text_start<=va<text_start+text_length:continue
        decoded+=1
        op=match[2].lower()
        operand=match[3].strip()
        if op=="call":
            target=direct(operand)
            if target is not None and text_start<=target<text_start+text_length:
                called[target].append(va);direct_count+=1
            elif target is None:indirect_count+=1
        elif op.startswith("j") or op in ("loop","loope","loopne","jecxz"):
            branches+=1
            if op in ("jmp","jmpq"):
                target=direct(operand)
                if target is not None and text_start<=target<text_start+text_length:
                    jumped[target].append(va);direct_jumps+=1
        if op in ("mov","push","lea"):
            for m in IMMED.finditer(operand):
                s=rdata_string(pe,int(m[1],16))
                if s is not None:nearby[va].add(s)
    candidates=set(vtable_owners)|set(called)
    # Sort string references once so 20k candidates can be indexed cheaply.
    site_refs=sorted(nearby.items())
    refs=Counter()
    for _,strings in site_refs:
        for s in strings:
            if any(word in s.lower() for word in KEYWORDS):refs[s]+=1
    addresses=[x[0] for x in site_refs]
    from bisect import bisect_left
    rows=[]
    for va in sorted(candidates):
        classes=vtable_owners.get(va,[])
        incoming=len(called.get(va,[]))
        start=bisect_left(addresses,va)
        strings=[]
        for site,values in site_refs[start:]:
            if site>=va+256:break
            strings.extend(sorted(values))
        rows.append({
            "entry_va":f"0x{va:08X}",
            "seed_evidence":("vtable+direct_call" if classes and incoming else
                             "vtable" if classes else "direct_call"),
            "vtable_owner_count":len(classes),"owner_examples":classes[:6],
            "direct_callsite_count":incoming,
            "direct_jump_to_entry_count":len(jumped.get(va,[])),
            "nearby_string_examples":strings[:7],
            "priority_hint":(
                any(any(k in c.lower() for k in KEYWORDS) for c in classes) or
                any(any(k in s.lower() for k in KEYWORDS) for s in strings)),
            "recovery_status":"candidate_only_no_function_boundary_or_C++_semantics",
        })
    return ({
        "linear_decoded_instruction_count":decoded,
        "direct_call_instructions_to_text":direct_count,
        "indirect_call_instructions":indirect_count,
        "branch_instructions":branches,
        "direct_unconditional_jumps_to_text":direct_jumps,
        "vtable_seed_unique":len(vtable_owners),
        "direct_call_target_unique":len(called),
        "union_entrypoint_candidates":len(candidates),
        "vtable_direct_call_overlap":len(set(vtable_owners)&set(called)),
        "game_script_string_reference_examples":refs.most_common(100),
    },rows)

def inspect_original(path:Path,kind:str,original_rtti:dict)->tuple[dict,list[dict]]:
    blob=path.read_bytes()
    digest=hashlib.sha256(blob).hexdigest()
    if kind not in HASHES or digest!=HASHES[kind]:
        raise CensusError("original "+kind+" DLL SHA256 mismatch")
    if original_rtti["sha256"]!=digest:
        raise CensusError("RTTI inventory does not match original DLL")
    pe=PE(blob)
    if pe.base!=BASES[kind]:raise CensusError("unexpected original image base")
    start,text=pe.section(".text")
    owners=defaultdict(list)
    for vt in original_rtti["vftables"]:
        for addr in vt["function_va"]:
            target=int(addr,16)
            if pe.in_section(target,".text"):
                if len(owners[target])<16 and vt["class"] not in owners[target]:
                    owners[target].append(vt["class"])
    p=subprocess.run(["objdump","-d","-M","intel","--no-show-raw-insn",str(path)],
                     capture_output=True,text=True,check=True,timeout=180)
    counts,rows=inspect_lines(p.stdout.splitlines(),start,len(text),owners,pe)
    return ({
        "dll":kind+".dll","sha256":digest,"original_image_base":f"0x{pe.base:08X}",
        "text_start":f"0x{start:08X}","text_section_bytes":len(text),
        "original_rtti_classes":original_rtti["counts"]["classes_with_hierarchy"],
        "original_vtable_count":original_rtti["counts"]["distinct_vftables"],
        "counts":counts,
        "known_code_address_meanings":{f"0x{va:08X}":s for va,s in KNOWN[kind].items()},
        "limitations":[
            "A potential code entrypoint is NOT a validated C++ function or its start/end boundaries.",
            "Linear objdump decoding can traverse embedded data; direct CALL target is only evidence.",
            "Indirect calls, thunks, callbacks, function pointers and tail jumps need independent analysis.",
            "A string within 256 bytes of a seed is a prioritization hint, NOT proof of method ownership.",
            "Original Windows x86 code and vtables cannot be executed by Quest ARM64.",
            "Full original game behavior and reverse engineering remain unfinished.",
        ],
    },rows)

def write_rows(path:Path,kind:str,rows:list[dict])->None:
    fields=("dll","entry_va","seed_evidence","vtable_owner_count",
            "owner_examples","direct_callsite_count",
            "direct_jump_to_entry_count","nearby_string_examples",
            "priority_hint","recovery_status")
    with path.open("w",newline="",encoding="utf-8") as stream:
        w=csv.DictWriter(stream,fieldnames=fields)
        w.writeheader()
        for row in rows:
            values={k:" | ".join(row[k]) if isinstance(row.get(k),list)
                    else row.get(k) for k in fields}
            values["dll"]=kind+".dll"
            w.writerow(values)

def main()->int:
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--client",type=Path,required=True)
    p.add_argument("--server",type=Path,required=True)
    p.add_argument("--rtti",type=Path,required=True,
                   help="JSON from prior original 2006 RTTI/VFTABLE scanner")
    p.add_argument("--output-dir",type=Path,required=True)
    args=p.parse_args()
    try:
        args.output_dir.mkdir(parents=True,exist_ok=True)
        rtti=json.loads(args.rtti.read_text(encoding="utf-8"))
        summary={}
        for kind in ("client","server"):
            report,rows=inspect_original(getattr(args,kind),kind,rtti[kind])
            prefix="SiN_2006_"+kind.capitalize()
            (args.output_dir/(prefix+"_Whole_DLL_Code_Census.json")).write_text(
                json.dumps(report,indent=2)+"\n",encoding="utf-8")
            write_rows(args.output_dir/(prefix+"_Code_Entry_Candidates.csv"),kind,rows)
            summary[kind]={
                "sha256":report["sha256"],"text_section_bytes":report["text_section_bytes"],
                "rtti_classes":report["original_rtti_classes"],
                **{k:v for k,v in report["counts"].items() if not isinstance(v,list)},
            }
        (args.output_dir/"SiN_2006_DLL_Code_Census_Summary.json").write_text(
            json.dumps(summary,indent=2)+"\n",encoding="utf-8")
        print(json.dumps(summary,indent=2))
    except (OSError,ValueError,subprocess.SubprocessError,KeyError,struct.error) as e:
        p.exit(1,"SiN full DLL census failed: "+str(e)+"\n")
    return 0

if __name__=="__main__":
    raise SystemExit(main())
