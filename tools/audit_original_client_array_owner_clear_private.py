#!/usr/bin/env python3
"""Private-only SHA-pinned retail PE32 auditor. Never publish binary bytes."""
import hashlib
import pathlib
import struct
import sys

EXPECTED_CLIENT = "6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d"
EXPECTED_SERVER = "568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650"
EXPECTED_BODY = "305edbceeb790e6a91f91fa5815f31c11f606f71e1831384350bb5c265186fe2"
VA = 0x24105FE0
SIZE = 39

def sha(data):
    return hashlib.sha256(data).hexdigest()

def verify_pe32(blob, digest):
    assert sha(blob) == digest, "complete retail DLL SHA256 mismatch"
    assert blob[:2] == b"MZ"
    pe = struct.unpack_from("<I", blob, 0x3C)[0]
    assert blob[pe:pe+4] == b"PE\\0\\0"
    assert struct.unpack_from("<H", blob, pe+4)[0] == 0x14C
    assert struct.unpack_from("<H", blob, pe+24)[0] == 0x10B

def main(client, server):
    c, s = pathlib.Path(client).read_bytes(), pathlib.Path(server).read_bytes()
    verify_pe32(c, EXPECTED_CLIENT)
    verify_pe32(s, EXPECTED_SERVER)
    # Original .text VMA 0x24001000, file offset 0x1000: RVA == file offset.
    body = c[VA - 0x24000000:VA - 0x24000000 + SIZE]
    assert len(body) == SIZE and sha(body) == EXPECTED_BODY
    assert body[-2:] == bytes.fromhex("5ec3")  # pop esi; ret
    assert c[VA - 0x24000000 + SIZE:VA - 0x24000000 + SIZE + 9] == b"\\xCC"*9
    call_offset = 0x24105FF0 - VA
    assert body[call_offset] == 0xE8
    rel = struct.unpack_from("<i", body, call_offset+1)[0]
    assert 0x24105FF5 + rel == 0x2416F940
    print("PASS: original PE32 full-file SHA256, 39-byte function, RET, INT3 and E8 target")

if __name__ == "__main__":
    if len(sys.argv) != 3:
        sys.exit("usage: auditor.py /private/client.dll /private/server.dll")
    main(*sys.argv[1:])
