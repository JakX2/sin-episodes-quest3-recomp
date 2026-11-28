#!/usr/bin/env python3
"""Phase1720: SHA-pin original 2006 PE32 allocator dispatch, not host ABI.

Unchanged original source 0x2416F8F0 is a 79-byte native method
that lazily resolves an allocator context via 0x2416F7F0 and then
calls engine allocator virtual +4 with one size argument (no
context), or virtual +0 with size/context/flags0 (with context).
All private --client work reads the owner original DLL only.
No original engine vtable, executable x86 methods or pointer
addresses are installed inside a portable client.
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

SOURCE_DISPATCH_VA=0x2416F8F0
SOURCE_DISPATCH_BYTES=79
SOURCE_DISPATCH_SHA="e4682f7b52bf956d9769e11f7390b3f6b17076d25e36f35951211b5a6e1fe82a"
SOURCE_CONTEXT_RESOLVER_VA=0x2416F7F0
SOURCE_CONTEXT_RESOLVER_BYTES=90
SOURCE_RESOLVER_SHA="c2a27c407aa1fa127dc525282abb96d283e1026826f428c0855df9480123cce0"
SOURCE_LAZY_FLAG=0x2437290C
SOURCE_CACHED_CONTEXT=0x24372908
SOURCE_ENGINE_ALLOCATOR_INTERFACE=0x242761CC
# Original x86 sparse byte witnesses; full-method SHA authenticates ALL
# original instructions without committing original executable bytes.
DISPATCH_WITNESSES=(
    (0x2416F8F0,"b801000000"),        # EAX=1
    (0x2416F8F5,"84050c293724"),      # test lazy original flag bit0
    (0x2416F8FD,"09050c293724"),      # set lazy flag before resolver
    (0x2416F903,"e8e8feffff"),        # CALL original context resolver
    (0x2416F908,"a308293724"),        # cache resolver pointer
    (0x2416F90F,"a108293724"),        # load cached pointer
    (0x2416F914,"85c07512"),          # test pointer -> context path
    (0x2416F918,"a1cc612724"),        # load engine allocator interface
    (0x2416F926,"ff5204"),            # vtable byte offset +4
    (0x2416F929,"c3"),                # RET fallback
    (0x2416F92A,"8b0dcc612724"),      # context interface
    (0x2416F934,"6a00"),              # original allocation flags 0
    (0x2416F93C,"ff12c3"),            # vtable byte offset +0; RET
)
RESOLVER_WITNESSES=(
    (0x2416F7F0,"a1cc612724"),       # engine interface source global
    (0x2416F7FC,"ff525c"),           # vtable byte offset +0x5C
    (0x2416F7FF,"84c0"),             # reject if virtual returned false
    (0x2416F801,"7441"),             # branch to original NULL return
    (0x2416F80B,"ff1598602724"),     # original Windows import call
    (0x2416F812,"ff159c602724"),     # original Windows import call
    (0x2416F826,"ff15a0602724"),     # original Windows import call
    (0x2416F837,"ff15a4602724"),     # original Windows import call
    (0x2416F844,"33c0"),             # NULL fallback
    (0x2416F849,"c3"),               # RET
)

def need(valid:bool,message:str)->None:
    if not valid:raise ValueError(message)

def verify_original(path:Path):
    binary=path.read_bytes()
    need(hashlib.sha256(binary).hexdigest()==ORIGINAL_CLIENT_SHA,
         "user unchanged 2006 PE32 client.dll whole SHA mismatch")
    pe=PE(binary)
    base,code=pe.sections[".text"]
    def at(va,count):
        offset=va-base
        need(count>=0 and 0<=offset<=len(code)-count,
             "source native instruction address out of .text bounds")
        return code[offset:offset+count]
    def check_function(va,size,digest):
        raw=at(va,size)
        need(hashlib.sha256(raw).hexdigest()==digest,
             f"whole native source allocator method changed at {va:#x}")
        need(raw[-1]==0xC3,
             "source allocator dispatcher/helper ends without RET")
        return raw
    check_function(SOURCE_DISPATCH_VA,SOURCE_DISPATCH_BYTES,SOURCE_DISPATCH_SHA)
    check_function(SOURCE_CONTEXT_RESOLVER_VA,SOURCE_CONTEXT_RESOLVER_BYTES,
                   SOURCE_RESOLVER_SHA)
    # The next native helper starts immediately after a one-byte INT3
    # at 0x2416F93F; the lazy resolver is followed by 6 CC bytes.
    need(at(SOURCE_DISPATCH_VA+SOURCE_DISPATCH_BYTES,1)==b"\xCC" and
         at(0x2416F940,1)==b"\xA1",
         "source allocator method boundary before next free helper changed")
    need(at(SOURCE_CONTEXT_RESOLVER_VA+SOURCE_CONTEXT_RESOLVER_BYTES,6)
         ==b"\xCC"*6,
         "original context resolver method padding changed")
    for address,opcode in DISPATCH_WITNESSES+RESOLVER_WITNESSES:
        raw=bytes.fromhex(opcode)
        need(at(address,len(raw))==raw,
             f"original native allocator branch/global/vtable witness changed at {address:#x}")
    call=at(0x2416F903,5)
    need(call[0]==0xE8 and 0x2416F903+5+
         struct.unpack_from("<i",call,1)[0]==SOURCE_CONTEXT_RESOLVER_VA,
         "source lazy allocator resolver E8 target changed")
    print("PASS Phase1720 owner-original allocator 79B dispatch and 90B "
          "context resolver SHA, original flag/context/engine globals, "
          "vtable+4 fallback vs vtable+0 context branches")


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
    need(len(sizes)==142,"source 142 original allocator route census changed")
    if args.self_test:
        for size,digest in ((SOURCE_DISPATCH_BYTES,SOURCE_DISPATCH_SHA),
                            (SOURCE_CONTEXT_RESOLVER_BYTES,SOURCE_RESOLVER_SHA)):
            need(hashlib.sha256(b"\x90"*size).hexdigest()!=digest,
                 "fabricated allocator x86 instructions falsely authenticated")
        need(SOURCE_DISPATCH_VA-SOURCE_CONTEXT_RESOLVER_VA==0x100 and
             SOURCE_CACHED_CONTEXT+4==SOURCE_LAZY_FLAG,
             "original allocator global layout or helper spacing changed")
        print("PASS Phase1720 original-free 79-byte allocator dispatch and "
              "90-byte resolver SHA refusal, 142 original entity source sizes")
    if args.client is not None:verify_original(args.client)

if __name__=="__main__":
    try:main()
    except (ValueError,OSError) as exc:
        print(f"FAIL Phase1720: {exc}",file=sys.stderr)
        sys.exit(1)
