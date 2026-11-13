#!/usr/bin/env python3
"""Phase205: strict owner-original SiN 2006 server/client allocator twin audit.

Owner-only SHA256 original PE32 files required for definitive verification.
Stores only digests, sparse original opcode witnesses, and fixed addresses.
"""
from __future__ import annotations
import argparse, hashlib, json, struct
from pathlib import Path
from audit_sin_original_pe_imports import PEImage, VerificationError, parse_imports

FULL_SHA={
 "server":"568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650",
 "client":"6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d",
}
BASE={"server":0x22000000,"client":0x24000000}
IAT={
 "server":{"g_pMemAlloc":0x223d2124,"GetProcessHeap":0x223d2024,
           "HeapAlloc":0x223d2020,"VirtualQuery":0x223d201c,"GetModuleFileNameA":0x223d2018},
 "client":{"g_pMemAlloc":0x242761cc,"GetProcessHeap":0x24276098,
           "HeapAlloc":0x2427609c,"VirtualQuery":0x242760a0,"GetModuleFileNameA":0x242760a4},
}
DATA={
 "server":{"anchor":0x225885d8,"powner":0x225885dc,"pflag":0x225885e0,
           "sowner":0x225885e4,"sflag":0x225885e8},
 "client":{"anchor":0x24372904,"powner":0x24372908,"pflag":0x2437290c,
           "sowner":0x24372910,"sflag":0x24372914},
}
# Each: original entry (server,client), length, original complete hashes,
# the ONLY 4-byte absolute operands permitted to differ, normalized full SHA,
# original post-ret INT3 count, disjoint sparse machine-code witnesses.
METHODS={
 "context":(
  (0x2226e290,0x2416f7f0),90,
  ("84049bbc203aa4d0f0fa107d0294be44dd7075dcd827a13f7c2fda933f9f0659",
   "c2a27c407aa1fa127dc525282abb96d283e1026826f428c0855df9480123cce0"),
  [(1,"g_pMemAlloc"),(0x1d,"GetProcessHeap"),(0x24,"HeapAlloc"),
   (0x32,"anchor"),(0x38,"VirtualQuery"),(0x49,"GetModuleFileNameA")],
  "ce5fa05dbc531d8e1e56c533e4daf49494b6a299dcab1c0f345c9b562d3078b9",6,
  {5:"8b088b",0x14:"680401",0x2a:"6a1c8d",0x56:"83c41cc3"}),
 "register_alloc":(
  (0x2226e2f0,0x2416f850),71,
  ("e2c2ae1408ce91691982e97c2d3836de3345664f7d177fee7c5313bab26d6389",
   "e42a8f93b8166c2510985587937279800f654aa5ec7483aa6efec9cfaa6f200c"),
  [(7,"pflag"),(0xf,"pflag"),(0x19,"powner"),(0x20,"powner"),
   (0x29,"g_pMemAlloc"),(0x38,"g_pMemAlloc")],
  "5fce91c2edbdc0a168b7bc0863a8435ae527df3779c2a11d128261f9cd39671e",9,
  {0:"b80100",5:"8405",0x2d:"8b088b",0x43:"56ff12c3"}),
 "register_realloc":(
  (0x2226e340,0x2416f8a0),74,
  ("2ab07ccb733a46a88571bb1cb32596b85937dc904b5454c2c1e714b4275fd4c2",
   "fd969723fbe917bc9107e2920679fed8dcfd5c91809fe57db7330651d4dbe7ac"),
  [(7,"sflag"),(0xf,"sflag"),(0x19,"sowner"),(0x20,"sowner"),
   (0x29,"g_pMemAlloc"),(0x39,"g_pMemAlloc")],
  "6b0df45d7f10cb4aafb611dc1431234c111a7c5bc27fb56f3f52c891fa1f893d",6,
  {0:"b80100",5:"8405",0x2d:"8b088b",0x46:"ff5208c3"}),
 "stack_alloc":(
  (0x2226e390,0x2416f8f0),79,
  ("8c8192a3d478efa799525609e38affe36e9548e5ee449e57aadc914237e13d1a",
   "e4682f7b52bf956d9769e11f7390b3f6b17076d25e36f35951211b5a6e1fe82a"),
  [(7,"pflag"),(0xf,"pflag"),(0x19,"powner"),(0x20,"powner"),
   (0x29,"g_pMemAlloc"),(0x3c,"g_pMemAlloc")],
  "90bc6feef7698711126a7ee4709f4df014192074dd2fe7b4f7fc9b23e0c7fea6",1,
  {0:"b80100",5:"8405",0x2d:"8b088b",0x4b:"50ff12c3"}),
 "free":(
  (0x2226e3e0,0x2416f940),18,
  ("4d45603c1544ce399c6783bb8fd0019698929a223663d256d112ce57d66868fc",
   "c597a60f0eca41a3544aef8a22df99bd70535de676d5bf7f73477f0865daea73"),
  [(1,"g_pMemAlloc")],
  "4dce485e5dfc86aeb657bbf2afc126697e94af00560fba6d1f38612be43ee2a7",14,
  {0:"a1",5:"8b088b",0xd:"50ff5214",0x11:"c3"}),
 "stack_realloc":(
  (0x2226e400,0x2416f960),90,
  ("fccf3ba0ea064576d8806a024df924f2281e8829d202ae99c023010f2283d110",
   "c763dd4ddaf1c66858586c95c266ee2d19585a9f655e91edb9bfcb3d6d9e3cc2"),
  [(7,"sflag"),(0xf,"sflag"),(0x19,"sowner"),(0x20,"sowner"),
   (0x29,"g_pMemAlloc"),(0x41,"g_pMemAlloc")],
  "103d20dddb5d5e77d3c8f47b494798b3c1e75e8336d095a53999344a67d667b1",6,
  {0:"b80100",5:"8405",0x2d:"8b088b",0x56:"ff5208c3"}),
}
NEW={("server","register_alloc"),("server","register_realloc"),
     ("client","free"),("client","stack_realloc")}
