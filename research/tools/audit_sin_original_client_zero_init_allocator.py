#!/usr/bin/env python3
"""Phase1719: owner-private original 2006 PE32 40B zeroing allocator wrappers.

Four native helper functions are SHA-attested in the unchanged owner
client.dll. All allocate via original 0x2416F8F0, then zero exactly
the requested allocation size with x86 REP STOSD and REP STOSB.
This only authenticates the native helper SUCCESS-path behavior, not
that underlying allocation service, out-of-memory behavior,
constructor initialization, or object ownership/vtable layout.
No original binary executable is copied to the public repository.
"""
from __future__ import annotations

import argparse
import hashlib
from pathlib import Path
import struct
import sys
from audit_sin_original_client_member_offsets import PE, ORIGINAL_CLIENT_SHA
from audit_sin_original_client_entity_allocation_prologues import (
    original_free as source_sizes_original_free,
)

WRAPPERS=(
    (0x240C4FF0,"e9428fee2e9eccc2fb344b2d86b9e238e56d5ad2b12cc128db314f7416513e9d"),
    (0x240C5020,"c3e3bd04c0614fb56f3111181075c0c8e6d85f7e2b16f165d1b971a36cfce502"),
    (0x240C5050,"dac60fe2bee174356d2923ebdac3777e89b621f2b87bb29098e8c158e6d74b7a"),
    (0x240C5080,"b9cd8e7a18106e5363019f1da995542a7ae2da1909a136f588e561ca6bb877e1"),
)
WRAPPER_LENGTH=40
PAD=8
NATIVE_SOURCE_ALLOCATOR=0x2416F8F0
# Sparse opcode evidence; original whole 40B function SHA pins the
# remaining instructions. The original copies vary ONLY in their
# E8 relative32 CALL displacement, not in the zero-init algorithm.
OPCODES=(
    (0,"56"),             # push esi
    (1,"8b742408"),       # mov esi,[esp+8], unsigned requested size
    (5,"57"),             # push edi
    (6,"56"),             # push esi (passed to 0x2416F8F0)
    (7,"e8"),             # call original allocator
    (12,"8bd0"),          # mov edx,eax (return allocation pointer)
    (14,"8bce"),          # mov ecx,esi
    (16,"c1e902"),        # shr ecx,2
    (19,"33c0"),          # xor eax,eax
    (21,"8bfa"),          # mov edi,edx
    (23,"f3ab"),          # REP STOS DWORD [EDI],EAX (zeros size/4)
    (25,"8bce"),          # mov ecx,esi
    (27,"83c404"),        # add esp,4 (pop size argument)
    (30,"83e103"),        # and ecx,3
    (33,"f3aa"),          # REP STOS BYTE [EDI],AL (zeros tail)
    (35,"5f"),            # pop edi
    (36,"8bc2"),          # mov eax,edx (return allocated pointer)
    (38,"5e"),            # pop esi
    (39,"c3"),            # ret
)

def need(value,message):
    if not value:raise ValueError(message)


def verify_original(path:Path):
    data=path.read_bytes()
    need(hashlib.sha256(data).hexdigest()==ORIGINAL_CLIENT_SHA,
         "unchanged user original client.dll SHA256 mismatch")
    pe=PE(data)
    base,code=pe.sections[".text"]
    original_normalized=None
    for va,sha in WRAPPERS:
        offset=va-base
        need(0<=offset<=len(code)-WRAPPER_LENGTH-PAD,
             "source native allocation wrapper outside .text")
        method=code[offset:offset+WRAPPER_LENGTH]
        need(hashlib.sha256(method).hexdigest()==sha,
             f"whole source 40B original zero-init wrapper changed {va:#x}")
        need(code[offset+WRAPPER_LENGTH:offset+WRAPPER_LENGTH+PAD]==
             b"\xCC"*PAD,
             "source original zero-init function boundary/INT3 drift")
        for idx,hex_bytes in OPCODES:
            want=bytes.fromhex(hex_bytes)
            need(method[idx:idx+len(want)]==want,
                 f"source zeroing instruction mismatch at {va+idx:#x}")
        displacement,=struct.unpack_from("<i",method,8)
        need(va+12+displacement==NATIVE_SOURCE_ALLOCATOR,
             f"source original allocation delegate not 0x2416F8F0 at {va:#x}")
        normalized=method[:8]+b"\x00"*4+method[12:]
        if original_normalized is None:
            original_normalized=normalized
        else:
            need(normalized==original_normalized,
                 "original 40B allocation helpers differ beyond CALL displacement")
    print("PASS Phase1719 owner-original four 40-byte zero-initializing "
          "native allocator wrappers, same underlying allocator and "
          "REP STOSD/STOSB source behavior; 142 creator sizes separately pinned")


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-test",action="store_true")
    parser.add_argument("--client",type=Path)
    parser.add_argument("--manifest",type=Path,default=Path(
        "research/manifests/original_client_142_entity_allocator_prologues_phase1718.csv"))
    parser.add_argument("--creator-catalog",type=Path,default=Path(
        "research/manifests/original_client_creator_source_map_phase1716.csv"))
    parser.add_argument("--prologue-header",type=Path,default=Path(
        "include/sin_compat/original_client_entity_allocation_prologues.hpp"))
    args=parser.parse_args()
    need(args.self_test or args.client is not None,
         "use --self-test or --client /private/client.dll")
    sizes=source_sizes_original_free(
        args.manifest,args.creator_catalog,args.prologue_header)
    need(len(sizes)==142,"source 142 original allocation route count changed")
    if args.self_test:
        for va,sha in WRAPPERS:
            need(hashlib.sha256(b"\x90"*WRAPPER_LENGTH).hexdigest()!=sha,
                 "invented native zero-init function falsely authenticated")
        need(WRAPPERS[0][0]==0x240C4FF0 and len(WRAPPERS)==4,
             "source original 4-function allocation family drift")
        print("PASS Phase1719 original-free source 142 entity sizes and "
              "four native 40-byte zero-init function SHA mutation refusal")
    if args.client is not None:verify_original(args.client)


if __name__=="__main__":
    try:main()
    except (ValueError,OSError) as exc:
        print(f"FAIL Phase1719: {exc}",file=sys.stderr)
        sys.exit(1)
