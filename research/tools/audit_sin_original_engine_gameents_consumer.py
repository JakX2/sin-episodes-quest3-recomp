#!/usr/bin/env python3
"""Read-only, SHA-pinned original 2006 engine.dll GameEnts consumer audit.

Use --engine with an owner-private DLL. Public --self-test exercises only
fabricated PE and x86 witness bytes. Never uploads, executes, or prints code.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path

ENGINE_SHA = "f3add5b41300e6d709fd0b492bfcdd37b9b3d20d9744f874c0b860bf021cec81"
IMAGE_BASE = 0x20000000
GAME_DLL_STRING = 0x202c39a0
GAME_ENTS_STRING = 0x202c3954
GAME_DLL_GLOBAL = 0x203e4484
GAME_ENTS_GLOBAL = 0x203e4488

# (name, original VA, bounded-window size, full-window SHA-256,
#  requested version VA, stored interface global VA, required result check)
FACTORIES = (
    ("ServerGameDLL003", 0x2015e04d, 18,
     "7b608c9f71cb7bb18ab8144b9241f3ab3d34fa5cbfe05627dd4a9aeb41de5195",
     GAME_DLL_STRING, GAME_DLL_GLOBAL, False),
    ("ServerGameEnts001", 0x2015e07e, 46,
     "5a649293c0133956f451cdcd9d7cf160b7e154afba5580a346b2313dade99fab",
     GAME_ENTS_STRING, GAME_ENTS_GLOBAL, True),
)
# (role, original VA, window size, full-window SHA-256, vtable byte offset)
CALLS = (
    ("SetDebugEdictBase", 0x20150f73, 23,
     "927281ad5fb1e6d1ce66d8ef1d00eb575576054f01d777d204abf093ceb5fca2", 0x04),
    ("MarkEntitiesAsTouching_A", 0x20172510, 22,
     "4a50273d618e1c88be4c7c8c8c7ccf5e5e86657d30abcf98e99735b6277d6ea7", 0x08),
    ("MarkEntitiesAsTouching_B", 0x20172780, 22,
     "2c1b37de9b05bfa29d54838933bbaeac5ccbca634d3c6fd94898df29719cf9be", 0x08),
    ("FreeContainingEntity", 0x20110e50, 15,
     "a6796e889d651bf7ee8d3b1a4082bf6901a100aadb72b1b5350a195557e07569", 0x0c),
    ("BaseEntityToEdict", 0x20092a70, 15,
     "8e656233b788a6c4fdcde41f01b2058f6cd250a6306cb9202f64fefd8dfaad2a", 0x10),
    ("EdictToBaseEntity", 0x20092a50, 13,
     "970826d33349d9311cb1c1cd4ea9d0703bd3734a6c06f6ede30f9fd427dbe6a5", 0x14),
    ("CheckTransmit", 0x20153186, 17,
     "81962b5cf3718ba56e83f6ff0f79b4466967a7759bc8d51491ab9fa5c88a19d6", 0x18),
)

def u32(value):
    return struct.pack("<I", value)

class PE32:
    def __init__(self, data):
        if len(data) < 0x100 or data[:2] != b"MZ":
            raise ValueError("not a DOS MZ image")
        header = struct.unpack_from("<I", data, 0x3c)[0]
        if header+24 > len(data) or data[header:header+4] != b"PE\0\0":
            raise ValueError("invalid NT header")
        machine, count = struct.unpack_from("<HH", data, header+4)
        opt_size = struct.unpack_from("<H", data, header+20)[0]
        opt = header+24
        if machine != 0x14c or not 1 <= count <= 96 or opt_size < 0x60:
            raise ValueError("not a supported i386 PE")
        if opt+opt_size+count*40 > len(data):
            raise ValueError("truncated section table")
        if struct.unpack_from("<H",data,opt)[0] != 0x10b:
            raise ValueError("not PE32")
        self.base = struct.unpack_from("<I",data,opt+28)[0]
        self.data = data
        self.sections = []
        for i in range(count):
            at = opt+opt_size+i*40
            name = data[at:at+8].rstrip(b"\0").decode("ascii")
            virt_size, rva, raw_size, file_off = struct.unpack_from("<IIII",data,at+8)
            if file_off+raw_size > len(data):
                raise ValueError("truncated PE section payload")
            self.sections.append((name,rva,virt_size,raw_size,file_off))

    def raw(self, va, length):
        if length <= 0 or va < self.base:
            raise ValueError("invalid mapped VA")
        rva = va-self.base
        for name,start,virt_size,raw_size,off in self.sections:
            if start <= rva and rva+length <= start+raw_size:
                return self.data[off+rva-start:off+rva-start+length]
        raise ValueError("VA has no file-backed bytes")

    def contains_data_global(self, va):
        rva = va-self.base
        return any(name == ".data" and start <= rva < start+virt_size
                   for name,start,virt_size,raw_size,off in self.sections)

def check_sha(blob, expected, strict=True):
    if strict and hashlib.sha256(blob).hexdigest() != expected:
        raise ValueError("original instruction-window SHA mismatch")

def check_factory(blob, version_va, global_va, required):
    # Standard x86 factory: push nullptr, push version, call pointer,
    # adjust stack, store eax. No complete original method bytes are copied.
    if len(blob)<18 or blob[:3] != bytes((0x6a,0,0x68)):
        raise ValueError("factory argument prelude differs")
    if blob[3:7] != u32(version_va) or blob[7:10] != b"\xff\x55\xfc":
        raise ValueError("wrong factory name or dynamic call")
    if blob[10:14] != b"\x83\xc4\x08\xa3" or blob[14:18] != u32(global_va):
        raise ValueError("wrong factory global store")
    if required and b"\x83\x3d"+u32(global_va)+b"\x00" not in blob[18:]:
        raise ValueError("missing GameEnts null-result check")

def check_call(blob, global_va, slot):
    # Load original owner global; then load vptr; call/jmp using slot offset.
    at = blob.find(b"\x8b\x0d"+u32(global_va))
    if at < 0:
        raise ValueError("GameEnts global not read")
    positions = [blob.find(op,at+6) for op in (b"\x8b\x01",b"\x8b\x11")]
    good = [pos for pos in positions if pos >= 0]
    if not good:
        raise ValueError("vtable read missing")
    vptr_at = min(good)
    if not any(blob.find(bytes((0xff, op, slot)),vptr_at+2) >= 0
               for op in (0x50,0x52,0x60)):
        raise ValueError("original operational virtual slot missing")

def audit(data):
    if hashlib.sha256(data).hexdigest() != ENGINE_SHA:
        raise ValueError("wrong original 2006 engine.dll SHA-256")
    pe = PE32(data)
    if pe.base != IMAGE_BASE or not pe.contains_data_global(GAME_ENTS_GLOBAL):
        raise ValueError("wrong engine image base or interface storage")
    for va, name in ((GAME_DLL_STRING,b"ServerGameDLL003\0"),
                     (GAME_ENTS_STRING,b"ServerGameEnts001\0")):
        if pe.raw(va,len(name)) != name:
            raise ValueError("factory version string mismatch")
    for name,va,size,sha,version_va,global_va,required in FACTORIES:
        blob = pe.raw(va,size)
        check_sha(blob,sha)
        check_factory(blob,version_va,global_va,required)
    slots = set()
    for name,va,size,sha,slot in CALLS:
        blob = pe.raw(va,size)
        check_sha(blob,sha)
        check_call(blob,GAME_ENTS_GLOBAL,slot)
        slots.add(slot)
    if slots != {4,8,12,16,20,24}:
        raise ValueError("not all six operational GameEnts slots found")
    return {
        "original_engine_sha_verified": True,
        "original_engine_image_base": hex(IMAGE_BASE),
        "engine_ServerGameEnts001_result_global": hex(GAME_ENTS_GLOBAL),
        "engine_factory_order_GameDLL_then_GameEnts": True,
        "engine_missing_GameEnts_returns_false_from_this_routine": True,
        "engine_vtable_calls_authenticated": len(CALLS),
        "engine_slot_offsets": [hex(x) for x in sorted(slots)],
        "engine_live_ownership_verified": False,
        "retail_windows_startup_verified": False,
        "original_bytes_exported": False,
    }

def self_test():
    version, glob = 0x11223344,0x55667788
    factory = (b"\x6a\x00\x68"+u32(version)+b"\xff\x55\xfc"
               +b"\x83\xc4\x08\xa3"+u32(glob)
               +b"\x83\x3d"+u32(glob)+b"\x00")
    check_factory(factory,version,glob,True)
    for wrong in (factory[:-1],
                  factory.replace(u32(version),u32(version+1)),
                  factory.replace(u32(glob),u32(glob+1),1)):
        try:
            check_factory(wrong,version,glob,True)
        except ValueError:
            pass
        else:
            raise AssertionError("altered factory was accepted")
    for slot in (4,8,12,16,20,24):
        sample = (b"\x90\x90\x8b\x0d"+u32(glob)
                  +b"\x8b\x11\x50\xff\x52"+bytes((slot,)))
        check_call(sample,glob,slot)
        for wrong in (sample.replace(u32(glob),u32(glob+1)),
                      sample[:-1]+bytes((slot+4,)),
                      sample.replace(b"\x8b\x11",b"\x90\x90")):
            try:
                check_call(wrong,glob,slot)
            except ValueError:
                pass
            else:
                raise AssertionError("altered vtable call accepted")
    try:
        check_sha(b"SYNTHETIC","0"*64)
    except ValueError:
        pass
    else:
        raise AssertionError("corrupt SHA fingerprint accepted")
    fake = bytearray(0x600)
    fake[:2] = b"MZ"
    struct.pack_into("<I",fake,0x3c,0x80)
    fake[0x80:0x84] = b"PE\0\0"
    struct.pack_into("<HH",fake,0x84,0x14c,2)
    struct.pack_into("<H",fake,0x94,0xe0)
    struct.pack_into("<H",fake,0x98,0x10b)
    struct.pack_into("<I",fake,0x98+28,IMAGE_BASE)
    for i,(name,rva,off) in enumerate(((b".text",0x1000,0x200),
                                       (b".rdata",0x2000,0x400))):
        at = 0x178+i*40
        fake[at:at+len(name)] = name
        struct.pack_into("<IIII",fake,at+8,0x200,rva,0x200,off)
    fake[0x200:0x204] = b"TEST"
    pe = PE32(fake)
    assert pe.raw(IMAGE_BASE+0x1000,4) == b"TEST"
    for wrong in (bytes(fake[:-1]),bytes(b"\0"*len(fake))):
        try:
            PE32(wrong)
        except ValueError:
            pass
        else:
            raise AssertionError("malformed PE32 accepted")
    try:
        pe.raw(IMAGE_BASE+0x1000,0x201)
    except ValueError:
        pass
    else:
        raise AssertionError("out-of-range PE bytes accepted")
    print("PASS original-free 2006 engine GameEnts consumer/factory/slot audit")

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("--self-test",action="store_true")
    group.add_argument("--engine",type=Path)
    args = parser.parse_args()
    if args.self_test:
        self_test()
    else:
        print(json.dumps(audit(args.engine.read_bytes()),indent=2,sort_keys=True))

if __name__ == "__main__":
    main()
