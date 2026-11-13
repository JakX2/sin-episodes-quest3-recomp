#!/usr/bin/env python3
"""Phase204: SHA-gated PE32 import preflight for owner-original SiN 2006 DLLs.

Read-only, stdlib-only. Does not embed/distribute original DLL bytes.
Run with --archive owner-local/bin.zip OR --server/--client owner-local DLLs.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
import zipfile
from pathlib import Path
from audit_sin_original_dll_interface_abi import PEImage, VerificationError

ORIGINALS = {
    "server": {
        "sha256": "568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650",
        "image_base": 0x22000000,
        "libs": [("KERNEL32.dll",59),("USER32.dll",3),("vstdlib.dll",27),("tier0.dll",26)],
    },
    "client": {
        "sha256": "6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d",
        "image_base": 0x24000000,
        "libs": [("WINMM.dll",3),("KERNEL32.dll",74),("USER32.dll",6),
                 ("GDI32.dll",22),("ADVAPI32.dll",3),("vstdlib.dll",30),("tier0.dll",16)],
    },
}

def _cstr(pe: PEImage, rva: int, max_length: int = 256) -> str:
    if not rva:
        raise VerificationError("zero PE import string RVA")
    raw = bytearray()
    for pos in range(max_length):
        byte = pe.at(pe.base + rva + pos, 1)[0]
        if byte == 0:
            if not raw: raise VerificationError("empty PE import name")
            return raw.decode("ascii")
        if not 0x20 <= byte < 0x7f:
            raise VerificationError("invalid PE import-name character")
        raw.append(byte)
    raise VerificationError("unterminated PE import name")

def parse_imports(pe: PEImage) -> list[dict]:
    data = pe.data
    header = struct.unpack_from("<I",data,0x3c)[0]
    optional = header + 24
    if optional + 104 > len(data):
        raise VerificationError("truncated optional header")
    if struct.unpack_from("<I",data,optional+92)[0] < 2:
        raise VerificationError("missing PE import directory entry")
    rva, size = struct.unpack_from("<II",data,optional+104)
    if not rva or size < 20 or size > 0x10000:
        raise VerificationError("invalid import directory")
    modules = []
    for index in range(min(size // 20,64)):
        oft, stamp, chain, name, iat = struct.unpack(
            "<IIIII", pe.at(pe.base + rva + index*20,20))
        if (oft,stamp,chain,name,iat) == (0,0,0,0,0):
            if not modules: raise VerificationError("empty import directory")
            return modules
        if not name or not iat:
            raise VerificationError("invalid PE import descriptor")
        dll = _cstr(pe,name,128)
        if any(module["dll"].casefold() == dll.casefold() for module in modules):
            raise VerificationError("duplicate import library")
        table = oft or iat
        symbols = []
        for j in range(4096):
            word = struct.unpack("<I",pe.at(pe.base+table+j*4,4))[0]
            if not word: break
            if word & 0x80000000:
                if word & 0x7fff0000:
                    raise VerificationError("reserved ordinal thunk bits")
                symbols.append({"ordinal":word & 0xffff})
            else:
                hint = struct.unpack("<H",pe.at(pe.base+word,2))[0]
                symbols.append({"name":_cstr(pe,word+2,320),"hint":hint})
        else:
            raise VerificationError("unterminated import lookup table")
        if not symbols: raise VerificationError("empty import lookup")
        modules.append({"dll":dll,"symbols":symbols})
    raise VerificationError("import descriptor terminator absent")

def inspect_one(data: bytes, role: str, *, verify_original: bool=True) -> dict:
    if role not in ORIGINALS: raise VerificationError("unknown game DLL role")
    expected = ORIGINALS[role]
    digest = hashlib.sha256(data).hexdigest()
    if verify_original and digest != expected["sha256"]:
        raise VerificationError("original "+role+".dll full-file SHA256 mismatch")
    pe = PEImage(data)
    if verify_original and pe.base != expected["image_base"]:
        raise VerificationError("original "+role+" PE32 image base mismatch")
    imports = parse_imports(pe)
    summary = [(m["dll"],len(m["symbols"])) for m in imports]
    if verify_original and summary != expected["libs"]:
        raise VerificationError("original "+role+" import dependency count drift")
    return {
        "role":role,"sha256":digest,"original_verified":verify_original,
        "image_base":hex(pe.base),"import_library_count":len(imports),
        "import_symbol_count":sum(n for _,n in summary),
        "libraries":[{"name":m["dll"],"count":len(m["symbols"]),
                      "by_ordinal":sum("ordinal" in v for v in m["symbols"])}
                     for m in imports],
    }

def audit(server: bytes, client: bytes) -> dict:
    a,b = inspect_one(server,"server"),inspect_one(client,"client")
    libs_a = {x["name"].lower() for x in a["libraries"]}
    libs_b = {x["name"].lower() for x in b["libraries"]}
    return {
        "original_2006_dll_imports":{"server":a,"client":b},
        "combined_import_entries":a["import_symbol_count"]+b["import_symbol_count"],
        "shared_libraries":sorted(libs_a & libs_b),
        "direct_engine_dll_import":"engine.dll" in (libs_a|libs_b),
        "limits":[
            "Original PE import tables do not describe all Source engine callback interfaces.",
            "Original CreateInterface negotiation and real Windows engine startup are unproven.",
            "No original code, game binaries or assets are distributed.",
        ],
    }

def main() -> None:
    p = argparse.ArgumentParser(description=__doc__)
    input_group = p.add_mutually_exclusive_group(required=True)
    input_group.add_argument("--archive",type=Path)
    input_group.add_argument("--server",type=Path)
    p.add_argument("--client",type=Path)
    p.add_argument("--output",type=Path)
    args = p.parse_args()
    if args.archive:
        if args.client: p.error("do not combine --archive with --client")
        try:
            with zipfile.ZipFile(args.archive) as archive:
                names=archive.namelist()
                if names.count("server.dll") != 1 or names.count("client.dll") != 1:
                    raise VerificationError("archive must contain one root server.dll and client.dll")
                server,client=archive.read("server.dll"),archive.read("client.dll")
        except zipfile.BadZipFile as exc:
            raise VerificationError("invalid owner archive") from exc
    else:
        if not args.client: p.error("--server requires --client")
        server,client=args.server.read_bytes(),args.client.read_bytes()
    report=json.dumps(audit(server,client),indent=2)+"\n"
    if args.output: args.output.write_text(report,encoding="utf-8")
    else: print(report,end="")

if __name__ == "__main__":
    main()
