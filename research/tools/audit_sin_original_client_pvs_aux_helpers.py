#!/usr/bin/env python3
"""Phase232: original SiN Episodes client PVS notifier & auxiliary list helpers.

Read-only original PE32 auditor. Exactly FOUR full native function body
SHA256 reviews after both complete owner-original retail DLL SHA256 gates.
No commercial DLL, full native original method bytes or disassembly emitted.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import struct
import zipfile
from pathlib import Path
from audit_sin_original_dll_interface_abi import ORIGINALS, PEImage, VerificationError
from audit_sin_original_core_factory_thunks import pe_highlow_relocations

# name, complete original native entry, full region size, code SHA256,
# following INT3 padding, original RET4 offsets, sparse instruction witnesses.
METHODS=(
 ("auxiliary_linked_list_append",0x24108680,105,
  "e6e8903a2ca41cf82c90d08ec01862893525edbb459db00db9de2ee6f3b05175",
  7,(0x66,),{
    0x00:"5657",0x02:"6a00",0x22:"66c74006ffff",
    0x30:"66897e0e",0x4E:"6683461201",0x59:"8b44240c",
  }),
 ("auxiliary_linked_list_unlink",0x24106C20,137,
  "17cb2bddf5ae89c032798869d9063ba42412e9a018f176e38928409529a3e743",
  7,(0x6C,0x86),{
    0x00:"53",0x01:"668b5c2408",0x06:"663b5914",
    0x17:"66395c0204",0x5E:"66891e",
    0x64:"66814112ffff",0x7E:"66814112ffff",
  }),
 ("pvs_notifier_register_helper",0x24108880,132,
  "30770d1648b0a0d71d62752ee3a9d108ac91bbf9cdccca479ab1a9a644c4796e",
  12,(0x81,),{
    0x00:"83ec08",0x11:"ff5014",0x1E:"ff5248",
    0x2A:"8db344800100",0x48:"8911",0x4D:"c6410800",
    0x51:"6689410a",0x55:"8b837c800100",
    0x6A:"a3b8fd3324",0x76:"e855feffff",
  }),
 ("pvs_notifier_unregister_helper",0x24108910,190,
  "51ce5be9512ec947d5fdfb5b87de3314d651657195ae12717ca7fe418cfdf394",
  2,(0x74,0xBB),{
    0x00:"83ec08",0x0F:"ff5014",0x1E:"ff5248",
    0x29:"8b867c800100",0x43:"6a00",
    0x60:"68c4732924",0x65:"ff15e0612724",
    0x83:"55",0x8F:"8bce",0x92:"e809e3ffff",
    0xAF:"e87cfeffff",
  }),
)
# Direct original E8 rel32 calls nested within these authentic methods.
# Destination method bodies are NOT credited unless separately reviewed.
CALLS=(
 (0x24108686,0x24106570),  # auxiliary reserve index
 (0x24108690,0x24106C20),  # unlink current index before append
 (0x241088B2,0x241086F0),  # PVS notifier secondary-list allocate
 (0x241088F6,0x24108750),  # PVS map link/setup
 (0x2410895C,0x24106B20),  # PVS map lookup
 (0x24108966,0x24105F40),  # retrieve map node by index
 (0x241089A2,0x24106CB0),  # unlink 16-byte linked node
 (0x241089BB,0x24108840),  # remove mapping
)
# The native warning branch must not be mistaken for tier0 Msg.
WARN_LITERAL_VA=0x242973C4
WARN_LITERAL=b"PVS notifier not in m_PVSNotifierMap\n\x00"
WARN_PUSH_VA=0x24108970
WARN_IAT_IMMEDIATE_VA=0x24108977
WARN_IAT_TARGET=0x242761E0  # original tier0.dll!Warning
# Two distinct original absolute MOV [global], EAX instructions.
PVS_GLOBAL_VA=0x2433FDB8
PVS_GLOBAL_STORES=(0x241088EA,0x2410894E)

def rel_call(site:int,target:int)->bytes:
    return b"\xe8"+struct.pack("<i",target-site-5)

def check_native(pe:PEImage,*,check_hash:bool=True,
                 original_provenance_verified:bool=False,
                 relocations:set[int]|None=None)->dict:
    if pe.base!=ORIGINALS["client"]["base"]:
        raise VerificationError("Phase232 requires original client PE32 base")
    if relocations is None:
        relocations=pe_highlow_relocations(pe)
    rows=[]
    count=0
    for name,va,n,digest,padding,returns,witnesses in METHODS:
        if pe.section_of(va)!=".text" or pe.section_of(va+n-1)!=".text":
            raise VerificationError("Phase232 complete native method outside .text: "+name)
        body=pe.at(va,n)
        if check_hash and hashlib.sha256(body).hexdigest()!=digest:
            raise VerificationError("Phase232 whole original native SHA256 drift: "+name)
        if pe.at(va+n,padding)!=b"\xcc"*padding:
            raise VerificationError("Phase232 trailing original INT3 drift: "+name)
        for off in returns:
            if body[off:off+3]!=b"\xc2\x04\x00":
                raise VerificationError("Phase232 native RET 4 stack ABI drift: "+name)
        for off,hex_bytes in witnesses.items():
            b=bytes.fromhex(hex_bytes)
            if body[off:off+len(b)]!=b:
                raise VerificationError("Phase232 sparse native instruction witness drift: "+name+" "+hex(off))
            count+=1
        rows.append({"original_method":name,"native_va":f"0x{va:08X}",
                     "full_original_native_code_bytes":n,
                     "complete_original_SHA256":digest if check_hash else None,
                     "native_ret4_branches":len(returns),
                     "trailing_INT3":padding})
    for site,target in CALLS:
        if pe.section_of(site)!=".text" or pe.at(site,5)!=rel_call(site,target):
            raise VerificationError("Phase232 original direct nested CALL drift: "+hex(site))
    if pe.at(WARN_LITERAL_VA,len(WARN_LITERAL))!=WARN_LITERAL:
        raise VerificationError("Phase232 original PVS notifier warning string drift")
    if (pe.at(WARN_PUSH_VA,5)!=b"\x68"+struct.pack("<I",WARN_LITERAL_VA) or
        WARN_PUSH_VA+1 not in relocations):
        raise VerificationError("Phase232 original warning string PUSH relocation drift")
    if (pe.at(WARN_IAT_IMMEDIATE_VA-2,6)!=b"\xff\x15"+
        struct.pack("<I",WARN_IAT_TARGET) or
        WARN_IAT_IMMEDIATE_VA not in relocations):
        raise VerificationError("Phase232 original tier0 Warning IAT relocation drift")
    for instruction in PVS_GLOBAL_STORES:
        if (pe.at(instruction,5)!=b"\xa3"+struct.pack("<I",PVS_GLOBAL_VA) or
            instruction+1 not in relocations):
            raise VerificationError("Phase232 native PVS global store relocation drift")
    return {
        "phase":232,
        "new_original_complete_SHA_reviewed_methods":len(rows)
           if check_hash and original_provenance_verified else 0,
        "new_original_native_x86_bytes":sum(x[2] for x in METHODS)
           if check_hash and original_provenance_verified else 0,
        "original_method_evidence":rows,
        "sparse_native_opcode_witnesses":count,
        "direct_original_CALL_rel32_edges":len(CALLS),
        "native_WARNING_uses_tier0_Warning":True,
        "native_warning_text":WARN_LITERAL.decode().rstrip("\0"),
        "original_absolute_PVS_global_store_count":len(PVS_GLOBAL_STORES),
        "original_auxiliary_stride_bytes":8,
        "original_auxiliary_node_previous_word_offset":4,
        "original_auxiliary_node_next_word_offset":6,
        "original_auxiliary_absent_index":0xFFFF,
        "original_auxiliary_active_word_count_offset":0x12,
    }

def audit(server_bytes:bytes,client_bytes:bytes)->dict:
    for module,raw in (("server",server_bytes),("client",client_bytes)):
        if hashlib.sha256(raw).hexdigest()!=ORIGINALS[module]["sha256"]:
            raise VerificationError("Phase232 BOTH full original retail DLL hashes required: "+module)
    report=check_native(PEImage(client_bytes),original_provenance_verified=True)
    if (report["new_original_complete_SHA_reviewed_methods"]!=4 or
        report["new_original_native_x86_bytes"]!=564):
        raise VerificationError("Phase232 original inventory drift")
    report["limits"]=[
        "PVS helper complete bodies are authenticated, but externally invoked entity virtuals and nested manager functions remain unreviewed.",
        "Eight-byte auxiliary list modeled with host-safe indices and injectible allocator; original engine heap and pointer graph not recreated.",
        "PVS notifier map has delegated full behavior to original nested 16-byte manager helpers and remains event-only in the host reference.",
        "Original tier0.dll Warning import identity and string are authenticated; imported implementation not reconstructed.",
        "This does not demonstrate retail Windows engine loading rebuilt gameplay DLLs.",
    ]
    return report

def main()->None:
    p=argparse.ArgumentParser(description=__doc__)
    s=p.add_mutually_exclusive_group(required=True)
    s.add_argument("--archive",type=Path)
    s.add_argument("--server",type=Path)
    p.add_argument("--client",type=Path)
    args=p.parse_args()
    if args.archive:
        if args.client:p.error("--archive conflicts with --client")
        with zipfile.ZipFile(args.archive) as z:
            if z.namelist().count("server.dll")!=1 or z.namelist().count("client.dll")!=1:
                raise VerificationError("Phase232 requires one root retail DLL pair")
            output=audit(z.read("server.dll"),z.read("client.dll"))
    else:
        if not args.client:p.error("--server requires --client")
        output=audit(args.server.read_bytes(),args.client.read_bytes())
    print(json.dumps(output,indent=2))
if __name__=="__main__":
    main()
