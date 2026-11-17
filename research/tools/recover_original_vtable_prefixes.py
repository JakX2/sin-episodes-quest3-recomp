#!/usr/bin/env python3
"""Authenticate owner-original SiN PE32 DLLs and seed reviewed interface vtables.

Outputs addresses and review metadata only, no code or pseudocode. Whole-file
SHA256 pins identify the original binaries, but do not verify method signatures.
"""
from __future__ import annotations

import argparse
import csv
import hashlib
import json
import struct
from dataclasses import dataclass
from pathlib import Path

MODULES = {
    "client.dll": {
        "sha256": "6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d",
        "size": 3592192, "base": 0x24000000,
        "pins": (
            ("GameClientExports001", 0x242FA4B0, 0x24296B18, 0x240FE9B0, 4),
            ("VClient011", 0x242FA4B8, 0x24296C6C, 0x240FE0E0, 48),
        ),
    },
    "server.dll": {
        "sha256": "568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650",
        "size": 6049792, "base": 0x22000000,
        "pins": (
            ("ServerGameDLL003", 0x22510650, 0x22442EFC, 0x221E7640, 31),
            ("ServerGameEnts001", 0x22510654, 0x22443030, 0x221E9360, 7),
            ("ServerGameClients003", 0x22510658, 0x22443054, 0x221E63F0, 15),
        ),
    },
}
SEED_FIELDS = ["entry", "interface", "slot", "source"]
SEED_PROVENANCE = "sha256_authenticated_original_pe32_vtable_prefix"


@dataclass(frozen=True)
class Section:
    name: str
    rva: int
    virtual_size: int
    raw_size: int
    raw_start: int
    characteristics: int

    @property
    def executable(self) -> bool:
        return bool(self.characteristics & 0x20000000)

    @property
    def readable(self) -> bool:
        return bool(self.characteristics & 0x40000000)


def u16(blob: bytes, offset: int) -> int:
    if offset < 0 or offset + 2 > len(blob):
        raise ValueError("PE32 read beyond original file")
    return struct.unpack_from("<H", blob, offset)[0]


def u32(blob: bytes, offset: int) -> int:
    if offset < 0 or offset + 4 > len(blob):
        raise ValueError("PE32 read beyond original file")
    return struct.unpack_from("<I", blob, offset)[0]


class PE32:
    def __init__(self, blob: bytes, expected_base: int):
        self.blob = blob
        if len(blob) < 0x100 or blob[:2] != b"MZ":
            raise ValueError("not MZ PE32")
        pe = u32(blob, 0x3C)
        if pe < 64 or pe + 24 > len(blob) or blob[pe:pe+4] != b"PE\x00\x00":
            raise ValueError("invalid PE32 header offset/signature")
        count = u16(blob, pe+6)
        optional_length = u16(blob, pe+20)
        characteristics = u16(blob, pe+22)
        opt = pe+24
        if (u16(blob, pe+4) != 0x14C or not 1 <= count <= 96
                or not (characteristics & 0x2000)
                or optional_length < 96 or opt+optional_length > len(blob)
                or u16(blob, opt) != 0x10B):
            raise ValueError("not an i386 PE32 DLL")
        self.base = u32(blob, opt+28)
        if self.base != expected_base:
            raise ValueError("PE32 image base does not match authenticated pin")
        header_end = opt+optional_length
        self.sections = []
        for i in range(count):
            pos = header_end+40*i
            if pos+40 > len(blob):
                raise ValueError("truncated PE32 section table")
            name = blob[pos:pos+8].split(b"\x00",1)[0].decode("ascii", "strict")
            vsize,rva,raw_size,raw_start = struct.unpack_from("<IIII",blob,pos+8)
            flags = u32(blob,pos+36)
            if raw_size and (raw_start < header_end or raw_start+raw_size > len(blob)):
                raise ValueError("invalid/truncated PE32 section raw data")
            self.sections.append(Section(name,rva,vsize,raw_size,raw_start,flags))
        if not any(section.executable for section in self.sections):
            raise ValueError("PE32 has no executable section")

    def locate(self, va: int, size: int = 4) -> tuple[Section,int]:
        if not self.base <= va <= 0xFFFFFFFF or size < 1:
            raise ValueError("VA outside PE32 image")
        rva = va-self.base
        matching = [section for section in self.sections
                    if section.rva <= rva and rva+size <= section.rva+section.raw_size]
        if len(matching) != 1:
            raise ValueError("VA not uniquely covered by file-backed PE32 section")
        section=matching[0]
        return section,section.raw_start+rva-section.rva

    def read_ptr(self,va:int) -> int:
        _,offset = self.locate(va,4)
        return u32(self.blob,offset)


