#!/usr/bin/env python3
"""Private read-only SHA-pinned audit of 2006 SiN client bf_read fields.

No retail bytes, original code, extracted disassembly or executable assets
are emitted. Public CI only uses --self-test; the real DLL stays with owner.
"""
import argparse
import hashlib
import json
import struct
import sys
import zipfile
from pathlib import Path

EXPECTED_SHA = "6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d"
IMAGE_BASE = 0x24000000
METHODS = {
    "bit_word_core": (
        0x240A4760, 0x76,
        "1e35e98014bd1194e61f7304aea1064290c9e380acc4945687c0c763745450c2"),
    "signed_long_core": (
        0x240A5080, 0x62,
        "5d3803e84d3a3f68627ba0cb9a57cfcb4421840e76baee1bc33a4a967e1a9e1a"),
    "sixteen_bit_wrapper": (
        0x240A5600, 0x08,
        "74b843fdd2c530a3188594e663ad1880b443dbb1f64dd715048b83d26244c3b7"),
    "user_message_dispatcher": (
        0x241D0000, 0xD9,
        "d4787460e846e3dc15fa05d6ae377a0a31e8635f666fc70ae06d212a0234fbdb"),
}


def u16(data, pos):
    if pos < 0 or pos + 2 > len(data):
        raise ValueError("truncated PE u16")
    return struct.unpack_from("<H", data, pos)[0]


def u32(data, pos):
    if pos < 0 or pos + 4 > len(data):
        raise ValueError("truncated PE u32")
    return struct.unpack_from("<I", data, pos)[0]


def parse_image(data):
    if len(data) < 0x100 or data[:2] != b"MZ":
        raise ValueError("not an MZ executable")
    header = u32(data, 0x3C)
    if header + 24 > len(data) or data[header:header+4] != b"PE\0\0":
        raise ValueError("invalid PE signature")
    section_count = u16(data, header + 6)
    optional_bytes = u16(data, header + 20)
    optional = header + 24
    if u16(data, optional) != 0x10B:
        raise ValueError("expected Win32 PE32 optional header")
    image_base = u32(data, optional + 28)
    table = optional + optional_bytes
    if table + section_count*40 > len(data):
        raise ValueError("truncated PE sections")
    sections = []
    for i in range(section_count):
        entry = table + i*40
        virtual_size = u32(data, entry+8)
        rva = u32(data, entry+12)
        raw_size = u32(data, entry+16)
        raw_at = u32(data, entry+20)
        if raw_at + raw_size > len(data):
            raise ValueError("raw section outside file")
        sections.append((rva, virtual_size, raw_size, raw_at))
    return image_base, sections


def region_at_va(data, image, va, size):
    image_base, sections = image
    if size <= 0 or va < image_base:
        raise ValueError("invalid image address")
    rva = va-image_base
    for start, vsize, raw_size, raw in sections:
        if start <= rva and rva-start+size <= min(vsize, raw_size):
            at = raw+(rva-start)
            return data[at:at+size]
    raise ValueError("VA not inside raw PE image section")


def assert_method(data, image, key):
    va, size, digest = METHODS[key]
    found = region_at_va(data, image, va, size)
    if hashlib.sha256(found).hexdigest() != digest:
        raise ValueError("retail original method body differs: " + key)
    return found


