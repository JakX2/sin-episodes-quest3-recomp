#!/usr/bin/env python3
"""SHA-pin original 2006 SiN client stat value setters without exporting code.

Read-only --client accepts ONLY owner-private original retail client.dll.
Public --self-test uses derived metadata, never loads proprietary binaries.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
from pathlib import Path
from audit_sin_original_stat_value_factory import (
    CASES, SHA256, raw, u32, LIVE_READER_SLOTS
)
# Original value-object vtable +0x18 primary setters and +0x2c
# secondary setters. Type 7 setter is NO-OP; its reader writes
# +0x0c directly after calling mark-dirty virtual +0x48.
PRIMARY=(
    0x242053E0,0x24205490,0x242053E0,0x24205490,
    0x24205490,0x24205490,0x24205490,0x24205760,
    0x242057F0,0x242057F0,0x242057F0,0x242057F0,
    0x24205930,0x242059E0,0x24205930,0x242059E0
)
SECONDARY=(
    0x242052D0,0x242052D0,0x24205540,0x242055D0,
    0x242055D0,0x242055D0,0x242055D0,0x242052D0,
    0x242052D0,0x242052D0,0x242052D0,0x24205890,
    0x242052D0,0x242052D0,0x24205AB0,0x24205B60
)
DIRTY=0x24205340
# Virtual CStatReader method dispatch for the SAME original class
# audited in audit_sin_original_stat_value_factory.py. Pattern shapes:
# call +0x08, +0x18 or both +0x18/+0x2c on target value object,
# and one exceptional direct +0x0c assignment after dirty mark.
READER_CALLS={
    0x04:(bytes.fromhex("ff 57 18"),),
    0x08:(bytes.fromhex("ff 57 18"),),
    0x0c:(bytes.fromhex("ff 57 18"),),
    0x10:(bytes.fromhex("ff 53 18"),bytes.fromhex("ff 53 2c")),
    0x14:(bytes.fromhex("ff 53 18"),bytes.fromhex("ff 53 2c")),
    0x18:(bytes.fromhex("ff 57 18"),),
    0x1c:(bytes.fromhex("ff 50 48"),bytes.fromhex("89 4e 0c")),
    0x20:(bytes.fromhex("ff 57 08"),),
    0x24:(bytes.fromhex("ff 57 08"),),
}
# Complete method spans include all instructions and padding up to the
# next original entry boundary. Only SHA hashes, never method bytes,
# are shared publicly.
BODY_SHA={
    "Int32PrimarySetter":(0x242053E0,0x2420540D,
        "cfcefb03b8e6dfe83c587d5c0ad37135602a140cabab80ef599737006e99b5fa"),
    "FloatPrimarySetter":(0x24205490,0x242054A3,
        "38b32dea910b18bedb14ddd8c96d628f926c8084e25cf7dacd169ca4494a1780"),
    "Int32SecondarySetter":(0x24205540,0x2420556D,
        "9e045408fe7fe5d3ceeb2a6e3d6c5cd3b7194285779c8572d6a67dee9f3af7d6"),
    "FloatSecondarySetter":(0x242055D0,0x242055E3,
        "d6863daa6bbc72d56335ca96c9dc3d2d59f866c61e83e974a02aa4a4b6019d8a"),
    "Type7DirectFloatReader":(0x24203B10,0x24203B35,
        "63d2d48bf000c8f499f62fae2bd65ecec3ea0a5192b918456a4a8d1fb96daf7d"),
    "FloatAuxSetter":(0x242057F0,0x24205803,
        "f551c724e24a95d7c988daab991ba614d9cb05d474d959d2321dfa2807beef8a"),
    "FloatMaxSetter":(0x24205930,0x24205951,
        "d6ead82cb208e864411d238450d25be68833c19c03dd4cff020b8a57d833329c"),
    "FloatMinSetter":(0x242059E0,0x24205A01,
        "936ab0cff6fc0be19739837b4bd820b644ab6e3a33b7b920491e2e13bf4441ed"),
    "Type7NoopSetter":(0x24205760,0x24205763,
        "e598d0c3ba86d917b177d7adde0556aa99bc355543c57aee0a3e50b684dd7e99"),
    "VirtualPrimaryForwarder":(0x24205270,0x24205275,
        "2578bf9fcbe58f262edb99b7e854a146a3118fe35e444aebf1becf2d9fc62e63"),
    "SetDirty":(0x24205340,0x24205345,
        "827582cade658b84ec53c96cffaccf772541754dade08b535cd1924a838d89c5"),
    "ScaledAverageCtor":(0x242057A0,0x242057CC,
        "ba6ac437bd9b64db4c095d8f76d1e42837f07451254afdae0663717caa66ddbd"),
    "FloatMaxCtor":(0x242058D0,0x242058FB,
        "1082be34d37408c0c8168f58a412c400ab350a523b94456db4d3950cf9e4ac4c"),
    "FloatMinCtor":(0x24205980,0x242059AB,
        "e792ee1b19be7de016e2200c31ac26e7215adf6f668d56a9e332f2bfcaaa3ff4"),
    "MaxWithAuxCtor":(0x24205A30,0x24205A6B,
        "4f350f601eb3016b8ad5d428dac8d776f89f1396fb931f9d7467014c3c09b46a"),
    "MinWithAuxCtor":(0x24205AE0,0x24205B1B,
        "3f7858714a55517beae1918fa11a0449ad6b3c98eaca9606ce3722e88931ab8a"),
}

def verify_metadata()->None:
    assert len(PRIMARY)==len(SECONDARY)==len(CASES)==16
    assert len(READER_CALLS)==len(LIVE_READER_SLOTS)==9
    assert set(READER_CALLS)=={row[0] for row in LIVE_READER_SLOTS}
    assert PRIMARY[7]==0x24205760
    assert PRIMARY[12]==PRIMARY[14]
    assert PRIMARY[13]==PRIMARY[15]
    assert SECONDARY[2]==0x24205540
    assert SECONDARY[3]==0x242055D0
    assert len(BODY_SHA)>=12
    assert all(end>start and len(digest)==64
               for start,end,digest in BODY_SHA.values())

def audit_private_original(data:bytes)->dict:
    verify_metadata()
    if hashlib.sha256(data).hexdigest()!=SHA256:
        raise ValueError("retail client.dll SHA mismatch")
    for key,(start,end,expected) in BODY_SHA.items():
        if hashlib.sha256(raw(data,start,end-start)).hexdigest()!=expected:
            raise ValueError("original mutator SHA changed: "+key)
    for t,row in enumerate(CASES):
        vtable=row[3]
        if u32(data,vtable+0x18)!=PRIMARY[t]:
            raise ValueError("original setter +0x18 differs for factory "+str(t))
        if u32(data,vtable+0x2c)!=SECONDARY[t]:
            raise ValueError("original paired +0x2c differs for factory "+str(t))
        if u32(data,vtable+0x48)!=DIRTY:
            raise ValueError("original dirty flag virtual differs")
    for slot,start,end,_,_ in LIVE_READER_SLOTS:
        code=raw(data,start,end-start)
        for opcode in READER_CALLS[slot]:
            if code.count(opcode)!=1:
                raise ValueError("live reader value update routing changed: "+hex(slot))
    # Prove that the integer setter uses x87 control word
    # RC=11 toward-zero and FISTP int32, not C++ cast of raw bits.
    for addr in (0x242053E0,0x24205540):
        code=raw(data,addr,48)
        if b"\x80\xcc\x0c" not in code or b"\xdb\x5e" not in code:
            raise ValueError("integer narrowing path diverged")
    # Confirm the original extrema compare gates are distinct.
    if b"\xf6\xc4\x41" not in raw(data,0x24205930,33):
        raise ValueError("float max strict-order compare changed")
    if b"\xf6\xc4\x05" not in raw(data,0x242059E0,33):
        raise ValueError("float min strict-order compare changed")
    return {
        "source":"SHA-verified SiN 2006 original retail client.dll",
        "owner_original_sha256":SHA256,
        "factory_types":len(CASES),
        "mutator_method_sha_regions":len(BODY_SHA),
        "live_reader_method_routes":len(READER_CALLS),
        "primary_setters_by_type":[hex(x) for x in PRIMARY],
        "secondary_setters_by_type":[hex(x) for x in SECONDARY],
        "type7_special":"reader writes float field +0x0c directly; normal +0x18 setter is no-op",
        "extrema":"types 12/14 strict max; 13/15 strict min",
        "unsafe_original_calls":"none performed"
    }

def self_test()->None:
    verify_metadata()
    first=bytes.fromhex("ff 57 18")
    assert READER_CALLS[0x0c]==(first,)
    assert READER_CALLS[0x14][1]==bytes.fromhex("ff 53 2c")
    assert BODY_SHA["SetDirty"][0]==0x24205340
    assert BODY_SHA["Type7NoopSetter"][1]-BODY_SHA["Type7NoopSetter"][0]==3
    print("PASS original-free 16-type setter and 9 CStatReader mutation-route fixtures")

def main()->None:
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--client",type=Path,
                        help="private, SHA-pinned original 2006 client DLL")
    parser.add_argument("--self-test",action="store_true")
    args=parser.parse_args()
    if args.self_test:self_test()
    elif args.client:
        print(json.dumps(audit_private_original(args.client.read_bytes()),indent=2))
    else:parser.error("specify --self-test or owner-private --client")
if __name__=="__main__":
    main()
