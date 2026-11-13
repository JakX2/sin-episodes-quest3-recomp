#!/usr/bin/env python3
"""Phase212: entire-original SHA256 audit of SiN 2006 client record router.

Validates the full 1036-byte x86 function, its 13-DWORD inline jump table,
27-byte type selector, 39 direct native CALL rel32 edges and real Warning
import. Distributes NO original DLL/code body. The helper functions reached
by the cases are NOT claimed to be reconstructed by this phase.
"""
from __future__ import annotations
import argparse,hashlib,json,struct
from pathlib import Path
from audit_sin_original_pe_imports import PEImage,VerificationError
from audit_sin_original_cross_dll_tier0_allocator import FULL_SHA,BASE
from audit_sin_original_allocator_game_callers import named_iat

ENTRY=0x241996f0
SIZE=1036
SHA="71406e900eef6ba6d8abd37ab73f8730c71c35d95a8700a8cb4ab7c9a2b6a7a6"
CASE_TABLE_VA=0x24199afc
SELECTOR_VA=0x24199b30
FOLLOWING_VA=0x24199b50
WARNING_STRING_VA=0x242a41e0
WARNING_IAT_VA=0x242761e0
WARNING="Bad field type\n"
CASE_DESTINATIONS=(
 0x24199ad5,0x24199847,0x2419987f,0x241998ad,
 0x241998e7,0x241999b7,0x2419997d,0x241999ef,
 0x24199a29,0x24199921,0x241997ce,0x24199a8e,
 0x24199ac7,
)
TYPE_CASES=(
 0,1,2,3,4,5,6,7,8,9,10,0,0,11,
 0,0,0,0,0,0,12,0,12,12,12,12,0,
)
# A SAMPLE of independently inspected complete instruction witnesses,
# disjoint from the 39 call rel32 sites. Whole-function SHA is the stronger
# evidence. Native opcodes remain sparse: no proprietary full-body dump.
WITNESSES={
 0x000:"8b442408", 0x00a:"837d1c00",
 0x01d:"85c0",0x040:"8d51d8",0x046:"0fbf51ea",
 0x06f:"83380a",0x074:"f6c602",0x080:"83f901",
 0x085:"84f1",0x08d:"83f902",0x092:"f6c601",
 0x0d0:"0fb689309b1924",0x0d7:"ff248dfc9a1924",
 0x256:"c1e902",0x25b:"f3a5",0x262:"f3a4",
 0x3d7:"68e0412a24",0x3dc:"ff15e0612724",
 0x3e5:"8b4c2410",0x3ec:"836c241401",0x409:"c21000",
}
# Exact original 39 native instruction-boundary E8-relative calls inside
# the body; entries are (call VA, concrete native destination VA). The first
# is recursive, and the others are opaque original specialist routines.
CALL_EDGES=(
 (0x24199825,0x241996f0),
 (0x2419984c,0x24198aa0),(0x2419985b,0x24198a70),
 (0x24199866,0x24197ca0),(0x24199875,0x24198470),
 (0x24199883,0x24198b80),(0x2419988f,0x24198b40),
 (0x24199899,0x24197d00),(0x241998a3,0x241984a0),
 (0x241998b2,0x24198e20),(0x241998c3,0x24198da0),
 (0x241998ce,0x241985a0),(0x241998dd,0x241986c0),
 (0x241998ec,0x24198f50),(0x241998fd,0x24198de0),
 (0x24199908,0x241987c0),(0x24199917,0x241989a0),
 (0x2419992e,0x24197a90),(0x24199964,0x24197ad0),
 (0x24199973,0x24198360),(0x24199982,0x24197e30),
 (0x24199993,0x24198a40),(0x2419999e,0x24197c30),
 (0x241999ad,0x24198440),(0x241999bc,0x24197db0),
 (0x241999cb,0x24198a10),(0x241999d6,0x24197b60),
 (0x241999e5,0x241983b0),(0x241999f4,0x24197d50),
 (0x24199a05,0x241989e0),(0x24199a10,0x24197b10),
 (0x24199a1f,0x24198380),(0x24199a2e,0x24197a90),
 (0x24199a78,0x24197b60),(0x24199a87,0x24198360),
 (0x24199a93,0x24199060),(0x24199aa2,0x24197e90),
 (0x24199ab1,0x241994c0),(0x24199ac0,0x241995e0),
)

def rel_call(site:int,dest:int)->bytes:
    return b"\xe8"+struct.pack("<i",dest-site-5)