CONTEXT_CALLERS={"register_alloc","register_realloc","stack_alloc","stack_realloc"}
EXPECTED_LIBS={"g_pMemAlloc":"tier0.dll","GetProcessHeap":"KERNEL32.dll",
               "HeapAlloc":"KERNEL32.dll","VirtualQuery":"KERNEL32.dll",
               "GetModuleFileNameA":"KERNEL32.dll"}

def imports_with_iat(pe:PEImage)->dict:
    data=pe.data;header=struct.unpack_from("<I",data,0x3c)[0]
    rva,_=struct.unpack_from("<II",data,header+24+104)
    # Phase204 parser first enforces bounded and terminated PE32 structures.
    manifest=parse_imports(pe);found={}
    for i,module in enumerate(manifest):
        oft,_,_,_,iat=struct.unpack("<IIIII",pe.at(pe.base+rva+20*i,20))
        for j,entry in enumerate(module["symbols"]):
            name=entry.get("name")
            if name not in EXPECTED_LIBS:continue
            if name in found:raise VerificationError("duplicate required import "+name)
            if not oft and name not in EXPECTED_LIBS:
                raise VerificationError("unexpected thunk")
            found[name]=(module["dll"],pe.base+iat+j*4)
    return found

def verify_methods(server:PEImage,client:PEImage,*,check_hash=True,check_imports=True)->dict:
    normalized={};opcode_count=0;operands_count=0;direct_calls=0
    for n,(role,pe) in enumerate((("server",server),("client",client))):
        if pe.base!=BASE[role]:raise VerificationError("original image base mismatch: "+role)
        if check_imports:
            seen=imports_with_iat(pe)
            for name,addr in IAT[role].items():
                if seen.get(name)!=(EXPECTED_LIBS[name],addr):
                    raise VerificationError("original PE imported IAT binding mismatch: "+role+" "+name)
        normalized[role]={}
        for name,(entries,length,shas,offsets,normalized_sha,padding,witnesses) in METHODS.items():
            va=entries[n]
            if pe.section_of(va)!=".text":raise VerificationError("original native function not in .text")
            raw=bytearray(pe.at(va,length))
            if check_hash and hashlib.sha256(raw).hexdigest()!=shas[n]:
                raise VerificationError("original complete native function SHA mismatch: "+role+" "+name)
            for off,hx in witnesses.items():
                code=bytes.fromhex(hx)
                if raw[off:off+len(code)]!=code:
                    raise VerificationError("original native opcode witness mismatch: "+role+" "+name)
                opcode_count+=1
            if pe.at(va+length,padding)!=b"\xcc"*padding:
                raise VerificationError("original native ret/INT3 boundary mismatch: "+role+" "+name)
            for off,symbol in offsets:
                ptr=IAT[role].get(symbol,DATA[role].get(symbol))
                if struct.unpack_from("<I",raw,off)[0]!=ptr:
                    raise VerificationError("original native absolute DWORD mismatch: "+role+" "+name)
                if pe.section_of(ptr)!=(".rdata" if symbol in IAT[role] else ".data"):
                    raise VerificationError("wrong original static memory section: "+role+" "+symbol)
                raw[off:off+4]=bytes(4)
                operands_count+=1
            if name in CONTEXT_CALLERS:
                site=va+0x13;destination=METHODS["context"][0][n]
                rel=b"\xe8"+struct.pack("<i",destination-site-5)
                if pe.at(site,5)!=rel:
                    raise VerificationError("native allocator context CALL edge changed: "+role+" "+name)
                direct_calls+=1
            normalized[role][name]=bytes(raw)
    for name,spec in METHODS.items():
        if normalized["server"][name]!=normalized["client"][name]:
            raise VerificationError("pair differs beyond original module absolute addresses: "+name)
        if check_hash and hashlib.sha256(normalized["server"][name]).hexdigest()!=spec[4]:
            raise VerificationError("normalized complete original function digest changed: "+name)
    return {
        "new_sha_verified_original_functions":len(NEW) if check_hash else 0,
        "new_sha_verified_original_bytes":sum(METHODS[name][1] for _,name in NEW) if check_hash else 0,
        "cross_dll_complete_twin_pairs":len(METHODS),
        "existing_original_functions_reverified":8 if check_hash else 0,
        "sparse_native_opcode_witnesses":opcode_count,
        "exact_absolute_address_operands":operands_count,
        "exact_direct_context_calls":direct_calls,
        "named_original_pe_imports_verified":2*len(EXPECTED_LIBS) if check_imports else 0,
        "normalized_full_function_sha256":{name:spec[4] for name,spec in METHODS.items()} if check_hash else None,
        "limits":["No cross-module allocation ownership is assumed",
                  "Original tier0 allocator implementation and Windows game engine startup remain opaque"],
    }

def audit(server:bytes,client:bytes)->dict:
    if hashlib.sha256(server).hexdigest()!=FULL_SHA["server"] or \
       hashlib.sha256(client).hexdigest()!=FULL_SHA["client"]:
        raise VerificationError("both complete SHA256-matched original 2006 DLLs required")
    return verify_methods(PEImage(server),PEImage(client))

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--server",type=Path,required=True)
    p.add_argument("--client",type=Path,required=True)
    p.add_argument("--output",type=Path)
    a=p.parse_args()
    report=json.dumps(audit(a.server.read_bytes(),a.client.read_bytes()),indent=2)+"\n"
    if a.output:a.output.write_text(report,encoding="utf-8")
    else:print(report,end="")
if __name__=="__main__":main()
