#!/usr/bin/env python3
"""Verify build-specific SiN weapon field descriptors in the user's own PE32 DLLs.

Independent structural PE reader; no game binaries are bundled. Descriptor format:
    type (u32), field-name VA (u32), object offset (u32), ... (52 bytes).
This recognizes metadata, not proof of a particular save/replication pathway.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import struct
from dataclasses import asdict, dataclass
from pathlib import Path

# Hashes previously established from the original 2006 SiN binaries.
EXPECTED_SHA256 = {
    "server": "568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650",
    "client": "6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d",
}
# Each tuple: metadata type id, object offset, expected number of descriptor copies.
EXPECTED_FIELDS = {
    "server": {
        "m_bReloadsSingly": (6, 0x508, 2),
        "m_bInReload": (6, 0x4AC, 2),
        "m_iClip1": (5, 0x4EC, 2),
        "m_iClip2": (5, 0x4F0, 2),
    },
    "client": {
        "m_bReloadsSingly": (6, 0x7FC, 1),
        "m_bInReload": (6, 0x7A0, 1),
        "m_iClip1": (5, 0x7E0, 1),
        "m_iClip2": (5, 0x7E4, 1),
    },
}


class VerificationError(ValueError):
    pass


@dataclass(frozen=True)
class Section:
    name: str
    rva: int
    virtual_size: int
    raw_offset: int
    raw_size: int


class PE32:
    def __init__(self, content: bytes):
        self.content = content
        if content[:2] != b"MZ" or len(content) < 0x40:
            raise VerificationError("Missing DOS MZ header")
        pe_offset = struct.unpack_from("<I", content, 0x3C)[0]
        if pe_offset + 24 > len(content) or content[pe_offset:pe_offset + 4] != b"PE\0\0":
            raise VerificationError("Missing PE header")
        machine, count = struct.unpack_from("<HH", content, pe_offset + 4)
        optional_size = struct.unpack_from("<H", content, pe_offset + 20)[0]
        optional_at = pe_offset + 24
        if machine != 0x14C or optional_size < 32 or optional_at + optional_size > len(content):
            raise VerificationError("Expected an x86 PE32 executable")
        if struct.unpack_from("<H", content, optional_at)[0] != 0x10B:
            raise VerificationError("Expected PE32 optional header")
        self.base = struct.unpack_from("<I", content, optional_at + 28)[0]
        self.sections = []
        start = optional_at + optional_size
        for i in range(count):
            offset = start + i * 40
            if offset + 40 > len(content):
                raise VerificationError("Truncated section header")
            name = content[offset:offset + 8].split(b"\0", 1)[0].decode("ascii", "replace")
            vsize, rva, rawsize, rawoffset = struct.unpack_from("<IIII", content, offset + 8)
            if rawoffset + rawsize > len(content):
                raise VerificationError("Section extends beyond file")
            self.sections.append(Section(name, rva, vsize, rawoffset, rawsize))

    def va_to_offset(self, va: int) -> int:
        rva = va - self.base
        for sec in self.sections:
            if sec.rva <= rva < sec.rva + min(sec.virtual_size, sec.raw_size):
                return sec.raw_offset + (rva - sec.rva)
        raise VerificationError(f"Unmapped VA {va:#x}")

    def section_bytes(self, name: str) -> tuple[bytes, Section]:
        matches = [s for s in self.sections if s.name == name]
        if len(matches) != 1:
            raise VerificationError(f"Expected one {name} section")
        sec = matches[0]
        return self.content[sec.raw_offset:sec.raw_offset + sec.raw_size], sec

    def descriptors(self, field_name: str) -> list[dict[str, int]]:
        raw, strings_sec = self.section_bytes(".rdata")
        needle = field_name.encode("ascii") + b"\0"
        offsets = []
        start = 0
        while True:
            idx = raw.find(needle, start)
            if idx < 0:
                break
            if idx == 0 or raw[idx - 1] == 0:
                offsets.append(idx)
            start = idx + len(needle)
        if len(offsets) != 1:
            raise VerificationError(f"Expected one exact .rdata string for {field_name}; found {len(offsets)}")
        string_va = self.base + strings_sec.rva + offsets[0]
        data, data_sec = self.section_bytes(".data")
        ptr = struct.pack("<I", string_va)
        entries = []
        at = 0
        while True:
            at = data.find(ptr, at)
            if at < 0:
                break
            # Descriptor pointer at +4 and object field offset at +8.
            if at >= 4 and at + 8 <= len(data) and (at - 4) % 4 == 0:
                t, pointer, off = struct.unpack_from("<III", data, at - 4)
                if t in (5, 6) and pointer == string_va and off < 0x1000:
                    entries.append({"descriptor_va": self.base + data_sec.rva + at - 4,
                                    "type_id": t, "object_offset": off})
            at += 4
        return entries


def inspect_binary(path: Path, role: str, *, enforce_hash: bool = True) -> dict:
    raw = path.read_bytes()
    sha = hashlib.sha256(raw).hexdigest()
    if enforce_hash and sha != EXPECTED_SHA256[role]:
        raise VerificationError(f"{role} SHA-256 mismatch: {sha}. This verifier targets a specific SiN build.")
    pe = PE32(raw)
    output = {}
    for field, (field_type, object_offset, expected_copies) in EXPECTED_FIELDS[role].items():
        matches = pe.descriptors(field)
        if len(matches) != expected_copies:
            raise VerificationError(f"{role} {field}: expected {expected_copies} descriptors, found {len(matches)}")
        for item in matches:
            if item["type_id"] != field_type or item["object_offset"] != object_offset:
                raise VerificationError(f"{role} {field}: unexpected descriptor {item}")
        output[field] = matches
    return {"role": role, "sha256": sha, "image_base": pe.base,
            "descriptor_stride_bytes": 52, "fields": output}


def main() -> int:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("server", type=Path, help="Original SiN server.dll")
    p.add_argument("client", type=Path, help="Original SiN client.dll")
    a = p.parse_args()
    try:
        evidence = [inspect_binary(a.server, "server"), inspect_binary(a.client, "client")]
    except (VerificationError, OSError) as error:
        p.exit(1, f"VERIFICATION FAILED: {error}\n")
    print(json.dumps({"result": "pass", "scope": "build-specific static metadata", "binaries": evidence}, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