def verify_native(client:PEImage,*,check_hash=True,check_imports=True,
                  check_tables=True)->dict:
    if client.base!=BASE["client"]:
        raise VerificationError("Phase212 original 2006 client.dll base changed")
    if client.section_of(ENTRY)!=".text":
        raise VerificationError("Phase212 entry not in original .text")
    body=client.at(ENTRY,SIZE)
    if check_hash and hashlib.sha256(body).hexdigest()!=SHA:
        raise VerificationError("Phase212 original complete 1036-byte function SHA mismatch")
    if client.at(ENTRY+SIZE,1)!=struct.pack("<B",CASE_DESTINATIONS[0]&0xff):
        # Inline jump table immediately follows RET; never mistake it for
        # a new function's INT3 padding.
        raise VerificationError("Phase212 original jump table boundary changed")
    if client.section_of(CASE_TABLE_VA)!=".text" or \
       client.section_of(SELECTOR_VA)!=".text" or \
       client.section_of(FOLLOWING_VA)!=".text":
        raise VerificationError("Phase212 original embedded table memory section changed")
    for off,hx in WITNESSES.items():
        code=bytes.fromhex(hx)
        if body[off:off+len(code)]!=code:
            raise VerificationError("Phase212 original opcode mismatch +%#x"%off)
    for site,dest in CALL_EDGES:
        if not ENTRY<=site<ENTRY+SIZE or client.section_of(dest)!=".text":
            raise VerificationError("Phase212 native CALL/callee outside original .text")
        if client.at(site,5)!=rel_call(site,dest):
            raise VerificationError("Phase212 original CALL rel32 mismatch "+hex(site))
    if check_tables:
        decoded=struct.unpack("<13I",client.at(CASE_TABLE_VA,13*4))
        if decoded!=CASE_DESTINATIONS:
            raise VerificationError("Phase212 complete 13-pointer original jump table changed")
        if tuple(client.at(SELECTOR_VA,27))!=TYPE_CASES:
            raise VerificationError("Phase212 complete 27-byte original type selector changed")
        if any(dst<ENTRY or dst>=ENTRY+SIZE for dst in decoded) or \
           any(index>=len(decoded) for index in TYPE_CASES):
            raise VerificationError("Phase212 invalid native branch selector bounds")
        if client.at(SELECTOR_VA+27,5)!=b"\xcc"*5:
            raise VerificationError("Phase212 original inline table/INT3 boundary changed")
    if client.section_of(WARNING_STRING_VA)!=".rdata" or \
       client.at(WARNING_STRING_VA,len(WARNING)+1)!=WARNING.encode()+b"\0":
        raise VerificationError("Phase212 original Bad field type warning literal changed")
    if check_imports and named_iat(client,"tier0.dll","Warning")!=[WARNING_IAT_VA]:
        raise VerificationError("Phase212 original tier0 Warning import/name/IAT changed")
    type_to_va=[CASE_DESTINATIONS[index] for index in TYPE_CASES]
    return {
      "new_sha_verified_original_functions":1 if check_hash else 0,
      "new_sha_verified_native_bytes":SIZE if check_hash else 0,
      "original_function_entry":hex(ENTRY),
      "complete_original_native_SHA256":SHA if check_hash else None,
      "original_type_count":len(TYPE_CASES),
      "original_case_count":len(CASE_DESTINATIONS),
      "original_opaque_direct_calls":len(CALL_EDGES),
      "original_unique_direct_call_targets":len({d for _,d in CALL_EDGES}),
      "original_opcode_witnesses":len(WITNESSES),
      "original_case_destination_VAs":[hex(x) for x in CASE_DESTINATIONS],
      "original_type_to_case":list(TYPE_CASES),
      "original_type_to_destination_VAs":[hex(x) for x in type_to_va],
      "special_type_10_nested_call":hex(CALL_EDGES[0][0]),
      "warning_type_values":[i for i,x in enumerate(TYPE_CASES) if x==12],
      "invalid_type_greater_than":26,
      "warning_import":"tier0.dll!Warning" if check_imports else None,
      "limits":[
        "39 native calls are tracked but most original target implementations remain opaque",
        "Branch and flag routing are reconstructed, not complete record write effects",
        "Original function is followed by INLINE jump/selector data, not separate code",
        "No native retail SiN campaign running reconstructed Windows game modules"
      ],
    }

def audit(server:bytes,client:bytes)->dict:
    if hashlib.sha256(server).hexdigest()!=FULL_SHA["server"] or \
       hashlib.sha256(client).hexdigest()!=FULL_SHA["client"]:
        raise VerificationError("Phase212 requires original complete server/client hashes")
    return verify_native(PEImage(client))

def main():
    a=argparse.ArgumentParser(description=__doc__)
    a.add_argument("--server",type=Path,required=True)
    a.add_argument("--client",type=Path,required=True)
    a.add_argument("--output",type=Path)
    args=a.parse_args()
    result=json.dumps(audit(args.server.read_bytes(),
                            args.client.read_bytes()),indent=2)+"\n"
    if args.output:args.output.write_text(result,encoding="utf-8")
    else:print(result,end="")
if __name__=="__main__":main()