def inspect_original(data):
    if hashlib.sha256(data).hexdigest() != EXPECTED_SHA:
        raise ValueError("original client.dll SHA256 mismatch")
    image = parse_image(data)
    if image[0] != IMAGE_BASE:
        raise ValueError("unexpected 2006 original image base")
    parts = {key: assert_method(data, image, key) for key in METHODS}
    core = parts["bit_word_core"]
    # SHA-pinned core uses ECX/this in EAX. These are selected field
    # accesses from the verified whole-method image, not raw decomp.
    checks = (
        (b"\x8b\x48\x0c", "current bit read at +0x0c"),
        (b"\x8b\x70\x08", "total bit count read at +0x08"),
        (b"\x89\x70\x0c", "current bit clamp at +0x0c"),
        (b"\xc6\x40\x10\x01", "overflow marker write at +0x10"),
        (b"\x8b\x30", "data pointer read at +0x00"),
        (b"\x89\x50\x0c", "current bit advance at +0x0c"),
    )
    for opcode, name in checks:
        if opcode not in core:
            raise ValueError("bf_read field guard failed: " + name)

    # The 16-bit wrapper has exactly one relative call to the SHA-pinned
    # conversion core, followed by a near return. No raw bytes logged.
    wrapper = parts["sixteen_bit_wrapper"]
    if len(wrapper) != 8 or wrapper[0:2] != bytes((0x6A, 0x10)) \
            or wrapper[2] != 0xE8 or wrapper[7] != 0xC3:
        raise ValueError("wrong 16-bit read wrapper")
    target = 0x240A5607 + struct.unpack_from("<i", wrapper, 3)[0]
    if target != 0x240A5080:
        raise ValueError("wrong signed-16 wrapper callee")

    # Dispatcher reads all six DWORDs 0/4/8/12/16/20 from its caller
    # bf_read*. The method hash proves the complete control flow too.
    dispatch = parts["user_message_dispatcher"]
    for opcode in (b"\x8b\x08", b"\x8b\x50\x04", b"\x8b\x48\x08",
                   b"\x8b\x50\x0c", b"\x8b\x48\x10", b"\x8b\x50\x14"):
        if opcode not in dispatch:
            raise ValueError("six-dword message reader copy incomplete")
    if dispatch[-3:] != b"\xc2\x08\x00":
        raise ValueError("dispatcher no longer returns with ret 8")
    return {
        "original_client_sha256_verified": True,
        "methods_sha256_verified": len(METHODS),
        "source_reader_copy_dwords": 6,
        "retail_field_offsets": {
            "data_pointer": 0,
            "data_bit_count": 8,
            "cursor": 12,
            "overflow": 16,
        },
        "16_bit_wrapper_target": "verified_signed_long_core",
        "retail_engine_packet_tested": False,
    }


def self_test():
    # Tiny invented PE, NOT a slice of any commercial binary.
    data = bytearray(0x600)
    data[:2] = b"MZ"
    struct.pack_into("<I", data, 0x3C, 0x80)
    data[0x80:0x84] = b"PE\0\0"
    struct.pack_into("<H", data, 0x86, 1)
    struct.pack_into("<H", data, 0x94, 0xE0)
    struct.pack_into("<H", data, 0x98, 0x10B)
    struct.pack_into("<I", data, 0x98+28, IMAGE_BASE)
    sec = 0x98+0xE0
    data[sec:sec+5] = b".text"
    struct.pack_into("<IIII", data, sec+8, 0x300, 0x1000, 0x300, 0x200)
    data[0x210:0x214] = b"ABCD"
    image = parse_image(data)
    if image[0] != IMAGE_BASE or \
            region_at_va(data, image, IMAGE_BASE+0x1010, 4) != b"ABCD":
        raise ValueError("synthetic PE address mapping failed")
    if region_at_va(data, image, IMAGE_BASE+0x1013, 1) != b"D":
        raise ValueError("synthetic PE offset mapping failed")
    try:
        region_at_va(data, image, IMAGE_BASE+0x1300, 2)
    except ValueError:
        pass
    else:
        raise ValueError("synthetic out-of-range PE mapping accepted")
    try:
        parse_image(b"not PE")
    except ValueError:
        pass
    else:
        raise ValueError("invalid synthetic PE accepted")
    return {"synthetic_pe_mapper": "PASS", "original_binary_used": False}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-test", action="store_true")
    input_group = parser.add_mutually_exclusive_group()
    input_group.add_argument("--client", type=Path)
    input_group.add_argument("--archive", type=Path)
    args = parser.parse_args()
    if args.self_test:
        report = self_test()
    elif args.client:
        report = inspect_original(args.client.read_bytes())
    elif args.archive:
        with zipfile.ZipFile(args.archive) as z:
            report = inspect_original(z.read("client.dll"))
    else:
        parser.error("specify --self-test, --client or --archive")
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    try:
        main()
    except (ValueError, FileNotFoundError, KeyError, zipfile.BadZipFile) as exc:
        print("FAILED private SHA-pinned bf_read audit: " + str(exc),
              file=sys.stderr)
        sys.exit(1)
