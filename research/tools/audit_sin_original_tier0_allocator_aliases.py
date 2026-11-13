#!/usr/bin/env python3
"""Phase207: strict 2006 original SiN server/client allocator alias audit.

Owner-original DLLs are NEVER included in this repository. Definitive
results require both complete original SHA256 files. No original complete
machine-code function bodies are embedded; only sparse opcode witnesses,
original entrypoints, hashes, and exact absolute relocation locations.
"""
from __future__ import annotations
import argparse, hashlib, json, struct
from pathlib import Path
from audit_sin_original_cross_dll_tier0_allocator import (
    PEImage, VerificationError, FULL_SHA, BASE, IAT, DATA, METHODS,
    imports_with_iat,
)

# (server VA, client VA, complete length, whole original native SHAs,
#  original exact absolute DWORD offsets, original INT3 padding,
#  independently recorded sparse x86 opcode witnesses, normalized native SHA)
REGIONS={
    "repeated_virtual20":(
        (0x2226e5e0,0x2416fb40),18,
        ("895cd1c5f870dd7ff343225897583863df86dcbe96010ecf12a3c3760a52e041",
         "3407dec8c90462f67a8084470e5d362fc741ed9ae0bf24befeb54b269ee70dc1"),
        [(1,"g_pMemAlloc")],14,
        {0:"a1",5:"8b08",7:"8b442404",0xb:"8b11",0xd:"50ff5220",0x11:"c3"},
        "0dc8d5feacd828692663970ee9ecfab1f1fc2f5044bb2c5321c85566cbd92111"
    ),
    "stack_alloc_alias_a":(
        (0x2226e600,0x2416fb60),79,
        ("631b6917d8e62170590843a21f22c22893e5609169125b67b2a0cbb623636e66",
         "49018cbed857fb9700e1514c35d82b715c2458c212371295863becb5b6ce1d50"),
        [(7,"pflag"),(0xf,"pflag"),(0x19,"powner"),(0x20,"powner"),
         (0x29,"g_pMemAlloc"),(0x3c,"g_pMemAlloc")],1,
        {0:"b801000000",5:"8405",0xb:"7512",0x1d:"eb05",0x24:"85c0",
         0x2d:"8b08",0x36:"ff5204",0x44:"6a00",0x4c:"ff12c3"},
        "d1e990a0f9f9e760cfe9b299f8bf4d22a3be4900688c23ca6bed1d4985aa1619"
    ),
    "stack_alloc_alias_b":(
        (0x2226e650,0x2416fbb0),79,
        ("5317deb75ca3c33c3bd1167bcb04e9353970665a5a59f40b5fb8cb5d5342effa",
         "ca40a5c6f99fb923ff0e69a34763d74a9a6e78d2d4bbc239d5abf48df5c3840c"),
        [(7,"pflag"),(0xf,"pflag"),(0x19,"powner"),(0x20,"powner"),
         (0x29,"g_pMemAlloc"),(0x3c,"g_pMemAlloc")],1,
        {0:"b801000000",5:"8405",0xb:"7512",0x1d:"eb05",0x24:"85c0",
         0x2d:"8b08",0x36:"ff5204",0x44:"6a00",0x4c:"ff12c3"},
        "3f45f5ca2f55b24b44e1ec348e64e8975dc8da3521076fe276bad7d990f7d4f3"
    ),
    "opaque_virtual1c_twoargs":(
        (0x2226e6a0,0x2416fc00),23,
        ("c6b2156b641595c0db8146e1b6461ef5a1668b5ee350d7959958ef70ae9f2b94",
         "7041a7c7eb1e3fabe53b9c4ca9b807b08ca01531ebdce8cddf8d7978d6cf0f27"),
        [(1,"g_pMemAlloc")],9,
        {0:"a1",5:"8b08",7:"8b442408",0xb:"8b11",0xd:"50",
         0xe:"8b442408",0x12:"50ff521c",0x16:"c3"},
        "7892d229bb0df17d075761ee827657e1dab3d045f5e55cab6cbf424659e865f4"
    ),
}
ALLOC_ALIAS_TYPES=("stack_alloc_alias_a","stack_alloc_alias_b")
ALL_STACK_ALLOC_EQUIVALENT_SHA=(
    "976227759a29da6c6e7521a12fec6c5796d2575d7c4636dbaa62cf70e406a2a3"
)

def rel_call(site:int,destination:int)->bytes:
    return b"\xe8"+struct.pack("<i",destination-site-5)

def normalized_stack_allocator(body:bytes)->bytes:
    original=bytearray(body)
    if len(original)!=79:raise VerificationError("expected complete original 79-byte allocator")
    for off in (7,0xf,0x19,0x20,0x29,0x3c):
        original[off:off+4]=bytes(4)
    # Exactly one Phase186/202 native CALL rel32 to owner resolver.
    original[0x14:0x18]=bytes(4)
    return bytes(original)