def verify_and_recover(blob:bytes, expected:dict,
                       ghidra_starts:set[int]|None=None) -> dict:
    if len(blob)!=expected["size"] or hashlib.sha256(blob).hexdigest()!=expected["sha256"]:
        raise ValueError("original DLL full-file SHA256 or size mismatch")
    pe=PE32(blob,expected["base"])
    results=[]
    for name,object_va,vtable_va,first_va,count in expected["pins"]:
        if not 1 <= count <= 256:
            raise ValueError("vtable prefix slot count outside bounded range")
        object_section,_=pe.locate(object_va)
        table_section,_=pe.locate(vtable_va,count*4)
        if not (object_section.readable and object_section.name==".data"
                and table_section.readable and table_section.name==".rdata"):
            raise ValueError("original singleton or vtable outside reviewed sections")
        if pe.read_ptr(object_va)!=vtable_va:
            raise ValueError(f"{name}: original singleton vptr disagrees with reviewed pin")
        col_section,_=pe.locate(vtable_va-4)
        col_va=pe.read_ptr(vtable_va-4)
        resolved_col_section,_=pe.locate(col_va,20)
        if col_section.name!=".rdata" or resolved_col_section.name!=".rdata":
            raise ValueError(f"{name}: missing MSVC32 vtable[-1] RTTI locator")
        if pe.read_ptr(col_va)!=0 or pe.read_ptr(col_va+4)>4096:
            raise ValueError(f"{name}: unexpected MSVC32 RTTI metadata")
        type_desc,hierarchy=pe.read_ptr(col_va+12),pe.read_ptr(col_va+16)
        type_section,_=pe.locate(type_desc)
        hierarchy_section,_=pe.locate(hierarchy)
        if (type_section.name!=".data" or not type_section.readable
                or hierarchy_section.name!=".rdata" or not hierarchy_section.readable):
            raise ValueError(f"{name}: invalid RTTI TypeDescriptor/hierarchy sections")
        slots=[]
        known=0
        for index in range(count):
            target=pe.read_ptr(vtable_va+index*4)
            code_section,_=pe.locate(target,1)
            if not code_section.executable:
                raise ValueError(f"{name}: vtable target outside executable section")
            recognized=None if ghidra_starts is None else target in ghidra_starts
            if recognized:known+=1
            slots.append({
                "slot":index,"pointer_entry_va":f"0x{vtable_va+index*4:08X}",
                "target_va":f"0x{target:08X}",
                "ghidra_exact_function_entry":recognized,
                "newly_verified_method_body":False,
            })
        if slots[0]["target_va"]!=f"0x{first_va:08X}":
            raise ValueError(f"{name}: first method disagrees with reviewed pin")
        if len({slot["target_va"] for slot in slots})!=len(slots):
            raise ValueError(f"{name}: duplicate entry in pinned unique prefix")
        results.append({
            "interface":name,"original_singleton_va":f"0x{object_va:08X}",
            "original_vtable_va":f"0x{vtable_va:08X}",
            "original_msvc_col_va":f"0x{col_va:08X}",
            "prefix_slots":count,
            "ghidra_exact_function_entries":known if ghidra_starts is not None else None,
            "ghidra_missing_exact_entries":count-known if ghidra_starts is not None else None,
            "slots":slots,
        })
    return {
        "full_file_sha256":expected["sha256"],
        "original_file_size":len(blob),
        "image_base":f"0x{expected['base']:08X}",
        "observed_vtable_slots":sum(r["prefix_slots"] for r in results),
        "ghidra_exact_function_entries":sum(r["ghidra_exact_function_entries"] for r in results)
            if ghidra_starts is not None else None,
        "interfaces":results,
    }


