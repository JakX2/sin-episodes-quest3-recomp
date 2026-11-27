#!/usr/bin/env python3
"""Original-only 2006 PE32 ServerGameClients003 complete vtable + slot0 ABI.

--server is a read-only, SHA-authenticated owner-private file audit.
--self-test invents altered player-limit machine words and fake pointers.
Never load commercial game code, export original binary bytes, or call slots.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
from audit_sin_original_engine_gameents_consumer import PE32

SERVER_SHA = "568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650"
IMAGE_BASE = 0x22000000
INTERFACE_NAME = "ServerGameClients003"
REG_WRAPPER = 0x223BF230
REG_WRAPPER_SHA = "9336d0906b45de2f0a62d0a3ff1b21369dffeb18353b33b3490e54e277a675d6"
REG_CTOR = 0x221FB5D0
NAME_VA = 0x22443328
FACTORY_VA = 0x221E8320
FACTORY_SHA = "c811e01f62d7f1a8ec30e417c67c5e99dbf5f6dd9fa6a7112f92790e4bdece9c"
STATIC_RECORD_VA = 0x225723F0
SINGLETON_VA = 0x22510658
VTABLE_VA = 0x22443054
VTABLE_SHA = "5d0631d25abacc8fea804b22e5bf489b2d667ce240417c3ab20cc06787eb535f"
VTABLE_TARGETS = (
    0x221E63F0, 0x221E8330, 0x221EA330, 0x221EA3A0, 0x221E8340,
    0x221E9650, 0x221E8510, 0x221E8370, 0x221E9C00, 0x221EA600,
    0x221E84D0, 0x221E85B0, 0x221E8530, 0x221E8520, 0x221E85E0,
)
VIRTUAL_COUNT = len(VTABLE_TARGETS)
NEXT_MARKER = b"WriteBool called with no active message\n\0"
PLAYER_LIMITS_METHOD_VA = VTABLE_TARGETS[0]
PLAYER_LIMITS_METHOD_SIZE = 33
PLAYER_LIMITS_METHOD_SHA = "5b5cbbc2f6e847b94658b3d595ffea6859a6a09d62124f13a838a7159b4785c3"


def require(cond, message):
    if not cond:
        raise ValueError(message)


def pack(value):
    return struct.pack("<I", value)


def u32(data, index=0):
    return struct.unpack_from("<I", data, index)[0]


def verify_vtable(pointer_bytes, after_bytes):
    require(VIRTUAL_COUNT == 15 and len(pointer_bytes) == 60,
            "original ServerGameClients003 has exactly 15 PE32 virtuals")
    require(after_bytes == NEXT_MARKER,
            "original ServerGameClients003 next data marker differs")
    for slot, expected in enumerate(VTABLE_TARGETS):
        require(u32(pointer_bytes, slot*4) == expected,
                "original 2006 ServerGameClients003 slot target differs: "
                + str(slot))
    return VIRTUAL_COUNT


def decode_wrapper(data):
    require(len(data) == 21,
            "original ServerGameClients003 static wrapper truncated")
    require(all(data[offset] == opcode
                for offset, opcode in ((0, 0x68), (5, 0x68),
                                       (10, 0xB9), (15, 0xE8),
                                       (20, 0xC3))),
            "unexpected original InterfaceReg wrapper opcode")
    require((u32(data, 1), u32(data, 6), u32(data, 11)) ==
            (NAME_VA, FACTORY_VA, STATIC_RECORD_VA),
            "original ServerGameClients003 static register operands differ")
    called = REG_WRAPPER+20+struct.unpack_from("<i",data,16)[0]
    require(called == REG_CTOR,
            "original static ServerGameClients003 did not call real InterfaceReg ctor")
    return (NAME_VA, FACTORY_VA, STATIC_RECORD_VA)


def verify_player_limits(code, minimum=1, maximum=32, default=1):
    """Source-authenticated exact MSVC x86 call signature, no execution.

    void __thiscall GetPlayerLimits(int& min, int& max, int& default):
      ECX is ignored, arguments are three DWORD references, RET 0x0C.
    Writes 1, 32, 1 respectively in the original PE32 method.
    """
    require(len(code) == PLAYER_LIMITS_METHOD_SIZE,
            "original player limits method not 33 bytes")
    require(code[:12] == bytes.fromhex("8b44240c8b4c24048b542408"),
            "wrong original x86 three ref argument loads")
    require((code[12:14],code[18:20],code[24:26]) ==
            (b"\xC7\x00", b"\xC7\x01", b"\xC7\x02"),
            "wrong original x86 limit-outparam stores")
    require((u32(code,14),u32(code,20),u32(code,26)) ==
            (default,minimum,maximum),
            "original source player limits must be 1/32/1")
    require(code[-3:] == b"\xC2\x0C\x00",
            "original player limit virtual does not clean 12 stack bytes")
    return (minimum,maximum,default)


def check_section(pe, va, size, section):
    rva = va-pe.base
    return any(s==section and start<=rva
               and rva+size<=start+max(virtual,raw)
               for s,start,virtual,raw,off in pe.sections)


def read_section(pe, va, size, section):
    require(check_section(pe,va,size,section),
            "original ServerGameClients method/data outside "+section)
    return pe.raw(va,size)


def verify(raw):
    require(hashlib.sha256(raw).hexdigest() == SERVER_SHA,
            "wrong original server.dll SHA-256")
    pe = PE32(raw)
    require(pe.base == IMAGE_BASE, "wrong original 2006 server PE32 base")
    wrapper = read_section(pe,REG_WRAPPER,21,".text")
    require(hashlib.sha256(wrapper).hexdigest() == REG_WRAPPER_SHA,
            "original ServerGameClients003 21-byte wrapper digest differs")
    decode_wrapper(wrapper)
    require(read_section(
        pe, NAME_VA, len(INTERFACE_NAME)+1,
        ".rdata") == INTERFACE_NAME.encode("ascii")+b"\0",
        "original version string is not ServerGameClients003")
    creator = read_section(pe,FACTORY_VA,6,".text")
    require(hashlib.sha256(creator).hexdigest()==FACTORY_SHA,
            "original ServerGameClients003 singleton factory digest differs")
    require(creator==b"\xB8"+pack(SINGLETON_VA)+b"\xC3",
            "original singleton creator returns wrong object address")
    require(check_section(pe,STATIC_RECORD_VA,12,".data"),
            "original InterfaceReg record not in static writable data")
    require(read_section(pe,SINGLETON_VA,4,".data")==pack(VTABLE_VA),
            "original singleton initial vptr does not reference expected vtable")
    table = read_section(
        pe,VTABLE_VA,4*VIRTUAL_COUNT+len(NEXT_MARKER),".rdata")
    require(hashlib.sha256(table[:4*VIRTUAL_COUNT]).hexdigest()==VTABLE_SHA,
            "original whole 15-pointer ServerGameClients vtable SHA mismatch")
    count=verify_vtable(table[:4*VIRTUAL_COUNT],table[4*VIRTUAL_COUNT:])
    for va in VTABLE_TARGETS:
        require(check_section(pe,va,1,".text"),
                "original ServerGameClients003 virtual method outside .text")
    code=read_section(
        pe,PLAYER_LIMITS_METHOD_VA,PLAYER_LIMITS_METHOD_SIZE,".text")
    require(hashlib.sha256(code).hexdigest()==PLAYER_LIMITS_METHOD_SHA,
            "original GetPlayerLimits full method SHA changed")
    limits=verify_player_limits(code)
    return {
        "original_server_sha256_verified":True,
        "original_interface_name":INTERFACE_NAME,
        "original_registration_wrapper_va":hex(REG_WRAPPER),
        "original_creator_va":hex(FACTORY_VA),
        "original_singleton_va":hex(SINGLETON_VA),
        "original_vtable_va":hex(VTABLE_VA),
        "original_complete_vtable_sha256":VTABLE_SHA,
        "original_virtual_slots_verified":count,
        "original_GetPlayerLimits_va":hex(PLAYER_LIMITS_METHOD_VA),
        "original_GetPlayerLimits_method_sha256":PLAYER_LIMITS_METHOD_SHA,
        "original_GetPlayerLimits_thiscall_stack_cleanup_bytes":12,
        "original_player_limit_values":{
            "minimum":limits[0],"maximum":limits[1],"default":limits[2]},
        "original_executable_bytes_exported":False,
        "original_code_executed":False,
        "original_game_startup_verified":False,
    }


def invented_limits(method_min,method_max,method_default):
    # Synthetic generated bytes use DIFFERENT values to the original game.
    # No commercial method bytes are embedded in public test fixtures.
    return (bytes.fromhex(
        "8b44240c8b4c24048b542408c700")+
        pack(method_default)+bytes.fromhex("c701")+
        pack(method_min)+bytes.fromhex("c702")+
        pack(method_max)+b"\xC2\x0C\x00")


def self_test():
    assert VIRTUAL_COUNT==15
    assert len(NEXT_MARKER)==41
    assert VTABLE_TARGETS[0]==PLAYER_LIMITS_METHOD_VA
    assert VTABLE_TARGETS[-1]==0x221E85E0
    table=b"".join(pack(target) for target in VTABLE_TARGETS)
    assert hashlib.sha256(table).hexdigest()==VTABLE_SHA
    assert verify_vtable(table,NEXT_MARKER)==15
    for pos in range(len(table)):
        other=bytearray(table);other[pos]^=1
        try: verify_vtable(bytes(other),NEXT_MARKER)
        except ValueError: pass
        else: raise AssertionError("corrupted original vtable was accepted")
    for marker in (b"",NEXT_MARKER[:-1],b"WriteBool called with NO active message\n\0"):
        try: verify_vtable(table,marker)
        except ValueError: pass
        else: raise AssertionError("wrong vtable boundary marker accepted")
    wrapper=(b"\x68"+pack(NAME_VA)+b"\x68"+pack(FACTORY_VA)+
             b"\xB9"+pack(STATIC_RECORD_VA)+b"\xE8"+
             struct.pack("<i",REG_CTOR-REG_WRAPPER-20)+b"\xC3")
    assert hashlib.sha256(wrapper).hexdigest()==REG_WRAPPER_SHA
    assert decode_wrapper(wrapper)==(NAME_VA,FACTORY_VA,STATIC_RECORD_VA)
    for pos in range(len(wrapper)):
        other=bytearray(wrapper);other[pos]^=1
        try: decode_wrapper(bytes(other))
        except ValueError: pass
        else: raise AssertionError("modified original registration accepted")
    # Invented 4/64/5 limits for public tests; do not embed actual
    # original 33-byte commercial method into the repository.
    invented=invented_limits(4,64,5)
    assert len(invented)==33
    assert verify_player_limits(invented,4,64,5)==(4,64,5)
    for pos in range(len(invented)):
        other=bytearray(invented);other[pos]^=1
        try: verify_player_limits(bytes(other),4,64,5)
        except ValueError: pass
        else: raise AssertionError("changed original limits x86 ABI accepted")
    try: verify(b"fabricated not original owner-server")
    except ValueError: pass
    else: raise AssertionError("fake original server SHA accepted")
    print("PASS original-free 2006 ServerGameClients003: 15-slot static vtable, register wrapper, GetPlayerLimits 3-reference ret12 ABI, literal limits 1/32/1 and per-byte corruption rejection")


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    mode=parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--self-test",action="store_true")
    mode.add_argument("--server",type=Path,
                      help="owner-private unchanged original server.dll")
    args=parser.parse_args()
    if args.self_test:
        self_test()
    else:
        print(json.dumps(verify(args.server.read_bytes()),
                         sort_keys=True,indent=2))
    return 0


if __name__=="__main__":
    raise SystemExit(main())
