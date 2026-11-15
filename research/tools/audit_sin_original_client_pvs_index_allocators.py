#!/usr/bin/env python3
"""Phase234: SHA-pin original SiN 2006 client PVS 8/16-byte index allocators.

Requires both unmodified owner-original retail DLLs; synthetic fixtures
never count as recovered functions. Does not emit proprietary function bytes.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
import zipfile
from pathlib import Path
from audit_sin_original_dll_interface_abi import ORIGINALS, PEImage, VerificationError

# Native VA, exact complete code size + digest, 6 INT3 after two RET4 exits,
# and sparse original x86 instruction witnesses. No whole code bytes stored.
METHODS=(
    ("aux_eight_byte_index_allocator",0x24106570,154,
     "3f972fba67fca64b208624a577230cec867af88128418bce19a6df633314097a",
     6,(0x77,0x97),{
       0x00:"568bf157",0x04:"668b7e10",0x08:"6681ffffff",
       0x0F:"0fb746143b4604",0x18:"6a01",0x1F:"668b7e14",
       0x31:"68407e2d24",0x36:"ff15fc612724",
       0x41:"8b060fb7d7",0x5F:"751966897c1004",
       0x7A:"8d4c1004",0x7E:"66c701ffff",
     }),
    ("pvs_sixteen_byte_index_allocator",0x24106610,154,
     "c03f2c59e9484db8a76c31258d43d12272493ec83813bcac23f5ef179a3e594e",
     6,(0x77,0x97),{
       0x00:"568bf157",0x04:"668b7e10",0x08:"6681ffffff",
       0x0F:"0fb746143b4604",0x18:"6a01",0x1F:"668b7e14",
       0x31:"68407e2d24",0x36:"ff15fc612724",
       0x41:"8b060fb7d7",0x5F:"751966897c100c",
       0x7A:"8d4c100c",0x7E:"66c701ffff",
     }),
)
CALLS=(
    (0x2410658A,0x24106010), # aux growth with PUSH 1; body opaque
    (0x2410662A,0x24106090), # 16-byte growth with PUSH 1; body opaque
    (0x24108686,0x24106570), # Phase232 8-byte append calls allocator
    (0x241086F6,0x24106610), # Phase233 16-byte append calls allocator
)
ERROR_STRING_VA=0x242D7E40
ERROR_STRING=b"CUtlLinkedList overflow!\n\x00"
ERROR_IAT=0x242761FC # original tier0.dll!Error thunk (not Warning)

def rel_call(site:int,target:int)->bytes:
    return b"\xE8"+struct.pack("<i",target-site-5)

def verify_native(client:PEImage,*,check_hash:bool=True,
                  authentic_provenance:bool=False)->dict:
    if client.base!=ORIGINALS["client"]["base"]:
        raise VerificationError("Phase234 original client PE base drift")
    if client.section_of(ERROR_STRING_VA)!=".rdata" or \
            client.at(ERROR_STRING_VA,len(ERROR_STRING))!=ERROR_STRING:
        raise VerificationError("Phase234 original tier0 Error string drift")
    entries=[]
    for name,va,length,digest,pad,ret4,anchors in METHODS:
        if client.section_of(va)!=".text" or \
                client.section_of(va+length-1)!=".text":
            raise VerificationError("Phase234 original method outside .text: "+name)
        code=client.at(va,length)
        if check_hash and hashlib.sha256(code).hexdigest()!=digest:
            raise VerificationError("Phase234 original whole-body SHA256 drift: "+name)
        if client.at(va+length,pad)!=b"\xCC"*pad:
            raise VerificationError("Phase234 native INT3 boundary drift: "+name)
        for offset in ret4:
            if code[offset:offset+3]!=b"\xC2\x04\x00":
                raise VerificationError("Phase234 original RET4 drift: "+name)
        for off,hx in anchors.items():
            raw=bytes.fromhex(hx)
            if code[off:off+len(raw)]!=raw:
                raise VerificationError("Phase234 original opcode witness drift: "+name+" "+hex(off))
        entries.append({"name":name,"native_entry":f"0x{va:08X}",
                        "native_code_bytes":length,"whole_original_sha256":digest if check_hash else None,
                        "ret4_paths":len(ret4),"trailing_int3":pad})
    for site,dest in CALLS:
        if client.section_of(site)!=".text" or client.at(site,5)!=rel_call(site,dest):
            raise VerificationError("Phase234 original direct CALL edge drift "+hex(site))
    return {"phase":234,
            "new_original_complete_sha_methods":len(entries) if check_hash and authentic_provenance else 0,
            "new_original_code_bytes":sum(x[2] for x in METHODS) if check_hash and authentic_provenance else 0,
            "methods":entries,"sparse_instruction_witnesses":sum(len(x[-1]) for x in METHODS),
            "original_call_edges":len(CALLS),"record_strides":[8,16],
            "prev_next_offsets":[[4,6],[12,14]],
            "manager_free_head_word":0x10,"manager_next_unused_word":0x14,
            "manager_capacity_dword":4,"manager_base_dword":0,
            "manager_cached_base_dword":0x18,
            "error_import":"tier0.dll!Error","error_IAT":f"0x{ERROR_IAT:08X}",
            "scope_limits":["Original growth bodies 0x24106010 and 0x24106090 are not recovered.",
              "Host allocator is synthetic storage, not native Source engine objects.",
              "Original client/server Windows gameplay DLL loading remains unverified."]}

def audit(server:bytes,client:bytes)->dict:
    for name,raw in (("server",server),("client",client)):
        if hashlib.sha256(raw).hexdigest()!=ORIGINALS[name]["sha256"]:
            raise VerificationError("Phase234 requires BOTH owner-original retail DLL SHA256s: "+name)
    return verify_native(PEImage(client),authentic_provenance=True)

def main()->None:
    p=argparse.ArgumentParser(description=__doc__)
    q=p.add_mutually_exclusive_group(required=True)
    q.add_argument("--archive",type=Path)
    q.add_argument("--server",type=Path)
    p.add_argument("--client",type=Path)
    args=p.parse_args()
    if args.archive:
        if args.client:p.error("--archive excludes --client")
        with zipfile.ZipFile(args.archive) as z:
            if z.namelist().count("client.dll")!=1 or z.namelist().count("server.dll")!=1:
                raise VerificationError("Phase234 requires one root DLL pair")
            report=audit(z.read("server.dll"),z.read("client.dll"))
    else:
        if not args.client:p.error("--server requires --client")
        report=audit(args.server.read_bytes(),args.client.read_bytes())
    print(json.dumps(report,indent=2))
if __name__=="__main__":main()
