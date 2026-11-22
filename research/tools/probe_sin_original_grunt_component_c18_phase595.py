#!/usr/bin/env python3
"""Source-specific PE32 probe for Grunt owner+0xC18 component provenance.

Requires the exact unchanged 2006 SiN Episodes: Emergence server.dll.
Does not decompile x86, recover a class name, or export proprietary bytes.
"""
import argparse
import hashlib
import json
import struct
import zipfile
from pathlib import Path

SERVER_SHA = "568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650"
CLIENT_SHA = "6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d"
GATE_VA = 0x2202D930
GATE_SIZE = 72
GATE_SHA = "969cc5659f53449812b567734994fbb656edc52fabffaa810dd15c0152f8f5a2"
DISPLACEMENT = b"\x18\x0c\x00\x00"
REGS = ("eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi")
# Only recognized single-byte opcode shapes with a ModR/M memory operand.
# A matched pattern is a CANDIDATE, not a certified instruction boundary.
OPCODES = {
    0x89: "write-register",
    0x8B: "read-register",
    0x8D: "address-of",
    0xC7: "write-immediate",
    0x83: "arithmetic-or-compare-imm8",
    0x81: "arithmetic-or-compare-imm32",
    0x80: "byte-arithmetic-or-compare",
    0x88: "write-byte",
    0x8A: "read-byte",
    0xFF: "group-five",
    0xD9: "x87-memory",
    0xDD: "x87-memory",
}
WRITE_KINDS = {"write-register", "write-immediate", "write-byte"}


class PE32:
    def __init__(self, data):
        if data[:2] != b"MZ" or len(data) < 0x100:
            raise ValueError("Not a valid PE MZ header")
        pe = struct.unpack_from("<I", data, 0x3C)[0]
        if pe + 24 > len(data) or data[pe:pe+4] != b"PE\0\0":
            raise ValueError("Missing PE signature")
        machine, count = struct.unpack_from("<HH", data, pe+4)
        optional_size = struct.unpack_from("<H", data, pe+20)[0]
        opt = pe + 24
        if machine != 0x14C or struct.unpack_from("<H", data, opt)[0] != 0x10B:
            raise ValueError("Expected 32-bit x86 PE")
        self.image_base = struct.unpack_from("<I", data, opt+28)[0]
        self.data = data
        self.sections = []
        sh = opt + optional_size
        for index in range(count):
            off = sh + index * 40
            if off + 40 > len(data):
                raise ValueError("Truncated PE section headers")
            name = data[off:off+8].rstrip(b"\0").decode("ascii", errors="replace")
            vsize, rva, raw_size, raw_at = struct.unpack_from("<IIII", data, off+8)
            n = min(vsize or raw_size, raw_size)
            if raw_at + n > len(data):
                raise ValueError("Truncated PE section data")
            self.sections.append((name, self.image_base+rva, data[raw_at:raw_at+n]))

    def read(self, address, size):
        for _name, start, raw in self.sections:
            if start <= address and address+size <= start+len(raw):
                off = address-start
                return raw[off:off+size]
        raise ValueError("Address not backed by mapped PE data: %#x" % address)

    def text(self):
        matches = [(start, raw) for name, start, raw in self.sections if name == ".text"]
        if len(matches) != 1:
            raise ValueError("Expected exactly one .text section")
        return matches[0]


def scan_member_displacements(raw, base_address):
    """Recognize plausible mod=10 x86 memory operands with +0xC18 disp32.

    This deliberately does NOT pretend to locate instruction boundaries. Each
    returned address is a candidate for targeted Ghidra disassembly.
    """
    sites = []
    at = 0
    while True:
        at = raw.find(DISPLACEMENT, at)
        if at < 0:
            break
        found = None
        # [base+disp32], with ModR/M byte immediately before displacement.
        if at >= 2:
            op, modrm = raw[at-2], raw[at-1]
            if ((modrm & 0xC0) == 0x80 and (modrm & 0x07) != 4
                    and op in OPCODES):
                if op != 0xC7 or ((modrm >> 3) & 7) == 0:
                    found = (at-2, op, modrm, modrm & 7, False)
        # [base+index*scale+disp32], where SIB precedes displacement.
        if found is None and at >= 3:
            op, modrm, sib = raw[at-3], raw[at-2], raw[at-1]
            if ((modrm & 0xC7) == 0x84 and op in OPCODES):
                if op != 0xC7 or ((modrm >> 3) & 7) == 0:
                    found = (at-3, op, modrm, sib & 7, True)
        if found:
            op_at, opcode, modrm, base, sib_present = found
            kind = OPCODES[opcode]
            sites.append({
                "candidate_va": "0x%08X" % (base_address+op_at),
                "displacement_va": "0x%08X" % (base_address+at),
                "opcode_class": kind,
                "base_register": REGS[base],
                "has_sib": sib_present,
                "could_assign_pointer": kind in WRITE_KINDS and base not in (4, 5),
                "note": "candidate encoding only; must verify Ghidra instruction boundary and owning class",
            })
        at += 1
    return sites


def candidate_direct_callers(raw, base_address, target_va):
    """List candidate E8 rel32 calls to the previously authenticated gate."""
    results = []
    for offset in range(len(raw)-4):
        if raw[offset] != 0xE8:
            continue
        displacement = struct.unpack_from("<i", raw, offset+1)[0]
        if (base_address+offset+5+displacement) & 0xFFFFFFFF == target_va:
            results.append("0x%08X" % (base_address+offset))
    return results


