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
    return {
        "original_client_sha256": PIN,
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
