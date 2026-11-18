#!/usr/bin/env python3
"""Hash-gated owner-local inspection of the original VClient011 Init prefix."""
import argparse
import hashlib
import json
import re
import runpy
import struct
from pathlib import Path

PE32 = runpy.run_path(str(Path(__file__).with_name(
    "Inspect-OwnerOriginalServerPE.py")))["PE32"]
PIN = "6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d"
EXPECTED = (
    "VEngineClient012", "VEngineModel012", "VEngineEffects001",
    "VEngineCvar002", "EngineTraceClient003", "VEngineRenderView011",
    "VDebugOverlay003", "VMaterialSystem076", "VEngineCache001",
    "VModelInfoClient003", "VEngineVGui001", "VEngineClientStringTable001",
    "SpatialPartition001", "VEngineShadowMgr002", "StaticPropMgrClient004",
    "IEngineSoundClient003", "VFileSystem012", "VEngineRandom001",
    "VENGINE_GAMEUIFUNCS_VERSION004", "GAMEEVENTSMANAGER002",
    "VSoundEmitter001",
)


def query_prefix(code, begin, resolve_name, failure):
    """Verify bounded original call/name/near-failure-branch relationships."""
    pushes = list(re.finditer(rb"\x6a\x00\x68(.{4})", code, re.DOTALL))
    result = []
    for i, hit in enumerate(pushes):
        end = pushes[i+1].start() if i+1 < len(pushes) else len(code)
        tail = code[hit.end():end]
        call = tail.find(b"\xff\xd6")
        branches = list(re.finditer(rb"\x0f\x84(.{4})", tail, re.DOTALL))
        if call < 0 or len(branches) != 1 or branches[0].start() <= call:
            raise ValueError("Client query lacks expected factory/null branch")
        branch = branches[0]
        destination = begin + hit.end() + branch.end() + struct.unpack(
            "<i", branch.group(1))[0]
        if destination != failure:
            raise ValueError("Client query has changed failure destination")
        result.append(resolve_name(struct.unpack("<I", hit.group(1))[0]))
    return tuple(result)


