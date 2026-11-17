#!/usr/bin/env python3
"""Read-only PE32 original server.dll factory/vtable metadata authentication.

Requires the SHA-pinned original file from an owner's private installation.
Never executes, modifies, embeds or uploads the binary. Output is limited to
registration identity and method RVAs, not original code or pseudocode.
"""
import argparse
import hashlib
import json
import re
import struct
from pathlib import Path

PIN = "568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650"
REG = (
    ("ServerGameDLL003", 0x3bf140, 0x1e7630, 0x510650, 0x442efc),
    ("ServerGameEnts001", 0x3bf210, 0x1e82c0, 0x510654, 0x443030),
    ("ServerGameClients003", 0x3bf230, 0x1e8320, 0x510658, 0x443054),
)
ENTS_SLOTS = (0x1e9360, 0x1e82d0, 0x1e9380, 0x1e82e0,
              0x1e94b0, 0x1e82f0, 0x1e94d0)

# Offset of a verified epilogue for each virtual entry, with the exact
# Windows x86 callee-cleanup byte count. These are metadata checks against
# the owner-private original, not executable method bodies.
ENTS_SLOT_ABI = (
    ("scalar_deleting_destructor", 0x1e937c, 4),
    ("SetDebugEdictBase", 0x1e82d9, 4),
    ("MarkEntitiesAsTouching", 0x1e94a8, 8),
    ("FreeContainingEntity", 0x1e82eb, 4),
    ("BaseEntityToEdict", 0x1e94c3, 4),
    ("EdictToBaseEntity", 0x1e8309, 4),
    ("CheckTransmit", 0x1e962f, 12),
)


# The first ten original ServerGameDLL003::DLLInit engineFactory requests,
# all independently recovered from the owner's SHA-authenticated server.
SERVER_DLLINIT_FIRST_TEN = (
    "VEngineServer021", "VoiceServer001", "VEngineCvar002",
    "VEngineServerStringTable001", "StaticPropMgrServer002",
    "VEngineRandom001", "IEngineSoundServer003", "SpatialPartition001",
    "VModelInfoServer001", "EngineTraceServer003",
)
SERVER_DLLINIT_FILESYSTEM = ("VFileSystem012",)
SERVER_DLLINIT_NEXT_FIVE = (
    "GAMEEVENTSMANAGER002", "VEngineCache001", "VSoundEmitter001",
    "ServerUploadGameStats001", "ISERVERPLUGINHELPERS001",
)

class PE32:
    def __init__(self, data):
        self.data = data
        if len(data) < 256 or data[:2] != b"MZ":
            raise ValueError("Not an MZ image")
        pe = struct.unpack_from("<I", data, 0x3c)[0]
        if pe + 0x78 > len(data) or data[pe:pe+4] != b"PE\x00\x00":
            raise ValueError("Invalid PE header")
        if struct.unpack_from("<H", data, pe+4)[0] != 0x14c:
            raise ValueError("Not x86 PE")
        n = struct.unpack_from("<H", data, pe+6)[0]
        opt_size = struct.unpack_from("<H", data, pe+20)[0]
        opt = pe + 24
        if not 1 <= n <= 96 or opt_size < 0x60 or opt+opt_size+n*40 > len(data):
            raise ValueError("Bad PE section table")
        if struct.unpack_from("<H", data, opt)[0] != 0x10b:
            raise ValueError("Not PE32 optional header")
        self.base = struct.unpack_from("<I", data, opt+28)[0]
        self.sections = []
        for i in range(n):
            o = opt+opt_size+i*40
            name = data[o:o+8].rstrip(b"\0").decode("ascii")
            vsize, rva, rawsize, fileoff = struct.unpack_from("<IIII", data, o+8)
            if fileoff+rawsize > len(data):
                raise ValueError("Truncated section")
            self.sections.append((name, rva, vsize, rawsize, fileoff))

    def off(self, rva, size=1):
        for name, start, vsize, rawsize, fileoff in self.sections:
            if start <= rva and rva + size <= start + rawsize:
                return fileoff + rva - start
        raise ValueError("Unmapped PE RVA: " + hex(rva))

    def raw(self, rva, size):
        p = self.off(rva, size)
        return self.data[p:p+size]

    def u32(self, rva):
        return struct.unpack("<I", self.raw(rva, 4))[0]

    def rva(self, va):
        if va < self.base:
            raise ValueError("Invalid VA")
        return va - self.base

    def string(self, rva):
        o = self.off(rva)
        end = self.data.find(b"\x00", o, o+128)
        if end < 0:
            raise ValueError("Unterminated factory string")
        return self.data[o:end].decode("ascii")


