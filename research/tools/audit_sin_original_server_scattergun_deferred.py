#!/usr/bin/env python3
"""Phase166: original SiN 2006 server ScatterGun deferred timer auditor.

Owner-only validation of BOTH original PE32 DLL checksums through Phase165,
one complete 240-byte CWeaponScatterGun handler, three complete tracked-float
setters, 59 independent sparse opcode anchors, six original direct CALLs,
and the incoming Phase165 tail JMP. Emits metadata only, no DLL code.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
from pathlib import Path

from audit_sin_original_server_scattergun_ammo import audit as phase165_audit
from verify_gameplay_entity_registry import PEImage, VerificationError

REGIONS={
    "scattergun_deferred_pending_dispatch":(
        0x2231C2F0,240,
        "a1150899b9ecb8e31f9c5b93d749cc6c93abe7529fc5a2ccdbf0f6749efc8916"),
    "player_tracked_float_setter":(
        0x2203D190,64,
        "afa46015b3a6f73bdab83b3095e84c7e0fe6aa42419e92cc5c5529c6dbf87357"),
    "scattergun_gate_timer_float_setter":(
        0x22173260,64,
        "d95240a043eb30317c36583e375a0cadcde6bc95f8856e5ccbdacb635ec7f8c3"),
    "scattergun_mirrored_timer_float_setter":(
        0x221732A0,64,
        "a0544a00be1504ca79951ac0e73096c194eab280690c5359884646a68f84819e"),
}
# Disjoint, intentionally partial x86 code witnesses. The four proprietary
# function bodies are not embedded: full-region SHA256 requires owner binaries.
OPCODES={
 0x2231C2F0:"51",0x2231C2F4:"80be8905000000",
 0x2231C2FB:"0f84dc000000",0x2231C301:"a190215722",
 0x2231C306:"d9869c040000",0x2231C30C:"d8580c",
 0x2231C316:"dfe0",0x2231C318:"f6c441",
 0x2231C31B:"0f84bb000000",0x2231C321:"83beec04000000",
 0x2231C328:"0f8eae000000",0x2231C32E:"80beac04000000",
 0x2231C335:"0f85a1000000",0x2231C343:"85ff",
 0x2231C34D:"6a00",0x2231C34F:"6a0d",
 0x2231C353:"c6868905000000",0x2231C35A:"ff92c4030000",
 0x2231C360:"80be7805000000",0x2231C369:"68f0aa4722",
 0x2231C377:"b826010000",0x2231C381:"ff9214030000",
 0x2231C387:"8b86ec030000",0x2231C395:"a190215722",
 0x2231C39A:"d8400c",0x2231C3A1:"81c70c060000",
 0x2231C3B3:"8b17",0x2231C3BA:"8bcb",
 0x2231C3C5:"8b0f",0x2231C3D0:"8d8ea0040000",
 0x2231C3DF:"c3",
 0x2203D190:"8b542404",0x2203D198:"dae9",
 0x2203D19C:"f6c444",0x2203D19F:"7b2a",
 0x2203D1A1:"80b9e4fbffff00",0x2203D1AA:"c681f4fbffff01",
 0x2203D1BA:"8b8160fbffff",0x2203D1C4:"830801",
 0x2203D1C7:"8b02",0x2203D1CD:"c20400",
 0x22173260:"8b542404",0x2217326A:"dfe0",
 0x2217326C:"f6c444",0x2217326F:"7b2a",
 0x22173271:"80b954fdffff00",0x2217327A:"c68164fdffff01",
 0x2217328A:"8b81d0fcffff",0x22173294:"830801",
 0x2217329D:"c20400",
 0x221732A0:"8b542404",0x221732AA:"dfe0",
 0x221732AC:"f6c444",0x221732AF:"7b2a",
 0x221732B1:"80b950fdffff00",0x221732BA:"c68160fdffff01",
 0x221732CA:"8b81ccfcffff",0x221732D4:"830801",
 0x221732DD:"c20400",
}
CALLS={
 0x2231C33C:0x222AD370,  # acquire owner/player
 0x2231C370:0x220ECB40,  # tracked selector resolver
 0x2231C390:0x220EEDE0,  # timing interval provider
 0x2231C3AE:0x2203D190,  # owner +0x60C tracked float
 0x2231C3C0:0x22173260,  # weapon +0x49C tracked float
 0x2231C3D6:0x221732A0,  # weapon +0x4A0 tracked float
}
TAIL_JUMPS={
 0x2231C441:0x2231C2F0,  # original Phase165 virtual update dispatch
}

def verify_original_deferred(pe:PEImage,*,check_hashes=True)->dict:
    if pe.base!=0x22000000:
        raise VerificationError("original server PE32 image base mismatch")
    for address,opcode_hex in OPCODES.items():
        expected=bytes.fromhex(opcode_hex)
        if pe.at(address,len(expected))!=expected:
            raise VerificationError("original ScatterGun deferred opcode mismatch at %#x"%address)
    for instructions,opcode in ((CALLS,0xE8),(TAIL_JUMPS,0xE9)):
        for site,target in instructions.items():
            expected=bytes((opcode,))+struct.pack("<i",target-site-5)
            if pe.at(site,5)!=expected:
                raise VerificationError("original ScatterGun deferred relative branch mismatch at %#x"%site)
    if check_hashes:
        for label,(start,length,wanted) in REGIONS.items():
            if hashlib.sha256(pe.at(start,length)).hexdigest()!=wanted:
                raise VerificationError("original ScatterGun deferred whole-function SHA mismatch: "+label)
    return {
      "original_handler_entry":"0x2231C2F0",
      "original_handler_native_bytes":240,
      "verified_complete_original_function_hashes":len(REGIONS) if check_hashes else 0,
      "verified_original_opcode_witnesses":len(OPCODES),
      "original_direct_call_edges":len(CALLS),
      "original_incoming_tail_jump_edges":len(TAIL_JUMPS),
      "tracked_float_setter_addresses":[
          "0x2203D190","0x22173260","0x221732A0"],
      "original_timer_guard":[
          "pending byte this+0x589 must be nonzero",
          "if float(this+0x49C) > clock(+0xC), return; unordered is treated as due",
          "signed 32-bit primary clip this+0x4EC must be >0",
          "blocking byte this+0x4AC must equal 0",
          "player getter 0x222AD370 must return a nonnull pointer",
      ],
      "original_dispatch_order":[
          "clear pending byte+0x589 BEFORE virtual+0x3C4 with (13,0)",
          "tracking byte+0x578 chooses result of 0x220ECB40 or literal 0x126",
          "call virtual+0x314 with selected 32-bit ID",
          "call 0x220EEDE0 with this+0x3EC, add freshly read clock",
          "set owner/player float+0x60C via 0x2203D190",
          "mirror resulting owner value to weapon+0x49C and weapon+0x4A0",
      ],
      "shared_float_setter_rule":[
          "skip update when float32 values compare numerically equal, including +0 vs -0",
          "NaN/unordered compare enters the update path even if bits are equal",
          "if owner mode byte+0x1F0 nonzero set dirty byte+0x200",
          "otherwise OR bit0 into nonnull pointer stored at owner+0x16C",
          "store new float32 value after notification",
      ],
      "limitations":[
          "Original vtable +0x3C4/+0x314 callbacks and selector/timing providers remain externally modeled.",
          "This is a portable branch/state model, not the original MSVC x86 game object ABI.",
          "Full secondary-fire routine 0x2231C810 and projectile behavior remain unresolved.",
      ],
    }

def audit(client_bytes:bytes,server_bytes:bytes)->dict:
    result=phase165_audit(client_bytes,server_bytes)
    result["original_server_scattergun_deferred"]=verify_original_deferred(
        PEImage(server_bytes))
    return result

def main()->None:
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--client",type=Path,required=True)
    parser.add_argument("--server",type=Path,required=True)
    parser.add_argument("--output",type=Path)
    args=parser.parse_args()
    result=json.dumps(audit(args.client.read_bytes(),args.server.read_bytes()),indent=2)+"\n"
    if args.output:
        args.output.parent.mkdir(parents=True,exist_ok=True)
        args.output.write_text(result,encoding="utf-8")
    else:
        print(result,end="")

if __name__=="__main__":
    main()
