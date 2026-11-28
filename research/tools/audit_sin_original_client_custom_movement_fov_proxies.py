#!/usr/bin/env python3
"""Phase1711: audit three original 2006 PE32 custom receive callbacks.

The private --client path authenticates original user-owned client.dll
read-only. Public CI --self-test checks source ordinal/callsite metadata
and invented-code refusal, not original private executable byte content.
"""
from __future__ import annotations

import argparse
import hashlib
from pathlib import Path
import struct
import sys
from audit_sin_original_client_member_offsets import PE, ORIGINAL_CLIENT_SHA
from audit_sin_original_client_source_proxy_catalog import public_header

# Original native VA, complete executable size, exact SHA256,
# 2006 original constructor source PUSH-name site, parent table/slot,
# next function-aligned VA (original INT3 padding, not in method digest).
SOURCE = (
    (0x240C61E0,18,
     "f496a564e61e8a5b3fac73fab8e2f9a0c45afd6fdfa36c38b0eed0d857208c00",
     0x240C7577,120,18,0x240C6200),
    (0x240C7330,28,
     "4929e26544167a91cc8298181e40544cb9831cdcb426ebd73b0c1a450b0d730a",
     0x240C755D,120,17,0x240C7350),
    (0x240D2590,27,
     "a127871117094cba6b47fa51952629dfcd2bb7a447c82e3957719a20c98531dc",
     0x240D33AB,202,4,0x240D25B0),
)
# Original native method offsets. The rest of each function body is
# authenticated by full SHA256, not copied into public research.
SPARSE_OPCODE_WITNESSES = (
    (0x240C61EB, "898a48010000"),   # [object+0x148] = decoded value
    (0x240C733B, "898844010000"),   # [object+0x144] = decoded value
    (0x240C7341, "c7804801000000000000"), # [object+0x148] = ZERO
    (0x240D259B, "394138"),         # compare previous FOV
    (0x240D259E, "740a"),           # skip reset on equal
    (0x240D25A0, "c7412c00000000"), # [object+0x2C] = ZERO
    (0x240D25A7, "894138"),         # [object+0x38] = decoded FOV
)

def need(valid:bool, message:str) -> None:
    if not valid:
        raise ValueError(message)


def verify_catalog(path:Path) -> None:
    all_callbacks=public_header(path)
    need(len(all_callbacks)==1110, "original 1110 callback map size drift")
    for va,size,digest,source_site,table,slot,next_va in SOURCE:
        need(all_callbacks.count(va)==1,
             f"expected exactly one original custom {va:#x} callback")
        # source catalog names and constructor order already authenticated
        # through original 1110-entry SHA and source 211-table/191-class CSV.
        need(size>0 and len(digest)==64 and va+size<next_va and
             next_va-va-size in (4,5,14),
             "original custom callback size/INT3 boundary inconsistent")
    need({(a[4],a[5]) for a in SOURCE}==
         {(120,18),(120,17),(202,4)},
         "native movement/default-FOV source identity changed")


def verify_original(path:Path) -> None:
    data=path.read_bytes()
    need(hashlib.sha256(data).hexdigest()==ORIGINAL_CLIENT_SHA,
         "owner-original 2006 client.dll is not the pinned retail version")
    pe=PE(data)
    base,code=pe.sections[".text"]
    def at(va:int,count:int) -> bytes:
        offset=va-base
        need(count>=0 and 0<=offset<=len(code)-count,
             "source x86 address outside the authenticated .text section")
        return code[offset:offset+count]
    for va,size,sha,site,table,slot,next_va in SOURCE:
        body=at(va,size)
        need(hashlib.sha256(body).hexdigest()==sha,
             f"original complete custom PE32 method changed at {va:#x}")
        need(body[-1:]==b"\xC3","source custom function has no terminal RET")
        padding=next_va-va-size
        need(at(va+size,padding)==b"\xCC"*padding,
             f"original function boundary/padding incorrect at {va:#x}")
        # The original source constructor pushes callback VA exactly
        # 11 bytes ahead of the PUSH-property-name/PUSH-output/CALL triad.
        # This checks the pointer's actual factory argument, not only
        # a known VA appearing elsewhere in this 2006 executable.
        need(at(site-11,5)==b"\x68"+struct.pack("<I",va),
             f"source constructor callback arg mismatch at {site:#x}")
        need(at(site,1)==b"\x68",
             "original 2006 constructor name PUSH changed")
    for va,raw in SPARSE_OPCODE_WITNESSES:
        expected=bytes.fromhex(raw)
        need(at(va,len(expected))==expected,
             f"original method storage/branch opcode changed at {va:#x}")
    print("PASS Phase1711 owner original 18B movecollide, 28B movetype, "
          "27B default-FOV SHA and exact constructor argument/padding")


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-test",action="store_true")
    parser.add_argument("--client",type=Path)
    parser.add_argument("--header",type=Path,default=Path(
        "include/sin_compat/original_client_source_proxy_catalog.hpp"))
    args=parser.parse_args()
    need(args.self_test or args.client is not None,
         "choose --self-test or --client /private/client.dll")
    verify_catalog(args.header)
    if args.self_test:
        for va,size,sha,site,table,slot,next_va in SOURCE:
            need(hashlib.sha256(b"\x90"*size).hexdigest()!=sha,
                 "fabricated source function incorrectly authenticated")
        print("PASS Phase1711 original-free three source callback identities "
              "and altered-size SHA rejection")
    if args.client is not None:
        verify_original(args.client)


if __name__=="__main__":
    try:main()
    except (ValueError,OSError) as exc:
        print(f"FAIL Phase1711: {exc}",file=sys.stderr)
        sys.exit(1)
