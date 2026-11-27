#!/usr/bin/env python3
"""Read-only SHA-pinned 2006 engine.dll edict reuse witnesses.

--engine inspects the owner's private PE32 file; --self-test uses only
invented instruction windows. This does not claim live engine ownership.
"""
import argparse
import hashlib
import json
from pathlib import Path
from audit_sin_original_engine_gameents_consumer import PE32,ENGINE_SHA,IMAGE_BASE

# Each independent fingerprint covers original file-backed instruction bytes.
WINDOWS=(
    ("reuse_transition",0x20110fb0,89,
     "469c928666877faddded91ca47d71fe3439f9270dfb9f2962f621a04833303e1"),
    ("indexed_edict_lookup",0x20111020,39,
     "aab72caf36338a6845884f0ed865ee4486677bf51bc479248f5d45daf1d1bfdc"),
    ("allocator_age_scan",0x20110ef0,151,
     "567f15c192da14cbbda102dcbd1099f90b947d192fc208ec814fb106cac7c0ee"),
    ("negative_index_select",0x20110ecc,220,
     "cae5a58071ef8531ac82c3d000dfce4c68a09d70c4d2eb20887f4e703ae3427b"),
)
# (window index, original VA, local short witness, semantic interpretation)
WITNESSES=(
    (0,0x20110fb1,"8b06d1e8a801","active bit 0x02 gate"),
    (0,0x20110fb7,"754e","already-active early return"),
    (0,0x20110fbb,"2b0d04db7920","subtract edict array base"),
    (0,0x20110fc1,"b867666666f7e9","signed divide by 20 index"),
    (0,0x20110fd2,"83f801","index below one allowed"),
    (0,0x20110fd7,"3b0564da7920","protected edict index upper bound"),
    (0,0x20110fdf,"8b0d88443e20","read original GameEnts interface"),
    (0,0x20110fe8,"ff520c","call GameEnts FreeContainingEntity +0x0c"),
    (0,0x20110feb,"830e02","set edict active bit 0x02"),
    (0,0x20110fee,"db056ceb7620","load signed 32-bit engine tick as x87"),
    (0,0x20110ffa,"d80d6cda7920","scale float owner time"),
    (0,0x20111000,"83460401","increment per-edict retail serial"),
    (0,0x20111004,"d95e10","store x87 float reuse timestamp at +0x10"),
    (1,0x20111020,"85f6","index sign gate"),
    (1,0x20111024,"3b3500db7920","compare against edict capacity"),
    (1,0x2011103a,"8b0d04db7920","load edict array owner base"),
    (1,0x20111040,"8d04b6","index multiplied by five"),
    (1,0x20111043,"8d0481","index multiplied by four: stride 0x14"),
    (2,0x20110ef8,"d94610","read reuse-time float from +0x10 during scan"),
    (2,0x20110f08,"db056ceb7620","age scan signed tick source"),
    (2,0x20110f16,"d80d6cda7920","age scan time scaling"),
    (2,0x20110f2f,"83c614","next edict record by 0x14"),
    (3,0x20110ecc,"a164da7920","protected last edict index"),
    (3,0x20110ed1,"8b1504db7920","original edict array base"),
    (3,0x20110ede,"8b15fcda7920","current used edict count"),
    (3,0x20110efb,"d81d40bf2d20","x87 compare against 2.0"),
    (3,0x20110f03,"f6c405","x87 status parity for 2.0 threshold"),
    (3,0x20110f1f,"dc1d60bf2d20","x87 compare elapsed against 1.0"),
    (3,0x20110f27,"f6c401","x87 elapsed status test"),
    (3,0x20110f36,"a100db7920","current edict capacity"),
    (3,0x20110f7a,"8305fcda792001","increase count on append"),
)

