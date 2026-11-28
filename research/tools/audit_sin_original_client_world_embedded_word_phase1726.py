#!/usr/bin/env python3
"""Phase1726: SHA-authenticate original CWorld ctor and portable 9B leaf.

Input must be the unchanged user-owned original 2006 PE32 client.dll.
Source constructors and other complex calls remain non-executable metadata.
No original DLL bytes are checked into the repository.
"""
from __future__ import annotations

import argparse
import hashlib
from pathlib import Path
import struct
import sys

CLIENT_SHA = "6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d"
CTOR_VA = 0x240CCE30
CTOR_LEN = 573
CTOR_SHA = "63fc9f1bbd6c8fe318f7faa1cd4aef448dd19553a6dbdc0d182b768855b34eca"
LEAF_VA = 0x24195260
LEAF_LEN = 9
LEAF_SHA = "cda72abcdbb1efdc80edc8549a889c40e93418986336d31914aa9a2d9e967cc7"
OWNER_BYTES = 0x4B0
WORD_OFFSET = 0xA4
# Offset in the complete 573-byte original constructor and actual E8 target.
CALLS = ((86, LEAF_VA), (174, 0x2411AEB0), (231, 0x24061540),
         (300, 0x240CB2F0), (398, 0x24061220),
         (436, 0x24061220), (558, 0x240C9F40))


def need(condition: bool, why: str) -> None:
    if not condition:
        raise ValueError(why)


def executable_text(original: bytes) -> tuple[int, bytes]:
    need(original[:2] == b"MZ" and len(original) >= 0x100,
         "missing PE DOS header")
    pe = struct.unpack_from("<I", original, 0x3C)[0]
    need(pe + 24 <= len(original) and original[pe:pe+4] == b"PE\0\0",
         "missing PE header")
    machine, count = struct.unpack_from("<HH", original, pe+4)
    optsize = struct.unpack_from("<H", original, pe+20)[0]
    need(machine == 0x14C and pe+24+optsize+count*40 <= len(original),
         "unsupported original PE section table")
    optional = pe + 24
    need(struct.unpack_from("<H", original, optional)[0] == 0x10B,
         "not original 32-bit PE")
    imagebase = struct.unpack_from("<I", original, optional+28)[0]
    for index in range(count):
        start = pe+24+optsize+index*40
        entry = original[start:start+40]
        if entry[:8].rstrip(b"\0") != b".text":
            continue
        virtual_size, section_rva, raw_size, file_offset = struct.unpack_from(
            "<IIII", entry, 8)
        need(file_offset+raw_size <= len(original) and raw_size >= virtual_size,
             "original .text extends beyond file")
        return imagebase+section_rva, original[file_offset:file_offset+raw_size]
    raise ValueError("original .text not found")


def sha_ok(block: bytes, length: int, sha: str) -> bool:
    return len(block) == length and hashlib.sha256(block).hexdigest() == sha


def read(text_base: int, code: bytes, va: int, count: int) -> bytes:
    off = va-text_base
    need(count>=0 and 0<=off<=len(code)-count,
         f"source VA outside executable section: {va:#x}")
    return code[off:off+count]


def verify_original(original: bytes) -> None:
    need(hashlib.sha256(original).hexdigest() == CLIENT_SHA,
         "original client.dll SHA-256 mismatch")
    base, code = executable_text(original)
    ctor = read(base,code,CTOR_VA,CTOR_LEN)
    leaf = read(base,code,LEAF_VA,LEAF_LEN)
    need(sha_ok(ctor,CTOR_LEN,CTOR_SHA),"whole CWorld constructor SHA drift")
    need(ctor[-1:]==b"\xC3" and read(base,code,CTOR_VA+CTOR_LEN,3)==b"\xCC"*3 and
         read(base,code,CTOR_VA+CTOR_LEN+3,1)==b"\x56",
         "CWorld 573B constructor/three-byte padding boundary drift")
    need(sha_ok(leaf,LEAF_LEN,LEAF_SHA),"whole embedded-word helper SHA drift")
    need(leaf==bytes.fromhex("8bc1c70000000000c3") and
         read(base,code,LEAF_VA+LEAF_LEN,7)==b"\xCC"*7,
         "source 9B helper is not EAX=ECX; [EAX]=0; RET")
    need(ctor[6:8]==b"\x8B\xF1" and
         ctor[59:65]==bytes.fromhex("8d8ea4000000"),
         "CWorld ctor no longer uses ECX owner and +0xA4 subobject")
    # All seven original direct callsites, including both calls to 0x24061220.
    for offset,target in CALLS:
        need(ctor[offset]==0xE8 and
             CTOR_VA+offset+5+struct.unpack_from("<i",ctor,offset+1)[0]==target,
             f"CWorld original callsite/target drift at +{offset:#x}")
    # Record the constructor's originally observed initialization boundaries:
    # both the early primary/secondary vtable stamps and the final sentinel.
    need(ctor[8:15]==bytes.fromhex("c74604bcde2724") and
         ctor[15:22]==bytes.fromhex("c74608f4622724") and
         ctor[29:35]==bytes.fromhex("c706ec952824") and
         ctor[35:42]==bytes.fromhex("c7460484952824"),
         "CWorld original vtable stamping drift")
    need(ctor[444:450]==bytes.fromhex("89865c040000"),
         "CWorld original +0x45C sentinel write drift")
    print("PASS Phase1726 unchanged original 573B CWorld ctor and 9B "
          "+0xA4 embedded DWORD zero helper; 7 E8 calls, ctor/vtable "
          "boundary, 3/7 INT3 padding, SHA authentication; no engine calls")


def self_test() -> None:
    leaf = bytes.fromhex("8bc1c70000000000c3")
    need(sha_ok(leaf,LEAF_LEN,LEAF_SHA), "leaf SHA fixture invalid")
    need(not sha_ok(leaf[:-1]+b"\x90",LEAF_LEN,LEAF_SHA),
         "one-byte leaf mutation passed")
    need(not sha_ok(b"\0"*CTOR_LEN,CTOR_LEN,CTOR_SHA),
         "fabricated constructor passed")
    need(len(CALLS)==7 and len(set(t for _,t in CALLS))==6 and
         CALLS[0]==(86,LEAF_VA) and WORD_OFFSET+4<=OWNER_BYTES,
         "source call inventory drift")
    print("PASS Phase1726 original-free ctor/leaf signatures and mutation refusal")


def main() -> None:
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--client",type=Path,
                        help="privately held unchanged 2006 owner client.dll")
    parser.add_argument("--self-test",action="store_true")
    args=parser.parse_args()
    need(args.self_test or args.client is not None,
         "use --self-test or --client /private/client.dll")
    if args.self_test: self_test()
    if args.client is not None: verify_original(args.client.read_bytes())


if __name__=="__main__":
    try: main()
    except (ValueError,OSError) as exc:
        print(f"FAIL Phase1726: {exc}",file=sys.stderr)
        sys.exit(1)
