#!/usr/bin/env python3
"""Phase213: original SiN 2006 client prediction field COPY specialist audit.

Read-only, SHA-gated to the user-owned complete original server/client DLLs.
Checks four complete native x86 functions and their exact Phase212 caller
instructions, not a guessed SourceVR serializer. No proprietary method body
bytes, raw complete disassembly, or game assets are distributed here.
"""
from __future__ import annotations
import argparse,hashlib,json,struct
from pathlib import Path
from audit_sin_original_pe_imports import PEImage,VerificationError
from audit_sin_original_cross_dll_tier0_allocator import FULL_SHA,BASE
from audit_sin_original_client_snapshot_record_router import (
    ENTRY as ROUTER_ENTRY, SIZE as ROUTER_SIZE, CASE_DESTINATIONS,
    CASE_TABLE_VA, SELECTOR_VA, TYPE_CASES,
)

# code VA, length, entire ORIGINAL method SHA256, following INT3 count,
# actual original direct CALL VA inside Phase212 router, case selector,
# sparse independently checked x86 opcode witnesses (never full code).
METHODS={
    "type5_dword_copy":(
        0x24198a10,34,
        "8c6cd7b6e20fc8e508282f71240c46268eda44d67f968d00b699b197caaeade8",
        14,0x241999cb,5,{
            0x00:"80792c00",0x06:"837c240401",0x0d:"8b4c2410",
            0x12:"8b742410",0x17:"8b7c2410",
            0x1b:"f3a5",0x1f:"c21000",
        }
    ),
    "type6_byte_copy":(
        0x24198a40,46,
        "61e309166f7591611a783cd90c9c749f686d2c202ad3280fe87d821d37fb6f02",
        2,0x24199993,6,{
            0x00:"80792c00",0x06:"837c240401",0x0d:"8b4c2410",
            0x1d:"c1e902",0x20:"f3a5",0x24:"83e103",
            0x27:"f3a4",0x2b:"c21000",
        }
    ),
    "type1_dword_copy":(
        0x24198a70,34,
        "8c6cd7b6e20fc8e508282f71240c46268eda44d67f968d00b699b197caaeade8",
        14,0x2419985b,1,{
            0x00:"80792c00",0x06:"837c240401",0x0d:"8b4c2410",
            0x12:"8b742410",0x17:"8b7c2410",
            0x1b:"f3a5",0x1f:"c21000",
        }
    ),
    "type2_null_terminated_copy":(
        0x24198b40,61,
        "98a4f883c300f0aeec3998db6434c5b7b7068552a6957140e352d64262af2e67",
        3,0x2419988f,2,{
            0x00:"80792c00",0x06:"837c240401",0x0e:"8b742410",
            0x15:"8d5001",0x18:"8a08",0x1d:"84c9",
            0x1f:"75f7",0x25:"2bc2",0x27:"8d4801",
            0x2c:"c1e902",0x2f:"f3a5",0x36:"f3a4",
            0x3a:"c20c00",
        }
    ),
}
def relative_call(site:int,target:int)->bytes:
    return b"\xe8"+struct.pack("<i",target-site-5)

def verify_native(client:PEImage,*,check_hash=True,check_switch=True)->dict:
    if client.base!=BASE["client"]:
        raise VerificationError("Phase213 unexpected original 2006 client PE32 base")
    evidence={}
    instruction_witnesses=0
    for name,(va,n,digest,padding,site,type_value,witnesses) in METHODS.items():
        if client.section_of(va)!=".text":
            raise VerificationError("Phase213 function outside original client .text "+name)
        body=client.at(va,n)
        if check_hash and hashlib.sha256(body).hexdigest()!=digest:
            raise VerificationError("Phase213 complete original method SHA256 mismatch "+name)
        if client.at(va+n,padding)!=b"\xcc"*padding:
            raise VerificationError("Phase213 native method RET/INT3 boundary drift "+name)
        for off,witness in witnesses.items():
            expected=bytes.fromhex(witness)
            if body[off:off+len(expected)]!=expected:
                raise VerificationError("Phase213 original x86 opcode witness drift "+name+" +%#x"%off)
            instruction_witnesses+=1
        if not (ROUTER_ENTRY<=site<ROUTER_ENTRY+ROUTER_SIZE):
            raise VerificationError("Phase213 specialist CALL not inside reviewed Phase212 router")
        if client.at(site,5)!=relative_call(site,va):
            raise VerificationError("Phase213 original record type specialist CALL drift "+name)
        if client.section_of(site)!=".text":
            raise VerificationError("Phase213 original direct CALL not in .text")
        if TYPE_CASES[type_value]!=type_value:
            raise VerificationError("Phase213 invalid original type/case association")
        evidence[name]={
            "method_va":hex(va),"method_size":n,
            "native_sha256":digest if check_hash else None,
            "case_type":type_value,"native_caller":hex(site),
            "original_ret_stack_bytes":12 if name=="type2_null_terminated_copy" else 16
        }
    # The two independent 34-byte original functions are NOT aliases of
    # one native address: they are exact byte-for-byte compiler duplicates.
    a=client.at(METHODS["type1_dword_copy"][0],34)
    b=client.at(METHODS["type5_dword_copy"][0],34)
    if a!=b:
        raise VerificationError("Phase213 two original complete 34-byte DWORD helpers differ")
    if check_switch:
        if struct.unpack("<13I",client.at(CASE_TABLE_VA,52))!=CASE_DESTINATIONS:
            raise VerificationError("Phase213 original Phase212 case table drift")
        if tuple(client.at(SELECTOR_VA,27))!=TYPE_CASES:
            raise VerificationError("Phase213 original Phase212 type selector drift")
    return {
        "new_sha_verified_original_client_functions":4 if check_hash else 0,
        "new_complete_original_native_bytes":175 if check_hash else 0,
        "complete_native_method_evidence":evidence,
        "sparse_opcode_witnesses":instruction_witnesses,
        "exact_original_type_case_call_edges":4,
        "verified_original_duplicate_dword_function_pairs":1,
        "original_type_case_associations":{
            "1":"DWORD count forward REP MOVSD",
            "2":"NUL-inclusive string forward REP MOVSD/REP MOVSB",
            "5":"DWORD count forward REP MOVSD, identical to type 1 helper",
            "6":"byte count forward REP MOVSD/REP MOVSB",
        },
        "original_switch_cross_check":bool(check_switch),
        "limits":[
            "Helpers are only one part of the four original specialist cases",
            "Original caller stack argument values and ownership remain opaque at the case level",
            "Portable safety bounds/NUL checks do not reproduce native faults",
            "Original Windows engine executing rebuilt client.dll gameplay is not yet demonstrated",
        ],
    }

def audit(server_bytes:bytes,client_bytes:bytes)->dict:
    if hashlib.sha256(server_bytes).hexdigest()!=FULL_SHA["server"] or \
       hashlib.sha256(client_bytes).hexdigest()!=FULL_SHA["client"]:
        raise VerificationError("Phase213 requires both complete SHA256-matched original 2006 game DLLs")
    return verify_native(PEImage(client_bytes))

def main()->None:
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--server",required=True,type=Path)
    p.add_argument("--client",required=True,type=Path)
    p.add_argument("--output",type=Path)
    args=p.parse_args()
    document=json.dumps(audit(args.server.read_bytes(),args.client.read_bytes()),indent=2)+"\n"
    if args.output:args.output.write_text(document,encoding="utf-8")
    else:print(document,end="")

if __name__=="__main__":
    main()
