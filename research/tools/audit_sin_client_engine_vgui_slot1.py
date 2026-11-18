#!/usr/bin/env python3
"""Read-only original 2006 SiN client VGUI post-acquisition ABI audit.

Does not output or commit original machine code. Requires the owner's
SHA-authenticated client.dll and never invokes any engine method.
"""
import argparse
import hashlib
import json
import pathlib
import struct

CLIENT_SHA256 = "6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d"
IMAGE_BASE = 0x24000000
# Entire bounded original function regions and the exact factory store
# (region offsets are VAs for the original ImageBase, not raw file offsets).
REGIONS = {
    "client_vgui_init": (0x241EDFB0, 123, "64cbfa079f093b1e577a59721396d73742eb4543b836e7fb0cc55d724783417d"),
    "engine_vgui_factory_store": (0x240FE208, 19, "3b51f9a38ad3f8f99a013cb1c0929ac8041ba09cf88900a64269ec76c3aab157"),
    "vgui_post_acquire_noop": (0x2422ECD0, 1, "ae3f4619b0413d70d3004b9131c3752153074e45725be13b9a148978895e359e"),
    "engine_vgui_slot1_arg2": (0x2422ECF0, 14, "d1f9d41fb9e6c8628fa934c2ac6a848a9aae39e604b44f7b4f1ddab5ceed29b3"),
    "one_time_converter_registration": (0x24009340, 101, "a0851e6f97bc087734723394de37dfb925db41c1764087f4c9e94ede1a1b384e"),
}
STRINGS = {
    0x24296A2C: "VEngineVGui001",
    0x242AC524: "CLIENT",
    0x242AC4F8: "CHudTextureHandle",
}

class OriginalPE:
    def __init__(self, data):
        self.data = data
        if len(data) < 0x100 or data[:2] != b"MZ":
            raise ValueError("Invalid PE DOS header")
        peoff = struct.unpack_from("<I", data, 0x3c)[0]
        if peoff + 24 > len(data) or data[peoff:peoff+4] != b"PE\0\0":
            raise ValueError("Invalid PE signature")
        nsects, optsize = struct.unpack_from("<HH", data, peoff+6)[0], struct.unpack_from("<H", data, peoff+20)[0]
        if optsize < 32 or struct.unpack_from("<H", data, peoff+24)[0] != 0x10b:
            raise ValueError("Not PE32")
        self.base = struct.unpack_from("<I", data, peoff+24+28)[0]
        self.sections=[]
        for idx in range(nsects):
            off=peoff+24+optsize+idx*40
            if off+40 > len(data): raise ValueError("Truncated PE section table")
            virtual_size,virtual_address,raw_size,raw_ptr=struct.unpack_from("<IIII",data,off+8)
            if raw_ptr+raw_size > len(data): raise ValueError("Invalid PE raw range")
            self.sections.append((virtual_address,raw_size,raw_ptr))

    def read(self, va, count):
        if count <= 0: raise ValueError("Invalid length")
        rva = va - self.base
        for begin,size,off in self.sections:
            if begin <= rva and count <= size and rva-begin <= size-count:
                return self.data[off+rva-begin:off+rva-begin+count]
        raise ValueError(f"VA not backed by a raw section: {va:#x}")

def verify_original(binary):
    data=pathlib.Path(binary).read_bytes()
    if hashlib.sha256(data).hexdigest()!=CLIENT_SHA256:
        raise ValueError("Original client full SHA256 mismatch; no report generated")
    pe=OriginalPE(data)
    if pe.base!=IMAGE_BASE:raise ValueError("Unexpected original ImageBase")
    for name,(va,size,expected) in REGIONS.items():
        if hashlib.sha256(pe.read(va,size)).hexdigest()!=expected:
            raise ValueError(f"Original {name} region mismatch")
    for va,text in STRINGS.items():
        if pe.read(va,len(text)+1)!=text.encode("ascii")+bytes(1):
            raise ValueError(f"Original string mismatch at {va:#x}")
    return {
        "evidence":"owner_original_2006_client_sha_verified",
        "client_sha256":CLIENT_SHA256,
        "checked_regions":list(REGIONS),
        "checked_strings":list(STRINGS.values()),
        "engine_vgui_interface":"VEngineVGui001",
        "original_client_global_va":"0x2433F6B0",
        "original_setup":{
            "vgui_module":"CLIENT",
            "interfaces":["VGUI_InputInternal001","VGUI_System010"],
            "one_time_converter_name":"CHudTextureHandle",
            "first_post_acquire_helper":"0x2422ECD0: returns without work",
            "last_post_acquire_helper":"0x2422ECF0: virtual slot 1, argument 2, returns 32-bit value",
            "init_succeeds_if_last_helper_nonzero":True,
        },
        "not_proven":["engine virtual method name","complete VGUI Init","GUI rendering","full client boot"],
    }

def main():
    ap=argparse.ArgumentParser()
    ap.add_argument("client",help="Original SHA-authenticated 2006 client.dll")
    args=ap.parse_args()
    print(json.dumps(verify_original(args.client),indent=2,sort_keys=True))

if __name__=="__main__":
    main()