def inspect(data):
    if hashlib.sha256(data).hexdigest() != PIN:
        raise ValueError("Original server.dll SHA256 mismatch")
    pe = PE32(data)
    if pe.base != 0x22000000:
        raise ValueError("Unexpected original server image base")
    records = []
    for name, reg, factory, obj, vtable in REG:
        wrapper = pe.raw(reg, 15)
        # Registration wrapper immediate operands: name and factory VA.
        if wrapper[0] != 0x68 or wrapper[5] != 0x68 or wrapper[10] != 0xb9:
            raise ValueError(name + " registration wrapper mismatch")
        if pe.string(pe.rva(struct.unpack_from("<I", wrapper, 1)[0])) != name:
            raise ValueError(name + " registration name mismatch")
        if pe.rva(struct.unpack_from("<I", wrapper, 6)[0]) != factory:
            raise ValueError(name + " registration factory mismatch")
        callback = pe.raw(factory, 6)
        if callback[0] != 0xb8 or callback[5] != 0xc3:
            raise ValueError(name + " singleton callback shape mismatch")
        if pe.rva(struct.unpack_from("<I", callback, 1)[0]) != obj:
            raise ValueError(name + " singleton pointer mismatch")
        if pe.rva(pe.u32(obj)) != vtable:
            raise ValueError(name + " singleton vtable mismatch")
        result = {
            "name": name,
            "registration_rva": hex(reg),
            "factory_callback_rva": hex(factory),
            "object_rva": hex(obj),
            "vtable_rva": hex(vtable),
        }
        if name == "ServerGameEnts001":
            slots = tuple(pe.rva(pe.u32(vtable+4*i))
                          for i in range(len(ENTS_SLOTS)))
            if slots != ENTS_SLOTS or pe.u32(vtable+4*len(slots)) != 0:
                raise ValueError("ServerGameEnts001 must have seven original slots")
            for slot in slots:
                if not any(s == ".text" and start <= slot < start+vsize
                           for s, start, vsize, rawsize, fileoff in pe.sections):
                    raise ValueError("Non-code entry in server entity vtable")
            for index, (method, end_rva, callee_bytes) in enumerate(ENTS_SLOT_ABI):
                # Reject stale/nonmatching originals, without disassembling
                # arbitrary or untrusted PE bytes.
                if not slots[index] <= end_rva < slots[index] + 0x1000:
                    raise ValueError("Method epilogue outside its slot")
                if pe.raw(end_rva, 3) != b"\xc2" + struct.pack("<H", callee_bytes):
                    raise ValueError(method + " x86 stack cleanup mismatch")
            result["vtable_slot_rvas"] = [hex(i) for i in slots]
            result["slot_signatures"] = [
                {"method": method, "callee_cleanup_bytes": callee_bytes}
                for method, _end_rva, callee_bytes in ENTS_SLOT_ABI
            ]
        records.append(result)
    # The original ServerGameDLL003 vtable's first method is native DLLInit.
    # Only its first ten sequential calls to the first engineFactory are
    # reported; the next request uses the filesystem factory and is out
    # of this deliberately bounded dependency probe.
    if pe.rva(pe.u32(0x442efc)) != 0x1e7640:
        raise ValueError("Original ServerGameDLL003 DLLInit slot mismatch")
    start_rva = 0x1e7640
    code = pe.raw(start_rva, 0x103)
    calls = list(re.finditer(rb"\x6a\x00\x68(.{4})\xff\xd6", code, re.DOTALL))
    versions = tuple(
        pe.string(pe.rva(struct.unpack("<I", match.group(1))[0]))
        for match in calls
    )
    if versions != SERVER_DLLINIT_FIRST_TEN:
        raise ValueError("Original server DLLInit first-ten query order mismatch")

    # The original implementation switches from engineFactory (ESI) to
    # fileSystemFactory (EDI), then back to ESI for five more mandatory
    # queries. Inspect only those three bounded call regions and assert
    # the exact versioned names, not arbitrary nearby code.
    def query_names(start_rva, byte_count, factory_call_bytes):
        matches = list(re.finditer(
            rb"\x6a\x00\x68(.{4})" + factory_call_bytes,
            pe.raw(start_rva, byte_count), re.DOTALL))
        return tuple(pe.string(pe.rva(struct.unpack("<I", match.group(1))[0]))
                     for match in matches)

    filesystem_versions = query_names(0x1e7743, 0x1d, rb"\xff\xd7")
    if filesystem_versions != SERVER_DLLINIT_FILESYSTEM:
        raise ValueError("Original DLLInit filesystem factory query mismatch")
    additional_versions = query_names(0x1e7760, 0x7d, rb"\xff\xd6")
    if additional_versions != SERVER_DLLINIT_NEXT_FIVE:
        raise ValueError("Original DLLInit post-filesystem engine query mismatch")
    return {"original_server_sha256": PIN, "format": "PE32 x86",
            "status": "ABI research only, no server gameplay implementation",
            "factory_registrations": records,
            "server_dllinit_first_ten_engine_queries": versions,
            "server_dllinit_filesystem_queries": filesystem_versions,
            "server_dllinit_next_five_engine_queries": additional_versions}