def analyze(server, client=None):
    if hashlib.sha256(server).hexdigest() != SERVER_SHA:
        raise ValueError("Unsupported or altered retail server.dll")
    if client is not None and hashlib.sha256(client).hexdigest() != CLIENT_SHA:
        raise ValueError("Unsupported or altered retail client.dll")
    pe = PE32(server)
    if pe.image_base != 0x22000000:
        raise ValueError("Unexpected original server PE image base")
    if hashlib.sha256(pe.read(GATE_VA, GATE_SIZE)).hexdigest() != GATE_SHA:
        raise ValueError("Previously authenticated +0xC18 gate does not match")
    text_va, text = pe.text()
    sites = scan_member_displacements(text, text_va)
    direct = candidate_direct_callers(text, text_va, GATE_VA)
    return {
        "status": "CANDIDATE_SITES_ONLY",
        "server_sha256": SERVER_SHA,
        "matched_client_sha256": CLIENT_SHA if client is not None else None,
        "authenticated_gate": "0x2202D930",
        "authenticated_gate_sha256": GATE_SHA,
        "displacement": "0xC18",
        "x86_instruction_boundaries_proven": False,
        "owner_class_identity_proven": False,
        "virtual_plus_0x60_implementation_proven": False,
        "candidate_count": len(sites),
        "candidate_pointer_assignments": sum(bool(x["could_assign_pointer"]) for x in sites),
        "candidate_gate_e8_callers": direct,
        "candidate_member_uses": sites,
        "proprietary_code_bytes_exported": False,
    }


def test_synthetic():
    raw = (
        b"\x90\x89\x86\x18\x0c\x00\x00"  # mov [esi+C18],eax
        b"\x8b\x87\x18\x0c\x00\x00"      # mov eax,[edi+C18]
        b"\x8b\x84\x24\x18\x0c\x00\x00" # mov eax,[esp+C18], stack-like
        b"\x90\x18\x0c\x00\x00"          # unmatched displacement literal
        b"\xc7\x86\x18\x0c\x00\x00\x00\x00\x00\x00"
    )
    got = scan_member_displacements(raw, 0x22001000)
    assert [x["opcode_class"] for x in got] == [
        "write-register", "read-register", "read-register", "write-immediate"
    ], got
    assert [x["base_register"] for x in got] == ["esi", "edi", "esp", "esi"]
    assert sum(bool(x["could_assign_pointer"]) for x in got) == 2
    assert candidate_direct_callers(b"\xe8\x00\x00\x00\x00", 0x22001000,
                                    0x22001005) == ["0x22001000"]
    assert candidate_direct_callers(b"\xe8\x00\x00\x00\x00", 0x22001000,
                                    0x2202D930) == []
    # A tiny synthetic x86 PE verifies section mapping. No retail DLL used.
    fake = bytearray(0x400)
    fake[:2] = b"MZ"
    struct.pack_into("<I", fake, 0x3C, 0x80)
    fake[0x80:0x84] = b"PE\0\0"
    struct.pack_into("<HH", fake, 0x84, 0x14C, 1)
    struct.pack_into("<H", fake, 0x94, 0xE0)
    struct.pack_into("<H", fake, 0x98, 0x10B)
    struct.pack_into("<I", fake, 0x98+28, 0x22000000)
    sh = 0x98+0xE0
    fake[sh:sh+5] = b".text"
    struct.pack_into("<IIII", fake, sh+8, 0x100, 0x1000, 0x100, 0x200)
    fake[0x200:0x200+len(raw)] = raw
    pe = PE32(bytes(fake))
    assert pe.read(0x22001000, len(raw)) == raw
    assert len(scan_member_displacements(*reversed(pe.text()))) == 4
    try:
        analyze(bytes(fake))
    except ValueError:
        pass
    else:
        raise AssertionError("Synthetic PE incorrectly accepted as retail")
    return "PHASE595_ORIGINAL_FREE_C18_PROBE_SYNTHETIC_PASS"


def main():
    p = argparse.ArgumentParser(description=__doc__)
    source = p.add_mutually_exclusive_group()
    source.add_argument("--bin-zip", type=Path,
                        help="Private ZIP containing client.dll and server.dll")
    source.add_argument("--server", type=Path,
                        help="Private original server.dll; only its SHA will be checked")
    p.add_argument("--out", type=Path, help="Write candidate-address-only JSON report")
    p.add_argument("--self-test", action="store_true")
    a = p.parse_args()
    if a.self_test:
        print(test_synthetic())
        if not a.bin_zip and not a.server:
            return
    if not (a.bin_zip or a.server):
        p.error("Pass --self-test or the exact retail binary with --server / --bin-zip")
    if a.bin_zip:
        with zipfile.ZipFile(a.bin_zip) as z:
            result = analyze(z.read("server.dll"), z.read("client.dll"))
    else:
        result = analyze(a.server.read_bytes())
    encoded = json.dumps(result, indent=2) + "\n"
    if a.out:
        a.out.write_text(encoded, encoding="utf-8")
        print("Candidate-address-only report:", a.out)
    else:
        print(encoded)
    print("Candidates:", result["candidate_count"],
          "potential assignments:", result["candidate_pointer_assignments"],
          "gate E8 callers:", len(result["candidate_gate_e8_callers"]))


if __name__ == "__main__":
    main()
