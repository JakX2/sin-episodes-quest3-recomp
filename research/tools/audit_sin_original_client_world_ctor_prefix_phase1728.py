#!/usr/bin/env python3
"""Phase1728: source-only CWorld 174-byte deterministic constructor prefix.

Private --client audits user-owned unchanged 2006 PE32 client.dll.
No original code bytes, image, or active 2006 pointers are committed.
The 26B embedded ctor at 0x2411AEB0 and 210B virtual helper at
0x2411AD50 are authenticated as BLOCKED executable dependencies.
"""
from __future__ import annotations

import argparse
import hashlib
from pathlib import Path
import struct
import sys

from audit_sin_original_client_world_embedded_word_phase1726 import (
    CLIENT_SHA, CTOR_VA, CTOR_LEN, CTOR_SHA, LEAF_VA, LEAF_LEN,
    LEAF_SHA, executable_text, need, read, sha_ok,
)
from audit_sin_original_client_world_lazy_static_phase1727 import expected_e8

PREFIX_BYTES = 174
EMBEDDED_CTOR_VA = 0x2411AEB0
EMBEDDED_CTOR_LEN = 26
EMBEDDED_CTOR_SHA = "869ab11c60efedddb83d348f93fa57e5a0d173b4fc584f6bcbe2771c6e35d0ae"
EMBEDDED_HELPER_VA = 0x2411AD50
EMBEDDED_HELPER_LEN = 210
EMBEDDED_HELPER_SHA = "624fbab5f4c8a0ddd9b3c35acb372107a9acb8d36a751ab7b84345983673b443"
SOURCE_INITIAL_VTABLE_WRITES = (
    (8,0x04,0x2427DEBC),
    (15,0x08,0x242762F4),
    (22,0x0C,0x24288E90),
    (29,0x00,0x242895EC),
    (35,0x04,0x24289584),
    (42,0x08,0x24289550),
    (49,0x0C,0x24289538),
)
SOURCE_ZERO_DWORD_WRITES = (
    (65,0x10),(68,0x14),(71,0x18),(74,0x1C),(77,0x20),
    (83,0x28),
    (100,0x0C8),(106,0x0CC),(112,0x0D0),(118,0x0D4),(124,0x0D8),
)
SOURCE_NEGATIVE_ONE_DWORD_WRITES = (
    (94,0x0B4),(130,0x14C),(136,0x154),(142,0x158),
    (148,0x15C),(160,0x160),(168,0x164),
)


