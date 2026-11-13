#!/usr/bin/env python3
"""Phase198: verify four full native SiN 2006 cache-tree rotation methods.

Strictly read-only owner-original PE32 audit. Never include commercial DLLs,
game assets or complete proprietary original x86 method bodies in the repo.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
from pathlib import Path
from audit_sin_original_server_post_emitter import PE32, VerificationError

SERVER_SHA = "568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650"
CLIENT_SHA = "6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d"
# (entry VA, complete original code length, complete method SHA256, node resolver)
REGIONS = {
    "scene_left": (0x221A6BB0, 237,
        "418e0affabf3ad24336f65de55a333ad822b4e486cfaeff9324dd44208b6ef93", 0x221A4D90),
    "scene_right": (0x221A6CA0, 240,
        "5c19981d1f75b91c07cd390555412d0861bbc9843c5d1a7ed90e6c780b05aa53", 0x221A4D90),
    "modelsounds_left": (0x221409B0, 237,
        "d4e2f2b7fdf594c9c344511b8627451dbef34d0937112b6c70d54033c9e94214", 0x2213E990),
    "modelsounds_right": (0x22140AA0, 240,
        "3b64fe5f8546d860f24d029ac9faf813a53adfd61b73c72719db8b8bb6a0e4ed", 0x2213E990),
}
# Each corresponding scene/model-sounds method is identical after zeroing
# only nine CALL rel32 displacement DWORDs. No normalization across rotation
# directions is permitted: left and right are distinct actual algorithms.
CALL_OFFSETS = {
    "left": (0x0B,0x17,0x36,0x45,0x62,0x82,0x8E,0x9B,0xB0),
    "right": (0x0B,0x16,0x35,0x45,0x63,0x83,0x8F,0x9D,0xB3),
}
NORMALIZED_SHA = {
    "left": "791ffa6910dfd63dcced4bb931e17c28c1288667106f27b808966cedef95eed9",
    "right": "ed7fc558c70d7259b710518c668610195b6e4335684cd746f991239c891f2717",
}
# These independently validated original opcode witnesses intentionally do
# NOT embed complete machine-code method bodies.
WITNESSES = {
    "left": {
        0x00:"53555657",0x04:"8b7c2414",0x10:"668b5802",
        0x1C:"668b08",0x1F:"0fb7c7",0x2E:"66894c2802",
        0x3B:"668138ffff",0x53:"66897cca04",0x58:"6681fbffff",
        0x74:"668944ca04",0x79:"663b7e10",0x93:"663938",
        0xAA:"66891cc1",0xBF:"66895cd002",0xC6:"66895e10",
        0xCA:"6681ffffff",0xD8:"66893cca",0xE1:"66895c2804",
        0xEA:"c20400",
    },
    "right": {
        0x00:"538b5c2408",0x10:"668b38",0x1B:"668b4002",
        0x2E:"66890429",0x3A:"66817802ffff",0x54:"66895cd004",
        0x59:"6681ffffff",0x75:"668944ca04",0x7A:"663b5e10",
        0x94:"66395802",0xAC:"66897cc102",0xC2:"66893cd0",
        0xC8:"66897e10",0xCC:"6681fbffff",0xDA:"66895cca02",
        0xE4:"66897c2804",0xED:"c20400",
    },
}
# The balancers in Phase197 call both rotation directions at these eight
# original CALL sites. Four calls per repository, all in server.dll.
INCOMING = {
    0x221A8F20+0x34C:0x221A6BB0,
    0x221A8F20+0x541:0x221A6BB0,
    0x221A8F20+0x38A:0x221A6CA0,
    0x221A8F20+0x503:0x221A6CA0,
    0x221429D0+0x34C:0x221409B0,
    0x221429D0+0x541:0x221409B0,
    0x221429D0+0x38A:0x22140AA0,
    0x221429D0+0x503:0x22140AA0,
}
# No post-ret alignment after the right methods; the next instructions
# belong to different functions and intentionally differ by repository.
FOLLOWING_RIGHT = {
    "scene_right":bytes.fromhex("56578b7c240c"),
    "modelsounds_right":bytes.fromhex("8b41048b4c24"),
}

def rel_call(site:int,destination:int)->bytes:
    return b"\xe8"+struct.pack("<i",destination-site-5)

def verify_native(pe:PE32, *, check_hash:bool=True)->dict:
    if pe.base!=0x22000000:
        raise VerificationError("Phase198 original server image base mismatch")
    normalized={}
    for name,(va,length,digest,resolver) in REGIONS.items():
        if pe.section(va)!=".text" or pe.section(resolver)!=".text":
            raise VerificationError("Phase198 expected original code section: "+name)
        direction=name.rsplit("_",1)[1]
        body=bytearray(pe.at(va,length))
        if check_hash and hashlib.sha256(body).hexdigest()!=digest:
            raise VerificationError("Phase198 complete original function hash mismatch: "+name)
        for off,hexcode in WITNESSES[direction].items():
            expected=bytes.fromhex(hexcode)
            if body[off:off+len(expected)]!=expected:
                raise VerificationError("Phase198 x86 witness mismatch at %#x"%(va+off))
        if body[-3:]!=b"\xc2\x04\x00":
            raise VerificationError("Phase198 native RET4 mismatch "+name)
        for off in CALL_OFFSETS[direction]:
            if body[off:off+5]!=rel_call(va+off,resolver):
                raise VerificationError("Phase198 native resolver CALL mismatch at %#x"%(va+off))
            body[off+1:off+5]=bytes(4)
        if direction=="left":
            if pe.at(va+length,3)!=b"\xcc"*3:
                raise VerificationError("Phase198 left function INT3 boundary mismatch")
        elif pe.at(va+length,6)!=FOLLOWING_RIGHT[name]:
            raise VerificationError("Phase198 right function next-entry boundary mismatch")
        normalized[name]=bytes(body)
    for src,dest in INCOMING.items():
        if pe.at(src,5)!=rel_call(src,dest):
            raise VerificationError("Phase198 original balancer incoming CALL mismatch at %#x"%src)
    for d in ("left","right"):
        if normalized["scene_"+d]!=normalized["modelsounds_"+d]:
            raise VerificationError("Phase198 corresponding native rotations differ beyond nine resolver calls: "+d)
        if check_hash and hashlib.sha256(normalized["scene_"+d]).hexdigest()!=NORMALIZED_SHA[d]:
            raise VerificationError("Phase198 normalized original method hash mismatch: "+d)
    return {
        "new_sha_verified_original_functions": 4 if check_hash else 0,
        "new_original_native_function_bytes": 954 if check_hash else 0,
        "complete_original_method_lengths": {name:reg[1] for name,reg in REGIONS.items()},
        "sparse_opcode_witnesses": 2*sum(len(w) for w in WITNESSES.values()),
        "exact_resolver_calls": sum(len(CALL_OFFSETS[name.rsplit("_",1)[1]]) for name in REGIONS),
        "exact_balancer_incoming_calls": len(INCOMING),
        "normalized_original_sha256": NORMALIZED_SHA if check_hash else None,
        "index_width_bits":16,"node_stride_bytes":24,"parent_word_offset":4,
        "root_word_offset":0x10,"original_stack_cleanup_bytes":4,
        "conclusions":[
            "Scene and model-sounds left rotation bodies normalize exactly",
            "Scene and model-sounds right rotation bodies normalize exactly",
            "Left/right parent, beta child and root rewrites have different code",
            "Key, payload and color bytes are not altered by rotation bodies",
        ],
        "limits":[
            "Native resolver calls and lazy static sentinel initialization remain separate",
            "Full Phase197 red-parent recoloring/fixup is not yet modeled",
            "Bounds and tree-consistency guards exist only in host-neutral tests",
            "No original executable-hosted SiN gameplay replacement DLL demonstrated",
        ]
    }

def audit(server:bytes,client:bytes)->dict:
    if hashlib.sha256(server).hexdigest()!=SERVER_SHA:
        raise VerificationError("Phase198 original full server.dll SHA256 mismatch")
    if hashlib.sha256(client).hexdigest()!=CLIENT_SHA:
        raise VerificationError("Phase198 original full client.dll SHA256 mismatch")
    return verify_native(PE32(server))

def main()->None:
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--server",required=True,type=Path)
    parser.add_argument("--client",required=True,type=Path)
    parser.add_argument("--output",type=Path)
    args=parser.parse_args()
    data=json.dumps(audit(args.server.read_bytes(),args.client.read_bytes()),indent=2)+"\n"
    if args.output:args.output.write_text(data,encoding="utf-8")
    else:print(data,end="")
if __name__=="__main__":
    main()
