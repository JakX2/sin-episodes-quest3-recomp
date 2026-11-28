#!/usr/bin/env python3
"""Phase1734: authenticate the guarded 357B CWorld finalizer prefix.

Private --client requires unchanged owner-provided 2006 PE32 client.dll.
Original engine global 0x242FA458 is a HARD STOP; no original code runs.
"""
from __future__ import annotations

import argparse
import hashlib
from pathlib import Path
import struct
import sys

from audit_sin_original_client_world_embedded_word_phase1726 import (
    CLIENT_SHA, executable_text, need, read, sha_ok,
)
from audit_sin_original_client_world_interpolator_zeroed_phase1729 import pe_section
from audit_sin_original_client_world_lazy_static_phase1727 import expected_e8
from audit_sin_original_client_world_pre_finalizer_phase1732 import (
    SOURCE_FINALIZER_VA, SOURCE_FINALIZER_LEN, SOURCE_FINALIZER_SHA,
)
from audit_sin_original_client_world_vec3_setters_phase1733 import (
    verify_original as verify_setters,
)

STOP_VA=0x240CA0A5
ORIGINAL_ENGINE_GLOBAL=0x242FA458
PREFIX_BYTES=357
# Full source function VA, executable bytes, SHA256, cleanup bytes,
# and a few source-specific branch identities.
METHODS=(
    (0x24094240,11,'10cb8dc13ea1c124f6ed344386f12345588b3404b1d43b5419a8f19b4d42030a',5),
    (0x2411B060,163,'d8358c2a6091cdcf5c81762906a981fd5dcbbe4c41a7001d7307e5c83ad7a4b7',13),
    (0x2411B110,115,'c36642c25a96fe82c5bc3515bac6b2db16b4148966c29c574bc481dc165762a7',13),
    (0x240C90B0,107,'3b67e75c16412f5d3ea3f971b11f0425cca7b96c4e610ad91c8f7d48537c0184',5),
    (0x240C6240,40,'c37ea3fd71abb3b241e02aa60d4578b4c3d19244f031d493e6990d9c912f74c5',8),
    (0x240C6890,16,'6da0764cf8d2a39b1d9e6d613ceb20c2c2d802718736aeb8d0af1bd991b27a30',0),
    (0x241858A0,34,'d36253475466a24fcbf3655bf16028e7aa6bae7c868550818819d70aea6ec454',14),
)
CALLS_IN_FINALIZER=(
    (0x46,0x2411AD50), (0x51,0x2411AD50),
    (0xB1,0x24094240),
    (0xE9,0x2411B060), (0xF1,0x2411B110),
    (0x107,0x240C90B0), (0x15A,0x241858A0),
)


