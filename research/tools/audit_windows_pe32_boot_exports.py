#!/usr/bin/env python3
"""Offline PE32/x86 game DLL export preflight; never emits binary bytes."""
import argparse
import json
import struct
from pathlib import Path


class PEError(ValueError):
    pass


def inspect(path):
    data = Path(path).read_bytes()

    def u16(o):
        if o < 0 or o + 2 > len(data):
            raise PEError("truncated PE field")
        return struct.unpack_from("<H", data, o)[0]

    def u32(o):
        if o < 0 or o + 4 > len(data):
            raise PEError("truncated PE field")
        return struct.unpack_from("<I", data, o)[0]

    if len(data) < 64 or data[:2] != b"MZ":
        raise PEError("missing DOS header")
    pe = u32(0x3c)
    if pe + 24 > len(data) or data[pe:pe+4] != b"PE\x00\x00":
        raise PEError("missing NT header")
    machine, count, opt_size = u16(pe+4), u16(pe+6), u16(pe+20)
    opt = pe+24
    if not 1 <= count <= 96 or opt+opt_size+count*40 > len(data):
        raise PEError("invalid section table")
    if opt_size < 104 or u16(opt) != 0x10b or machine != 0x14c:
        raise PEError("not Windows x86 PE32")
    if u32(opt+92) < 1:
        raise PEError("missing export data directory")
    export_rva, export_size = u32(opt+96), u32(opt+100)
    if not export_rva or export_size < 40 or export_rva+export_size > 0x100000000:
        raise PEError("invalid export directory")
    sections = []
    for i in range(count):
        s = opt+opt_size+i*40
        sections.append((u32(s+12), max(u32(s+8), u32(s+16)),
                         u32(s+20), u32(s+16)))

    def offset(rva, size=1):
        if size < 0 or rva+size > 0x100000000:
            raise PEError("invalid RVA range")
        for base, extent, raw, raw_size in sections:
            if base <= rva and rva+size <= base+extent and rva+size <= base+raw_size:
                o = raw+rva-base
                if o+size <= len(data):
                    return o
        raise PEError("unmapped or truncated RVA")

    ed = offset(export_rva, 40)
    funcs, names = u32(ed+20), u32(ed+24)
    if funcs == 0 or funcs > 100000 or names > funcs:
        raise PEError("invalid export counts")
    function_table, name_table, ordinal_table = u32(ed+28), u32(ed+32), u32(ed+36)
    offset(function_table, funcs*4)
    offset(name_table, names*4)
    offset(ordinal_table, names*2)
    exports = []
    create_interface_valid = False
    for i in range(names):
        rva = u32(offset(name_table+i*4, 4))
        start = offset(rva)
        end = data.find(b"\x00", start, min(len(data), start+512))
        if end < 0:
            raise PEError("unterminated export name")
        try:
            name = data[start:end].decode("ascii")
        except UnicodeDecodeError as exc:
            raise PEError("non-ASCII export name") from exc
        ordinal = u16(offset(ordinal_table+i*2, 2))
        if ordinal >= funcs:
            raise PEError("export ordinal out of bounds")
        target_rva = u32(offset(function_table+ordinal*4, 4))
        if target_rva == 0:
            raise PEError("named export has null function RVA")
        forwarded = export_rva <= target_rva < export_rva+export_size
        if forwarded:
            start_forwarder = offset(target_rva)
            if data.find(b"\x00", start_forwarder, min(len(data), start_forwarder+512)) < 0:
                raise PEError("unterminated export forwarder")
        else:
            offset(target_rva)
        if name == "CreateInterface" and not forwarded:
            create_interface_valid = True
        exports.append(name)
    return {"pe32_x86": True, "create_interface_export": create_interface_valid,
            "export_count": len(exports), "exports": sorted(exports)}


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("client", type=Path)
    ap.add_argument("server", type=Path)
    ap.add_argument("--show-exports", action="store_true")
    args = ap.parse_args()
    result = {}
    for label, path in (("client", args.client), ("server", args.server)):
        try:
            item = inspect(path)
            if not args.show_exports:
                item.pop("exports")
            result[label] = {"status": "pass" if item["create_interface_export"] else "fail", **item}
        except (OSError, PEError) as exc:
            result[label] = {"status": "fail", "error": str(exc)}
    result["overall"] = "pass" if all(result[k]["status"] == "pass" for k in ("client", "server")) else "fail"
    print(json.dumps(result, indent=2, sort_keys=True))
    return 0 if result["overall"] == "pass" else 1


if __name__ == "__main__":
    raise SystemExit(main())
