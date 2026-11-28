#!/usr/bin/env python3
"""Phase1730: verify 398-byte pre-vector World ctor boundary against 2006 PE32.

No original executable bytes are shipped. This boundary cannot construct a
retail game object; original dynamic vector insertion is NOT invoked.
"""
from __future__ import annotations

import argparse
import hashlib
from pathlib import Path
import struct
import sys

from audit_sin_original_client_world_embedded_word_phase1726 import (
    CLIENT_SHA, CTOR_VA, CTOR_LEN, CTOR_SHA, executable_text, need, read, sha_ok,
)
from audit_sin_original_client_world_lazy_static_phase1727 import (
    SOURCE_FAMILIES, expected_e8,
)
from audit_sin_original_client_world_interpolator_zeroed_phase1729 import (
    verify_original as verify_original_embedded,
)

FIRST_STATIC_CALLER_OFFSET=231
SECOND_STATIC_CALLER_OFFSET=300
VECTOR_INSERT_CALLER_OFFSET=398
SOURCE_VECTOR_INSERT_VA=0x24061220
SOURCE_VECTOR_INSERT_LEN=121
SOURCE_VECTOR_INSERT_SHA="3e3868e86685fa86f4bcbb796a35de8b8277cddb8b69e17f203a673757edb948"
# Source instruction byte offsets for the post-subobject writes. The
# portable snapshots record destination offsets/values in their headers.
POST_WRITE_INSTRUCTION_OFFSETS=(
    179,186,192,209,215,222,225,228,
    236,239,242,250,252,255,258,261,273,280,287,290,297,
    305,308,311,323,325,328,334,337,344,350,356,
)


def verify_original(original:bytes)->None:
    need(hashlib.sha256(original).hexdigest()==CLIENT_SHA,
         "unchanged owner original PE32 client.dll whole SHA mismatch")
    verify_original_embedded(original)
    base,code=executable_text(original)
    ctor=read(base,code,CTOR_VA,CTOR_LEN)
    need(sha_ok(ctor,CTOR_LEN,CTOR_SHA),
         "original CWorld full 573-byte method SHA changed")
    need(CTOR_VA+VECTOR_INSERT_CALLER_OFFSET==0x240CCFBE,
         "first dynamic vector callsite no longer follows 398 source bytes")
    need(ctor[VECTOR_INSERT_CALLER_OFFSET:VECTOR_INSERT_CALLER_OFFSET+1]==b"\xe8",
         "source CWorld vector insertion not at expected first boundary")
    expected_e8(base,code,CTOR_VA+VECTOR_INSERT_CALLER_OFFSET,
                SOURCE_VECTOR_INSERT_VA)
    expected_e8(base,code,CTOR_VA+FIRST_STATIC_CALLER_OFFSET,SOURCE_FAMILIES[0][0])
    expected_e8(base,code,CTOR_VA+SECOND_STATIC_CALLER_OFFSET,SOURCE_FAMILIES[1][0])
    need(len(POST_WRITE_INSTRUCTION_OFFSETS)==32 and
         POST_WRITE_INSTRUCTION_OFFSETS==tuple(sorted(POST_WRITE_INSTRUCTION_OFFSETS)),
         "original ordered CWorld field-write count drift")
    # Original after 26B embedded subobject constructor changes its vtable;
    # the 32 source writes occur around the two lazy static initializers.
    def dword_at(offset:int)->int:
        return struct.unpack_from("<I",ctor,offset)[0]
    need(ctor[179:182]==bytes.fromhex("c74500") and
         dword_at(182)==0x24288E3C and
         ctor[186:188]==b"\x89\xbe" and dword_at(188)==0x1D8 and
         ctor[192:194]==b"\x89\xbe" and dword_at(194)==0x1E0,
         "world embedded vtable override or sentinel offset drift")
    need(ctor[209:211]==b"\xc7\x07" and
         dword_at(211)==0x24278694 and
         ctor[215:218]==bytes.fromhex("c7472c") and
         dword_at(218)==0x24289524,
         "source first embedded container vtable/setup drift")
    need(ctor[261:263]==b"\xc7\x07" and
         dword_at(263)==0x24278AA4 and
         ctor[273:276]==bytes.fromhex("c74500") and
         dword_at(276)==0x242893F0 and
         ctor[280:283]==bytes.fromhex("c7452c") and
         dword_at(283)==0x24289510,
         "source first post-static vtable/second container init drift")
    need(ctor[337:340]==bytes.fromhex("c74500") and
         dword_at(340)==0x24289418 and
         ctor[344:346]==b"\x89\x86" and dword_at(346)==0x460 and
         ctor[350:352]==b"\x89\x86" and dword_at(352)==0x464 and
         ctor[356:358]==b"\x89\x86" and dword_at(358)==0x468,
         "source second post-static vtable/0x460 sentinel drift")
    need(ctor[371:375]==bytes.fromhex("897c2418") and
         ctor[390:398]==bytes.fromhex("c744241802000000"),
         "first vector insertion stack/local argument setup drift")
    # The next native helper increments vector length, can grow its
    # 12-byte element storage and uses CRT memmove; intentionally blocked.
    helper=read(base,code,SOURCE_VECTOR_INSERT_VA,SOURCE_VECTOR_INSERT_LEN)
    need(sha_ok(helper,SOURCE_VECTOR_INSERT_LEN,SOURCE_VECTOR_INSERT_SHA) and
         helper[-3:]==b"\xc2\x08\x00" and
         b"\xe8" in helper and b"\x0c" in helper,
         "source vector insertion no longer 12B element RET8 helper")
    print("PASS Phase1730 private original 2006 PE32 398B CWorld pre-vector "
          "prefix, paired 85B lazy calls, embedded 26+210B no-op virtual "
          "path, original 32 post-subobject stores and first allocator-"
          "capable 12B vector insertion refusal at 0x240CCFBE")


def self_test()->None:
    need(CTOR_VA+VECTOR_INSERT_CALLER_OFFSET==0x240CCFBE and
         SOURCE_VECTOR_INSERT_VA==0x24061220 and
         len(POST_WRITE_INSTRUCTION_OFFSETS)==32,
         "original prefix boundary or post-store census changed")
    need(not sha_ok(b"\x00"*CTOR_LEN,CTOR_LEN,CTOR_SHA),
         "fabricated 573B source original accepted")
    print("PASS Phase1730 original-free 398-byte boundary and SHA mutation preflight")


def main()->None:
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--client",type=Path,
                   help="private unchanged 2006 PE32 client.dll")
    p.add_argument("--self-test",action="store_true")
    args=p.parse_args()
    need(args.self_test or args.client is not None,
         "use --self-test or --client /private/client.dll")
    if args.self_test:self_test()
    if args.client is not None:verify_original(args.client.read_bytes())


if __name__=="__main__":
    try:main()
    except (ValueError,OSError) as exc:
        print(f"FAIL Phase1730: {exc}",file=sys.stderr)
        sys.exit(1)
