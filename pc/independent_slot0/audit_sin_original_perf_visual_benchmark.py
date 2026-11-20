#!/usr/bin/env python3
"""SHA-authenticated owner 2006 CPerfVisualBenchmark::Init metadata audit.

Read-only original client.dll / bin.zip, never copies or exports method
bytes. Public CI must use --self-test, which constructs fabricated bytes.
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

INIT_VA=0x24182800
INIT_SIZE=0x44D
INIT_SHA="b0d4cc7e256fd830307d51db70757bb857c0951ec220976a4e281c796ae89b51"
INSERT_VA=0x24182500
INSERT_SIZE=0x8D
INSERT_SHA="1edb9978199fcb2d8eb80dab896085e7093588d130306130ea8b38c0efeb8989"
FIRST_EMPTY=0x242d330e
LOW=0x242d3a6c
HIGH=0x242d6ccc
ONE_HALF=0x3fc00000
TEN=0x41200000
# (source CALL VA, command pointer/name, two options, label VA/text, weight)
# Exact text references are metadata, never original compiled objects.
RECORDS=(
 (0x24182838, FIRST_EMPTY,"",FIRST_EMPTY,"",FIRST_EMPTY,"",0x242d4b6c,"Default",ONE_HALF),
 (0x24182872, 0x242a22c4,"r_drawdetailprops",LOW,"0",HIGH,"1",0x242a22b4,"detail props",ONE_HALF),
 (0x241828a2, 0x242a22a8,"r_drawworld",LOW,"0",HIGH,"1",0x242a2298,"world geometry",ONE_HALF),
 (0x241828d2, 0x242a2288,"r_drawentities",LOW,"0",HIGH,"1",0x242a227c,"entities",ONE_HALF),
 (0x24182902, 0x242a2274,"r_3dsky",LOW,"0",HIGH,"1",0x242a2268,"3D skybox",ONE_HALF),
 (0x24182932, 0x242a2258,"r_drawdecals",LOW,"0",HIGH,"1",0x242a2250,"decals",ONE_HALF),
 (0x24182962, 0x242972c8,"mat_stub",HIGH,"1",LOW,"0",0x242a2234,"material system and below",ONE_HALF),
 (0x2418299a, 0x242a2220,"mat_viewportscale",0x242a221c,".2",0x2428528c,"1.0",0x242a2210,"fillrate",ONE_HALF),
 (0x241829ca, 0x242a21fc,"r_drawstaticprops",LOW,"0",HIGH,"1",0x242a2210,"fillrate",ONE_HALF),
 (0x241829fa, 0x242a21e8,"r_drawbrushmodels",LOW,"0",HIGH,"1",0x242a21d8,"brush models",ONE_HALF),
 (0x24182a2a, 0x242a21c0,"r_renderoverlayfragment",LOW,"0",HIGH,"1",0x242a21b4,"overlays",ONE_HALF),
 (0x24182a5a, 0x242a21a8,"r_drawdisp",LOW,"0",HIGH,"1",0x242a2198,"displacements",ONE_HALF),
 (0x24182a8a, 0x242a2188,"r_drawviewmodel",LOW,"0",HIGH,"1",0x24286120,"viewmodel",ONE_HALF),
 (0x24182aba, 0x24297a04,"cl_drawhud",LOW,"0",HIGH,"1",0x242a2184,"hud",ONE_HALF),
 (0x24182aea, 0x242a1c14,"r_drawparticles",LOW,"0",HIGH,"1",0x242a2178,"particles",ONE_HALF),
 (0x24182b1a, 0x242943c8,"r_drawsprites",LOW,"0",HIGH,"1",0x242a2170,"sprites",ONE_HALF),
 (0x24182b4a, 0x242a2164,"mat_bumpmap",LOW,"0",HIGH,"1",0x242a2154,"bump mapping",TEN),
 (0x24182b7a, 0x242a2144,"mat_specular",LOW,"0",HIGH,"1",0x242a2138,"specularity",TEN),
 (0x24182baa, 0x242a2128,"mat_drawwater",LOW,"0",HIGH,"1",0x24285b18,"water",ONE_HALF),
 (0x24182bda, 0x242a211c,"r_dynamic",LOW,"0",HIGH,"1",0x242a2108,"dynamic lighting",ONE_HALF),
 (0x24182c0a, 0x24297d6c,"r_shadows",LOW,"0",HIGH,"1",0x242a2100,"shadows",ONE_HALF),
 (0x24182c3a, 0x242936c4,"r_drawropes",LOW,"0",HIGH,"1",0x242a20f8,"ropes",ONE_HALF),
)
# Final two explicit original writes: this+0x18=0 and return true.
FINAL=(0x24182c41,bytes.fromhex("c64518005db0015b83c418c3"))
# The 24-byte native helper copies 6 DWORDs, may grow vector and
# memmove tail, and returns with callee stack cleanup (ret 8).
HELPER_MARKERS=(
 (0x24182503,bytes.fromhex("8b460c")),
 (0x24182506,bytes.fromhex("8b4e04")),
 (0x2418251e,bytes.fromhex("83460c01")),
 (0x2418253e,bytes.fromhex("8d147f")),
 (0x24182560,bytes.fromhex("8b4c2410")),
 (0x24182583,bytes.fromhex("894814")),
 (0x2418258a,bytes.fromhex("c20800")),
)

def check_method(method,helper,check_digest=True):
    if len(method)!=INIT_SIZE or len(helper)!=INSERT_SIZE:
        raise ValueError("original benchmark method/helper length mismatch")
    if check_digest:
        if hashlib.sha256(method).hexdigest()!=INIT_SHA:
            raise ValueError("original CPerfVisualBenchmark Init hash mismatch")
        if hashlib.sha256(helper).hexdigest()!=INSERT_SHA:
            raise ValueError("original benchmark insertion helper hash mismatch")
    for entry in RECORDS:
        va=entry[0]
        at=va-INIT_VA
        opcode=b"\xe8"+struct.pack("<i",INSERT_VA-va-5)
        if method[at:at+5]!=opcode:
            raise ValueError("original benchmark insertion CALL mismatch at %#x"%va)
    for va,code in (FINAL,):
        at=va-INIT_VA
        if method[at:at+len(code)]!=code:
            raise ValueError("original benchmark final state/return mismatch")
    for va,code in HELPER_MARKERS:
        at=va-INSERT_VA
        if helper[at:at+len(code)]!=code:
            raise ValueError("original benchmark copy/move helper guard mismatch")
    if len(RECORDS)!=22 or sum(entry[-1]==TEN for entry in RECORDS)!=2:
        raise ValueError("unexpected benchmark source metadata")
    return {"original_method_sha_verified": bool(check_digest),
            "original_insert_helper_sha_verified": bool(check_digest),
            "insertion_calls_verified":len(RECORDS),
            "descriptor_bytes_copied_by_helper":24,
            "known_initialized_fields_per_descriptor":5,
            "last_copied_dword_value_authenticated":False,
            "two_high_weight_records":2,
            "return_value":True}

def private_audit(dll):
    if hashlib.sha256(dll).hexdigest()!=EXPECTED_SHA:
        raise ValueError("original client.dll global SHA mismatch")
    image=parse_image(dll)
    if image[0]!=0x24000000:
        raise ValueError("original 2006 image base mismatch")
    method=region_at_va(dll,image,INIT_VA,INIT_SIZE)
    helper=region_at_va(dll,image,INSERT_VA,INSERT_SIZE)
    report=check_method(method,helper)
    for row in RECORDS:
        for va,expected in ((row[1],row[2]),(row[3],row[4]),
                            (row[5],row[6]),(row[7],row[8])):
            actual=region_at_va(dll,image,va,len(expected)+1)
            if actual!=expected.encode("ascii")+b"\0":
                raise ValueError("original benchmark descriptor string mismatch at %#x"%va)
    report.update(original_client_sha_verified=True,
                  descriptor_text_refs_verified=22*4,
                  native_convar_mutations_invoked=False,
                  game_boot_verified=False)
    return report

def self_test():
    method=bytearray(b"\xcc"*INIT_SIZE)
    helper=bytearray(b"\xcc"*INSERT_SIZE)
    for row in RECORDS:
        va=row[0]
        at=va-INIT_VA
        method[at:at+5]=b"\xe8"+struct.pack("<i",INSERT_VA-va-5)
    at=FINAL[0]-INIT_VA
    method[at:at+len(FINAL[1])]=FINAL[1]
    for va,code in HELPER_MARKERS:
        at=va-INSERT_VA
        helper[at:at+len(code)]=code
    actual=check_method(method,helper,check_digest=False)
    assert actual["insertion_calls_verified"]==22
    for location in (RECORDS[0][0],RECORDS[21][0],FINAL[0]):
        mutated=bytearray(method)
        mutated[location-INIT_VA]^=1
        try:check_method(mutated,helper,check_digest=False)
        except ValueError:pass
        else:raise ValueError("missing original method corruption refusal")
    mutated=bytearray(helper)
    mutated[0x24182583-INSERT_VA]^=1
    try:check_method(method,mutated,check_digest=False)
    except ValueError:pass
    else:raise ValueError("missing helper corruption refusal")
    return {"original_free_synthetic_benchmark_audit":"PASS",
            "descriptor_calls":len(RECORDS),
            "no_game_binaries_used":True}

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--self-test",action="store_true")
    group=p.add_mutually_exclusive_group()
    group.add_argument("--client",type=Path)
    group.add_argument("--archive",type=Path)
    a=p.parse_args()
    if a.self_test:report=self_test()
    elif a.client:report=private_audit(a.client.read_bytes())
    elif a.archive:
        with zipfile.ZipFile(a.archive) as z:
            report=private_audit(z.read("client.dll"))
    else:p.error("provide --self-test, --client or --archive")
    print(json.dumps(report,sort_keys=True))

if __name__=="__main__":
    try:main()
    except (AssertionError,ValueError,FileNotFoundError,
            KeyError,UnicodeError,zipfile.BadZipFile) as exc:
        print("FAIL original performance benchmark SHA audit: "+str(exc),
              file=sys.stderr)
        sys.exit(1)
