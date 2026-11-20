#!/usr/bin/env python3
"""Phase 488: read-only original SiN soundscape WAV/MP3 source inventory.

Private --scripts validates the same seven SHA256-pinned original 2006
files. Outputs counts / safe marker and format statistics ONLY.
Public --self-test uses invented wave declarations; no game assets.
No claim is made about actual sound emitter prefix semantics, DSP,
random scheduling or real game playback.
"""
import argparse
import collections
import hashlib
import json
import re
import sys
import zipfile
from pathlib import Path
from audit_sin_original_soundscape_catalog import EXPECTED, parse
from audit_sin_original_soundscape_manifest import (
    FILES_OBSERVED, MANIFEST_PATH, inspect_manifest,
)

EXPECTED_WAVES = {
    "documents":7,
    "definitions":301,
    "definitions_with_waves":284,
    "looping_groups":281,
    "random_groups":150,
    "rndwave_blocks":150,
    "looping_waves":281,
    "random_waves":943,
    "raw_wave_occurrences":1224,
    "unique_raw_waves":510,
    "wav_occurrences":782,
    "mp3_occurrences":442,
    "star_leading":449,
    "paren_leading":91,
    "compound_star_paren":20,
    "unmarked":684,
    "maximum_random_candidates":20,
    "minimum_random_candidates":2,
}
PINNED_RANDOM_WAVE_GROUP_SIZES = {
    2:1,3:7,4:30,5:41,6:18,7:29,8:5,9:1,10:5,
    11:1,12:5,15:3,16:2,17:1,20:1
}
VALID_PATH = re.compile(r"^[A-Za-z0-9_./-]+$")

def normalize(raw):
    if not isinstance(raw,str) or not 6<=len(raw)<=240:
        raise ValueError("invalid soundscape wave token")
    i=0
    while i<len(raw) and raw[i] in "*)":
        i+=1
    if i>2 or i==len(raw):
        raise ValueError("invalid soundscape control marker")
    markers,asset=raw[:i],raw[i:]
    if (not VALID_PATH.fullmatch(asset) or
        "/" not in asset or
        any(x in ("",".","..") for x in asset.split("/")) or
        not asset.lower().endswith((".wav",".mp3"))):
        raise ValueError("unsafe soundscape-relative wave path")
    return markers,asset

def inventory(named_scripts):
    counts=collections.Counter()
    originals=[]
    def_names=set()
    for path,text in named_scripts:
        roots=parse(text)
        counts["documents"]+=1
        for name,nodes in roots:
            folded=name.lower()
            if not name or folded in def_names:
                raise ValueError("duplicate source root")
            def_names.add(folded)
            counts["definitions"]+=1
            wave_in_root=0

            def visit(values):
                nonlocal wave_in_root
                for key,value in values:
                    name=key.lower()
                    if name=="playlooping":
                        if not isinstance(value,list):
                            raise ValueError("looping group is not a block")
                        waves=[v for k,v in value if k.lower()=="wave"]
                        if len(waves)==0:
                            raise ValueError("empty looping wave")
                        counts["looping_groups"]+=1
                        for wave in waves:
                            if not isinstance(wave,str):
                                raise ValueError("nested looping wave")
                            register(wave,"loop")
                            wave_in_root+=1
                    elif name=="playrandom":
                        if not isinstance(value,list):
                            raise ValueError("random group is not a block")
                        counts["random_groups"]+=1
                        candidates=0
                        for inner_key,inner_value in value:
                            if inner_key.lower()!="rndwave":
                                if isinstance(inner_value,list):
                                    raise ValueError("unexpected random nested block")
                                continue
                            if not isinstance(inner_value,list):
                                raise ValueError("rndwave is not a block")
                            counts["rndwave_blocks"]+=1
                            for cand_key,cand_value in inner_value:
                                if cand_key.lower()!="wave":
                                    continue
                                if not isinstance(cand_value,str):
                                    raise ValueError("nested random wave")
                                register(cand_value,"random")
                                candidates+=1
                                wave_in_root+=1
                        if candidates==0:
                            raise ValueError("empty random waves")
                        counts["maximum_random_candidates"]=max(
                            counts["maximum_random_candidates"],candidates)
                        counts["minimum_random_candidates"]=min(
                            counts["minimum_random_candidates"],candidates)
                            if counts["random_groups"]>1 else candidates
                        counts["random_group_size_"+str(candidates)]+=1
                    elif name=="wave" and isinstance(value,str):
                        raise ValueError("unattached wave property")
                    elif isinstance(value,list):
                        visit(value)

            def register(raw,group):
                markers,asset=normalize(raw)
                originals.append(raw.lower())
                counts["raw_wave_occurrences"]+=1
                counts["looping_waves" if group=="loop"
                       else "random_waves"]+=1
                counts["wav_occurrences" if asset.lower().endswith(".wav")
                       else "mp3_occurrences"]+=1
                if not markers:counts["unmarked"]+=1
                elif markers.startswith("*"):counts["star_leading"]+=1
                else:counts["paren_leading"]+=1
                if markers=="*)":counts["compound_star_paren"]+=1

            visit(nodes)
            if wave_in_root>0:
                counts["definitions_with_waves"]+=1

    counts["unique_raw_waves"]=len(set(originals))
    report={key:counts[key] for key in EXPECTED_WAVES}
    report["random_group_sizes"]={
        str(k):v for k,v in sorted(
            (int(k.split("_")[-1]),v)
            for k,v in counts.items() if k.startswith("random_group_size_"))}
    return report

