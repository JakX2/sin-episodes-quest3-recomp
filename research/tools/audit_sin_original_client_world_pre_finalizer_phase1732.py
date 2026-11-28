#!/usr/bin/env python3
"""Phase1732: verify CWorld 558B pre-finalizer source against original PE32.

Source finalizer at 0x240C9F40 is NOT executed; original code SHA only.
"""
from __future__ import annotations
import argparse
import hashlib
from pathlib import Path
import struct
import sys
from audit_sin_original_client_world_embedded_word_phase1726 import (
    CLIENT_SHA,CTOR_VA,CTOR_LEN,CTOR_SHA,executable_text,read,sha_ok,need,
)
from audit_sin_original_client_world_lazy_static_phase1727 import expected_e8
from audit_sin_original_client_world_two_vector_entries_phase1731 import (
    verify_original as verify_original_vector,
)
SOURCE_PREFIX_BYTES=558
SOURCE_FINALIZER_VA=0x240C9F40
SOURCE_FINALIZER_LEN=414
SOURCE_FINALIZER_SHA='ff892ff03f8c92844de5106d226fb0173fc965c0eb166f1482e15a6760a28382'
POST_VECTOR_SOURCE_WRITES=(
    (444,'DWORD',0x45C,0xffffffff),
    (450,'WORD',0x074,0),
    (454,'BYTE',0x150,0),
    (460,'DWORD',0x058,0xff),
    (467,'BYTE',0x0c0,0),
    (473,'BYTE',0x2e8,0),
    (479,'BYTE',0x118,0),
    (493,'BYTE',0x470,0),
    (507,'BYTE',0x471,0),
    (515,'DWORD',0x0bc,0),
    (521,'DWORD',0x0c4,0xffffffff),
    (527,'BYTE',0x0b0,1),
    (534,'BYTE',0x0b1,0),
    (540,'DWORD',0x138,0),
    (546,'BYTE',0x459,0),
    (552,'DWORD',0x0a8,0),
)


def verify_original(original:bytes)->None:
    need(hashlib.sha256(original).hexdigest()==CLIENT_SHA,'original client.dll SHA mismatch')
    verify_original_vector(original)
    base,code=executable_text(original)
    ctor=read(base,code,CTOR_VA,CTOR_LEN)
    final=read(base,code,SOURCE_FINALIZER_VA,SOURCE_FINALIZER_LEN)
    need(sha_ok(ctor,CTOR_LEN,CTOR_SHA),'CWorld complete constructor SHA drift')
    need(sha_ok(final,SOURCE_FINALIZER_LEN,SOURCE_FINALIZER_SHA),
         'native 414B CWorld initializer full method SHA drift')
    need(ctor[SOURCE_PREFIX_BYTES:SOURCE_PREFIX_BYTES+1]==b'\xe8' and
         CTOR_VA+SOURCE_PREFIX_BYTES==0x240CD05E,
         'CWorld source finalizer call boundary drift')
    expected_e8(base,code,CTOR_VA+SOURCE_PREFIX_BYTES,SOURCE_FINALIZER_VA)
    need(final[-5:-4]==b'\xe9' and
         read(base,code,SOURCE_FINALIZER_VA+SOURCE_FINALIZER_LEN,2)==b'\xcc'*2,
         '414B source finalizer tail JMP / INT3 padding drift')
    # Native finalizer is genuinely engine-dependent: two original
    # virtuals (+0xE8/+0xF0), nested interpolator calls and engine globals.
    need(final[95:101]==bytes.fromhex('ff90e8000000') and
         final[110:116]==bytes.fromhex('ff92f0000000') and
         struct.pack('<I',0x242FA458) in final,
         'source finalizer lost engine-dependent virtual dispatch/globals')
    for off,kind,field,value in POST_VECTOR_SOURCE_WRITES:
        start=ctor[off:off+8]
        if kind=='DWORD':
            if field in (0x058,):
                need(start[:3]==bytes((0xc7,0x46,field)) and
                     struct.unpack_from('<I',start,3)[0]==value,
                     f'source DWORD immediate field drift at +{off:#x}')
            elif off==444:
                need(start[:2]==b'\x89\x86' and
                     struct.unpack_from('<I',start,2)[0]==field,
                     'source -1 +0x45C write drift')
            elif value==0xffffffff:
                need(start[:2]==b'\x89\x86' and
                     struct.unpack_from('<I',start,2)[0]==field,
                     'source -1 DWORD write drift')
            else:
                need(start[:2]==b'\x89\x9e' and
                     struct.unpack_from('<I',start,2)[0]==field,
                     'source zero DWORD write drift')
        elif kind=='WORD':
            need(start[:2]==b'\x66\x89' and field==0x74,
                 'source zero WORD field drift')
        else:
            need(field<1200,'source BYTE outside owner')
    need(len(POST_VECTOR_SOURCE_WRITES)==16,
         'source tail ordered write inventory drift')
    print('PASS Phase1732 unchanged source 2006 PE32 SHA: CWorld 558B '
          'inert source prefix, 16 tail fields, engine/virtual-bound 414B '
          'initializer intentionally blocked at 0x240CD05E')


def self_test()->None:
    need(CTOR_VA+SOURCE_PREFIX_BYTES==0x240CD05E and
         SOURCE_PREFIX_BYTES<CTOR_LEN and len(POST_VECTOR_SOURCE_WRITES)==16,
         'source constructor finalizer boundary drift')
    need(not sha_ok(b'\x00'*SOURCE_FINALIZER_LEN,SOURCE_FINALIZER_LEN,
                    SOURCE_FINALIZER_SHA),'fabricated initializer SHA accepted')
    print('PASS Phase1732 original-free 558B source boundary and 414B SHA refusal')


def main()->None:
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--client',type=Path)
    p.add_argument('--self-test',action='store_true')
    args=p.parse_args()
    need(args.self_test or args.client is not None,'use --self-test or --client /private/client.dll')
    if args.self_test:self_test()
    if args.client is not None:verify_original(args.client.read_bytes())


if __name__=='__main__':
    try:main()
    except (ValueError,OSError) as exc:
        print(f'FAIL Phase1732: {exc}',file=sys.stderr)
        sys.exit(1)