def synthetic_test():
    """Test parsing and strict SHA rejection with an invented minimal PE."""
    b = bytearray(0x600)
    b[:2] = b"MZ"
    struct.pack_into("<I", b, 0x3c, 0x80)
    b[0x80:0x84] = b"PE\x00\x00"
    struct.pack_into("<H", b, 0x84, 0x14c)
    struct.pack_into("<H", b, 0x86, 1)
    struct.pack_into("<H", b, 0x94, 0xe0)
    struct.pack_into("<H", b, 0x98, 0x10b)
    struct.pack_into("<I", b, 0x98+28, 0x22000000)
    sec = 0x80+24+0xe0
    b[sec:sec+5] = b".text"
    struct.pack_into("<IIII", b, sec+8, 0x200, 0x1000, 0x200, 0x400)
    b[0x400:0x404] = b"TEST"
    pe = PE32(bytes(b))
    assert pe.raw(0x1000, 4) == b"TEST"
    assert pe.rva(0x22001004) == 0x1004
    try:
        pe.raw(0x1100, 0x200)
    except ValueError:
        pass
    else:
        raise AssertionError("Unmapped RVA was incorrectly accepted")
    try:
        inspect(bytes(b))
    except ValueError as exc:
        assert "SHA256 mismatch" in str(exc)
    else:
        raise AssertionError("Synthetic binary passed private SHA pin")
    print("PASS synthetic server PE32 parser and original-SHA guard")


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("original_server_dll", nargs="?", type=Path)
    ap.add_argument("--self-test", action="store_true")
    args = ap.parse_args()
    if args.self_test:
        synthetic_test()
    elif args.original_server_dll:
        print(json.dumps(inspect(args.original_server_dll.read_bytes()), indent=2))
    else:
        ap.error("Provide owner original server.dll or --self-test")


if __name__ == "__main__":
    main()