def load_ghidra_starts(path:Path)->set[int]:
    with path.open(newline="",encoding="utf-8-sig") as inp:
        reader=csv.DictReader(inp)
        if not reader.fieldnames or "entry" not in reader.fieldnames:
            raise ValueError("Ghidra atlas lacks entry column")
        starts=set()
        for row in reader:
            raw=row["entry"]
            if not raw or len(raw)>16:
                raise ValueError("invalid Ghidra entry address")
            starts.add(int(raw.removeprefix("0x"),16))
        return starts


def recover(original_dir:Path, ghidra_dir:Path|None=None)->dict:
    modules={}
    for module,spec in MODULES.items():
        starts=load_ghidra_starts(ghidra_dir/f"{module}_functions.csv") \
            if ghidra_dir is not None else None
        modules[module]=verify_and_recover((original_dir/module).read_bytes(),spec,starts)
    observed=sum(item["observed_vtable_slots"] for item in modules.values())
    known=sum(item["ghidra_exact_function_entries"] for item in modules.values()
              if item["ghidra_exact_function_entries"] is not None)
    return {
        "schema":"sin-owner-original-interface-vtable-prefix-v1",
        "evidence_tier":"SHA256_authenticated_original_PE32_vtable_pointer_values_only",
        "ghidra_atlas_compared":ghidra_dir is not None,
        "total_confirmed_vtable_pointer_values":observed,
        "preexisting_ghidra_function_entry_matches":known if ghidra_dir is not None else None,
        "potential_missing_ghidra_entries":observed-known if ghidra_dir is not None else None,
        "newly_verified_original_method_bodies":0,
        "newly_reviewed_original_method_signatures":0,
        "engine_load_tests":0,
        "original_code_or_pseudocode_embedded":False,
        "modules":modules,
    }


def write_report(report:dict,output_dir:Path)->None:
    output_dir.mkdir(parents=True,exist_ok=True)
    (output_dir/"original_vtable_prefix_review.json").write_text(
        json.dumps(report,sort_keys=True,indent=2)+"\n",encoding="utf-8")
    for module,data in report["modules"].items():
        with (output_dir/f"{module}_vtable_method_seeds.csv").open(
                "w",newline="",encoding="utf-8") as out:
            writer=csv.DictWriter(out,fieldnames=SEED_FIELDS)
            writer.writeheader()
            rows=[]
            for interface in data["interfaces"]:
                for slot in interface["slots"]:
                    rows.append({
                        "entry":slot["target_va"][2:].lower(),
                        "interface":interface["interface"],
                        "slot":slot["slot"],
                        "source":SEED_PROVENANCE,
                    })
            writer.writerows(sorted(rows,key=lambda x:(x["entry"],x["interface"],x["slot"])))


def main()->None:
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--original-dir",required=True,type=Path)
    parser.add_argument("--ghidra-dir",type=Path)
    parser.add_argument("--out-dir",required=True,type=Path)
    args=parser.parse_args()
    report=recover(args.original_dir,args.ghidra_dir)
    write_report(report,args.out_dir)
    print(json.dumps({
        "slots":report["total_confirmed_vtable_pointer_values"],
        "ghidra_preexisting":report["preexisting_ghidra_function_entry_matches"],
        "missing_exact_function_entries":report["potential_missing_ghidra_entries"],
        "original_verified_bodies_added":0,
    },sort_keys=True))


if __name__=="__main__":
    main()
