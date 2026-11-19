#!/usr/bin/env python3
"""SHA-pinned, owner-only original 2006 SiN filesystem ABI audit.

Only call with the owner's original FileSystem_Steam.dll, or --self-test.
No original code, data, pseudocode or binary is exported to the repository.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path

PIN = "1a1675fa4b459df19f95a85ea9059eecea01f7862d2b153051385171da79d14b"
BASE = 0x10000000
OWNER = 0x1F4F0
PRIMARY = 0x1B630
SECONDARY = 0x1B61C
REGISTRATIONS = (
    ("VFileSystem012", 0x1BA6C, 0x19900, 0xD6E0, 0x1F4E0),
    ("VBaseFileSystem007", 0x1BA7C, 0x19920, 0xD6F0, 0x1F4D4),
)
PRIMARY_SLOTS = (0x7C90, 0x7D30, 0x8D00, 0x7920, 0x2C20, 0x2CE0,
                 0x8DF0, 0x2D60, 0x2F30, 0x2FC0, 0x7FD0, 0x5280,
                 0x5400, 0x7DD0)
SECONDARY_SLOTS = (0x1500, 0x1510, 0x1520, 0xC760, 0xC7A0)
# First directly verified callee-cleanup return inside each target body.
RETURNS = {0: (0x7CE5, 12), 2: (0x8DE3, 12), 3: (0x7966, 4),
           6: (0x8E55, 8), 7: (0x2DB2, 4), 10: (0x80DC, 8)}


def require(condition, error):
    if not condition:
        raise ValueError(error)


class PE:
    def __init__(self, data):
        self.data = data
        require(data[:2] == b"MZ" and len(data) >= 512, "bad DOS header")
        pe = struct.unpack_from("<I", data, 0x3C)[0]
        require(pe + 248 <= len(data) and data[pe:pe+4] == b"PE\0\0",
                "bad PE header")
        machine, count = struct.unpack_from("<HH", data, pe+4)
        optsize = struct.unpack_from("<H", data, pe+20)[0]
        require(machine == 0x14C and count == 4 and optsize == 224,
                "not the expected 32-bit PE sections")
        require(struct.unpack_from("<H", data, pe+24)[0] == 0x10B,
                "not PE32")
        self.base = struct.unpack_from("<I", data, pe+52)[0]
        self.timestamp = struct.unpack_from("<I", data, pe+8)[0]
        require(self.base == BASE, "image base mismatch")
        self.sections = {}
        for i in range(count):
            off = pe+24+optsize+40*i
            require(off+40 <= len(data), "truncated section header")
            name = data[off:off+8].split(b"\0", 1)[0].decode("ascii")
            vs, va, rawsize, rawoff = struct.unpack_from("<IIII", data, off+8)
            require(rawoff+rawsize <= len(data), "truncated section raw data")
            self.sections[name] = (va, vs, rawoff, rawsize)
        require(set(self.sections) == {".text", ".rdata", ".data", ".reloc"},
                "unexpected section set")
        require(self.sections[".data"][1] > self.sections[".data"][3],
                "expected zero-filled owner object section")

    def raw(self, rva, n):
        for start, vs, rawoff, rawsize in self.sections.values():
            if start <= rva and rva+n <= start+min(vs, rawsize):
                return self.data[rawoff+rva-start:rawoff+rva-start+n]
        raise ValueError("RVA outside file-backed PE section")

    def u32(self, rva):
        return struct.unpack("<I", self.raw(rva, 4))[0]

    def va(self, rva):
        return self.base+rva

    def contains(self, name, ptr):
        start, vs, _, _ = self.sections[name]
        return start <= ptr-self.base < start+vs


def audit(data, original=True):
    sha = hashlib.sha256(data).hexdigest()
    if original:
        require(sha == PIN, "original FileSystem_Steam.dll SHA256 mismatch")
    p = PE(data)
    if original:
        require(p.timestamp == 0x44B80CBA, "original linker timestamp mismatch")
    require(p.contains(".data", p.va(OWNER)), "shared owner not in .data")
    for version, name, reg, factory, regobject in REGISTRATIONS:
        require(p.raw(name, len(version)+1) == version.encode()+b"\0",
                version+" name mismatch")
        require(p.raw(reg, 1) == b"\x68" and
                p.u32(reg+1) == p.va(name), version+" factory name operand")
        require(p.raw(reg+5, 1) == b"\x68" and
                p.u32(reg+6) == p.va(factory), version+" factory operand")
        require(p.raw(reg+10, 1) == b"\xB9" and
                p.u32(reg+11) == p.va(regobject), version+" registration object")
        require(p.raw(factory, 1) == b"\xB8" and
                p.u32(factory+1) == p.va(OWNER) and
                p.raw(factory+5, 1) == b"\xC3",
                version+" did not return the shared owner")
    for instruction, destination, table in (
        (0x198B6, OWNER, PRIMARY),
        (0x198C0, OWNER+4, SECONDARY),
    ):
        require(p.raw(instruction, 2) == b"\xC7\x05" and
                p.u32(instruction+2) == p.va(destination) and
                p.u32(instruction+6) == p.va(table),
                "constructor dual-vtable write mismatch")
    for name, table, functions in (
        ("IBaseFileSystem", PRIMARY, PRIMARY_SLOTS),
        ("IAppSystem", SECONDARY, SECONDARY_SLOTS),
    ):
        for index, entry in enumerate(functions):
            pointer = p.u32(table+4*index)
            require(p.contains(".text", pointer) and pointer == p.va(entry),
                    f"{name} slot {index} mismatch")
    for slot, (ret, argument_bytes) in RETURNS.items():
        require(p.raw(ret, 3) == bytes((0xC2, argument_bytes, 0)),
                f"virtual method {slot} x86 argument cleanup mismatch")
    return {
        "file_sha256": sha,
        "interface_versions": [item[0] for item in REGISTRATIONS],
        "shared_owner_rva": f"0x{OWNER:X}",
        "primary_vtable_rva": f"0x{PRIMARY:X}",
        "secondary_vtable_rva": f"0x{SECONDARY:X}",
        "read_related_method_rvas": {
            "Read": "0x7C90", "Open": "0x8D00", "Close": "0x7920",
            "SizeByName": "0x8DF0", "SizeByHandle": "0x2D60",
            "FileExists": "0x7FD0",
        },
        "checked_vtable_entries": 19,
        "verdict": "STATIC_ORIGINAL_ABI_AUTHENTICATED" if original else
                   "SYNTHETIC_STRUCTURE_ONLY",
        "real_engine_method_invoked": False,
    }


def synthetic():
    data = bytearray(0x21000)
    data[:2] = b"MZ"
    struct.pack_into("<I", data, 0x3C, 0xF8)
    pe = 0xF8
    data[pe:pe+4] = b"PE\0\0"
    struct.pack_into("<HH", data, pe+4, 0x14C, 4)
    struct.pack_into("<H", data, pe+20, 224)
    struct.pack_into("<H", data, pe+24, 0x10B)
    struct.pack_into("<I", data, pe+52, BASE)
    struct.pack_into("<I", data, pe+8, 0x44B80CBA)
    sections = (
        (".text", 0x189C7, 0x1000, 0x19000, 0x1000),
        (".rdata", 0x33D7, 0x1A000, 0x4000, 0x1A000),
        (".data", 0x313C, 0x1E000, 0x1000, 0x1E000),
        (".reloc", 0x18FA, 0x22000, 0x2000, 0x1F000),
    )
    for index, (name, vs, va, rawsize, rawoff) in enumerate(sections):
        off = pe+24+224+40*index
        data[off:off+len(name)] = name.encode()
        struct.pack_into("<IIII", data, off+8, vs, va, rawsize, rawoff)
    for version, name, reg, factory, regobject in REGISTRATIONS:
        data[name:name+len(version)+1] = version.encode()+b"\0"
        data[reg] = data[reg+5] = 0x68
        data[reg+10] = 0xB9
        for rva, value in (
            (reg+1, name), (reg+6, factory), (reg+11, regobject),
        ):
            struct.pack_into("<I", data, rva, BASE+value)
        data[factory] = 0xB8
        struct.pack_into("<I", data, factory+1, BASE+OWNER)
        data[factory+5] = 0xC3
    for at, dest, table in (
        (0x198B6, OWNER, PRIMARY),
        (0x198C0, OWNER+4, SECONDARY),
    ):
        data[at:at+2] = b"\xC7\x05"
        struct.pack_into("<II", data, at+2, BASE+dest, BASE+table)
    for table, entries in (
        (PRIMARY, PRIMARY_SLOTS),
        (SECONDARY, SECONDARY_SLOTS),
    ):
        for index, address in enumerate(entries):
            struct.pack_into("<I", data, table+index*4, BASE+address)
    for ret, cleanup in RETURNS.values():
        data[ret:ret+3] = bytes((0xC2, cleanup, 0))
    return bytes(data)


def self_test():
    good = synthetic()
    require(audit(good, original=False)["checked_vtable_entries"] == 19,
            "synthetic baseline failed")
    mutations = (0x1BA6C, 0x19901, 0xD6E1, 0x198B8,
                 PRIMARY+7*4, SECONDARY+2*4, 0x2DB3)
    for index in mutations:
        bad = bytearray(good)
        bad[index] ^= 0x7F
        try:
            audit(bytes(bad), original=False)
        except ValueError:
            pass
        else:
            raise AssertionError(f"corrupt synthetic RVA {index:#x} accepted")
    try:
        audit(good, original=True)
    except ValueError as exc:
        require("SHA256" in str(exc), "synthetic file bypassed original gate")
    else:
        raise AssertionError("synthetic file passed owner SHA gate")
    return {"status": "PASS", "synthetic_mutations_refused": len(mutations),
            "original_bytes_used": False}


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("dll", nargs="?")
    p.add_argument("--self-test", action="store_true")
    args = p.parse_args()
    if args.self_test:
        print(json.dumps(self_test(), sort_keys=True))
        return
    if not args.dll:
        p.error("provide the owner-original FileSystem_Steam.dll or --self-test")
    print(json.dumps(audit(Path(args.dll).read_bytes()), indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
