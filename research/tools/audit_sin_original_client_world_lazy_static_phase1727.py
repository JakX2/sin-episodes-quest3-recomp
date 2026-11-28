#!/usr/bin/env python3
"""Phase1727: original-only SHA audit of two CWorld lazy globals and cleanup.

Private --client requires unchanged owner-provided 2006 PE32 client.dll.
No code or binary from the original DLL is included in the repository.
The paired init/cleanup code is classified, NOT used as runtime callbacks.
"""
from __future__ import annotations

import argparse
import hashlib
from pathlib import Path
import struct
import sys

from audit_sin_original_client_world_embedded_word_phase1726 import (
    CLIENT_SHA, CTOR_VA, executable_text, need, read, sha_ok,
)

CRT_CALLBACK_REGISTRAR_VA = 0x24248788
CRT_CALLBACK_REGISTRAR_LEN = 18
CRT_CALLBACK_REGISTRAR_SHA = (
    "8758b5f180d70ed03fcb7124f3970d139513901e47c04547bce0e0a00e18912b"
)

# (initializer VA, whole method SHA, owner ctor call displacement,
# 9-DWORD start, DWORD guard, original cleanup thunk, cleanup whole SHA,
# cleanup-dependent method, cleanup tail jump)
SOURCE_FAMILIES = (
    (0x24061540,"2375fa7d76b464959cb48d3fd709c4697a8d3818f7d8c342477fa3a27287b3a8",
     231,0x24324698,0x243246BC,0x24271DA0,
     "93923f6b4cbf8f409779d51921c9ab0bef9bf49c6074ede4784adee9ab6554fc",
     0x240608F0,0x240602B0),
    (0x240CB2F0,"d3aeb04ef3c87baf9f1e02c10bc274215926e0ecfcd554e13912c023b70c3c03",
     300,0x24338274,0x24338298,0x24272B10,
     "f6e2a137e8e06bf7403eb8ab63413a86dce6ae6efb07583871b4a689eeef4ba2",
     0x240C7050,0x240C5FA0),
)
SOURCE_INIT_LEN = 85
SOURCE_CLEANUP_LEN = 62
SOURCE_OWNER_CTOR_BYTES = 573


def expected_e8(text_base: int, code: bytes, caller: int, target: int) -> None:
    block=read(text_base,code,caller,5)
    need(block[:1]==b"\xe8" and
         caller+5+struct.unpack_from("<i",block,1)[0]==target,
         f"native E8 source-target drift: {caller:#x} expected {target:#x}")


def expected_e9(text_base: int, code: bytes, caller: int, target: int) -> None:
    block=read(text_base,code,caller,5)
    need(block[:1]==b"\xe9" and
         caller+5+struct.unpack_from("<i",block,1)[0]==target,
         f"native E9 source-target drift: {caller:#x} expected {target:#x}")