def inspect(data):
    if hashlib.sha256(data).hexdigest() != PIN:
        raise ValueError("Original client.dll SHA256 mismatch")
    pe = PE32(data)
    if pe.base != 0x24000000:
        raise ValueError("Original client image base mismatch")
    reg = pe.raw(0x268300, 15)
    if reg[0] != 0x68 or reg[5] != 0x68 or reg[10] != 0xb9:
        raise ValueError("Client factory registration mismatch")
    if pe.string(pe.rva(struct.unpack_from("<I", reg, 1)[0])) != "VClient011":
        raise ValueError("Client interface name mismatch")
    callback = pe.rva(struct.unpack_from("<I", reg, 6)[0])
    if callback != 0xfd840:
        raise ValueError("Client singleton factory mismatch")
    creator = pe.raw(callback, 6)
    if creator[0] != 0xb8 or creator[5] != 0xc3:
        raise ValueError("Client singleton creator mismatch")
    obj = pe.rva(struct.unpack_from("<I", creator, 1)[0])
    if obj != 0x2fa4b8:
        raise ValueError("Client singleton location mismatch")
    vtable = pe.rva(pe.u32(obj))
    if vtable != 0x296c6c or pe.rva(pe.u32(vtable)) != 0xfe0e0:
        raise ValueError("Client vtable/Init slot mismatch")
    expected_slots = (
        0xfe0e0, 0xff3d0, 0xfdb40, 0xfdc00, 0xff4a0, 0xfd990,
        0xfd8b0, 0xfd8d0, 0xfd8f0, 0xfd970, 0xfd980, 0xfd9a0,
        0xfd9b0, 0xfd9c0, 0xfd9d0, 0xfd9e0, 0xfd9f0, 0xfda20,
        0xfda60, 0xfda30, 0xfdaa0, 0xfdab0, 0xfdac0, 0xfdad0,
        0xfdc20, 0xfdb20, 0xfdc30, 0xfdc40, 0xfdc60, 0xfdc70,
        0xfdc80, 0xff850, 0xfedf0, 0xfdcd0, 0xfdce0, 0xff570,
        0xff5c0, 0xfdcf0, 0xfefd0, 0xff000, 0xff040, 0xff080,
        0xff0c0, 0xfdd10, 0xfdd00, 0xfe600, 0xfe630, 0xfe670,
    )
    actual_slots = tuple(pe.rva(pe.u32(vtable+4*i)) for i in range(48))
    _, text_start, text_size, _, _ = next(s for s in pe.sections if s[0]==".text")
    if actual_slots != expected_slots or not all(text_start<=x<text_start+text_size for x in actual_slots):
        raise ValueError("Client original complete vtable mismatch")
    next_word = pe.u32(vtable+4*48)
    if pe.base+text_start<=next_word<pe.base+text_start+text_size:
        raise ValueError("Unexpected additional client vtable slot")
    begin = 0xfe109
    versions = query_prefix(pe.raw(begin, 0xfe31b-begin), begin,
                            lambda va: pe.string(pe.rva(va)), 0xfe59e)
    if versions != EXPECTED:
        raise ValueError("Original client 21-query sequence mismatch")
    # The next original step dispatches sound-emitter virtual slot zero,
    # passing the same app-system factory and checking its boolean result.
    if pe.raw(0xfe31b, 9) != bytes.fromhex("8b10568bc8ff1284c0"):
        raise ValueError("Original sound emitter Connect boundary mismatch")
    # Original post-Connect CommandLine import and material acquisition.
    # Validate the observed call offsets and argument names independently.
    if pe.raw(0xfe32a,6) != bytes.fromhex("ff1550622724") or \
       pe.raw(0xfe339,3) != bytes.fromhex("ff5228") or \
       pe.raw(0xfe34e,2) != bytes.fromhex("ffd6") or \
       pe.raw(0xfe367,3) != bytes.fromhex("ff5020"):
        raise ValueError("Original post-Connect client callback slots mismatch")
    strings = ((0xfe332,"-textmode"), (0xfe349,"VMaterialSystemStub001"),
               (0xfe362,"MaterialSystemHardwareConfig012"))
    for push,name in strings:
        if pe.raw(push,1)!=b"\x68" or pe.string(pe.rva(pe.u32(push+1)))!=name:
            raise ValueError("Original client post-Connect argument mismatch")
    branch = pe.raw(0xfe371,6)
    if branch[:2]!=b"\x0f\x84" or 0xfe377+struct.unpack_from("<i",branch,2)[0]!=0xfe59e:
        raise ValueError("Original required hardware-config failure edge mismatch")
    # GetScreenSize calls VEngineClient slot 5 with height then
    # width pushes; Gaussian attach follows the acquired random stream.
    call = pe.raw(0xfe377,5)
    if call[0]!=0xe8 or 0xfe37c+struct.unpack_from("<i",call,1)[0]!=0xffab0:
        raise ValueError("Original client screen helper edge mismatch")
    if pe.raw(0xffab0,22)!=bytes.fromhex(
            "8b0d84f633248b016814fc33246810fc3324ff5014c3"):
        raise ValueError("Original client screen callback ABI mismatch")
    if pe.raw(0xfe37c,18)!=bytes.fromhex(
            "8b0dccf6332451b900f73324ff1544622724"):
        raise ValueError("Original client Gaussian attach mismatch")
    if pe.raw(0x2681f0,14)!=bytes.fromhex(
            "6a00b900f73324ff1540622724c3"):
        raise ValueError("Original Gaussian static initializer mismatch")
    # Initializer lifecycle follows console registration. The original
    # linked-list constructor exists, but no direct caller or absolute
    # function address was found in the mapped raw sections. Indirect
    # runtime construction is not ruled out by this metadata.
    def expect_call(source,target):
        raw=pe.raw(source,5)
        if raw[0]!=0xe8 or source+5+struct.unpack_from("<i",raw,1)[0]!=target:
            raise ValueError("Original client initializer edge mismatch")
    expect_call(0xfe39b,0x164120)
    expect_call(0x16414d,0x1640f0)
    expect_call(0xff422,0x1640f0)
    if pe.raw(0x164130,3)!=bytes.fromhex("ff5604") or \
       pe.raw(0x164109,3)!=bytes.fromhex("ff5608"):
        raise ValueError("Original initializer callback field mismatch")
    _,data_start,data_size,data_raw,_=next(s for s in pe.sections if s[0]==".data")
    head=0x369374
    if not (data_start+data_raw<=head and head+4<=data_start+data_size):
        raise ValueError("Original initializer list head mapping mismatch")
    code=pe.raw(text_start,text_size)
    direct=[]
    for i in range(len(code)-4):
        if code[i] in (0xe8,0xe9) and text_start+i+5+struct.unpack_from("<i",code,i+1)[0]==0x1640c0:
            direct.append(text_start+i)
    abs_refs=0
    for n,rva,vs,raw,off in pe.sections:
        if n in (".text",".rdata",".data"):
            abs_refs+=data[off:off+min(vs,raw)].count(struct.pack("<I",pe.base+0x1640c0))
    if direct or abs_refs:
        raise ValueError("Unexpected direct client initializer constructor references")
    return {
        "original_client_sha256": PIN,
        "object_initializer": {"constructor_rva":hex(0x1640c0),
                               "initialize_rva":hex(0x164120),
                               "free_rva":hex(0x1640f0),
                               "head_rva":hex(head),"record_size":16,
                               "direct_constructor_references":len(direct),
                               "absolute_constructor_references":abs_refs},
        "screen_random": {"screen_helper_rva":hex(0xffab0),
                          "engine_slot":5,
                          "width_global_rva":hex(0x33fc10),
                          "height_global_rva":hex(0x33fc14),
                          "gaussian_constructor_iat_rva":hex(0x276240),
                          "gaussian_attach_iat_rva":hex(0x276244),
                          "gaussian_storage_rva":hex(0x33f700)},
        "post_connect": {"commandline_import_rva":hex(0x276250),
                         "find_parm_slot":10, "flag":"-textmode",
                         "optional_factory":"VMaterialSystemStub001",
                         "hardware_config_slot":8,
                         "required_hardware_config":"MaterialSystemHardwareConfig012"},
        "interface": "VClient011",
        "init_rva": hex(0xfe0e0),
        "vtable_slot_count": len(actual_slots),
        "vtable_slots_rva": [hex(x) for x in actual_slots],
        "initial_required_app_system_queries": versions,
        "next_boundary": "VSoundEmitter001::Connect",
        "status": "original metadata only; client startup incomplete",
    }


def self_test():
    start, failure = 0x1000, 0x1100
    first = b"\x6a\x00\x68" + struct.pack("<I", 123) + b"\xff\xd6\x85\xc0\x0f\x84"
    code = first + struct.pack("<i", failure-(start+len(first)+4))
    assert query_prefix(code,start,lambda va: str(va),failure) == ("123",)
    try:
        query_prefix(code,start,lambda va: str(va),failure+1)
    except ValueError:
        pass
    else:
        raise AssertionError("Accepted changed failure destination")
    try:
        inspect(b"not the original")
    except ValueError as exc:
        assert "SHA256 mismatch" in str(exc)
    else:
        raise AssertionError("Accepted invented original")
    print("PASS synthetic client query-prefix and hash guard")


if __name__ == "__main__":
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("original_client", nargs="?", type=Path)
    parser.add_argument("--self-test", action="store_true")
    args=parser.parse_args()
    if args.self_test:
        self_test()
    elif args.original_client:
        print(json.dumps(inspect(args.original_client.read_bytes()), indent=2))
    else:
        parser.error("Supply the owner-original DLL or --self-test")