def verify_native(server:PEImage,client:PEImage,*,check_hash:bool=True,
                  check_imports:bool=True)->dict:
    verified={}
    witness_count=0
    absolute_count=0
    native_call_count=0
    all_equivalent=[]
    for index,(role,pe) in enumerate((("server",server),("client",client))):
        if pe.base!=BASE[role]:
            raise VerificationError("Phase207 wrong original PE32 image base "+role)
        if check_imports:
            resolved=imports_with_iat(pe)
            if resolved.get("g_pMemAlloc")!=("tier0.dll",IAT[role]["g_pMemAlloc"]):
                raise VerificationError("Phase207 original tier0 imported allocator slot/name mismatch")
        verified[role]={}
        for name,(entries,n,shas,absolute,padding,witnesses,digest) in REGIONS.items():
            va=entries[index]
            if pe.section_of(va)!=".text":
                raise VerificationError("Phase207 native function outside original .text "+name)
            body=bytearray(pe.at(va,n))
            if check_hash and hashlib.sha256(body).hexdigest()!=shas[index]:
                raise VerificationError("Phase207 original complete method SHA mismatch "+role+" "+name)
            for off,hx in witnesses.items():
                expected=bytes.fromhex(hx)
                if body[off:off+len(expected)]!=expected:
                    raise VerificationError("Phase207 sparse opcode witness mismatch "+role+" "+name)
                witness_count+=1
            if pe.at(va+n,padding)!=b"\xcc"*padding:
                raise VerificationError("Phase207 native RET/INT3 boundary mismatch "+role+" "+name)
            for off,symbol in absolute:
                target=IAT[role].get(symbol,DATA[role].get(symbol))
                if struct.unpack_from("<I",body,off)[0]!=target:
                    raise VerificationError("Phase207 original embedded static/IAT operand mismatch")
                if pe.section_of(target)!=(".rdata" if symbol in IAT[role] else ".data"):
                    raise VerificationError("Phase207 native referenced segment mismatch")
                body[off:off+4]=bytes(4)
                absolute_count+=1
            if name in ALLOC_ALIAS_TYPES:
                helper=METHODS["context"][0][index]
                site=va+0x13
                if pe.at(site,5)!=rel_call(site,helper):
                    raise VerificationError("Phase207 exact alias owner helper CALL mismatch")
                native_call_count+=1
                all_equivalent.append(normalized_stack_allocator(pe.at(va,n)))
            verified[role][name]=bytes(body)
    for name,(_,_,_,_,_,_,normalized_sha) in REGIONS.items():
        if verified["server"][name]!=verified["client"][name]:
            raise VerificationError("Phase207 original paired whole methods differ beyond embedded addresses")
        if check_hash and hashlib.sha256(verified["server"][name]).hexdigest()!=normalized_sha:
            raise VerificationError("Phase207 normalized original complete-body SHA mismatch "+name)
    for index,(role,pe) in enumerate((("server",server),("client",client))):
        # The already-reviewed baseline stack allocation in Phase205 is a
        # THIRD distinct native entrypoint per DLL. Reverify its original
        # complete SHA, the direct CALL edge and precise normalized identity,
        # but do NOT count it a second time in the original-function atlas.
        original=METHODS["stack_alloc"]
        base=original[0][index]
        base_body=pe.at(base,original[1])
        if check_hash and hashlib.sha256(base_body).hexdigest()!=original[2][index]:
            raise VerificationError("Phase207 existing Phase205 base allocation original SHA drift")
        if pe.at(base+0x13,5)!=rel_call(base+0x13,METHODS["context"][0][index]):
            raise VerificationError("Phase207 prior native stack allocator CALL drift")
        native_call_count+=1
        all_equivalent.append(normalized_stack_allocator(base_body))
        # Duplicate vtable +0x20 body is independently identical to Phase206
        # original first wrapper in the same module, not just similar logic.
        old=(0x2226e5c0,0x2416fb20)[index]
        prior=pe.at(old,18)
        if prior!=pe.at(REGIONS["repeated_virtual20"][0][index],18):
            raise VerificationError("Phase207 duplicated native +0x20 wrapper differs from Phase206")
    if any(blob!=all_equivalent[0] for blob in all_equivalent[1:]):
        raise VerificationError("Phase207 all six old/new original stack allocation bodies differ")
    if check_hash and hashlib.sha256(all_equivalent[0]).hexdigest()!=ALL_STACK_ALLOC_EQUIVALENT_SHA:
        raise VerificationError("Phase207 normalized six-original stack alloc SHA drift")
    return {
        "new_sha_verified_original_functions":8 if check_hash else 0,
        "new_original_native_function_bytes":398 if check_hash else 0,
        "old_original_methods_reverified_without_recounting":4 if check_hash else 0,
        "new_cross_dll_pairs":len(REGIONS),
        "exact_original_embedded_static_operands":absolute_count,
        "sparse_opcode_witnesses":witness_count,
        "original_direct_context_calls_checked":native_call_count,
        "six_distinct_stack_alloc_entrypoints_normalized_sha256":
            ALL_STACK_ALLOC_EQUIVALENT_SHA if check_hash else None,
        "per_pair_original_full_normalized_sha256":
            {k:v[6] for k,v in REGIONS.items()} if check_hash else None,
        "opaque_virtual_byte_offsets":["0x20","0x1c"],
        "limits":[
            "Virtual call identity and allocator implementation remain unknown",
            "Original DLLs have independent cached owner/global memory",
            "No original Windows-engine campaign startup demonstrated",
        ],
    }

def audit(server:bytes,client:bytes)->dict:
    if hashlib.sha256(server).hexdigest()!=FULL_SHA["server"] or \
       hashlib.sha256(client).hexdigest()!=FULL_SHA["client"]:
        raise VerificationError("Phase207 needs owner-original complete SHA matched DLLs")
    return verify_native(PEImage(server),PEImage(client))

def main()->None:
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--server",type=Path,required=True)
    p.add_argument("--client",type=Path,required=True)
    p.add_argument("--output",type=Path)
    args=p.parse_args()
    result=json.dumps(audit(args.server.read_bytes(),args.client.read_bytes()),indent=2)+"\n"
    if args.output:args.output.write_text(result,encoding="utf-8")
    else:print(result,end="")
if __name__=="__main__":
    main()
