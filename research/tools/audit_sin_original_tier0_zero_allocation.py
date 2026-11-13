#!/usr/bin/env python3
"""Phase206: owner-original SHA256 audit of SiN 2006 tier0 allocator edges.

Checks two native zero-fill allocators, two opaque virtual tail jumps and two
single-argument virtual dispatch wrappers. Does not distribute any binary
or name the unknown tier0 vtable +0x58/+0x20 methods.
"""
from __future__ import annotations
import argparse,hashlib,json,struct
from pathlib import Path
from audit_sin_original_pe_imports import PEImage, VerificationError
from audit_sin_original_cross_dll_tier0_allocator import (
    FULL_SHA, BASE, IAT, DATA, imports_with_iat
)

# (original server VA, original client VA, native code size,
#  complete original server SHA256, complete original client SHA256,
#  original absolute DWORD slots, normalized complete-body SHA256,
#  original next-function INT3 padding, sparse instruction witnesses)
REGIONS={
    "zero_alloc":(
        (0x2226e460,0x2416f9c0),107,
        "c9a369e37d6bf6a269cf66f1dcab52f38a95b8c73384671d44ad500251da0861",
        "12b19a35967f33db88219a0ca0abf348b2ecf8091651a418107f0f19ae6eb512",
        [(0x11,"pflag"),(0x1a,"pflag"),(0x24,"powner"),
         (0x2b,"powner"),(0x34,"g_pMemAlloc"),(0x44,"g_pMemAlloc")],
        "4500ad93361a49d8c8bd267b336464671dda03c58e2e2f9b15c3c7f7b358a99a",
        5,
        {0:"568b742408",5:"0faf74240c",0x52:"8bd08bce",0x56:"c1e902",
         0x59:"33c08bfa",0x5d:"f3ab",0x61:"83e103",0x64:"f3aa",0x66:"5f8bc25ec3"},
    ),
    "opaque_vtable_58_tailjump":(
        (0x2226e5a0,0x2416fb00),12,
        "be248372e1ae80572616db55e092bcfcbcbbed484a1e139b15752a334b7d8d57",
        "ea5a906b1f443fc6c98e936b39d629d3b492e994551a8281a11031b5619ff856",
        [(1,"g_pMemAlloc")],
        "02df19bf8bf17c957a3d7e95234ca58058edd92624a02815b1d476e5550f7e4c",
        4,
        {9:"ff6258"},
    ),
    "opaque_vtable_20_call":(
        (0x2226e5c0,0x2416fb20),18,
        "895cd1c5f870dd7ff343225897583863df86dcbe96010ecf12a3c3760a52e041",
        "3407dec8c90462f67a8084470e5d362fc741ed9ae0bf24befeb54b269ee70dc1",
        [(1,"g_pMemAlloc")],
        "0dc8d5feacd828692663970ee9ecfab1f1fc2f5044bb2c5321c85566cbd92111",
        14,
        {0xb:"8b11",0xe:"ff5220",0x11:"c3"},
    )
}