def inspect_windows(blobs,strict=False):
    if len(blobs)!=len(WINDOWS):
        raise ValueError("missing authenticated engine edict windows")
    for blob,(name,va,size,sha) in zip(blobs,WINDOWS):
        if len(blob)!=size:
            raise ValueError("original allocator window truncated: "+name)
        if strict and hashlib.sha256(blob).hexdigest()!=sha:
            raise ValueError("whole instruction window hash mismatch: "+name)
    for which,va,code,label in WITNESSES:
        start=WINDOWS[which][1]
        at=va-start
        raw=bytes.fromhex(code)
        if at<0 or at+len(raw)>len(blobs[which]) or \
           blobs[which][at:at+len(raw)]!=raw:
            raise ValueError("original edict reuse opcode mismatch: "+label)
    return {
        "original_engine_edict_reuse_windows":len(WINDOWS),
        "original_engine_edict_reuse_opcode_witnesses":len(WITNESSES),
        "original_engine_stride_bytes":0x14,
        "original_engine_reserved_range_includes_index_one":True,
        "original_engine_gameents_cleanup_before_reuse_stamping":True,
        "original_engine_reuse_serial_increment_is_32bit":True,
        "original_engine_reuse_time_field_offset":0x10,
        "full_original_window_sha_verified":bool(strict),
        "live_engine_allocation_owner_verified":False,
        "original_code_exported":False,
    }

def inspect_age_constants(early,elapsed):
    if early!=bytes.fromhex("00000040") or elapsed!=bytes.fromhex("000000000000f03f"):
        raise ValueError("original reuse-age thresholds are not 2.0f and 1.0d")
    return True

def audit(data):
    if hashlib.sha256(data).hexdigest()!=ENGINE_SHA:
        raise ValueError("unexpected original 2006 engine.dll SHA")
    pe=PE32(data)
    if pe.base!=IMAGE_BASE:
        raise ValueError("unexpected original engine image base")
    blobs=[pe.raw(va,size) for _,va,size,_ in WINDOWS]
    result=inspect_windows(blobs,True)
    inspect_age_constants(pe.raw(0x202dbf40,4),pe.raw(0x202dbf60,8))
    result["original_engine_reuse_age_float_threshold"]=2.0
    result["original_engine_reuse_elapsed_double_threshold"]=1.0
    result["whole_engine_sha_verified"]=True
    return result

def self_test():
    blobs=[bytearray(b"\xcc"*size) for _,_,size,_ in WINDOWS]
    for index,va,code,label in WITNESSES:
        at=va-WINDOWS[index][1]
        raw=bytes.fromhex(code)
        blobs[index][at:at+len(raw)]=raw
    inspect_windows(blobs,False)
    inspect_age_constants(bytes.fromhex("00000040"),bytes.fromhex("000000000000f03f"))
    for incorrect in ((b"\0"*4,bytes.fromhex("000000000000f03f")),
                      (bytes.fromhex("00000040"),b"\0"*8)):
        try:inspect_age_constants(*incorrect)
        except ValueError:pass
        else:raise AssertionError("mutated age constants accepted")
    for index,va,code,label in WITNESSES:
        changed=[bytearray(blob) for blob in blobs]
        changed[index][va-WINDOWS[index][1]]^=1
        try:inspect_windows(changed,False)
        except ValueError:pass
        else:raise AssertionError("corrupt witness accepted: "+label)
    for i in range(len(blobs)):
        shorter=[bytearray(blob) for blob in blobs]
        shorter[i]=shorter[i][:-1]
        try:inspect_windows(shorter,False)
        except ValueError:pass
        else:raise AssertionError("truncated original window accepted")
    try:inspect_windows(blobs,True)
    except ValueError:pass
    else:raise AssertionError("synthetic bytes passed original SHA")
    print(f"PASS original-free 2006 engine edict reuse/selection: {len(WITNESSES)} opcode witnesses, {len(WINDOWS)} SHA windows, original 2.0/1.0 threshold literals and corruption tests")

def main():
    p=argparse.ArgumentParser(description=__doc__)
    a=p.add_mutually_exclusive_group(required=True)
    a.add_argument("--self-test",action="store_true")
    a.add_argument("--engine",type=Path)
    args=p.parse_args()
    if args.self_test:self_test()
    else:print(json.dumps(audit(args.engine.read_bytes()),indent=2,sort_keys=True))

if __name__=="__main__":main()
