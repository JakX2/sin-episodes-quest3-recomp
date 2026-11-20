#!/usr/bin/env python3
"""SiN Episodes 2006 base AutoGameSystem destructor/registry erase.

--client/--archive: SHA-authenticate original retail client.dll,
26-byte base destructor at 0x24156820 and 90-byte global registry
first-match removal at 0x241565E0, plus selected exact source
instruction windows and original direct call targets.
--self-test: fabricated opcode windows and mutation refusals only.
No original game object, client.dll code or engine data exported.
"""
import argparse
import hashlib
import json
import struct
import sys
import zipfile
from pathlib import Path
from audit_sin_original_bf_read_candidate_layout import (
    EXPECTED_SHA,parse_image,region_at_va,
)
BASE_DTOR=0x24156820
BASE_SIZE=26
BASE_SHA="cecc59ef0571ba365a6ce38669ea8c6a229ce3669a8f9f00bc27d06b3e892b8a"
REG_REMOVE=0x241565E0
REMOVE_SIZE=90
REMOVE_SHA="f71baaca312397a0a87e72799b2b20bb69214751462124cd73e3b4e19b1f0807"
GLOBAL_REGISTRY=0x242FBFA8
GLOBAL_COUNT=0x242FBFB4
BASE_VPTR=0x24293B1C
MEMMOVE=0x24248B50
BASE_WINDOWS=(
    (0x24156820,"518d0424"),
    (0x24156824,"c7011c3b2924"),
    (0x2415682A,"890c2450"),
    (0x2415682E,"b9a8bf2f24"),
    (0x24156833,"e8a8fdffff"),
    (0x24156838,"59c3"),
)
REMOVE_WINDOWS=(
    (0x241565E0,"568bf18b560c33c085d27e4a"),
    (0x241565EC,"8b4c2408538b1e578b39"),
    (0x241565F6,"8bcb39397411"),
    (0x241565FC,"83c00183c1043b460c7cf1"),
    (0x24156607,"5f5b5ec20400"),
    (0x2415660D,"83f8ff7422"),
    (0x24156612,"2bd083ea0185d27e15"),
    (0x2415661B,"03d28d048303d2528d48045150"),
    (0x24156628,"e823250f00"),
    (0x2415662D,"83c40c83460cff"),
    (0x24156634,"5f5b5ec20400"),
)
DIRECT_CALLS=(
    (BASE_DTOR,0x24156833,REG_REMOVE),
    (REG_REMOVE,0x24156628,MEMMOVE),
)
def inspect(method,start,size,sha,windows,whole):
    if len(method)!=size:
        raise ValueError("2006 AutoGameSystem method length mismatch")
    if whole and hashlib.sha256(method).hexdigest()!=sha:
        raise ValueError("2006 AutoGameSystem method complete SHA differs")
    for va,op in windows:
        token=bytes.fromhex(op)
        if method[va-start:va-start+len(token)]!=token:
            raise ValueError("original game-system method opcode changed at %#x"%va)
    for origin,va,target in DIRECT_CALLS:
        if origin!=start:continue
        token=b"\xe8"+struct.pack("<i",target-va-5)
        if method[va-start:va-start+5]!=token:
            raise ValueError("original game-system direct target changed at %#x"%va)
    return len(windows)
def audit(dtor,remove,whole):
    b=inspect(dtor,BASE_DTOR,BASE_SIZE,BASE_SHA,BASE_WINDOWS,whole)
    r=inspect(remove,REG_REMOVE,REMOVE_SIZE,REMOVE_SHA,REMOVE_WINDOWS,whole)
    return {
        "complete_original_26B_base_destructor_sha_verified":bool(whole),
        "complete_original_90B_registry_remove_sha_verified":bool(whole),
        "source_base_destructor_va":hex(BASE_DTOR),
        "source_global_registry_va":hex(GLOBAL_REGISTRY),
        "source_global_registry_count_va":hex(GLOBAL_COUNT),
        "source_base_virtual_table_va":hex(BASE_VPTR),
        "source_remove_first_matching_pointer_only":True,
        "source_duplicate_pointer_entries_preserved_after_first_remove":True,
        "source_stable_memmove_after_match":True,
        "source_unmodified_on_empty_or_missing_identity":True,
        "source_does_not_zero_trailing_pointer_slot":True,
        "source_memmove_va":hex(MEMMOVE),
        "source_erase_target_size_bytes":4,
        "source_selected_opcode_witnesses":b+r,
        "source_direct_call_targets_verified":len(DIRECT_CALLS),
        "original_registry_memory_mutated":False,
    }
def private(client):
    if hashlib.sha256(client).hexdigest()!=EXPECTED_SHA:
        raise ValueError("owner SHA-pinned 2006 client.dll differs")
    pe=parse_image(client)
    if pe[0]!=0x24000000:
        raise ValueError("original PE32 image base mismatch")
    r=audit(region_at_va(client,pe,BASE_DTOR,BASE_SIZE),
            region_at_va(client,pe,REG_REMOVE,REMOVE_SIZE),True)
    r.update(original_retail_client_sha_authenticated=True,
             original_game_code_bytes_exported=False)
    return r
def self_test():
    methods=[]
    for va,size,windows in (
        (BASE_DTOR,BASE_SIZE,BASE_WINDOWS),
        (REG_REMOVE,REMOVE_SIZE,REMOVE_WINDOWS)):
        fake=bytearray(bytes([0xCC])*size)
        for site,op in windows:
            token=bytes.fromhex(op)
            fake[site-va:site-va+len(token)]=token
        methods.append(fake)
    assert audit(*methods,False)["source_direct_call_targets_verified"]==2
    for i,(va,_,windows) in enumerate((
        (BASE_DTOR,BASE_SIZE,BASE_WINDOWS),
        (REG_REMOVE,REMOVE_SIZE,REMOVE_WINDOWS))):
        for site,_ in windows:
            broken=[bytes(x) for x in methods]
            wrong=bytearray(broken[i])
            wrong[site-va]^=1
            broken[i]=bytes(wrong)
            try:audit(*broken,False)
            except ValueError:pass
            else:raise ValueError("damaged synthetic registry cleanup accepted")
    return {
        "original_free_auto_game_system_registry_remove_audit":"PASS",
        "source_selected_opcodes":len(BASE_WINDOWS)+len(REMOVE_WINDOWS),
        "direct_original_call_targets":len(DIRECT_CALLS),
        "no_original_dll_bytes":True,
    }
def main():
    p=argparse.ArgumentParser(description=__doc__)
    group=p.add_mutually_exclusive_group(required=True)
    group.add_argument("--client",type=Path)
    group.add_argument("--archive",type=Path)
    group.add_argument("--self-test",action="store_true")
    args=p.parse_args()
    if args.self_test:out=self_test()
    elif args.client:out=private(args.client.read_bytes())
    else:
        with zipfile.ZipFile(args.archive) as z:
            out=private(z.read("client.dll"))
    print(json.dumps(out,sort_keys=True))
if __name__=="__main__":
    try:main()
    except (ValueError,AssertionError,FileNotFoundError,
            KeyError,UnicodeError,zipfile.BadZipFile) as exc:
        print("FAIL original AutoGameSystem registry source audit: "+str(exc),
              file=sys.stderr)
        sys.exit(1)