def verify_native(server:PEImage,client:PEImage,*,check_hash=True,
                  check_imports=True)->dict:
    normalized={}
    n_witnesses=0;n_operands=0;n_direct_calls=0
    for role_index,(role,pe) in enumerate((("server",server),("client",client))):
        if pe.base!=BASE[role]:
            raise VerificationError("Phase206 original PE32 image base mismatch "+role)
        if check_imports:
            imported=imports_with_iat(pe)
            if imported.get("g_pMemAlloc")!=("tier0.dll",IAT[role]["g_pMemAlloc"]):
                raise VerificationError("Phase206 original tier0 g_pMemAlloc IAT name/slot mismatch")
        normalized[role]={}
        for name,(entries,n,server_sha,client_sha,slots,digest,padding,witnesses) in REGIONS.items():
            va=entries[role_index]
            if pe.section_of(va)!=".text":
                raise VerificationError("Phase206 original function not in .text")
            native=bytearray(pe.at(va,n))
            expected=(server_sha,client_sha)[role_index]
            if check_hash and hashlib.sha256(native).hexdigest()!=expected:
                raise VerificationError("Phase206 complete original native SHA mismatch "+role+" "+name)
            for offset,hexcode in witnesses.items():
                code=bytes.fromhex(hexcode)
                if native[offset:offset+len(code)]!=code:
                    raise VerificationError("Phase206 original opcode mismatch "+role+" "+name)
                n_witnesses+=1
            if pe.at(va+n,padding)!=bytes([0xcc])*padding:
                raise VerificationError("Phase206 original function INT3 boundary mismatch "+role+" "+name)
            if name=="zero_alloc":
                site=va+0x1e
                helper=(0x2226e290,0x2416f7f0)[role_index]
                rel=b"\xe8"+struct.pack("<i",helper-site-5)
                if pe.at(site,5)!=rel:
                    raise VerificationError("Phase206 native zero allocator helper CALL mismatch")
                n_direct_calls+=1
            for off,key in slots:
                expected_addr=IAT[role].get(key,DATA[role].get(key))
                if struct.unpack_from("<I",native,off)[0]!=expected_addr:
                    raise VerificationError("Phase206 original static/IAT DWORD mismatch "+role+" "+name)
                if pe.section_of(expected_addr)!=(".rdata" if key in IAT[role] else ".data"):
                    raise VerificationError("Phase206 wrong original static operand memory section")
                native[off:off+4]=bytes(4)
                n_operands+=1
            normalized[role][name]=bytes(native)
    for name,row in REGIONS.items():
        if normalized["server"][name]!=normalized["client"][name]:
            raise VerificationError("Phase206 paired complete original machine code differs beyond globals")
        if check_hash and hashlib.sha256(normalized["server"][name]).hexdigest()!=row[5]:
            raise VerificationError("Phase206 normalized complete native method SHA mismatch "+name)
    return {
        "new_sha_verified_original_functions":len(REGIONS)*2 if check_hash else 0,
        "new_original_native_function_bytes":2*sum(x[1] for x in REGIONS.values()) if check_hash else 0,
        "full_original_function_pair_count":len(REGIONS),
        "exact_original_embedded_absolute_operands":n_operands,
        "sparse_opcode_witnesses":n_witnesses,
        "exact_direct_owner_helper_calls":n_direct_calls,
        "original_tier0_imports_checked":2 if check_imports else 0,
        "original_normalized_full_method_sha256":{k:v[6] if False else v[5] for k,v in REGIONS.items()} if check_hash else None,
        "zero_allocator_native_semantics":{
            "arguments":"two stack uint32 operands","native_count_times_size":"low 32 bits IMUL",
            "allocator_virt_basic_byte_offset":4,"allocator_virt_owner_byte_offset":0,
            "zero_write_order":"REP STOSD followed by REP STOSB",
            "native_oom_check":False,"native_overflow_check":False,
        },
        "opaque_vtable_method_offsets":["0x58","0x20"],
        "limits":["Opaque tier0 virtual slot semantics intentionally not invented",
                  "Portable host model uses synthetic tokens and extra bounds guards",
                  "Real original retail Windows game-engine startup not demonstrated"],
    }

def audit(server:bytes,client:bytes)->dict:
    if hashlib.sha256(server).hexdigest()!=FULL_SHA["server"] or \
       hashlib.sha256(client).hexdigest()!=FULL_SHA["client"]:
        raise VerificationError("Phase206 requires owner-original both complete DLL SHA256s")
    return verify_native(PEImage(server),PEImage(client))

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--server",type=Path,required=True)
    p.add_argument("--client",type=Path,required=True)
    p.add_argument("--output",type=Path)
    a=p.parse_args()
    data=json.dumps(audit(a.server.read_bytes(),a.client.read_bytes()),indent=2)+"\n"
    if a.output:a.output.write_text(data,encoding="utf-8")
    else:print(data,end="")
if __name__=="__main__":main()
