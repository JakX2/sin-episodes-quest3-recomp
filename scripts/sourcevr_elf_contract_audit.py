#!/usr/bin/env python3
"""Bounded, read-only ELF64 dynamic-linkage reconnaissance for SourceVR modules.

Use on ONE extracted owner-held .so at a time. This is *not* an engine loader,
linker, compatibility shim, APK extractor, or proof of runtime compatibility.
Python standard library only. Never commit private ELF input or audit output.
"""
import argparse
import json
from pathlib import Path
import struct
import sys

MAX_FILE = 512 * 1024 * 1024
MAX_SECTIONS = 8192
MAX_STRING_TABLE = 8 * 1024 * 1024
MAX_SYMBOLS = 200000
MAX_SAMPLES = 30

DYN_SYMBOLS = 11
DYNAMIC = 6
NEEDED = 1
SONAME = 14
SHT_NOBITS = 8
EM = {3: "x86", 40: "arm", 62: "x86_64", 183: "aarch64"}
ENTRY_POINTS = (
    "CreateInterface",
    "GiveFnptrsToDll",
    "GetEntityAPI2",
    "GetEntityAPI",
    "GetNewDLLFunctions",
)
INTERFACE_HINTS = ("VClient", "VEngine", "ServerGame", "VGUI", "ClientDLL", "VPhysics")


class ElfAuditError(ValueError):
    pass


def unpack_at(fmt, data, offset=0):
    size = struct.calcsize(fmt)
    if offset < 0 or offset + size > len(data):
        raise ElfAuditError("truncated ELF metadata")
    return struct.unpack_from(fmt, data, offset)


def checked_region(offset, size, file_size, max_size):
    if offset < 0 or size < 0 or size > max_size or offset > file_size - size:
        raise ElfAuditError("ELF section is outside file or exceeds inspection limit")
    return offset, size


def read_region(file, file_size, offset, size, maximum):
    checked_region(offset, size, file_size, maximum)
    file.seek(offset)
    data = file.read(size)
    if len(data) != size:
        raise ElfAuditError("short read from ELF input")
    return data


def get_string(table, offset):
    if offset < 0 or offset >= len(table):
        return ""
    stop = table.find(b"\0", offset)
    if stop < 0:
        return ""
    return table[offset:stop].decode("utf-8", errors="replace")


def section_headers(file, size, header):
    # ELF64 header has SH offset, entry size, section count and name table index.
    shoff, shentsize, shnum = header[5], header[10], header[11]
    if shnum == 0 or shoff == 0:
        return []
    if shnum > MAX_SECTIONS or shentsize < 64 or shentsize > 512:
        raise ElfAuditError("unsupported or oversized section header table")
    table_size = shentsize * shnum
    data = read_region(file, size, shoff, table_size, MAX_SECTIONS * 512)
    sections = []
    for index in range(shnum):
        section = unpack_at("<IIQQQQIIQQ", data, index * shentsize)
        # (name, type, flags, addr, offset, size, link, info, align, entry_size)
        if section[1] != SHT_NOBITS:
            checked_region(section[4], section[5], size, MAX_FILE)
        sections.append(section)
    return sections


