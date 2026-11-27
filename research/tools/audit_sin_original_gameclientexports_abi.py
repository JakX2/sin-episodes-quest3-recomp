#!/usr/bin/env python3
"""SHA-pinned, owner-private original SiN 2006 GameClientExports001 ABI audit.

Public --self-test constructs only invented PE32 pointers/x86 call operands.
Private --client reads the unchanged original file and emits metadata only.
No original machine code, raw binary image, or game process is uploaded,
loaded, modified or executed.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import struct
from audit_sin_original_engine_gameents_consumer import PE32

CLIENT_SHA = "6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d"
IMAGE_BASE = 0x24000000
NAME = "GameClientExports001"
REG_WRAPPER = 0x242682B0
REG_WRAPPER_SHA = "5ab82a24da9808d2e172eaab5d85ffd71e7f075dcfab70ec85188d4686e1919b"
REG_CTOR = 0x24164160
NAME_VA = 0x24296E10
CREATOR_VA = 0x240FD810
CREATOR_SHA = "e65c1e17c05a9b36167fe1e9bee4af19b9055e0b9764c299ee84d053f73d2d26"
RECORD_VA = 0x2433F7E4
SINGLETON_VA = 0x242FA4B0
VTABLE_VA = 0x24296B18
VTABLE_SHA = "23ced2e618d2df47dff35544c12c237d3edf7d7b5a25c823b02d1f433c65dc82"
VTABLE_TARGETS = (
    0x240FE9B0, 0x240FE950, 0x240FE970, 0x240FE990,
)
AFTER_VTABLE = b"g_ParticleMgr.Update\0\0"
# Complete original x86 function SHA, not original bytes.
METHODS = (
    ("scalar_deleting_destructor", 0x240FE9B0, 31,
     "c1b42cad1fbf2c68930e72c983f094ae254a3292aa65d90835c33b97dc50a421"),
    ("forward_slot1", 0x240FE950, 20,
     "f2d5d18b7bd577abae8bbaec5338c5e0d350a6a9b5f1903f2a6ee7c9eae4e1b2"),
    ("forward_slot2", 0x240FE970, 22,
     "22ee0da255959eeba09b869bbbe5aea7a18328f23c474ce38b203af2df5222a2"),
    ("forward_slot3", 0x240FE990, 22,
     "16e4a8e804463ce6a29653986bc7f88b381d8d535b5b1100847b74424f7bc40f"),
)
RESOLVE_GLOBAL_VA = 0x241E5C00
SLOT1_HANDLER_VA = 0x241E51B0
SLOT23_HANDLER_VA = 0x241E5B20
DESTRUCTOR_BASE_VPTR = 0x2429664C
DESTRUCTOR_DELETE_VA = 0x2416FD60


def check(cond, message):
    if not cond:
        raise ValueError(message)


def u32(data, off=0):
    return struct.unpack_from("<I", data, off)[0]


def pack(value):
    return struct.pack("<I", value)


def checked_section(pe, va, size, wanted):
    rva = va - pe.base
    return any(section == wanted and first <= rva
               and rva + size <= first + max(virt, raw)
               for section, first, virt, raw, offset in pe.sections)


def read_section(pe, va, size, section):
    check(checked_section(pe, va, size, section),
          "GameClientExports001 outside authenticated " + section)
    return pe.raw(va, size)


def decode_registration(data, wrapper_va, expected_ctor, name, creator, record):
    check(len(data) == 21, "wrong static registration extent")
    check(all(data[at] == opcode
              for at, opcode in ((0, 0x68), (5, 0x68), (10, 0xB9),
                                 (15, 0xE8), (20, 0xC3))),
          "wrong original InterfaceReg wrapper x86 instruction form")
    check((u32(data, 1), u32(data, 6), u32(data, 11)) ==
          (name, creator, record), "wrong original InterfaceReg operands")
    destination = wrapper_va + 20 + struct.unpack_from("<i", data, 16)[0]
    check(destination == expected_ctor, "wrong original InterfaceReg constructor")
    return (name, creator, record)


def call_target(code, start_va, opcode_at):
    check(code[opcode_at] == 0xE8, "missing direct original call")
    rel = struct.unpack_from("<i", code, opcode_at + 1)[0]
    return start_va + opcode_at + 5 + rel


def verify_deleting_destructor(code, method_va, parent_vtable, delete_target):
    """Recognize MSVC x86 scalar deleting destructor (flags&1, ret 4).

    Requires exact structural shape and original call and vtable operands.
    Does NOT call it and does not infer a public destructor API.
    """
    check(len(code) == 31, "scalar deleting destructor extent")
    check(code[:10] == bytes.fromhex("f644240401568bf1c706"),
          "scalar deleting destructor flags/this/vptr pattern")
    check(u32(code, 10) == parent_vtable,
          "scalar deleting destructor parent vtable operand")
    check(code[14:18] == bytes.fromhex("740956e8"),
          "scalar deleting destructor branch/call shape")
    check(call_target(code, method_va, 17) == delete_target,
          "scalar deleting destructor delete target")
    check(code[22:] == bytes.fromhex("83c4048bc65ec20400"),
          "scalar deleting destructor cleanup/thiscall ret 4")
    return 4


def verify_forward(code, method_va, slot, resolver_va, handler_va):
    check(slot in (1, 2, 3), "unrecognized original game-export virtual slot")
    check(len(code) == (20 if slot == 1 else 22),
          "original forwarding slot extent")
    check(code[:4] == bytes.fromhex("8b442404"),
          "original forwarding slot not reading one stack argument")
    first = 5 if slot == 1 else 7
    second = 12 if slot == 1 else 14
    if slot == 1:
        check(code[4] == 0x50, "slot1 missing argument push")
    else:
        check(code[4:7] == (
            bytes.fromhex("6a0150") if slot == 2
            else bytes.fromhex("6a0050")),
            "slot2/3 missing exact forced flag + argument push")
    check(code[second-2:second] == bytes.fromhex("8bc8"),
          "forwarding slot does not pass resolved object through ECX")
    check(code[-3:] == bytes.fromhex("c20400"),
          "original forwarding virtual does not clean four argument bytes")
    check(call_target(code, method_va, first) == resolver_va,
          "forwarding slot resolved a different original service")
    check(call_target(code, method_va, second) == handler_va,
          "forwarding slot called wrong original service")
    return 4


def verify_vtable(pointer_bytes, after_bytes):
    check(len(pointer_bytes) == 16, "original client export virtual count not four")
    check(after_bytes == AFTER_VTABLE,
          "original four-slot vtable boundary marker differs")
    for slot, va in enumerate(VTABLE_TARGETS):
        check(u32(pointer_bytes, slot*4) == va,
              "original GameClientExports001 virtual slot changed: "
              + str(slot))
    return len(VTABLE_TARGETS)


def verify(client):
    check(hashlib.sha256(client).hexdigest() == CLIENT_SHA,
          "original owner client.dll full-file SHA-256 mismatch")
    pe = PE32(client)
    check(pe.base == IMAGE_BASE, "wrong original client PE32 image base")
    wrapper = read_section(pe, REG_WRAPPER, 21, ".text")
    check(hashlib.sha256(wrapper).hexdigest() == REG_WRAPPER_SHA,
          "original GameClientExports001 register wrapper digest mismatch")
    decode_registration(wrapper, REG_WRAPPER, REG_CTOR,
                        NAME_VA, CREATOR_VA, RECORD_VA)
    check(read_section(pe, NAME_VA, len(NAME)+1, ".rdata") ==
          NAME.encode("ascii") + b"\0", "original client interface name changed")
    creator = read_section(pe, CREATOR_VA, 6, ".text")
    check(hashlib.sha256(creator).hexdigest() == CREATOR_SHA,
          "original GameClientExports001 creator digest mismatch")
    check(creator == b"\xB8" + pack(SINGLETON_VA) + b"\xC3",
          "original client GameClientExports creator does not return singleton")
    check(checked_section(pe, RECORD_VA, 12, ".data"),
          "InterfaceReg storage is not original client static writable storage")
    check(read_section(pe, SINGLETON_VA, 4, ".data") == pack(VTABLE_VA),
          "original GameClientExports001 singleton vptr mismatch")
    vtable = read_section(
        pe, VTABLE_VA, 16+len(AFTER_VTABLE), ".rdata")
    check(hashlib.sha256(vtable[:16]).hexdigest() == VTABLE_SHA,
          "original complete 4-pointer vtable SHA mismatch")
    total = verify_vtable(vtable[:16], vtable[16:])
    for va in VTABLE_TARGETS:
        check(checked_section(pe, va, 1, ".text"),
              "original GameClientExports001 virtual target outside .text")
    for name, va, count, sha in METHODS:
        code = read_section(pe, va, count, ".text")
        check(hashlib.sha256(code).hexdigest() == sha,
              "original GameClientExports001 method SHA mismatch: " + name)
        if va == VTABLE_TARGETS[0]:
            verify_deleting_destructor(
                code, va, DESTRUCTOR_BASE_VPTR, DESTRUCTOR_DELETE_VA)
        elif va == VTABLE_TARGETS[1]:
            verify_forward(code, va, 1, RESOLVE_GLOBAL_VA, SLOT1_HANDLER_VA)
        elif va == VTABLE_TARGETS[2]:
            verify_forward(code, va, 2, RESOLVE_GLOBAL_VA, SLOT23_HANDLER_VA)
        elif va == VTABLE_TARGETS[3]:
            verify_forward(code, va, 3, RESOLVE_GLOBAL_VA, SLOT23_HANDLER_VA)
    return {
        "original_client_sha256_verified": True,
        "original_GameClientExports001_registration_va": hex(REG_WRAPPER),
        "original_GameClientExports001_singleton_va": hex(SINGLETON_VA),
        "original_GameClientExports001_vtable_va": hex(VTABLE_VA),
        "original_GameClientExports001_vtable_sha256": VTABLE_SHA,
        "original_GameClientExports001_virtual_slots_verified": total,
        "slot0_scalar_deleting_destructor_verified": True,
        "slots1_through_3_thiscall_single_dword_argument_verified": True,
        "slots2_and_3_force_distinct_boolean_flags": True,
        "original_virtual_call_target_addresses": [
            hex(value) for value in VTABLE_TARGETS],
        "original_method_signature_types_fully_reconstructed": False,
        "independent_GameClientExports001_registered": False,
        "original_executable_bytes_exported": False,
        "original_dll_loaded_or_executed": False,
        "original_game_startup_verified": False
    }


def synthetic_wrapper(wrapper_va, name, creator, record, ctor):
    delta = ctor - wrapper_va - 20
    return (b"\x68" + pack(name) + b"\x68" + pack(creator) +
            b"\xB9" + pack(record) + b"\xE8" +
            struct.pack("<i", delta) + b"\xC3")


def synthetic_forward(va, slot, resolver, handler):
    prefix = b"\x8B\x44\x24\x04"
    if slot == 1:
        prefix += b"\x50"
    else:
        prefix += b"\x6A" + bytes([1 if slot == 2 else 0]) + b"\x50"
    def relcall(addr):
        return b"\xE8" + struct.pack(
            "<i", addr - (va + len(prefix) + 5))
    # Caller creates the first call, sets ECX, then calls the handler.
    first = prefix + relcall(resolver)
    second = first + b"\x8B\xC8"
    offset = va + len(second)
    return (second + b"\xE8" +
            struct.pack("<i", handler - (offset + 5)) +
            b"\xC2\x04\x00")


def synthetic_dtor(va, parent, delete):
    prefix = bytes.fromhex("f644240401568bf1c706") + pack(parent) + \
        bytes.fromhex("740956e8")
    return (prefix + struct.pack("<i", delete - (va+22)) +
            bytes.fromhex("83c4048bc65ec20400"))


def self_test():
    assert len(VTABLE_TARGETS) == 4 and len(AFTER_VTABLE) == 22
    ptrs = b"".join(pack(x) for x in VTABLE_TARGETS)
    assert hashlib.sha256(ptrs).hexdigest() == VTABLE_SHA
    assert verify_vtable(ptrs, AFTER_VTABLE) == 4
    for pos in range(len(ptrs)):
        corrupt = bytearray(ptrs); corrupt[pos] ^= 0x01
        try: verify_vtable(bytes(corrupt), AFTER_VTABLE)
        except ValueError: pass
        else: raise AssertionError("corrupted original vtable was accepted")
    for marker in (b"", AFTER_VTABLE[:-1], b"g_ParticleMgr.Updates\0"):
        try: verify_vtable(ptrs, marker)
        except ValueError: pass
        else: raise AssertionError("wrong vtable terminator was accepted")
    wrapper = synthetic_wrapper(
        REG_WRAPPER, NAME_VA, CREATOR_VA, RECORD_VA, REG_CTOR)
    assert len(wrapper) == 21
    assert hashlib.sha256(wrapper).hexdigest() == REG_WRAPPER_SHA
    assert decode_registration(wrapper, REG_WRAPPER, REG_CTOR,
                               NAME_VA, CREATOR_VA, RECORD_VA) == (
            NAME_VA, CREATOR_VA, RECORD_VA)
    for pos in range(len(wrapper)):
        corrupt = bytearray(wrapper); corrupt[pos] ^= 0x01
        try: decode_registration(
            bytes(corrupt), REG_WRAPPER, REG_CTOR,
            NAME_VA, CREATOR_VA, RECORD_VA)
        except ValueError: pass
        else: raise AssertionError("wrong static original registration accepted")
    fixture_dtor = synthetic_dtor(
        0x25006000, 0x2505A100, 0x25100450)
    assert len(fixture_dtor) == 31
    assert verify_deleting_destructor(
        fixture_dtor, 0x25006000, 0x2505A100, 0x25100450) == 4
    for pos in range(len(fixture_dtor)):
        corrupt = bytearray(fixture_dtor); corrupt[pos] ^= 0x01
        try: verify_deleting_destructor(
            bytes(corrupt), 0x25006000, 0x2505A100, 0x25100450)
        except ValueError: pass
        else: raise AssertionError("wrong scalar deleting destructor accepted")
    for slot in (1, 2, 3):
        va = 0x25006100 + slot*0x40
        code = synthetic_forward(
            va, slot, 0x250A1000, 0x250A2000 + slot*0x40)
        assert len(code) == (20 if slot == 1 else 22)
        assert verify_forward(
            code, va, slot, 0x250A1000,
            0x250A2000 + slot*0x40) == 4
        for pos in range(len(code)):
            corrupt = bytearray(code); corrupt[pos] ^= 0x01
            try: verify_forward(bytes(corrupt), va, slot,
                                0x250A1000, 0x250A2000 + slot*0x40)
            except ValueError: pass
            else: raise AssertionError(
                "wrong forwarding thiscall slot accepted: "+str(slot))
    try: verify(b"fabricated original client data not trusted")
    except ValueError: pass
    else: raise AssertionError("fabricated game client SHA accepted")
    print("PASS original-free 2006 GameClientExports001: 4-slot table, scalar deleting destructor ret4, three single-DWORD thiscall forwards, exact flags/calls and byte-corruption refusal")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    modes = parser.add_mutually_exclusive_group(required=True)
    modes.add_argument("--self-test", action="store_true")
    modes.add_argument("--client", type=Path,
                       help="owner-private exact original client.dll")
    args = parser.parse_args()
    if args.self_test:
        self_test()
    else:
        print(json.dumps(verify(args.client.read_bytes()),
                         sort_keys=True, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
