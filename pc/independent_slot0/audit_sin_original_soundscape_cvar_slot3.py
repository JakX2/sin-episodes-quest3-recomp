#!/usr/bin/env python3
"""SHA-original 61-byte SiN 2006 C_SoundscapeSystem virtual slot3.

Read-only --client/--archive authenticate full original 2006 client.dll,
original vtable, complete original method SHA, two literal ConVar names,
global cache writes and two virtual ICvar::FindVar slot3 calls.
Public --self-test uses *fabricated* code windows and mutations only.
Never exports commercial binaries, calls engine functions or shares
original raw method bytes.
"""
import argparse
import hashlib
import json
import sys
import struct
import zipfile
from pathlib import Path
from audit_sin_original_bf_read_candidate_layout import (
    EXPECTED_SHA,parse_image,region_at_va,
)

METHOD=0x240F3C40
SIZE=61
SHA="8a2bce6161d8de0973141bfae7d6c151091ee75933291d1c1d61b15d65b82040"
SYSTEM_VTABLE=0x24293C34
ORIGINAL_INTERFACE_SINGLETON=0x2433F690
CACHE_MIXER=0x2433E4AC
CACHE_DSP=0x2433E4A8
MIXER_NAME_VA=0x24293C70
DSP_NAME_VA=0x24293C64
SOURCE_WINDOWS=(
    (0x240F3C40,"833dace4332400"), # if mixer cache nonzero skip
    (0x240F3C49,"8b0d90f63324"), # original ICvar* global
    (0x240F3C51,"68703c2924"), # snd_soundmixer
    (0x240F3C56,"ff500c"), # original ICvar virtual slot3
    (0x240F3C59,"a3ace43324"), # original cached mixer pointer
    (0x240F3C5E,"833da8e4332400"), # if dsp cache nonzero skip
    (0x240F3C67,"8b0d90f63324"), # same ICvar* global
    (0x240F3C6F,"68643c2924"), # dsp_volume
    (0x240F3C74,"ff520c"), # original ICvar virtual slot3
    (0x240F3C77,"a3a8e43324"), # original cached dsp pointer
    (0x240F3C7C,"c3"), # original void return
)
def verify(method,whole=True):
    if len(method)!=SIZE:
        raise ValueError("original soundscape CVar method size mismatch")
    if whole and hashlib.sha256(method).hexdigest()!=SHA:
        raise ValueError("complete original soundscape slot3 SHA mismatch")
    for va,raw in SOURCE_WINDOWS:
        pattern=bytes.fromhex(raw)
        at=va-METHOD
        if method[at:at+len(pattern)]!=pattern:
            raise ValueError("original soundscape ICvar opcode mismatch at %#x"%va)
    return {
        "original_2006_full_method_sha_verified":bool(whole),
        "original_method_va":hex(METHOD),
        "original_method_bytes":SIZE,
        "original_system_vtable_slot":3,
        "original_engine_cvar_interface_virtual_slot":3,
        "original_convar_names":["snd_soundmixer","dsp_volume"],
        "original_cache_sites":[hex(CACHE_MIXER),hex(CACHE_DSP)],
        "existing_nonnull_cache_skips_find":True,
        "read_only_lookup_no_convar_value_writes":True,
        "selected_source_instruction_witnesses":len(SOURCE_WINDOWS),
        "real_owner_engine_invoked":False,
    }
def private(client):
    if hashlib.sha256(client).hexdigest()!=EXPECTED_SHA:
        raise ValueError("private original client.dll whole SHA mismatch")
    image=parse_image(client)
    if image[0]!=0x24000000:
        raise ValueError("private original 2006 PE32 base differs")
    slot=struct.unpack("<I",region_at_va(
        client,image,SYSTEM_VTABLE+3*4,4))[0]
    if slot!=METHOD:
        raise ValueError("original soundscape slot3 no longer maps to method")
    for va,name in ((MIXER_NAME_VA,"snd_soundmixer"),
                    (DSP_NAME_VA,"dsp_volume")):
        actual=region_at_va(client,image,va,len(name)+1)
        if actual!=name.encode("ascii")+b"\0":
            raise ValueError("original soundscape convar literal differs")
    r=verify(region_at_va(client,image,METHOD,SIZE))
    r.update(original_retail_client_whole_sha_verified=True,
             original_vtable_slot_and_literals_verified=True,
             original_method_or_game_bytes_exported=False)
    return r
def synthetic():
    fake=bytearray(bytes([0xCC])*SIZE)
    for va,raw in SOURCE_WINDOWS:
        token=bytes.fromhex(raw)
        fake[va-METHOD:va-METHOD+len(token)]=token
    out=verify(fake,False)
    assert out["original_engine_cvar_interface_virtual_slot"]==3
    for va,_ in SOURCE_WINDOWS:
        damaged=bytearray(fake)
        damaged[va-METHOD]^=1
        try:verify(damaged,False)
        except ValueError:pass
        else:raise ValueError("fake original soundscape cvar mutation accepted")
    return {
        "original_free_soundscape_convar_slot3_audit":"PASS",
        "source_opcode_witnesses":len(SOURCE_WINDOWS),
        "no_original_binary_loaded":True,
    }
def main():
    p=argparse.ArgumentParser(description=__doc__)
    choices=p.add_mutually_exclusive_group(required=True)
    choices.add_argument("--client",type=Path)
    choices.add_argument("--archive",type=Path)
    choices.add_argument("--self-test",action="store_true")
    args=p.parse_args()
    if args.self_test:result=synthetic()
    elif args.client:result=private(args.client.read_bytes())
    else:
        with zipfile.ZipFile(args.archive) as z:
            result=private(z.read("client.dll"))
    print(json.dumps(result,sort_keys=True))
if __name__=="__main__":
    try:main()
    except (ValueError,AssertionError,FileNotFoundError,
            KeyError,UnicodeError,zipfile.BadZipFile) as exc:
        print("FAIL original soundscape CVar slot3 audit: "+str(exc),
              file=sys.stderr)
        sys.exit(1)
