#!/usr/bin/env python3
"""Read-only SHA-pinned metadata audit for original CClientShadowMgr::Init.

Private owner use: --archive path/to/bin.zip or --client path/to/client.dll.
Public original-free CI: --self-test. Never emit original executable bytes.
"""
import argparse
import hashlib
import json
import sys
import zipfile
from pathlib import Path
from audit_sin_original_bf_read_candidate_layout import (
    EXPECTED_SHA, parse_image, region_at_va,
)

VA = 0x24118880
LENGTH = 0x92
METHOD_SHA = "32dbbce9d4da9dce01522425c0531510640a5841a662c01abcec443df3d0d438"
# Single selected instruction signatures on the entirely SHA-matched
# 2006 original method. These are ABI/side-effect metadata guards, NOT
# a commercial source-code or disassembly release.
MARKERS = (
    ("reset_cd", 19, bytes.fromhex("c686cd00000000")),
    ("construct_member14", 26, bytes.fromhex("e871ef0400")),
    ("virtual_direction_60", 64, bytes.fromhex("ff5060")),
    ("virtual_length_6c", 76, bytes.fromhex("ff526c")),
    ("virtual_bias_70", 88, bytes.fromhex("ff5070")),
    ("set_mode_d8", 91, bytes.fromhex("c786d800000008000000")),
    ("external_flag_branch", 107, bytes.fromhex("83792c00")),
    ("conditional_helper", 115, bytes.fromhex("e818fdffff")),
    ("engine_callback_1d0", 133, bytes.fromhex("ff92d0010000")),
    ("return_true", 139, bytes.fromhex("b0015e83c40cc3")),
)


def verify_markers(method):
    if len(method) != LENGTH:
        raise ValueError("shadow method length mismatch")
    for name, pos, opcode in MARKERS:
        if method[pos:pos+len(opcode)] != opcode:
            raise ValueError("shadow init missing ordered field/call: " + name)
    # Method's relative conditional helper target is authenticated
    # through the SHA and known VA. No original function is invoked.
    import struct
    displacement = struct.unpack_from("<i", method, 116)[0]
    if 0x241188F8 + displacement != 0x24118610:
        raise ValueError("conditional original shadow helper target differs")
    return True


def audit(data):
    if hashlib.sha256(data).hexdigest() != EXPECTED_SHA:
        raise ValueError("expected owner original client.dll SHA256")
    image = parse_image(data)
    if image[0] != 0x24000000:
        raise ValueError("wrong original 2006 PE32 image base")
    method = region_at_va(data, image, VA, LENGTH)
    if hashlib.sha256(method).hexdigest() != METHOD_SHA:
        raise ValueError("original ShadowMgr Init body SHA256 mismatch")
    verify_markers(method)
    return {
        "private_original_client_sha256_verified": True,
        "private_original_shadow_init_sha256_verified": True,
        "original_top_level_call_sites_guarded": len(MARKERS),
        "known_state_writes": ["+0xcd <- 0", "+0xd8 <- 8"],
        "external_engine_helpers_still_unimplemented": True,
        "retail_game_started": False,
    }


def self_test():
    fake = bytearray(LENGTH)
    for name, pos, code in MARKERS:
        fake[pos:pos+len(code)] = code
    # The synthetic relative-call marker is exactly the same small
    # instruction encoding as the authenticated reference. No original
    # method body or game asset is included.
    assert verify_markers(fake)
    altered = bytearray(fake)
    altered[64] ^= 1
    try:
        verify_markers(altered)
    except ValueError:
        pass
    else:
        raise ValueError("missing guard should reject synthetic mutation")
    altered = bytearray(fake)
    altered[116] ^= 1
    try:
        verify_markers(altered)
    except ValueError:
        pass
    else:
        raise ValueError("incorrect conditional helper target accepted")
    return {"synthetic_shadow_init_marker_audit": "PASS",
            "original_binary_used": False}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-test", action="store_true")
    group = parser.add_mutually_exclusive_group()
    group.add_argument("--archive", type=Path)
    group.add_argument("--client", type=Path)
    args = parser.parse_args()
    if args.self_test:
        report = self_test()
    elif args.client:
        report = audit(args.client.read_bytes())
    elif args.archive:
        with zipfile.ZipFile(args.archive) as z:
            report = audit(z.read("client.dll"))
    else:
        parser.error("choose --self-test, --archive or --client")
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    try:
        main()
    except (FileNotFoundError, ValueError, AssertionError,
            KeyError, zipfile.BadZipFile) as exc:
        print("FAILED original ShadowMgr Init audit: " + str(exc),
              file=sys.stderr)
        sys.exit(1)