def verify_original(original:bytes)->None:
    need(hashlib.sha256(original).hexdigest()==CLIENT_SHA,
         'whole original 2006 client.dll SHA mismatch')
    verify_setters(original)
    base,code=executable_text(original)
    final=read(base,code,SOURCE_FINALIZER_VA,SOURCE_FINALIZER_LEN)
    need(sha_ok(final,SOURCE_FINALIZER_LEN,SOURCE_FINALIZER_SHA),
         'whole 414B original CWorld finalizer SHA drift')
    need(SOURCE_FINALIZER_VA+PREFIX_BYTES==STOP_VA and
         final[PREFIX_BYTES:PREFIX_BYTES+6] == b'\x8b\x0d'+struct.pack('<I',ORIGINAL_ENGINE_GLOBAL),
         'source prefix no longer stops before first 0x242FA458 engine dereference')
    need(final[0x5F:0x65]==bytes.fromhex('ff90e8000000') and
         final[0x6E:0x74]==bytes.fromhex('ff92f0000000'),
         'source finalizer no longer calls original x87 Vec3 setters')
    for offset,target in CALLS_IN_FINALIZER:
        expected_e8(base,code,SOURCE_FINALIZER_VA+offset,target)
    method={}
    for va,n,sha,pad in METHODS:
        block=read(base,code,va,n)
        need(sha_ok(block,n,sha),f'original source helper whole SHA drift {va:#x}')
        if pad:
            need(read(base,code,va+n,pad)==b'\xcc'*pad,
                 f'original source helper INT3 boundary drift {va:#x}')
        method[va]=block
    leaf=method[0x24094240]
    need(leaf==bytes.fromhex('c781e002000000000000c3'),
         '11B native leaf no longer clears owner+0x2E0 then returns')
    zero_state_1=method[0x2411B060]
    zero_state_2=method[0x2411B110]
    need(zero_state_1[8:17] == bytes.fromhex('397e200f848d000000') and
         zero_state_2[8:16] == bytes.fromhex('66395e24570fb77e'),
         'subobject 0x20/0x24 zero-equality early-return paths drift')
    reset=method[0x240C90B0]
    need(reset[:16]==bytes.fromhex('568bf18b46048d4e04ff500c84c07415'),
         'World reset virtual +0xC / zero-return branch drift')
    need(bytes.fromhex('668b86b8000000663dffff') in reset and
         bytes.fromhex('668b8636010000663dffff') in reset,
         'World reset original FFFF handle fast path drift')
    first_virtual=method[0x240C6240]
    need(first_virtual[:18]==bytes.fromhex('8379780a750332c0c383795c0074168b4174'),
         'first virtual condition no longer starts with +0x78/+0x5C flags')
    second_virtual=method[0x240C6890]
    need(second_virtual[:4]==bytes.fromhex('837948ff') and
         second_virtual[-3:]==bytes.fromhex('32c0c3'),
         'second virtual handle sentinel/refusal behavior drift')
    use_flags=method[0x241858A0]
    need(use_flags[:7]==bytes.fromhex('807c2404007411') and
         use_flags[24:34]==bytes.fromhex('80a143010000fec20400'),
         'source 0x241858A0 with argument 0 no longer AND flags+0x143,0xFE')
    rd_base,rd,_,_=pe_section(original,'.rdata')
    def vtable(va):
        off=va-rd_base
        need(0<=off<=len(rd)-4,'world CWorld vtable out of .rdata')
        return struct.unpack_from('<I',rd,off)[0]
    need(vtable(0x24289584+0x0C)==0x240C6240 and
         vtable(0x24289550+0x20)==0x240C6890,
         'source CWorld subobject virtual visibility method route drift')
    print('PASS Phase1734 owner 2006 original SHA: first 357/414B '
          'CWorld finalizer source instructions, 7 exact direct E8 calls, '
          '2 x87 virtual Vec3 setters, zero-state no-op interpolator '
          'methods, 11B +0x2E0 leaf, 107B visibility helper, 34B flag '
          'helper and stop before original 0x242FA458 engine dereference')


def self_test()->None:
    need(SOURCE_FINALIZER_VA+PREFIX_BYTES==STOP_VA and len(CALLS_IN_FINALIZER)==7,
         'source original startup boundary/call count changed')
    need(not sha_ok(b'\x90'*SOURCE_FINALIZER_LEN,SOURCE_FINALIZER_LEN,
                    SOURCE_FINALIZER_SHA),'fabricated 414B finalizer passed SHA')
    need(all(len(sha)==64 for _,_,sha,_ in METHODS),
         'source full helper SHA metadata drift')
    print('PASS Phase1734 original-free source finalizer prefix fingerprints')


def main()->None:
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--client',type=Path,
                   help='private unchanged 2006 PE32 client.dll')
    p.add_argument('--self-test',action='store_true')
    args=p.parse_args()
    need(args.self_test or args.client is not None,
         'use --self-test or --client /private/client.dll')
    if args.self_test:self_test()
    if args.client is not None:verify_original(args.client.read_bytes())


if __name__=='__main__':
    try:main()
    except (OSError,ValueError) as exc:
        print(f'FAIL Phase1734: {exc}',file=sys.stderr)
        sys.exit(1)
