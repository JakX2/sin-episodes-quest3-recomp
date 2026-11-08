#!/usr/bin/env python3
"""Read-only binary witness audit of 2006 SiN parent-aware absolute velocity.

Requires the owner's unmodified original server.dll. Never embeds or prints
original DLL bytes, proprietary source or raw disassembly.
"""
import argparse
import hashlib
import json
from pathlib import Path
from audit_sin_original_jessica_throw import PE32
SERVER_SHA="568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650"
HASHES={
 "set_absolute_velocity":(0x22141A00,0x22141B33,"c5a9e2bb9cef0f468373a3a7205a17a1fefe297c1ddcbcebc140349c873d5c4f"),
 "refresh_absolute_velocity":(0x2213D750,0x2213D85B,"561a2275fbbc974ac2ea6a370721727b5438d6373acade32828886f73db79c0f"),
 "inverse_rotate_vector":(0x22268E90,0x22268EE3,"3e2cad90453193df6cc81e111c572a0e405710ff8d7f2dbc6b71b213683f3919"),
 "rotate_vector":(0x22268C80,0x22268CD3,"46dd34f70a52e5260a3da8f5a28f32171ae7cb6b295532f9cacc811c85d515d1"),
 "set_local_vector":(0x2213E1A0,0x2213E1FC,"420c7f56ea985be90c76930062edaa940fe207273477fd065be9a17de42d9d2d"),
 "invalidate_velocity_dependencies":(0x2214B620,0x2214B725,"eb873028895c799af7fcb9f012da1ca146fba246bcc983815843b3c27abe90f0"),
}
CALLS={
 0x22141A49:0x2214B620, 0x22141AA4:0x2213E1A0,
 0x22141AC1:0x2213D750, 0x22141AFC:0x2213FA60,
 0x22141B12:0x22268E90, 0x22141B25:0x2213E1A0,
 0x2213D7FE:0x2213FA60, 0x2213D816:0x22268C80,
 0x2213D82E:0x2213D750, 0x2214BAC4:0x22141A00,
}
ANCHORS={
 0x22141A0B:"d98650020000",     # cached abs +0x250
 0x22141A45:"6a04",             # mask 4 invalidation
 0x22141A4E:"80a6b9000000ef",   # clear abs dirty bit12
 0x22141A6F:"8b86f0000000",     # parent EHANDLE +0xF0
 0x22141A9E:"8d8ec8020000",     # local velocity +0x2C8
 0x22141AB2:"8b87b8000000",     # parent flags +0xB8
 0x22141AC6:"d903d8a750020000", # subtract parent abs
 0x22141B06:"81c768020000",     # parent matrix +0x268
 0x2213D756:"8b87b8000000",     # abs dirty bit
 0x2213D76A:"80e4ef",           # clear dirty
 0x2213D7A2:"8d97c8020000",     # unparented local velocity
 0x2213D803:"81c750020000",     # child's absolute
 0x2213D80A:"8d9668020000",     # parent matrix
 0x2213E1DF:"ff907c020000",     # local setter virtual +0x27C
 0x2214B629:"f6c304",           # invalidate flag 4
}
def verify(data):
    sha=hashlib.sha256(data).hexdigest()
    if sha!=SERVER_SHA:
        raise ValueError("original server.dll SHA-256 mismatch")
    pe=PE32(data)
    if pe.base!=0x22000000:
        raise ValueError("original image base mismatch")
    for name,(first,last,digest) in HASHES.items():
        if hashlib.sha256(pe.read(first,last-first)).hexdigest()!=digest:
            raise ValueError("original function changed: "+name)
    for addr,target in CALLS.items():
        if pe.branch_target(addr,0xE8)!=target:
            raise ValueError("relative call target mismatch: "+hex(addr))
    for addr,encoded in ANCHORS.items():
        b=bytes.fromhex(encoded)
        if pe.read(addr,len(b))!=b:
            raise ValueError("instruction witness mismatch: "+hex(addr))
    return {"original_server_sha256":sha,"complete_function_hashes":len(HASHES),
            "relative_calls":len(CALLS),"instruction_anchors":len(ANCHORS),
            "absolute_velocity_offset":"0x250","local_velocity_offset":"0x2C8",
            "parent_handle_offset":"0xF0","matrix_offset":"0x268",
            "invalidation_mask":4,"original_binary_embedded":False}
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--server",required=True,type=Path)
    args=p.parse_args()
    try:print(json.dumps(verify(args.server.read_bytes()),indent=2))
    except (OSError,ValueError) as ex:p.exit(1,"FAIL: "+str(ex)+"\n")
if __name__=="__main__":main()
