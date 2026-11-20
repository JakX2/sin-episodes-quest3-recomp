#!/usr/bin/env python3
"""Original 2006 SiN soundscape dual resource path-ID audit.

Private --client/--archive: SHA authenticate the exact original client.dll,
the entire C_SoundscapeSystem::Init and per-file helper methods, selected
instruction witnesses, relative CALLs and original string literals.
Public --self-test: fabricated code windows only, never includes game bytes.
Does not invoke original soundscape/KeyValues or the user's game engine.
"""
import argparse
import hashlib
import json
import struct
import sys
import zipfile
from pathlib import Path
from audit_sin_original_bf_read_candidate_layout import (
    EXPECTED_SHA, parse_image, region_at_va,
)

INIT=0x240F4160
INIT_BYTES=343
INIT_SHA="0fbb82377e54b7119c2c0e1a0b107c8958b354010993e03a481829cb2a2d661a"
SCRIPT_LOADER=0x240F4070
LOADER_BYTES=233
LOADER_SHA="69d75f737e839fd49af874f8cda11ad9b8b95feb51b2e6d084c34a2eac5c9bfa"
KEYVALUES_LOAD=0x24164A50
MANIFEST_STRING=0x24293D08
GAME_STRING=0x24293B58
FILE_KEY_STRING=0x24293D00
OPTIONAL_MAP_FORMAT=0x24293D2C
FILE_SYSTEM_GLOBAL=0x2433F6BC
# Keep only short, selected verification windows. Never export
# a full original code region or raw input PE.
INIT_WINDOWS=(
    (0x240F4169,"89b5cc000000"), # this+0xCC=0
    (0x240F4180,"682c3d2924"), # scripts/soundscapes_%s.txt format
    (0x240F41B7,"a1bcf63324"), # original filesystem singleton
    (0x240F41BC,"68583b2924"), # manifest pathID="GAME"
    (0x240F41C1,"68083d2924"), # manifest path
    (0x240F41C6,"50"), # filesystem pointer
    (0x240F41C9,"e882080700"), # KeyValues::LoadFromFile
    (0x240F41D0,"0f84bf000000"), # manifest missing/parse fail branch
    (0x240F4219,"e852feffff"), # per-file helper
    (0x240F4274,"6a0057ff5228"), # optional map FileExists(path,null)
    (0x240F4281,"e8eafdffff"), # optional map script helper
    (0x240F4290,"b001"), # original Init TRUE after loaded manifest
    (0x240F4295,"68083d2924"), # failed manifest diagnostic
    (0x240F42B2,"b001"), # original Init TRUE after failed manifest
)
FILE_WINDOWS=(
    (0x240F4073,"6a28"), # fresh KeyValues allocation
    (0x240F4089,"57"), # individual script filename
    (0x240F4097,"a1bcf63324"), # same filesystem global
    (0x240F409C,"6a00"), # pathID == nullptr, NOT "GAME"
    (0x240F409E,"5750"), # script filename and filesystem
    (0x240F40A6,"e8a5090700"), # KeyValues::LoadFromFile
    (0x240F40CB,"8b74240c"), # this object, on loaded document
    (0x240F40CF,"8b7e28"), # per-object soundscape record count
    (0x240F40D2,"8b4620"), # per-object record capacity
    (0x240F40EE,"83460c01"), # append a named record
    (0x240F414C,"8bce"), # failure helper deletes KeyValues
    (0x240F4156,"c20400"), # returns void, caller arg cleaned
)
CALLS=(
    (INIT,0x240F41C9,KEYVALUES_LOAD),
    (INIT,0x240F4219,SCRIPT_LOADER),
    (INIT,0x240F4281,SCRIPT_LOADER),
    (SCRIPT_LOADER,0x240F40A6,KEYVALUES_LOAD),
)
LITERALS={
    MANIFEST_STRING:"scripts/soundscapes_manifest.txt",
    GAME_STRING:"GAME",
    FILE_KEY_STRING:"file",
    OPTIONAL_MAP_FORMAT:"scripts/soundscapes_%s.txt",
}
def method_audit(start,data,expected_size,expected_hash,witnesses,
                 whole_sha):
    if len(data)!=expected_size:
        raise ValueError("original source function length mismatch")
    if whole_sha and hashlib.sha256(data).hexdigest()!=expected_hash:
        raise ValueError("full original source function SHA mismatch")
    for va,hexop in witnesses:
        expected=bytes.fromhex(hexop)
        at=va-start
        if data[at:at+len(expected)]!=expected:
            raise ValueError("original source instruction differs at %#x"%va)
    for method_base,va,target in CALLS:
        if method_base!=start:continue
        at=va-start
        expected=b"\xe8"+struct.pack("<i",target-va-5)
        if data[at:at+5]!=expected:
            raise ValueError("original source CALL differs at %#x"%va)
    return len(witnesses)
