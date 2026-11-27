#!/usr/bin/env python3
"""Read-only exact original 2006 PE32 VClient011 complete 48-slot vtable audit.

Owner-private --client mode authenticates the complete original DLL, creator,
static InterfaceReg registration, exported CreateInterface, 48 contiguous
vtable pointers and their .text destinations. Output is derived metadata
only: NO proprietary code or original DLL image is uploaded/committed.

Public --self-test uses invented pointer tables and corruption fixtures.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
from audit_sin_original_engine_gameents_consumer import PE32

CLIENT_SHA = "6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d"
IMAGE_BASE = 0x24000000
CREATE_INTERFACE_VA = 0x24164180
CREATE_INTERFACE_BYTES = 114
CREATE_INTERFACE_SHA = "f74837ce59ab917becd4c214e627ed91762085cb52883617278a206118267eec"
REGISTRATION_CTOR_VA = 0x24164160
REGISTRATION_CTOR_BYTES = 32
REGISTRATION_CTOR_SHA = "48699a1f29f3e63113789813739fcb588bfdc154be3047b2a5e712ea873c1a80"
HEAD_GLOBAL_VA = 0x24369378
REGISTER_VCLIENT_VA = 0x24268300
REGISTER_VCLIENT_SHA = "ba18f2a12188966d90f536c7cf9c84364f598aa0cc4e80913616ccd96ef1aa3e"
VCLIENT_NAME_VA = 0x24296E28
VCLIENT_FACTORY_VA = 0x240FD840
VCLIENT_CREATOR_SHA = "72b920d06b2ea5b37dc9fe0b0a8821128ab5d7604d4bae35a9f6e01a86aa6c"
VCLIENT_INTERFACE_REG_VA = 0x2433F7F0
VCLIENT_SINGLETON_VA = 0x242FA4B8
VCLIENT_VTABLE_VA = 0x24296C6C
VCLIENT_VTABLE_SHA = "9d28eccf91315f7a6ecc2394386d7b4077919d8649d34297a951eb7a1cafa664"
VCLIENT_NEXT_DATA = b"Precached\x00"
VCLIENT_INIT_VA = 0x240FE0E0
VCLIENT_INIT_BYTES = 0x4C4
VCLIENT_INIT_SHA = "14d81449866220aaef1f5f906b577179ea107df78a424a364c715261adbb35db"

# Zero-based actual original 2006 VClient011 virtual method targets.
# None of these target addresses gives a signature or safe call contract.
VCLIENT_VIRTUAL_TARGETS = (
    0x240FE0E0, 0x240FF3D0, 0x240FDB40, 0x240FDC00,
    0x240FF4A0, 0x240FD990, 0x240FD8B0, 0x240FD8D0,
    0x240FD8F0, 0x240FD970, 0x240FD980, 0x240FD9A0,
    0x240FD9B0, 0x240FD9C0, 0x240FD9D0, 0x240FD9E0,
    0x240FD9F0, 0x240FDA20, 0x240FDA60, 0x240FDA30,
    0x240FDAA0, 0x240FDAB0, 0x240FDAC0, 0x240FDAD0,
    0x240FDC20, 0x240FDB20, 0x240FDC30, 0x240FDC40,
    0x240FDC60, 0x240FDC70, 0x240FDC80, 0x240FF850,
    0x240FEDF0, 0x240FDCD0, 0x240FDCE0, 0x240FF570,
    0x240FF5C0, 0x240FDCF0, 0x240FEFD0, 0x240FF000,
    0x240FF040, 0x240FF080, 0x240FF0C0, 0x240FDD10,
    0x240FDD00, 0x240FE600, 0x240FE630, 0x240FE670,
)
VTABLE_BYTES = len(VCLIENT_VIRTUAL_TARGETS) * 4
SLOT33 = 0x240FDCD0


def pack_u32(value):
    return struct.pack("<I", value)


def checked_section(pe, address, length, name):
    """Check that the requested entire source address extent is in a PE section."""
    rva = address - pe.base
    return any(section == name and start <= rva
               and rva + length <= start + virtual_size
               for section, start, virtual_size, raw_size, file_offset
               in pe.sections)


def verify_vtable(pointer_bytes, next_bytes):
    """Validate all 48 original targets and adjacent non-pointer boundary."""
    if len(VCLIENT_VIRTUAL_TARGETS) != 48 or len(pointer_bytes) != 192:
        raise ValueError("VClient011 vtable is not exactly 48 PE32 pointers")
    if len(set(VCLIENT_VIRTUAL_TARGETS)) != 48:
        raise ValueError("VClient011 original slot target duplication")
    if next_bytes != VCLIENT_NEXT_DATA:
        raise ValueError("VClient011 vtable end marker/extent differs")
    for slot, expected in enumerate(VCLIENT_VIRTUAL_TARGETS):
        if struct.unpack_from("<I", pointer_bytes, slot * 4)[0] != expected:
            raise ValueError("VClient011 original vtable slot mismatch: "
                             + str(slot))
    return len(VCLIENT_VIRTUAL_TARGETS)


def verify_register_wrapper(blob):
    """Recover the exact 21-byte original static registration form."""
    if not isinstance(blob, bytes) or len(blob) != 21:
        raise ValueError("VClient011 InterfaceReg wrapper length differs")
    if blob[0] != 0x68 or blob[5] != 0x68 or blob[10] != 0xB9 or (
            blob[15] != 0xE8 or blob[20] != 0xC3):
        raise ValueError("VClient011 registration uses unexpected x86 form")
    name, creator, record = (
        struct.unpack_from("<I", blob, off)[0] for off in (1, 6, 11))
    if (name, creator, record) != (
            VCLIENT_NAME_VA, VCLIENT_FACTORY_VA, VCLIENT_INTERFACE_REG_VA):
        raise ValueError("VClient011 static registration pointers differ")
    rel = struct.unpack_from("<i", blob, 16)[0]
    if REGISTER_VCLIENT_VA + 20 + rel != REGISTRATION_CTOR_VA:
        raise ValueError("VClient011 registration constructor call differs")
    return (name, creator, record)


def section_backed_original(pe, address, extent, expected_section):
    if not checked_section(pe, address, extent, expected_section):
        raise ValueError("VClient011 PE32 method/data points outside expected "
                         + expected_section)
    return pe.raw(address, extent)


def verify(data):
    if hashlib.sha256(data).hexdigest() != CLIENT_SHA:
        raise ValueError("original 2006 client.dll SHA-256 mismatch")
    pe = PE32(data)
    if pe.base != IMAGE_BASE:
        raise ValueError("wrong original 2006 client PE32 image base")
    wrapper = section_backed_original(
        pe, REGISTER_VCLIENT_VA, 21, ".text")
    if hashlib.sha256(wrapper).hexdigest() != REGISTER_VCLIENT_SHA:
        raise ValueError("original VClient011 registration wrapper SHA mismatch")
    verify_register_wrapper(wrapper)
    if section_backed_original(
            pe, VCLIENT_NAME_VA, len(b"VClient011\x00"),
            ".rdata") != b"VClient011\x00":
        raise ValueError("VClient011 version name differs")
    creator = section_backed_original(
        pe, VCLIENT_FACTORY_VA, 6, ".text")
    if hashlib.sha256(creator).hexdigest() != VCLIENT_CREATOR_SHA:
        raise ValueError("original VClient011 singleton creator SHA mismatch")
    if creator != b"\xb8" + pack_u32(VCLIENT_SINGLETON_VA) + b"\xc3":
        raise ValueError("VClient011 creator does not return the singleton")
    if not checked_section(
            pe, VCLIENT_INTERFACE_REG_VA, 12, ".data"):
        raise ValueError("original InterfaceReg node not statically writable")
    singleton_vptr = section_backed_original(
        pe, VCLIENT_SINGLETON_VA, 4, ".data")
    if singleton_vptr != pack_u32(VCLIENT_VTABLE_VA):
        raise ValueError("original VClient011 singleton initial vptr differs")
    full_vtable = section_backed_original(
        pe, VCLIENT_VTABLE_VA, VTABLE_BYTES + len(VCLIENT_NEXT_DATA),
        ".rdata")
    if (hashlib.sha256(full_vtable[:VTABLE_BYTES]).hexdigest()
            != VCLIENT_VTABLE_SHA):
        raise ValueError("whole original VClient011 vtable SHA mismatch")
    slot_count = verify_vtable(full_vtable[:VTABLE_BYTES],
                               full_vtable[VTABLE_BYTES:])
    for target in VCLIENT_VIRTUAL_TARGETS:
        section_backed_original(pe, target, 1, ".text")
    for address, extent, expected_sha in (
            (CREATE_INTERFACE_VA, CREATE_INTERFACE_BYTES, CREATE_INTERFACE_SHA),
            (REGISTRATION_CTOR_VA, REGISTRATION_CTOR_BYTES,
             REGISTRATION_CTOR_SHA),
            (VCLIENT_INIT_VA, VCLIENT_INIT_BYTES, VCLIENT_INIT_SHA)):
        data_slice = section_backed_original(
            pe, address, extent, ".text")
        if hashlib.sha256(data_slice).hexdigest() != expected_sha:
            raise ValueError("source-authenticated original method SHA mismatch")
    return {
        "original_client_sha256_verified": True,
        "original_CreateInterface_va": hex(CREATE_INTERFACE_VA),
        "original_VClient011_name": "VClient011",
        "original_VClient011_static_registration_va": hex(REGISTER_VCLIENT_VA),
        "original_VClient011_singleton_va": hex(VCLIENT_SINGLETON_VA),
        "original_VClient011_vtable_va": hex(VCLIENT_VTABLE_VA),
        "original_VClient011_vtable_sha256": VCLIENT_VTABLE_SHA,
        "original_VClient011_vtable_slots_verified": slot_count,
        "original_VClient011_init_slot": 0,
        "original_VClient011_init_method_va": hex(VCLIENT_INIT_VA),
        "original_VClient011_shutdown_slot": 1,
        "original_VClient011_slot33_va": hex(SLOT33),
        "original_VClient011_last_method_va": hex(VCLIENT_VIRTUAL_TARGETS[-1]),
        "original_executable_bytes_exported": False,
        "original_code_executed": False,
        "retail_windows_startup_verified": False,
    }


def self_test():
    assert VCLIENT_VIRTUAL_TARGETS[0] == VCLIENT_INIT_VA
    assert VCLIENT_VIRTUAL_TARGETS[1] == 0x240FF3D0
    assert VCLIENT_VIRTUAL_TARGETS[33] == SLOT33
    assert VCLIENT_VIRTUAL_TARGETS[-1] == 0x240FE670
    pointer_bytes = b"".join(pack_u32(x) for x in VCLIENT_VIRTUAL_TARGETS)
    assert len(pointer_bytes) == 192
    assert hashlib.sha256(pointer_bytes).hexdigest() == VCLIENT_VTABLE_SHA
    assert verify_vtable(pointer_bytes, VCLIENT_NEXT_DATA) == 48
    for byte_position in range(len(pointer_bytes)):
        altered = bytearray(pointer_bytes)
        altered[byte_position] ^= 0x01
        try:
            verify_vtable(bytes(altered), VCLIENT_NEXT_DATA)
        except ValueError:
            pass
        else:
            raise AssertionError("corrupt VClient011 original vtable accepted")
    for wrong in (
            b"", VCLIENT_NEXT_DATA[:-1],
            b"Precache\x00", b"\x00" * len(VCLIENT_NEXT_DATA)):
        try:
            verify_vtable(pointer_bytes, wrong)
        except ValueError:
            pass
        else:
            raise AssertionError("invalid end of vtable accepted")
    for bad in (pointer_bytes[:-4], pointer_bytes + b"\x00\x00\x00\x00"):
        try:
            verify_vtable(bad, VCLIENT_NEXT_DATA)
        except ValueError:
            pass
        else:
            raise AssertionError("short/long original vtable accepted")
    rel = REGISTRATION_CTOR_VA - (REGISTER_VCLIENT_VA + 20)
    wrapper = (b"\x68" + pack_u32(VCLIENT_NAME_VA)
               + b"\x68" + pack_u32(VCLIENT_FACTORY_VA)
               + b"\xb9" + pack_u32(VCLIENT_INTERFACE_REG_VA)
               + b"\xe8" + struct.pack("<i", rel) + b"\xc3")
    assert hashlib.sha256(wrapper).hexdigest() == REGISTER_VCLIENT_SHA
    assert verify_register_wrapper(wrapper) == (
        VCLIENT_NAME_VA, VCLIENT_FACTORY_VA, VCLIENT_INTERFACE_REG_VA)
    for position in range(len(wrapper)):
        altered = bytearray(wrapper)
        altered[position] ^= 1
        try:
            verify_register_wrapper(bytes(altered))
        except ValueError:
            pass
        else:
            raise AssertionError("corrupt VClient011 registration accepted")
    try:
        verify(b"fabricated unsupported client image")
    except ValueError:
        pass
    else:
        raise AssertionError("fabricated game client image accepted")
    print("PASS original-free 2006 VClient011 48-slot vtable and registration: every pointer-byte corruption, section and strict tail delimiter, Init/Shutdown/slot33 positions, 21-byte static wrapper")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    modes = parser.add_mutually_exclusive_group(required=True)
    modes.add_argument("--self-test", action="store_true")
    modes.add_argument("--client", type=Path,
                       help="owner-private unmodified original client.dll")
    args = parser.parse_args()
    if args.self_test:
        self_test()
    else:
        print(json.dumps(verify(args.client.read_bytes()),
                         sort_keys=True, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