def verify_original(original: bytes) -> None:
    need(hashlib.sha256(original).hexdigest()==CLIENT_SHA,
         "unchanged owner original PE32 client.dll SHA mismatch")
    base,code=executable_text(original)
    ctor=read(base,code,CTOR_VA,CTOR_LEN)
    leaf=read(base,code,LEAF_VA,LEAF_LEN)
    embedded=read(base,code,EMBEDDED_CTOR_VA,EMBEDDED_CTOR_LEN)
    virtual=read(base,code,EMBEDDED_HELPER_VA,EMBEDDED_HELPER_LEN)
    need(sha_ok(ctor,CTOR_LEN,CTOR_SHA),"whole original CWorld constructor SHA drift")
    need(sha_ok(leaf,LEAF_LEN,LEAF_SHA),"original first constructor helper SHA drift")
    need(sha_ok(embedded,EMBEDDED_CTOR_LEN,EMBEDDED_CTOR_SHA),
         "original 26B embedded constructor SHA drift")
    need(sha_ok(virtual,EMBEDDED_HELPER_LEN,EMBEDDED_HELPER_SHA),
         "original 210B virtual helper SHA drift")
    need(embedded[-1:]==b"\xc3" and
         read(base,code,EMBEDDED_CTOR_VA+EMBEDDED_CTOR_LEN,6)==b"\xcc"*6,
         "26B embedded constructor native method boundary drift")
    need(virtual[-3:]==b"\xc2\x04\x00" and
         read(base,code,EMBEDDED_HELPER_VA+EMBEDDED_HELPER_LEN,14)==b"\xcc"*14,
         "210B virtual helper RET4 and padding drift")
    need(ctor[:8]==bytes.fromhex("83ec0c5355568bf1"),
         "CWorld original ctor prologue changes owner pointer ABI")

    for at,offset,val in SOURCE_INITIAL_VTABLE_WRITES:
        if offset==0:
            need(ctor[at:at+2]==b"\xc7\x06" and
                 struct.unpack_from("<I",ctor,at+2)[0]==val,
                 f"native owner vtable stamp drift at ctor+{at:#x}")
        else:
            need(ctor[at:at+3]==bytes((0xC7,0x46,offset)) and
                 struct.unpack_from("<I",ctor,at+3)[0]==val,
                 f"native owner sub-vtable stamp drift at ctor+{at:#x}")
    need(ctor[56:59]==bytes.fromhex("33db57") and
         ctor[59:65]==bytes.fromhex("8d8ea4000000") and
         ctor[80:83]==bytes.fromhex("885e24") and
         ctor[91:94]==bytes.fromhex("83cfff") and
         ctor[154:160]==bytes.fromhex("8dae6c010000") and
         ctor[166:168]==bytes.fromhex("8bcd"),
         "source CWorld register/subobject ownership boundary drift")
    for at,offset in SOURCE_ZERO_DWORD_WRITES:
        if offset<=0x28:
            need(ctor[at:at+3]==bytes((0x89,0x5E,offset)),
                 f"EBX=0 field write drift at ctor+{at:#x}")
        else:
            need(ctor[at:at+2]==b"\x89\x9e" and
                 struct.unpack_from("<I",ctor,at+2)[0]==offset,
                 f"EBX=0 extended field write drift at ctor+{at:#x}")
    for at,offset in SOURCE_NEGATIVE_ONE_DWORD_WRITES:
        need(ctor[at:at+2]==b"\x89\xbe" and
             struct.unpack_from("<I",ctor,at+2)[0]==offset,
             f"EDI=-1 sentinel write drift at ctor+{at:#x}")
    need(ctor[174:175]==b"\xe8" and CTOR_VA+PREFIX_BYTES==0x240CCEDE,
         "source prefix unexpectedly includes 0x16C external constructor")
    expected_e8(base,code,CTOR_VA+86,LEAF_VA)
    expected_e8(base,code,CTOR_VA+174,EMBEDDED_CTOR_VA)

    # The next 26B helper itself writes a source vtable to this+0
    # and FFFF to this+0x26, then invokes a 210B helper with virtual
    # dispatch. It is not safe to fold these into a portable whole ctor.
    need(embedded[0:3]==b"\x56\x8b\xf1" and
         embedded[3:5]==b"\x6a\x00" and
         embedded[5:7]==b"\xc7\x06" and
         struct.unpack_from("<I",embedded,7)[0]==0x2429806C and
         embedded[11:17]==bytes.fromhex("66c74626ffff") and
         embedded[22:26]==bytes.fromhex("8bc65ec3"),
         "original 26B embedded object init source ABI drift")
    expected_e8(base,code,EMBEDDED_CTOR_VA+17,EMBEDDED_HELPER_VA)
    need(virtual[0:4]==bytes.fromhex("8b442404") and
         b"\xff\x52\x48" in virtual and
         b"\xff\x50\x48" in virtual,
         "210B embedded helper lost engine-dependent virtual dispatch")
    print("PASS Phase1728 original client SHA + 174B constructor prefix: "
          "27 exact source writes including 9B +0xA4 helper, vtable "
          "overwrite order and -1 sentinels, call boundary at 0x2411AEB0, "
          "26B embedded ctor and 210B virtual helper whole SHA")


def self_test() -> None:
    need(PREFIX_BYTES==174 and CTOR_VA+PREFIX_BYTES==0x240CCEDE,
         "source prefix boundary changed")
    need(len(SOURCE_INITIAL_VTABLE_WRITES)==7 and
         len(SOURCE_ZERO_DWORD_WRITES)==11 and
         len(SOURCE_NEGATIVE_ONE_DWORD_WRITES)==7,
         "source direct write inventory changed")
    need(7+11+7+2==27,"source 27 write count not reconciled")
    for n,sha in ((EMBEDDED_CTOR_LEN,EMBEDDED_CTOR_SHA),
                  (EMBEDDED_HELPER_LEN,EMBEDDED_HELPER_SHA)):
        need(not sha_ok(b"\x90"*n,n,sha),
             "fabricated subobject constructor falsely verified")
    print("PASS Phase1728 original-free prefix/sentinel inventory and SHA mutation refusal")


def main() -> None:
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--client",type=Path,
                        help="private unchanged 2006 PE32 client.dll")
    parser.add_argument("--self-test",action="store_true")
    args=parser.parse_args()
    need(args.self_test or args.client is not None,
         "use --self-test or --client /private/client.dll")
    if args.self_test: self_test()
    if args.client is not None: verify_original(args.client.read_bytes())


if __name__=="__main__":
    try: main()
    except (OSError,ValueError) as exc:
        print(f"FAIL Phase1728: {exc}",file=sys.stderr)
        sys.exit(1)