def audit_elf(path):
    source = Path(path)
    file_size = source.stat().st_size
    if file_size < 64 or file_size > MAX_FILE:
        raise ElfAuditError("ELF must be between 64 bytes and 512 MiB")
    with source.open("rb") as stream:
        ident = stream.read(64)
        if ident[:4] != b"\x7fELF":
            raise ElfAuditError("not an ELF binary")
        if ident[4] != 2 or ident[5] != 1:
            raise ElfAuditError("only little-endian ELF64 is supported")
        if ident[6] != 1:
            raise ElfAuditError("unrecognized ELF version")
        header = unpack_at("<HHIQQQIHHHHHH", ident, 16)
        machine = header[1]
        sections = section_headers(stream, file_size, header)

        needed = []
        soname = None
        dynamic_present = False
        dynsym_present = False
        exports = set()
        imports = set()
        export_count = import_count = 0
        for section in sections:
            kind, offset, length, linked, ent_size = (
                section[1], section[4], section[5], section[6], section[9]
            )
            if kind not in (DYN_SYMBOLS, DYNAMIC):
                continue
            if linked >= len(sections):
                raise ElfAuditError("dynamic section has invalid string-table link")
            str_section = sections[linked]
            if str_section[1] != 3:  # SHT_STRTAB
                raise ElfAuditError("dynamic section is not linked to a string table")
            strings = read_region(
                stream, file_size, str_section[4], str_section[5], MAX_STRING_TABLE
            )
            if kind == DYNAMIC:
                dynamic_present = True
                if ent_size not in (0, 16):
                    raise ElfAuditError("unexpected ELF64 dynamic entry size")
                if length % 16 != 0:
                    raise ElfAuditError("partial ELF64 dynamic entry")
                raw = read_region(stream, file_size, offset, length, MAX_STRING_TABLE)
                for pos in range(0, len(raw), 16):
                    tag, value = unpack_at("<qQ", raw, pos)
                    if tag == 0:
                        break
                    if tag == NEEDED:
                        name = get_string(strings, value)
                        if name and name not in needed:
                            needed.append(name)
                    elif tag == SONAME:
                        soname = get_string(strings, value) or None
            if kind == DYN_SYMBOLS:
                dynsym_present = True
                if ent_size != 24 or length % 24 != 0 or length // 24 > MAX_SYMBOLS:
                    raise ElfAuditError("unexpected ELF64 dynamic symbol table layout")
                raw = read_region(stream, file_size, offset, length, MAX_SYMBOLS * 24)
                for pos in range(0, len(raw), 24):
                    name_offset, info, other, section_idx, value, sym_size = unpack_at(
                        "<IBBHQQ", raw, pos
                    )
                    name = get_string(strings, name_offset)
                    if not name:
                        continue
                    binding = info >> 4
                    symbol_type = info & 15
                    if binding not in (1, 2) or symbol_type in (3, 4):
                        continue
                    if section_idx == 0:
                        imports.add(name)
                        import_count += 1
                    else:
                        exports.add(name)
                        export_count += 1

    entry_checks = {name: name in exports for name in ENTRY_POINTS}
    interface_hints = sorted(
        name for name in exports
        if any(hint in name for hint in INTERFACE_HINTS)
    )[:MAX_SAMPLES]
    # Only give compact examples; never serialize an entire proprietary symbol table.
    sample_imports = sorted(
        name for name in imports
        if "CreateInterface" in name or "dlopen" in name
        or "dlsym" in name or "JNI_" in name
    )[:MAX_SAMPLES]
    warnings = [
        "A dynamic symbol may be absent because of visibility/stripping.",
        "CreateInterface export alone does not prove SourceVR loads third-party game DLLs.",
        "ELF symbol names and DT_NEEDED do not establish SDK headers, engine version or ABI compatibility.",
        "This parser only inspects ELF64 little-endian section metadata; no execution takes place.",
    ]
    if not sections:
        warnings.append("No ELF section-header table; symbol/import details remain unknown.")
    if not dynsym_present:
        warnings.append("No SHT_DYNSYM found; exported entry-point status is unknown, not disproven.")
    if not dynamic_present:
        warnings.append("No SHT_DYNAMIC found; dependent library list may be incomplete.")
    return {
        "format_version": 1,
        "file_name": source.name,
        "file_size": file_size,
        "elf_class": "ELF64",
        "endianness": "little",
        "machine_id": machine,
        "machine": EM.get(machine, "unknown_%d" % machine),
        "elf_type": header[0],
        "has_section_headers": bool(sections),
        "has_dynamic_symbols": dynsym_present,
        "has_dynamic_entries": dynamic_present,
        "soname": soname,
        "needed": needed,
        "export_count": export_count,
        "import_count": import_count,
        "entry_point_exports": entry_checks if dynsym_present else None,
        "interface_name_hints": interface_hints,
        "runtime_linker_import_samples": sample_imports,
        "warnings": warnings,
    }


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("library", type=Path, help="One private, extracted ELF .so")
    parser.add_argument("-o", "--output", type=Path, default=Path("sourcevr-elf-audit.json"))
    args = parser.parse_args(argv)
    try:
        report = audit_elf(args.library)
        args.output.write_text(
            json.dumps(report, indent=2, ensure_ascii=False) + "\n", encoding="utf-8"
        )
        print("%s: %s, exported=%d, needed=%d" % (
            report["file_name"], report["machine"], report["export_count"], len(report["needed"])
        ))
        print("Wrote:", args.output)
        return 0
    except (OSError, ElfAuditError) as exc:
        print("ELF interface inspection failed:", exc, file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
