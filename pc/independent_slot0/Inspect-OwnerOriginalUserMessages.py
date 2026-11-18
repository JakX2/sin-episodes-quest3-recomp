#!/usr/bin/env python3
"""Read-only authentication of shared original SiN user-message metadata."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import runpy
import struct

PE32 = runpy.run_path(str(Path(__file__).with_name("Inspect-OwnerOriginalServerPE.py")))["PE32"]
PINS = {
    "server": ("568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650",
               0x22000000, 0x2cfc20, 0x2cfede, 0x229a0, 0x224db198),
    "client": ("6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d",
               0x24000000, 0x20f670, 0x20f92e, 0x1d0280, 0x243010a8),
}
PATTERN = rb"(?:\x8b\x0d(.{4})\x6a(.)|\x6a(.)\x8b\x0d(.{4}))\x68(.{4})\xe8(.{4})"


def parse_registration(code, begin, target, registry, resolve):
    if len(code) % 18:
        raise ValueError("Incomplete user-message registration record")
    result = []
    for offset in range(0, len(code), 18):
        hit = re.fullmatch(PATTERN, code[offset:offset+18], re.DOTALL)
        if not hit:
            raise ValueError("Unexpected registration record")
        global_ptr = struct.unpack("<I", hit[1] or hit[4])[0]
        if global_ptr != registry:
            raise ValueError("Unexpected registration receiver")
        if begin+offset+18+struct.unpack("<i", hit[6])[0] != target:
            raise ValueError("Unexpected registration helper")
        size = struct.unpack("b", hit[2] or hit[3])[0]
        name = resolve(struct.unpack("<I", hit[5])[0])
        if not name or size < -1 or any(x["name"] == name for x in result):
            raise ValueError("Invalid or duplicate message metadata")
        result.append({"id": len(result), "name": name, "bytes": size})
    return result


def inspect(path, role):
    pin, base, start, end, target, registry = PINS[role]
    data = path.read_bytes()
    if hashlib.sha256(data).hexdigest() != pin:
        raise ValueError(role+" original SHA256 mismatch")
    pe = PE32(data)
    if pe.base != base or pe.raw(end, 1) != b"\xc3":
        raise ValueError("Registration boundary mismatch")
    result = parse_registration(pe.raw(start, end-start), start, target, registry,
                                lambda va: pe.string(pe.rva(va)))
    if len(result) != 39:
        raise ValueError("Original message count mismatch")
    return result


def expected_catalog():
    text = Path(__file__).with_name("sin_user_message_catalog.h").read_text()
    rows = re.findall(r'\{"([^"]+)",(-?\d+)\}, // (\d+)', text)
    return [{"id": int(i), "name": n, "bytes": int(s)} for n,s,i in rows]


def self_test():
    start, helper, receiver = 0x1000, 0x1200, 0x12345678
    def record(at, size, name, reverse):
        load = b"\x8b\x0d"+struct.pack("<I", receiver)
        push = b"\x6a"+struct.pack("b", size)
        return ((push+load if reverse else load+push)+b"\x68"+struct.pack("<I",name)
                +b"\xe8"+struct.pack("<i", helper-at-18))
    first = record(start, 1, 1, False)
    second = record(start+18, -1, 2, True)
    rows = parse_registration(first+second,start,helper,receiver,lambda p:str(p))
    assert rows == [{"id":0,"name":"1","bytes":1},{"id":1,"name":"2","bytes":-1}]
    for bad in (first+second[:-1], first[:-1]+b"\x00", first+record(start+18,1,1,False)):
        try:
            parse_registration(bad,start,helper,receiver,lambda p:str(p))
        except ValueError:
            pass
        else:
            raise AssertionError("Accepted invalid registration stream")
    expected=expected_catalog()
    assert len(expected)==39 and [x["id"] for x in expected]==list(range(39))
    assert expected[0]["name"]=="Train" and expected[-1]["name"]=="PoliceRadioMessage"
    print("PASS synthetic original user-message metadata parser")


if __name__ == "__main__":
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--server",type=Path)
    p.add_argument("--client",type=Path)
    p.add_argument("--self-test",action="store_true")
    args=p.parse_args()
    if args.self_test:
        self_test()
    elif args.server and args.client:
        server,client=inspect(args.server,"server"),inspect(args.client,"client")
        if server!=client or server!=expected_catalog():
            raise ValueError("Original client/server or independent catalog differ")
        print(json.dumps({"matched_originals":True,"messages":server},indent=2))
    else:
        p.error("Supply both owner-private original DLL paths or --self-test")
