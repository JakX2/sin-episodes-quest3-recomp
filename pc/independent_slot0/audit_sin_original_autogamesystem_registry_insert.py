#!/usr/bin/env python3
"""Original 2006 game-system global ordered insertion and append audit.

Private --archive or --client: SHA-verify the user-owned retail client.dll,
complete 100-byte insertion helper at 0x24156640 and 17-byte append
wrapper at 0x24156800; verify selected x86 instruction windows and
three real direct CALL targets. Never execute original engine functions.
Public --self-test: fabricated instruction byte windows only.
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
INSERT=0x24156640
INSERT_BYTES=100
INSERT_SHA="486336177fcab7b1b790c9cffa47767574b21790bb88d23ea570c71883d197e2"
APPEND=0x24156800
APPEND_BYTES=17
APPEND_SHA="d8347541722acb5f3079e4a815ecfa670797097a8f64173fd638291a4f60b711"
ENSURE_CAPACITY=0x241562F0
MEMMOVE=0x24248B50
INSERT_WINDOWS=(
    (0x24156640,"568bf1"),
    (0x24156643,"8b460c8b4e04"),
    (0x24156649,"8d50013bd1"),
    (0x2415664E,"577e0d"),
    (0x24156651,"2bc183c001508bce"),
    (0x24156659,"e892fcffff"),
    (0x2415665E,"83460c01"),
    (0x24156662,"8b460c8b7c240c8b0e"),
    (0x2415666B,"2bc783e801"),
    (0x24156670,"85c0894e10"),
    (0x24156675,"7e15"),
    (0x24156677,"03c003c08d0cb9"),
    (0x2415667E,"505183c10451"),
    (0x24156684,"e8c7240f00"),
    (0x24156689,"83c40c"),
    (0x2415668C,"8b0e8d04b985c07408"),
    (0x24156695,"8b5424108b0a8908"),
    (0x2415669D,"8bc75f5ec20800"),
)
APPEND_WINDOWS=(
    (0x24156800,"8b442404"),
    (0x24156804,"8b510c"),
    (0x24156807,"5052"),
    (0x24156809,"e832feffff"),
    (0x2415680E,"c20400"),
)
CALLS=(
    (INSERT,0x24156659,ENSURE_CAPACITY),
    (INSERT,0x24156684,MEMMOVE),
    (APPEND,0x24156809,INSERT),
)
def inspect(method,start,sz,digest,witnesses,whole):
    if len(method)!=sz:
        raise ValueError("original game-system insertion function size mismatch")
    if whole and hashlib.sha256(method).hexdigest()!=digest:
        raise ValueError("original insertion full-method SHA mismatch")
    for site,hexop in witnesses:
        token=bytes.fromhex(hexop)
        if method[site-start:site-start+len(token)]!=token:
            raise ValueError("original game-system insertion opcode changed at %#x"%site)
    for owner,site,target in CALLS:
        if owner!=start:continue
        expected=b"\xe8"+struct.pack("<i",target-site-5)
        if method[site-start:site-start+5]!=expected:
            raise ValueError("original insertion call target differs at %#x"%site)
    return len(witnesses)
def audit(insert,append,whole):
    i=inspect(insert,INSERT,INSERT_BYTES,INSERT_SHA,INSERT_WINDOWS,whole)
    a=inspect(append,APPEND,APPEND_BYTES,APPEND_SHA,APPEND_WINDOWS,whole)
    return {
        "original_insert_100_byte_whole_sha_verified":bool(whole),
        "original_append_17_byte_whole_sha_verified":bool(whole),
        "original_registry_insertion_va":hex(INSERT),
        "original_registry_append_va":hex(APPEND),
        "original_capacity_grow_va":hex(ENSURE_CAPACITY),
        "original_stable_memmove_va":hex(MEMMOVE),
        "original_list_count_offset":"0xC",
        "original_list_capacity_offset":"0x4",
        "original_list_pointer_mirror_offset":"0x10",
        "original_append_passes_active_count_as_insert_index":True,
        "original_insert_shifts_later_four_byte_pointers_right":True,
        "original_duplicate_pointer_suppression":False,
        "original_insert_returns_index":True,
        "original_selected_opcode_witnesses":i+a,
        "original_direct_call_targets_verified":len(CALLS),
        "original_allocator_growth_called":False,
    }
def private(client):
    if hashlib.sha256(client).hexdigest()!=EXPECTED_SHA:
        raise ValueError("retail 2006 original client.dll SHA differs")
    image=parse_image(client)
    if image[0]!=0x24000000:
        raise ValueError("original PE32 image base mismatch")
    output=audit(region_at_va(client,image,INSERT,INSERT_BYTES),
                 region_at_va(client,image,APPEND,APPEND_BYTES),True)
    output.update(original_client_whole_sha_verified=True,
                  original_method_bytes_exported=False)
    return output
def self_test():
    fake=[]
    for base,size,windows in (
        (INSERT,INSERT_BYTES,INSERT_WINDOWS),
        (APPEND,APPEND_BYTES,APPEND_WINDOWS)):
        raw=bytearray(b"\xcc"*size)
        for site,opcode in windows:
            b=bytes.fromhex(opcode)
            raw[site-base:site-base+len(b)]=b
        fake.append(raw)
    output=audit(*fake,False)
    assert output["original_direct_call_targets_verified"]==3
    for i,(base,_,windows) in enumerate((
        (INSERT,INSERT_BYTES,INSERT_WINDOWS),
        (APPEND,APPEND_BYTES,APPEND_WINDOWS))):
        for site,_ in windows:
            bad=[bytes(x) for x in fake]
            changed=bytearray(bad[i])
            changed[site-base]^=1
            bad[i]=bytes(changed)
            try:audit(*bad,False)
            except ValueError:pass
            else:raise ValueError("corrupted fabricated insertion accepted")
    return {
        "original_free_game_system_registry_insert_audit":"PASS",
        "source_opcode_witnesses":len(INSERT_WINDOWS)+len(APPEND_WINDOWS),
        "original_direct_call_targets_checked":len(CALLS),
        "proprietary_client_dll_used":False,
    }
def main():
    p=argparse.ArgumentParser(description=__doc__)
    group=p.add_mutually_exclusive_group(required=True)
    group.add_argument("--archive",type=Path)
    group.add_argument("--client",type=Path)
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
    except (ValueError,AssertionError,FileNotFoundError,KeyError,
            UnicodeError,zipfile.BadZipFile) as exc:
        print("FAIL original registry insertion source audit: "+str(exc),
              file=sys.stderr)
        sys.exit(1)