def audit(init,helper,whole_sha):
    return {
        "original_init_opcode_witnesses":
            method_audit(INIT,init,INIT_BYTES,INIT_SHA,INIT_WINDOWS,whole_sha),
        "original_per_file_opcode_witnesses":
            method_audit(SCRIPT_LOADER,helper,LOADER_BYTES,LOADER_SHA,
                         FILE_WINDOWS,whole_sha),
        "both_original_full_method_shas_match":whole_sha,
        "manifest_LoadFromFile":"rb/GAME",
        "individual_file_LoadFromFile":"rb/nullptr",
        "map_specific_FileExists":"path/nullptr",
        "original_Init_returns_true_on_manifest_failure":True,
        "original_each_file_load_side_effect_is_void":True,
        "real_game_execution":False,
    }
def private(client):
    if hashlib.sha256(client).hexdigest()!=EXPECTED_SHA:
        raise ValueError("original SiN client.dll whole SHA mismatch")
    image=parse_image(client)
    if image[0]!=0x24000000:
        raise ValueError("original PE image base changed")
    init=region_at_va(client,image,INIT,INIT_BYTES)
    helper=region_at_va(client,image,SCRIPT_LOADER,LOADER_BYTES)
    out=audit(init,helper,True)
    for va,literal in LITERALS.items():
        expected=literal.encode("ascii")+b"\0"
        if region_at_va(client,image,va,len(expected))!=expected:
            raise ValueError("original literal resource differs at %#x"%va)
    out.update(
        original_retail_client_whole_sha_verified=True,
        four_original_resource_literal_pointers_verified=True,
        original_dll_or_script_bytes_exported=False)
    return out
def self_test():
    init=bytearray(bytes([0xCC])*INIT_BYTES)
    helper=bytearray(bytes([0xCC])*LOADER_BYTES)
    for start,blob,fields in (
        (INIT,init,INIT_WINDOWS),
        (SCRIPT_LOADER,helper,FILE_WINDOWS)):
        for va,hexop in fields:
            raw=bytes.fromhex(hexop)
            blob[va-start:va-start+len(raw)]=raw
    out=audit(init,helper,False)
    assert out["original_init_opcode_witnesses"]==len(INIT_WINDOWS)
    assert out["original_per_file_opcode_witnesses"]==len(FILE_WINDOWS)
    for start,blob,witnesses,other in (
        (INIT,init,INIT_WINDOWS,helper),
        (SCRIPT_LOADER,helper,FILE_WINDOWS,init)):
        for va,_ in witnesses:
            broken=bytearray(blob)
            broken[va-start]^=1
            try:
                if start==INIT:audit(broken,other,False)
                else:audit(other,broken,False)
            except ValueError:
                pass
            else:
                raise ValueError("corrupt fabricated original pathID accepted")
    return {
        "source_free_original_soundscape_pathid_audit":"PASS",
        "source_instruction_witnesses":len(INIT_WINDOWS)+len(FILE_WINDOWS),
        "source_direct_call_targets":len(CALLS),
        "private_original_game_bytes_used":False,
    }
def main():
    ap=argparse.ArgumentParser(description=__doc__)
    group=ap.add_mutually_exclusive_group(required=True)
    group.add_argument("--client",type=Path)
    group.add_argument("--archive",type=Path)
    group.add_argument("--self-test",action="store_true")
    args=ap.parse_args()
    if args.self_test:result=self_test()
    elif args.client:result=private(args.client.read_bytes())
    else:
        with zipfile.ZipFile(args.archive) as z:
            result=private(z.read("client.dll"))
    print(json.dumps(result,sort_keys=True))
if __name__=="__main__":
    try:main()
    except (AssertionError,ValueError,FileNotFoundError,
            KeyError,UnicodeError,zipfile.BadZipFile) as exc:
        print("FAIL original soundscape pathID audit: "+str(exc),
              file=sys.stderr)
        sys.exit(1)
