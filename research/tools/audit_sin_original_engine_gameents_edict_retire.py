#!/usr/bin/env python3
"""Read-only, SHA-pinned 2006 engine.dll edict retirement witness auditor.

--engine validates the owner's private engine bytes but exports metadata only.
--self-test constructs completely fabricated x86 witnesses without a DLL.
"""
import argparse
import hashlib
import json
from pathlib import Path
from audit_sin_original_engine_gameents_consumer import PE32, ENGINE_SHA, IMAGE_BASE

# Original engine virtual GameEnts +0x0C consumer, 30 and 92 source bytes.
# Only hashes and short evidence snippets are committed, never original code.
WINDOWS = (
    ("direct_free",0x20110e50,30,
     "fbe3675f5a7f441f9715872da4255c6a401e6f916c1222a7c8c8e5894e68a679"),
    ("indexed_free",0x20110e70,92,
     "66ff927d95b2ef135ac84d7802704b2020c17e10eb4fbe739a644300fa9a8509"),
)
# (window index, original VA, small byte witness, independently understood)
WITNESSES = (
    (0,0x20110e50,"8b0d88443e20","read GameEnts interface global"),
    (0,0x20110e56,"8326fd","clear edict flag bit 1"),
    (0,0x20110e5c,"ff500c","call GameEnts virtual +0x0c"),
    (0,0x20110e5f,"c7461000000000","clear edict trailing +0x10"),
    (0,0x20110e66,"c74604ffffffff","retire edict serial +0x04"),
    (1,0x20110e76,"3b05fcda7920","upper bound index check"),
    (1,0x20110e92,"8b0d04db7920","load owner edict base pointer"),
    (1,0x20110e98,"8d0480","multiply edict index by five"),
    (1,0x20110e9b,"8d3481","multiply by four for 0x14 stride"),
    (1,0x20110ea2,"d1eaf6c201","test edict state flag bit 1"),
    (1,0x20110ea9,"8b0d88443e20","read GameEnts interface global"),
    (1,0x20110eb6,"ff500c","call GameEnts virtual +0x0c"),
    (1,0x20110eb9,"c7461000000000","clear indexed edict trailing +0x10"),
    (1,0x20110ec0,"c74604ffffffff","retire indexed edict serial +0x04"),
)

def inspect_windows(blobs, strict):
    if len(blobs)!=len(WINDOWS):
        raise ValueError("two engine retirement windows required")
    for blob,(name,va,length,sha) in zip(blobs,WINDOWS):
        if len(blob)!=length:
            raise ValueError("engine edict witness window truncated: "+name)
        if strict and hashlib.sha256(blob).hexdigest()!=sha:
            raise ValueError("original engine retirement SHA mismatch: "+name)
    for window,va,raw,label in WITNESSES:
        start=WINDOWS[window][1]
        at=va-start
        pattern=bytes.fromhex(raw)
        if (at<0 or at+len(pattern)>len(blobs[window]) or
            blobs[window][at:at+len(pattern)]!=pattern):
            raise ValueError("engine edict opcode mismatch: "+label)
    return {
        "original_engine_edict_stride_bytes": 0x14,
        "original_engine_edict_flags_offset": 0,
        "original_engine_edict_serial_offset": 4,
        "original_engine_edict_next_offset": 0x10,
        "original_engine_free_before_serial_poison": True,
        "original_engine_GameEnts_virtual_offset": 0x0c,
        "original_engine_retire_witness_windows": len(blobs),
        "original_engine_retire_opcode_witnesses": len(WITNESSES),
        "full_original_window_hashes_verified": bool(strict),
        "original_bytes_exported": False,
        "retail_engine_boot_verified": False,
    }

def audit(data):
    if hashlib.sha256(data).hexdigest()!=ENGINE_SHA:
        raise ValueError("unexpected original 2006 engine.dll SHA")
    pe=PE32(data)
    if pe.base!=IMAGE_BASE:
        raise ValueError("unexpected original PE32 image base")
    blobs=[pe.raw(va,length) for name,va,length,sha in WINDOWS]
    out=inspect_windows(blobs,True)
    out["original_engine_whole_file_sha_verified"]=True
    return out

def self_test():
    windows=[bytearray(bytes((0xcc,))*length) for _,_,length,_ in WINDOWS]
    for index,va,raw,label in WITNESSES:
        at=va-WINDOWS[index][1]
        windows[index][at:at+len(bytes.fromhex(raw))]=bytes.fromhex(raw)
    inspect_windows(windows,False)
    for index,va,raw,label in WITNESSES:
        copy=[bytearray(blob) for blob in windows]
        copy[index][va-WINDOWS[index][1]]^=0x01
        try:
            inspect_windows(copy,False)
        except ValueError:
            pass
        else:
            raise AssertionError("corrupted x86 original-free witness accepted: "+label)
    for corrupt in ([windows[0][:-1],windows[1]],
                    [windows[0],windows[1][:-1]]):
        try:
            inspect_windows(corrupt,False)
        except ValueError:
            pass
        else:
            raise AssertionError("short original-free window accepted")
    try:
        inspect_windows(windows,True)
    except ValueError:
        pass
    else:
        raise AssertionError("fabricated byte stream accepted as original")
    print("PASS original-free 2006 engine GameEnts edict retirement witnesses and corruption tests")

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    mode=parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--self-test",action="store_true")
    mode.add_argument("--engine",type=Path)
    args=parser.parse_args()
    if args.self_test:
        self_test()
    else:
        print(json.dumps(audit(args.engine.read_bytes()),sort_keys=True,indent=2))

if __name__=="__main__":
    main()