def validate(original: bytes) -> None:
    need(hashlib.sha256(original).hexdigest()==CLIENT_SHA,
         "unchanged owner original PE32 client.dll SHA mismatch")
    base,code=executable_text(original)
    reg=read(base,code,CRT_CALLBACK_REGISTRAR_VA,CRT_CALLBACK_REGISTRAR_LEN)
    need(sha_ok(reg,CRT_CALLBACK_REGISTRAR_LEN,CRT_CALLBACK_REGISTRAR_SHA),
         "original CRT callback registration wrapper SHA drift")
    need(reg[:4]==bytes.fromhex("ff742404") and
         reg[-1:]==b"\xc3", "CRT wrapper ABI drift")
    expected_e8(base,code,CRT_CALLBACK_REGISTRAR_VA+4,0x24248706)

    ctor=read(base,code,CTOR_VA,SOURCE_OWNER_CTOR_BYTES)
    need(ctor[-1:]==b"\xc3", "parent constructor boundary drift")
    for (va,sha,calloffset,global_va,guard_va,cleanup_va,
         cleanup_sha,nested_va,tail_va) in SOURCE_FAMILIES:
        init=read(base,code,va,SOURCE_INIT_LEN)
        cleanup=read(base,code,cleanup_va,SOURCE_CLEANUP_LEN)
        need(sha_ok(init,SOURCE_INIT_LEN,sha),
             f"original lazy initializer whole SHA drift at {va:#x}")
        need(sha_ok(cleanup,SOURCE_CLEANUP_LEN,cleanup_sha),
             f"original registered cleanup full SHA drift at {cleanup_va:#x}")
        need(guard_va==global_va+36,"original static 9 DWORD layout drift")
        need(read(base,code,va+SOURCE_INIT_LEN,11)==b"\xcc"*11,
             "native initializer INT3 padding drift")
        need(read(base,code,cleanup_va+SOURCE_CLEANUP_LEN,2)==b"\xcc"*2,
             "native cleanup INT3 padding drift")
        # 85B canonical one-shot body:
        # EAX=1; test BYTE [guard],AL; JNE already_ready;
        # OR DWORD [guard],EAX; EAX=0; push cleanup thunk;
        # MOV [slot],EAX x9; call CRT; add esp,4; mov eax,&slot0; ret.
        need(init[0:5]==bytes.fromhex("b801000000") and
             init[5:7]==b"\x84\x05" and
             struct.unpack_from("<I",init,7)[0]==guard_va and
             init[11:13]==b"\x75\x42" and
             init[13:15]==b"\x09\x05" and
             struct.unpack_from("<I",init,15)[0]==guard_va and
             init[19:22]==b"\x33\xc0\x68" and
             struct.unpack_from("<I",init,22)[0]==cleanup_va and
             init[76:79]==b"\x83\xc4\x04" and
             init[79:80]==b"\xb8" and
             struct.unpack_from("<I",init,80)[0]==global_va and
             init[-1:]==b"\xc3", "85-byte source lazy static structure drift")
        source_order=(0,4,8,12,16,20,24,32,28)
        for n,offset in enumerate(source_order):
            at=26+n*5
            need(init[at:at+1]==b"\xa3" and
                 struct.unpack_from("<I",init,at+1)[0]==global_va+offset,
                 f"9-DWORD zero order drift: {va:#x}+{at:#x}")
        expected_e8(base,code,va+71,CRT_CALLBACK_REGISTRAR_VA)
        expected_e8(base,code,CTOR_VA+calloffset,va)
        # 62B registered cleanup checks last global DWORD and conditionally
        # calls a nonportable, separately unimplemented native list destructor,
        # then tail-jumps a different unimplemented cleanup helper.
        need(cleanup[0:3]==b"\x56\x33\xf6" and
             cleanup[3:5]==b"\x39\x35" and
             struct.unpack_from("<I",cleanup,5)[0]==global_va+32 and
             cleanup[9:11]==b"\x74\x28" and
             cleanup[11:12]==b"\xb9" and
             struct.unpack_from("<I",cleanup,12)[0]==global_va and
             cleanup[51:52]==b"\xb9" and
             struct.unpack_from("<I",cleanup,52)[0]==global_va and
             cleanup[56:57]==b"\x5e" and
             cleanup[57:58]==b"\xe9",
             "original registered cleanup's conditional/global-pointer ABI drift")
        for n,offset in enumerate((32,24,16,20,28)):
            at=21+n*6
            need(cleanup[at:at+2]==b"\x89\x35" and
                 struct.unpack_from("<I",cleanup,at+2)[0]==global_va+offset,
                 f"registered cleanup global zero write drift at {cleanup_va:#x}+{at:#x}")
        expected_e8(base,code,cleanup_va+16,nested_va)
        expected_e9(base,code,cleanup_va+57,tail_va)
    print("PASS Phase1727 whole client SHA, twin 85B guard/9-DWORD/CRT-"
          "registered initializers, both 62B conditional cleanup thunks, "
          "18B CRT wrapper, 2 original CWorld constructor callsites")


def self_test() -> None:
    need(len(SOURCE_FAMILIES)==2,"source family cardinality mismatch")
    va_set={row[0] for row in SOURCE_FAMILIES}
    need(len(va_set)==2,"duplicate source native helper")
    for va,sha,off,global_va,guard_va,cleanup_va,cleanup_sha,_,_ in SOURCE_FAMILIES:
        need(len(sha)==64 and len(cleanup_sha)==64 and global_va+36==guard_va,
             "source hashes and static layout metadata malformed")
        need(not sha_ok(b"\0"*SOURCE_INIT_LEN,SOURCE_INIT_LEN,sha) and
             not sha_ok(b"\0"*SOURCE_CLEANUP_LEN,SOURCE_CLEANUP_LEN,cleanup_sha),
             "fabricated original helper accepted")
        need(va>0 and off in (231,300) and cleanup_va>0,
             "original ctor dependency inventory drift")
    print("PASS Phase1727 original-free two helper families and SHA mutation refusal")


def main() -> None:
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--client",type=Path,
                    help="private unchanged 2006 SiN Episodes PE32 client.dll")
    ap.add_argument("--self-test",action="store_true")
    args=ap.parse_args()
    need(args.self_test or args.client is not None,
         "use --self-test or --client /private/client.dll")
    if args.self_test: self_test()
    if args.client is not None: validate(args.client.read_bytes())


if __name__=="__main__":
    try: main()
    except (OSError,ValueError) as exc:
        print(f"FAIL Phase1727: {exc}",file=sys.stderr)
        sys.exit(1)
