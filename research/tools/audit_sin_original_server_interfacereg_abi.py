#!/usr/bin/env python3
"""Original-free or SHA-pinned owner-private audit of 2006 server.dll factories.

Discovers no code by guessing: known retail PE32 windows were independently
inspected in the owner's ORIGINAL SHA-verified binary. Emit metadata only.
No original DLL is loaded, patched, executed, or committed in public CI.

--self-test uses invented x86 wrapper/InterfaceReg/creator byte fragments.
--server validates the owner's complete original server.dll privately.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
from audit_sin_original_engine_gameents_consumer import PE32

SERVER_SHA = "568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650"
IMAGE_BASE = 0x22000000
CREATE_INTERFACE = 0x221FB5F0
INTERFACE_REG_CONSTRUCTOR = 0x221FB5D0
REGISTRY_HEAD = 0x22573C5C
CONSTRUCTOR_SHA = "5e4a38bbe846cdfddece56859b823676d516af3f171181d7415ebbdfc1376654"
CREATE_INTERFACE_SHA = "39ed7fe9be839faa563964d7f5c2ad810dcd1992511131659da7fe823194b98a"

# (public requested name, global initializer wrapper, wrapper-window SHA,
#  string VA, creator-function VA, static InterfaceReg storage VA,
#  creator-returned singleton VA, singleton first vtable pointer)
REGISTRATIONS = (
    ("ServerGameDLL003", 0x223BF140,
     "1d704ab1bab3ed7cdbbac122cf11d73b1c78a1269eb8f6651c03c7072a15a074",
     0x224432AC, 0x221E7630, 0x225722EC, 0x22510650, 0x22442EFC),
    ("ServerGameEnts001", 0x223BF210,
     "6c46b07d21b9fa3c4a216b491ec0719e94c4b1dd6d2a72bbf9b13bb9df79209a",
     0x22443314, 0x221E82C0, 0x225723E4, 0x22510654, 0x22443030),
    ("ServerGameClients003", 0x223BF230,
     "9336d0906b45de2f0a62d0a3ff1b21369dffeb18353b33b3490e54e277a675d6",
     0x22443328, 0x221E8320, 0x225723F0, 0x22510658, 0x22443054),
)

# Original CServerGameEnts singleton vptr at 0x22510654 points to
# 0x22443030. Seven pointers occupy slots +0x00 .. +0x18, followed
# by a zero DWORD then the next vtable's MSVC RTTI complete-object
# locator. This seven-slot count is proved by the original on-disk PE32.
SERVER_GAMEENTS_VTABLE = 0x22443030
SERVER_GAMEENTS_SLOTS = (
    0x221E9360, 0x221E82D0, 0x221E9380, 0x221E82E0,
    0x221E94B0, 0x221E82F0, 0x221E94D0,
)

def check_gameents_vtable(blob):
    """Check all seven original PE32 virtual targets and next delimiter."""
    if len(blob) != 8 * 4 or len(SERVER_GAMEENTS_SLOTS) != 7:
        raise ValueError("invalid original GameEnts seven-slot extent")
    for slot, method in enumerate(SERVER_GAMEENTS_SLOTS):
        if u32(blob, slot * 4) != method:
            raise ValueError("original GameEnts virtual slot changed")
    if u32(blob, 7 * 4) != 0:
        raise ValueError("original GameEnts next-vtable delimiter differs")
    return len(SERVER_GAMEENTS_SLOTS)

def u32(data, offset):
    return struct.unpack_from("<I", data, offset)[0]

def check_sha(blob, expected):
    if hashlib.sha256(blob).hexdigest() != expected:
        raise ValueError("original bounded ABI window hash mismatch")

def decode_constructor(blob, head_global):
    """Native MSVC x86 __thiscall InterfaceReg constructor, 32 bytes.

    Input in ECX = 12-byte record, stack args creator and interface name.
    Stores creator+0x00, name+0x04, prior list head+0x08, then sets head.
    """
    if not isinstance(blob, (bytes, bytearray)) or len(blob) != 0x20:
        raise ValueError("invalid original InterfaceReg ctor extent")
    expected = (
        bytes.fromhex("8b5424048bc18b4c24088948048910") +
        bytes.fromhex("8b0d") + struct.pack("<I", head_global) +
        bytes.fromhex("894808a3") + struct.pack("<I", head_global) +
        bytes.fromhex("c20800")
    )
    if blob != expected:
        raise ValueError("InterfaceReg ctor field layout or list update differs")
    return {"record_bytes": 12, "creator_offset": 0, "name_offset": 4,
            "next_offset": 8, "return_stack_pop_bytes": 8}

def decode_registration(blob, wrapper_va, ctor_va):
    """21-byte source-authenticated static registration wrapper."""
    if not isinstance(blob, (bytes, bytearray)) or len(blob) != 21:
        raise ValueError("invalid InterfaceReg registration extent")
    if bytes([blob[x] for x in (0, 5, 10, 15, 20)]) != bytes.fromhex(
            "6868b9e8c3"):
        raise ValueError("unexpected registration x86 opcodes")
    target = wrapper_va + 20 + struct.unpack_from("<i", blob, 16)[0]
    if target != ctor_va:
        raise ValueError("static registration called wrong InterfaceReg ctor")
    return u32(blob, 1), u32(blob, 6), u32(blob, 11)

def decode_singleton_creator(blob):
    """Original six-byte mov eax,imm32; ret, no speculative invocation."""
    if (not isinstance(blob, (bytes, bytearray)) or len(blob) != 6
            or blob[0] != 0xB8 or blob[5] != 0xC3):
        raise ValueError("original creator is not a direct singleton return")
    return u32(blob, 1)

def check_createinterface_body(blob):
    if len(blob) != 0x72:
        raise ValueError("CreateInterface body truncated")
    # SHA anchors the full original function; these checks explain the
    # audited ABI/error behavior without exporting its executable bytes.
    if (blob[0:3] != bytes.fromhex("555657")
            or blob[3:5] != bytes.fromhex("8b3d")
            or u32(blob, 5) != REGISTRY_HEAD
            or blob[0x52:0x58] != bytes.fromhex("c70001000000")
            or blob[0x66:0x6C] != bytes.fromhex("c70000000000")
            or blob[0x6C:0x6E] != bytes.fromhex("ff17")):
        raise ValueError("original CreateInterface register/return semantics differ")

def _read_cstr(pe, rva):
    data = bytearray()
    for i in range(128):
        b = pe.raw(IMAGE_BASE + rva + i, 1)
        if b == b"\0":
            return bytes(data).decode("ascii")
        data.extend(b)
    raise ValueError("unterminated original PE export string")

def check_named_createinterface_export(data, pe):
    e_lfanew = u32(data, 0x3c)
    opt = e_lfanew + 24
    rva, size = struct.unpack_from("<II", data, opt + 96)
    if not rva or size < 40:
        raise ValueError("original server has no PE32 export directory")
    row = pe.raw(IMAGE_BASE + rva, 40)
    name_rva, ordinal_base, functions, names, function_rva, names_rva, ords_rva = (
        struct.unpack_from("<IIIIIII", row, 12))
    if _read_cstr(pe, name_rva) != "server.dll" or functions != 5 or names != 5:
        raise ValueError("unexpected original server export identity/count")
    found = False
    for i in range(names):
        string_rva = u32(pe.raw(IMAGE_BASE + names_rva + 4*i, 4), 0)
        if _read_cstr(pe, string_rva) != "CreateInterface":
            continue
        ord_index = struct.unpack_from(
            "<H", pe.raw(IMAGE_BASE + ords_rva + 2*i, 2))[0]
        if ord_index >= functions:
            raise ValueError("CreateInterface export ordinal outside table")
        target_rva = u32(
            pe.raw(IMAGE_BASE + function_rva + 4*ord_index, 4), 0)
        if IMAGE_BASE + target_rva != CREATE_INTERFACE:
            raise ValueError("original CreateInterface export target differs")
        found = True
    if not found:
        raise ValueError("original server CreateInterface export missing")
    return names

def audit(data):
    if hashlib.sha256(data).hexdigest() != SERVER_SHA:
        raise ValueError("unexpected original 2006 server.dll SHA")
    pe = PE32(data)
    if pe.base != IMAGE_BASE or not pe.contains_data_global(REGISTRY_HEAD):
        raise ValueError("server image base or InterfaceReg head storage differs")
    check_sha(pe.raw(INTERFACE_REG_CONSTRUCTOR, 0x20), CONSTRUCTOR_SHA)
    layout = decode_constructor(pe.raw(INTERFACE_REG_CONSTRUCTOR, 0x20),
                                REGISTRY_HEAD)
    body = pe.raw(CREATE_INTERFACE, 0x72)
    check_sha(body, CREATE_INTERFACE_SHA)
    check_createinterface_body(body)
    named_exports = check_named_createinterface_export(data, pe)
    gameents_slots = check_gameents_vtable(
        pe.raw(SERVER_GAMEENTS_VTABLE, 8 * 4))
    verified = []
    for (name, wrapper, window_sha, name_va, creator,
         record, singleton, vtable) in REGISTRATIONS:
        block = pe.raw(wrapper, 21)
        check_sha(block, window_sha)
        got_name, got_creator, got_record = decode_registration(
            block, wrapper, INTERFACE_REG_CONSTRUCTOR)
        if (got_name, got_creator, got_record) != (name_va, creator, record):
            raise ValueError("original static InterfaceReg operands differ")
        if pe.raw(name_va, len(name)+1) != name.encode("ascii")+b"\0":
            raise ValueError("original registration interface string mismatch")
        if not pe.contains_data_global(record):
            raise ValueError("original InterfaceReg record not in server .data")
        returned = decode_singleton_creator(pe.raw(creator, 6))
        if returned != singleton or not pe.contains_data_global(returned):
            raise ValueError("original creator returns wrong singleton storage")
        if u32(pe.raw(singleton, 4), 0) != vtable:
            raise ValueError("original singleton vptr address differs")
        verified.append({
            "name": name, "original_wrapper_va": hex(wrapper),
            "original_creator_va": hex(creator),
            "original_record_va": hex(record),
            "original_singleton_va": hex(singleton),
            "original_vtable_va": hex(vtable),
        })
    return {
        "original_server_sha256_verified": True,
        "original_server_CreateInterface_export_va": hex(CREATE_INTERFACE),
        "original_server_named_exports": named_exports,
        "original_registered_interface_count_verified": len(verified),
        "original_ServerGameEnts001_virtual_slots_verified": gameents_slots,
        "original_ServerGameEnts001_slot_targets": [
            hex(va) for va in SERVER_GAMEENTS_SLOTS],
        "original_InterfaceReg_native_layout": layout,
        "original_interface_registrations": verified,
        "original_executable_bytes_exported": False,
        "actual_retail_factory_dispatch_executed": False,
        "independent_replacement_factory_registered": False,
        "retail_windows_startup_verified": False,
    }

def make_registration(wrapper_va, name_va, creator_va, record_va, ctor_va):
    displacement = ctor_va - (wrapper_va + 20)
    return (b"\x68" + struct.pack("<I", name_va) +
            b"\x68" + struct.pack("<I", creator_va) +
            b"\xb9" + struct.pack("<I", record_va) +
            b"\xe8" + struct.pack("<i", displacement) + b"\xc3")

def self_test():
    assert len(SERVER_SHA) == 64 and IMAGE_BASE == 0x22000000
    assert len(REGISTRATIONS) == 3
    assert tuple(row[0] for row in REGISTRATIONS) == (
        "ServerGameDLL003", "ServerGameEnts001", "ServerGameClients003")
    vtable = b"".join(struct.pack("<I", item)
                      for item in SERVER_GAMEENTS_SLOTS + (0,))
    assert check_gameents_vtable(vtable) == 7
    for i in range(len(vtable)):
        invalid = bytearray(vtable)
        invalid[i] ^= 1
        try:
            check_gameents_vtable(invalid)
        except ValueError:
            pass
        else:
            raise AssertionError("corrupted original GameEnts vtable accepted")
    try:
        check_gameents_vtable(vtable[:-4])
    except ValueError:
        pass
    else:
        raise AssertionError("truncated original GameEnts vtable accepted")
    dummy_head = 0x33112200
    ctor = (bytes.fromhex("8b5424048bc18b4c24088948048910") +
            bytes.fromhex("8b0d") + struct.pack("<I", dummy_head) +
            bytes.fromhex("894808a3") + struct.pack("<I", dummy_head) +
            bytes.fromhex("c20800"))
    layout = decode_constructor(ctor, dummy_head)
    assert layout["record_bytes"] == 12 and layout["next_offset"] == 8
    for off in range(len(ctor)):
        mutated = bytearray(ctor)
        mutated[off] ^= 1
        try:
            decode_constructor(mutated, dummy_head)
        except ValueError:
            pass
        else:
            raise AssertionError("corrupted 12-byte InterfaceReg ABI accepted")
    for row in REGISTRATIONS:
        name, wrapper, _, name_va, creator, record, singleton, vtable = row
        block = make_registration(
            wrapper, name_va, creator, record, INTERFACE_REG_CONSTRUCTOR)
        assert len(block) == 21
        assert decode_registration(
            block, wrapper, INTERFACE_REG_CONSTRUCTOR) == (
            name_va, creator, record)
        for offset in range(21):
            changed = bytearray(block)
            changed[offset] ^= 1
            try:
                # The wrapper-only decoder rejects structural changes,
                # while legitimate-looking changed operands are refused
                # by exact source-authenticated operand verification.
                result = decode_registration(
                    changed, wrapper, INTERFACE_REG_CONSTRUCTOR)
                if result != (name_va, creator, record):
                    continue
            except ValueError:
                continue
            raise AssertionError("corrupted static InterfaceReg record accepted")
        valid_creator = b"\xb8"+struct.pack("<I", singleton)+b"\xc3"
        assert decode_singleton_creator(valid_creator) == singleton
        for bad in (valid_creator[:-1],b"\x90"+valid_creator[1:],
                    valid_creator[:-1]+b"\x90"):
            try:
                decode_singleton_creator(bad)
            except ValueError:
                pass
            else:
                raise AssertionError("invalid creator return accepted")
    try:
        audit(b"not original server.dll")
    except ValueError:
        pass
    else:
        raise AssertionError("fake original server accepted")
    print("PASS original-free 2006 server InterfaceReg: 12-byte linked records, three SHA-backed factory wrappers, seven GameEnts virtual slots, singleton return and corruption rejections")

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    modes = parser.add_mutually_exclusive_group(required=True)
    modes.add_argument("--self-test", action="store_true")
    modes.add_argument("--server", type=Path)
    args = parser.parse_args()
    if args.self_test:
        self_test()
    else:
        print(json.dumps(audit(args.server.read_bytes()), indent=2, sort_keys=True))

if __name__ == "__main__":
    main()
