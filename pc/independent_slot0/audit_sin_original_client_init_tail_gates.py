#!/usr/bin/env python3
"""Read-only original SiN client VClient011::Init tail gate auditor.

Never logs or publishes original executable bytes or instruction bodies.
Public CI runs --self-test with a fabricated method region only.
"""
import argparse
import hashlib
import json
import struct
import sys
import zipfile
from pathlib import Path
from audit_sin_original_bf_read_candidate_layout import (
    EXPECTED_SHA, parse_image, region_at_va,
)

BASE = 0x240FE486
END = 0x240FE5A4
REGION_LEN = END - BASE
REGION_SHA = "e302338aece1719a43720fa454e80cf1866d925e723d50d403af3f0d5457bfe9"
ORIGINAL_FAILURE = 0x240FE59E
ORIGINAL_SUCCESS = 0x240FE595

# Original E8 rel32 call edges in the exact order of this bounded tail.
DIRECT = (
    (0x240FE4A3, 0x24144D30),
    (0x240FE4B3, 0x24156860),  # first explicit bool gate
    (0x240FE4EE, 0x24053A90),
    (0x240FE4FD, 0x241EE030),
    (0x240FE502, 0x24085330),
    (0x240FE507, 0x240F85D0),
    (0x240FE50C, 0x241E5330),
    (0x240FE512, 0x241CD170),
    (0x240FE529, 0x241E5C00),
    (0x240FE540, 0x24182C60),  # second explicit bool gate
    (0x240FE554, 0x241ACF50),
    (0x240FE56B, 0x2418D1C0),
    (0x240FE581, 0x241DAB30),
    (0x240FE590, 0x240FCDD0),
)
# Original FF /r direct virtual calls, inspected byte-for-byte.
VIRTUAL = (
    (0x240FE491, "ff12"),
    (0x240FE49B, "ff5004"),
    (0x240FE4B0, "ff5208"),
    (0x240FE4C8, "ff5014"),
    (0x240FE4E2, "ff12"),
    (0x240FE4EC, "ff10"),
    (0x240FE4FB, "ff12"),
    (0x240FE524, "ff5004"),
    (0x240FE538, "ff5204"),
    (0x240FE560, "ff5624"),
    (0x240FE577, "ff5624"),
    (0x240FE58D, "ff5624"),
)
# Both original branch destinations lead to the *same* return-zero block.
GATE_BYTES = (
    (0x240FE4B8, "84c00f84de000000"),
    (0x240FE548, "84c07452"),
)
RETURN_BYTES = (
    (0x240FE595, "b8010000005ec20c00"),
    (0x240FE59E, "33c05ec20c00"),
)


def at(region, va, count):
    offset = va-BASE
    if offset < 0 or offset+count > len(region):
        raise ValueError("out of original tail region")
    return region[offset:offset+count]


def check_tail(region, check_sha=True):
    if len(region) != REGION_LEN:
        raise ValueError("original VClient011 Init tail length mismatch")
    if check_sha and hashlib.sha256(region).hexdigest() != REGION_SHA:
        raise ValueError("authenticated original tail SHA256 mismatch")
    for va, target in DIRECT:
        expected = b"\xe8" + struct.pack("<i", target - va - 5)
        if at(region, va, 5) != expected:
            raise ValueError("original direct CALL target mismatch at %#x" % va)
    for va, opcode in VIRTUAL:
        code = bytes.fromhex(opcode)
        if at(region, va, len(code)) != code:
            raise ValueError("original vtable call mismatch at %#x" % va)
    for va, opcode in GATE_BYTES + RETURN_BYTES:
        code = bytes.fromhex(opcode)
        if at(region, va, len(code)) != code:
            raise ValueError("original boolean gate or return mismatch at %#x" % va)
    # First gate: relative conditional jump end 0x240FE4C0.
    if 0x240FE4C0 + struct.unpack_from("<i", region, 0x240FE4BC - BASE)[0] \
            != ORIGINAL_FAILURE:
        raise ValueError("first gate not aimed at shared failure return")
    # Second gate: short JZ at 0x240FE54A with next instruction 0xFE54C.
    if 0x240FE54C + struct.unpack_from("<b", region, 0x240FE54B - BASE)[0] \
            != ORIGINAL_FAILURE:
        raise ValueError("second gate not aimed at shared failure return")
    return {
        "original_tail_full_sha_verified": bool(check_sha),
        "original_tail_bytes": len(region),
        "original_direct_calls_verified": len(DIRECT),
        "original_vtable_calls_verified": len(VIRTUAL),
        "explicit_bool_failure_gates": 2,
        "shared_failure_va": "%#x" % ORIGINAL_FAILURE,
        "original_success_return_va": "%#x" % ORIGINAL_SUCCESS,
        "real_replacement_boot": False,
    }


def synthetic_test():
    region = bytearray(b"\xcc" * REGION_LEN)
    def put(va, code):
        offset = va-BASE
        region[offset:offset+len(code)] = code
    for va, target in DIRECT:
        put(va, b"\xe8" + struct.pack("<i", target-va-5))
    for va, op in VIRTUAL + GATE_BYTES + RETURN_BYTES:
        put(va, bytes.fromhex(op))
    result = check_tail(region, check_sha=False)
    assert result["original_direct_calls_verified"] == 14
    assert result["original_vtable_calls_verified"] == 12
    assert result["explicit_bool_failure_gates"] == 2
    for location in (0x240FE4B3, 0x240FE540, 0x240FE4BA, 0x240FE54A):
        damaged = bytearray(region)
        damaged[location-BASE] ^= 1
        try:
            check_tail(damaged, check_sha=False)
        except ValueError:
            pass
        else:
            raise ValueError("failed to refuse mutated gate at %#x" % location)
    return {"synthetic_original_init_tail_gate_oracle": "PASS",
            "original_bytes_used": False}


def inspect(data):
    if hashlib.sha256(data).hexdigest() != EXPECTED_SHA:
        raise ValueError("original game client.dll whole SHA256 mismatch")
    pe = parse_image(data)
    if pe[0] != 0x24000000:
        raise ValueError("wrong original image base")
    return check_tail(region_at_va(data, pe, BASE, REGION_LEN), check_sha=True)


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--self-test", action="store_true")
    group = p.add_mutually_exclusive_group()
    group.add_argument("--client", type=Path)
    group.add_argument("--archive", type=Path)
    args = p.parse_args()
    if args.self_test:
        report = synthetic_test()
    elif args.client:
        report = inspect(args.client.read_bytes())
    elif args.archive:
        with zipfile.ZipFile(args.archive) as z:
            report = inspect(z.read("client.dll"))
    else:
        p.error("require --self-test, --client or --archive")
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    try:
        main()
    except (FileNotFoundError, ValueError, zipfile.BadZipFile,
            KeyError, AssertionError) as exc:
        print("FAIL original VClient011::Init tail SHA-gate audit: "+str(exc),
              file=sys.stderr)
        sys.exit(1)