def private(archive):
    with zipfile.ZipFile(archive) as z:
        inspect_manifest(z.read(MANIFEST_PATH),set(z.namelist()))
        original=[]
        for path in FILES_OBSERVED:
            data=z.read(path)
            if hashlib.sha256(data).hexdigest()!=EXPECTED[path]:
                raise ValueError("original wave source SHA mismatch: "+path)
            original.append((path,data.decode("latin1")))
    report=inventory(original)
    for key,required in EXPECTED_WAVES.items():
        if report[key]!=required:
            raise ValueError("retail wave source count changed: "+key)
    if report["random_group_sizes"]!={
        str(k):v for k,v in sorted(PINNED_RANDOM_WAVE_GROUP_SIZES.items())
    }:
        raise ValueError("retail random wave candidate size distribution changed")
    report.update(
        original_scripts_sha_verified=7,
        original_manifest_sha_verified=True,
        sound_files_exported=False,
        engine_audio_playback_verified=False,
    )
    return report

def selftest():
    source='''"A" {
      "playlooping" { "wave" "*ambient/a.wav" "volume" "0.5" }
      "playrandom" { "time" "1.2"
        "rndwave" { "wave" ")ambient/b.mp3"
                    "wave" "*)ambient/c.wav" } }
    }
    "B" { "playlooping" { "wave" "ambient/d.wav" } }'''
    result=inventory([("scripts/invented.txt",source)])
    expected={
        "definitions":2,
        "definitions_with_waves":2,
        "looping_groups":2,
        "random_groups":1,
        "rndwave_blocks":1,
        "raw_wave_occurrences":4,
        "looping_waves":2,
        "random_waves":2,
        "unique_raw_waves":4,
        "wav_occurrences":3,
        "mp3_occurrences":1,
        "star_leading":2,
        "paren_leading":1,
        "compound_star_paren":1,
        "unmarked":1,
        "maximum_random_candidates":2,
        "minimum_random_candidates":2,
    }
    for key,wanted in expected.items():
        if result.get(key)!=wanted:
            raise ValueError("synthetic wave inventory wrong: "+key)
    for bad in ("../escape.wav","C:/escape.wav","sound\\escape.wav",
                "*ambient/bad.flac","*)"):
        try:normalize(bad)
        except ValueError:pass
        else:raise ValueError("unsafe fabricated wave accepted: "+bad)
    for broken in ('"A" { "wave" "ambient/unattached.wav" }',
                   '"A" { "playrandom" { "time" "1" } }',
                   '"A" { "playlooping" { "pitch" "1" } }'):
        try:inventory([("synthetic",broken)])
        except ValueError:pass
        else:raise ValueError("malformed synthetic wave group accepted")
    return {
        "original_free_wave_inventory_selftest":"PASS",
        "original_assets_read":False,
        "no_original_audio_data_emitted":True,
    }

def main():
    p=argparse.ArgumentParser(description=__doc__)
    g=p.add_mutually_exclusive_group(required=True)
    g.add_argument("--self-test",action="store_true")
    g.add_argument("--scripts",type=Path)
    a=p.parse_args()
    print(json.dumps(selftest() if a.self_test else private(a.scripts),
                     sort_keys=True))

if __name__=="__main__":
    try:main()
    except (AssertionError,ValueError,UnicodeError,FileNotFoundError,
            zipfile.BadZipFile,KeyError) as exc:
        print("FAIL SHA-pinned soundscape wave inventory: "+str(exc),
              file=sys.stderr)
        sys.exit(1)
