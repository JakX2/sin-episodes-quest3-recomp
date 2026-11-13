#!/usr/bin/env python3
"""Phase208: owner-original SiN 2006 tier0 forwarding and alloc alias audit.

Owner-supplied SHA256-matched client.dll/server.dll are read only. Public
metadata includes whole-original hashes, exact VAs, sparse opcode witnesses,
and address/call edges. It NEVER embeds full original x86 method bodies.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
from pathlib import Path
from audit_sin_original_cross_dll_tier0_allocator import (
    PEImage, VerificationError, FULL_SHA, BASE, IAT, DATA, METHODS,
    imports_with_iat
)
from audit_sin_original_tier0_allocator_aliases import (
    normalized_stack_allocator, ALL_STACK_ALLOC_EQUIVALENT_SHA
)

# Original server VA, client VA, complete body size, SHA256 per original
# DLL, original absolute operand fields, full normalized SHA256,
# original INT3 alignment size, sparse disjoint original opcode witnesses.
REGIONS = {
    "slot00_three": (
        (0x2226e720,0x2416fc80),27,
        ("59032b061ad657a4b65e535c30bf8597ed1fe4c28ba3cbae8b6dc0aec80dbd9c",
         "6cc143aece17495514ffc1e1c9e7f58b04b2391670ea68fe66b475813979cefb"),
        [(1,"g_pMemAlloc")],
        "1deb0a8b86f723e5d933e56018d5a6b264bdedbfa7c7b63000f718beeb1fabc0",5,
        {0:"a1",5:"8b08",7:"8b44240c",0x18:"ff12",0x1a:"c3"}
    ),
    "slot10_three": (
        (0x2226e740,0x2416fca0),28,
        ("7dd353c922692127a061e2b9dcbb9c4a75021744bdee3ad0d5e98cf6f125bb83",
         "a870764ec093252a4945e3eb8bc80af4f9cc5472c189e3db7ee1c1002579cc0e"),
        [(1,"g_pMemAlloc")],
        "f1a52a8bb50bca7c814cc5d0828bf20bb8bb79d0cabe7ac6bd9044742ec8d790",4,
        {0:"a1",5:"8b08",7:"8b44240c",0x18:"ff5210",0x1b:"c3"}
    ),
    "slot08_four": (
        (0x2226e760,0x2416fcc0),33,
        ("028af88c5784da167afc6bcd718877bb57ff3a21e528f35a930ba78501677c8d",
         "1e32b0610e9e25e09c3e8d12cc4c57f199e81b27f74f4c6964e4efb8279eedaa"),
        [(1,"g_pMemAlloc")],
        "7e3ce6c117dafe526caad7d64bc346b80f08068ddf2d3979c1ed483869faf76c",15,
        {0:"a1",5:"8b08",7:"8b442410",0x1d:"ff5208",0x20:"c3"}
    ),
    "cached_alias_a": (
        (0x2226e790,0x2416fcf0),79,
        ("1ba57fe89ebae2110c154315e1371c8481cc0bd1cf8363a4c83689219ff9714f",
         "84fffc04db883f146d64edf1f474b61b5fe03cb36e95b5fcd717ece17e793acf"),
        [(7,"pflag"),(0xf,"pflag"),(0x19,"powner"),(0x20,"powner"),
         (0x29,"g_pMemAlloc"),(0x3c,"g_pMemAlloc")],
        "53d3c04bde4a3b487f28a5a8414491a6670308634a68b91b7de654c8d1df844a",1,
        {0:"b801000000",5:"8405",0xb:"7512",0x13:"e8",0x24:"85c0",
         0x36:"ff5204",0x4c:"ff12c3"}
    ),
    "slot00_skip_middle": (
        (0x2226e7e0,0x2416fd40),27,
        ("f903cbb82919679192b60918d3ee9f9087444206f9cfc37eb4ee09ac607e5565",
         "06b28fa8c3b5d5701c214e37a09179591199cf5de9364c3f37e0349fbafc7b17"),
        [(1,"g_pMemAlloc")],
        "aeef623a0c226b09a07a52d5152705fad021dc65e1731dc5827b28fb9368bcad",5,
        {0:"a1",7:"8b442410",0xe:"8b442410",0x13:"8b44240c",0x18:"ff12c3"}
    ),
    "duplicate_free": (
        (0x2226e800,0x2416fd60),18,
        ("4d45603c1544ce399c6783bb8fd0019698929a223663d256d112ce57d66868fc",
         "c597a60f0eca41a3544aef8a22df99bd70535de676d5bf7f73477f0865daea73"),
        [(1,"g_pMemAlloc")],
        "4dce485e5dfc86aeb657bbf2afc126697e94af00560fba6d1f38612be43ee2a7",14,
        {0:"a1",7:"8b442404",0xe:"ff5214c3"}
    ),
    "cached_alias_b": (
        (0x2226e820,0x2416fd80),79,
        ("f0b61188dfa38e2eea163e724678c5ec46319874062766bbe9b78dd29dcf1884",
         "44318a97f116b1b629cb2fe1794e14c28ed58b61a7606b7784a4e7b785c20d08"),
        [(7,"pflag"),(0xf,"pflag"),(0x19,"powner"),(0x20,"powner"),
         (0x29,"g_pMemAlloc"),(0x3c,"g_pMemAlloc")],
        "8d5564c5eda06fb666eecd04aea2064c429e5e904a3387b116297f9daa0cbe00",1,
        {0:"b801000000",5:"8405",0xb:"7512",0x13:"e8",0x24:"85c0",
         0x36:"ff5204",0x4c:"ff12c3"}
    ),
}
ALIAS_NAMES=("cached_alias_a","cached_alias_b")
# Four client methods were already SHA-cataloged by Phase185.
PREVIOUSLY_VERIFIED={
    ("client","cached_alias_a"),
    ("client","slot00_skip_middle"),
    ("client","duplicate_free"),
    ("client","cached_alias_b"),
}
EXPECTED_NEW_COUNT=10
EXPECTED_NEW_BYTES=379

def call32(site:int,target:int)->bytes:
    return b"\xe8"+struct.pack("<i",target-site-5)

def verify_native(server:PEImage,client:PEImage,*,check_hash:bool=True,
                  check_imports:bool=True)->dict:
    normalized={}
    witnesses=0
    operands=0
    context_calls=0
    original_frees_compared=0
    for index,(role,pe) in enumerate((("server",server),("client",client))):
        if pe.base!=BASE[role]:
            raise VerificationError("Phase208 original PE32 base mismatch: "+role)
        if check_imports:
            imports=imports_with_iat(pe)
            if imports.get("g_pMemAlloc")!=("tier0.dll",IAT[role]["g_pMemAlloc"]):
                raise VerificationError("Phase208 exact original tier0.dll g_pMemAlloc IAT mismatch")
        normalized[role]={}
        for name,(entries,size,shas,slots,normalized_sha,padding,sparse) in REGIONS.items():
            va=entries[index]
            if pe.section_of(va)!=".text":
                raise VerificationError("Phase208 original native code not in .text: "+name)
            raw=bytearray(pe.at(va,size))
            if check_hash and hashlib.sha256(raw).hexdigest()!=shas[index]:
                raise VerificationError("Phase208 complete original native SHA256 mismatch: "+role+" "+name)
            for off,codehex in sparse.items():
                code=bytes.fromhex(codehex)
                if raw[off:off+len(code)]!=code:
                    raise VerificationError("Phase208 sparse x86 witness mismatch: "+role+" "+name)
                witnesses+=1
            if pe.at(va+size,padding)!=b"\xcc"*padding:
                raise VerificationError("Phase208 native RET/INT3 boundary mismatch: "+role+" "+name)
            if name in ALIAS_NAMES:
                site=va+0x13
                helper=METHODS["context"][0][index]
                if pe.at(site,5)!=call32(site,helper):
                    raise VerificationError("Phase208 alias direct owner helper CALL mismatch: "+role)
                context_calls+=1
                # The two new server aliases and two previously reviewed
                # client aliases are also bitwise equal to the SIX old
                # stack allocation aliases from Phase207 after ONLY
                # normalization of explicit six x86 DWORDs and a rel32.
                body=pe.at(va,size)
                if check_hash and hashlib.sha256(normalized_stack_allocator(body)).hexdigest()!=ALL_STACK_ALLOC_EQUIVALENT_SHA:
                    raise VerificationError("Phase208 cross-phase original stack allocator identity mismatch")
            if name=="duplicate_free":
                prior=METHODS["free"][0][index]
                expected=pe.at(prior,18)
                if expected!=pe.at(va,18):
                    raise VerificationError("Phase208 repeated native +0x14 free wrapper differs")
                original_frees_compared+=1
            for off,key in slots:
                pointer=IAT[role].get(key,DATA[role].get(key))
                if struct.unpack_from("<I",raw,off)[0]!=pointer:
                    raise VerificationError("Phase208 original module global/IAT operand mismatch")
                if pe.section_of(pointer)!=(".rdata" if key in IAT[role] else ".data"):
                    raise VerificationError("Phase208 original absolute target section mismatch: "+key)
                raw[off:off+4]=b"\0"*4
                operands+=1
            normalized[role][name]=bytes(raw)
    for name,(_,_,_,_,norm_sha,_,_) in REGIONS.items():
        if normalized["server"][name]!=normalized["client"][name]:
            raise VerificationError("Phase208 original twin differs beyond proven globals: "+name)
        if check_hash and hashlib.sha256(normalized["server"][name]).hexdigest()!=norm_sha:
            raise VerificationError("Phase208 normalized complete original SHA256 mismatch: "+name)
    counted=[(role,name,spec[1]) for role in ("server","client")
             for name,spec in REGIONS.items()
             if (role,name) not in PREVIOUSLY_VERIFIED]
    if len(counted)!=EXPECTED_NEW_COUNT or sum(size for _,_,size in counted)!=EXPECTED_NEW_BYTES:
        raise VerificationError("Phase208 previously reviewed function count drift")
    return {
        "new_sha_verified_original_functions":EXPECTED_NEW_COUNT if check_hash else 0,
        "new_original_x86_bytes":EXPECTED_NEW_BYTES if check_hash else 0,
        "previously_reviewed_client_functions_reverified":len(PREVIOUSLY_VERIFIED) if check_hash else 0,
        "complete_original_functions_SHA_checked":14 if check_hash else 0,
        "complete_original_server_client_pairs":len(REGIONS),
        "sparse_opcode_witnesses":witnesses,
        "exact_absolute_globals_or_IAT_operands":operands,
        "original_direct_alias_context_calls":context_calls,
        "native_free_wrapper_identity_checks":original_frees_compared,
        "original_tier0_iat_name_checks":2 if check_imports else 0,
        "original_complete_normalized_sha256":{
            k:v[4] for k,v in REGIONS.items()} if check_hash else None,
        "limits":[
            "Four already-reviewed client methods from Phase185 were not recounted",
            "Imported tier0 allocator virtual methods are NOT recovered here",
            "Native stack load order is proven; opaque vtable method semantics are not",
            "No original retail Windows game process executing reconstructed gameplay",
        ]
    }

def audit(server:bytes,client:bytes)->dict:
    if hashlib.sha256(server).hexdigest()!=FULL_SHA["server"] or \
       hashlib.sha256(client).hexdigest()!=FULL_SHA["client"]:
        raise VerificationError("Phase208 requires BOTH complete owner-original SiN DLLs")
    return verify_native(PEImage(server),PEImage(client))

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--server",type=Path,required=True)
    parser.add_argument("--client",type=Path,required=True)
    parser.add_argument("--output",type=Path)
    args=parser.parse_args()
    result=json.dumps(audit(args.server.read_bytes(),args.client.read_bytes()),indent=2)+"\n"
    if args.output:args.output.write_text(result,encoding="utf-8")
    else:print(result,end="")

if __name__=="__main__":
    main()
