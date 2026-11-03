#!/usr/bin/env python3
"""Read-only SourceVR 0.5.5 split-ZIP native/overlay/VR manifest audit.

The last .zip volume is sufficient for the central directory, manifests,
*.load files and the small Android bootstrap binaries. Earlier split
.z01/.z02 volumes are NOT required for this metadata audit. It does not
claim CRC/SHA verification of blob contents stored in the other volumes.
"""
from __future__ import annotations
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path, PurePosixPath
import re
import struct
import zipfile

ROOT = "source vr/assets/sourcevr/v1/"
GAMES = ("entropyzero","ep1","ep2","hl2","lostcoast","portal","portal2")
HEX64 = re.compile(r"[0-9a-f]{64}\Z")
BOOTSTRAP = {
    "libsourcevrbootstrap.so": "04fc065225478cb599a04e04da239ebe4d19c3aea3ce373f2adb908cb1bd9565",
    "libhl2vrbridge.so": "b3a8802d3b419d7480a5000549e27406ab1c213bdb3a1fd6e2743ef515d5dfbe",
}
class ManifestError(ValueError):
    pass

def require(ok:bool, reason:str):
    if not ok: raise ManifestError(reason)

def read_text(z:zipfile.ZipFile,path:str)->str:
    try: return z.read(path).decode("utf-8")
    except (KeyError, UnicodeDecodeError, zipfile.BadZipFile) as exc:
        raise ManifestError("unreadable manifest/list: "+path) from exc

def check_filename(name:str):
    path=PurePosixPath(name)
    require(name and not name.startswith("/") and ".." not in path.parts
            and "\\" not in name and "\x00" not in name,
            "unsafe manifest path: "+repr(name))

def parse_native_manifest(text:str,game:str):
    lines=text.splitlines()
    require(bool(lines) and lines[0]==f"SOURCEVR_NATIVE_V1\t{game}",
            "unexpected native header for "+game)
    rows=[]
    for idx,line in enumerate(lines[1:]):
        fields=line.split("\t")
        require(len(fields)==6,"native field count for "+game)
        number,digest,source,destination,blob,deps=fields
        require(number==f"{idx:04d}","native entry ordering for "+game)
        require(bool(HEX64.fullmatch(digest)),"invalid native digest")
        require(source.endswith(".so") and source==destination,"invalid library filename")
        require(blob==f"sourcevr/v1/native/blobs/{digest}/{source}",
                "native blob path/hash mismatch")
        check_filename(blob)
        dependencies=[] if deps=="-" else deps.split(",")
        require(all(x.endswith(".so") and "/" not in x for x in dependencies),
                "invalid dependencies")
        rows.append({"index":idx,"digest":digest,"module":source,
                     "path":blob,"dependencies":dependencies})
    names=[x["module"] for x in rows]
    require(len(names)==len(set(names)),"duplicate native module")
    all_names=set(names)
    for row in rows:
        require(all(x in all_names for x in row["dependencies"]),
                f"unsatisfied {game} dependency of {row['module']}")
    return rows

def parse_asset_manifest(text:str,scope:str,game:str):
    rows=[]
    for line in text.splitlines():
        digest,separator,path=line.partition("  ")
        require(separator=="  " and bool(HEX64.fullmatch(digest)),
                f"invalid {scope}/{game} asset manifest line")
        check_filename(path)
        rows.append((digest,path))
    require(len(set(path for _,path in rows))==len(rows),
            f"duplicate {scope}/{game} path")
    return rows

def inspect_sourcevr_zip(path:Path, reference:dict|None=None, verify_bootstrap=True):
    require(path.is_file(),"missing ZIP file")
    try:
        z=zipfile.ZipFile(path)
    except (OSError,zipfile.BadZipFile) as exc:
        raise ManifestError("invalid final SourceVR split ZIP") from exc
    with z:
        members=set(z.namelist())
        profiles={}
        native_blobs=set()
        for game in GAMES:
            manifest=ROOT+f"native/{game}.manifest"
            load=ROOT+f"native/{game}.load"
            rows=parse_native_manifest(read_text(z,manifest),game)
            expected_paths=[r["path"] for r in rows]
            load_paths=read_text(z,load).splitlines()
            require(load_paths==expected_paths,f"{game} load order differs from manifest")
            for row in rows:
                archive_path="source vr/assets/"+row["path"]
                require(archive_path in members,f"missing {game} native member: {archive_path}")
                native_blobs.add(archive_path)
            profiles[game]=rows
        assets={}
        for kind in ("overlay","vr"):
            group={}
            for member in sorted(members):
                if not member.startswith(ROOT+kind+"/") or not member.endswith(".manifest"):
                    continue
                game=PurePosixPath(member).stem
                rows=parse_asset_manifest(read_text(z,member),kind,game)
                for digest,_ in rows:
                    require(ROOT+f"blobs/sha256/{digest}" in members,
                            f"missing referenced {kind}/{game} blob: {digest}")
                group[game]={"assets":len(rows),"unique_blobs":len({v[0] for v in rows})}
            require(len(group)==7,"expected seven "+kind+" manifests")
            assets[kind]=group
        props={}
        for line in read_text(z,"source vr/assets/version.properties").splitlines():
            if "=" in line and not line.startswith("#"):
                key,value=line.split("=",1);props[key]=value
        require(props.get("vrSemver")=="0.5.5","not the supplied 0.5.5 release")
        bootstrap={}
        for name,digest in BOOTSTRAP.items():
            full=f"source vr/lib/arm64-v8a/{name}"
            data=z.read(full)
            actual=hashlib.sha256(data).hexdigest()
            if verify_bootstrap:
                require(actual==digest,name+" SHA-256 mismatch")
            require(data.startswith(b"\x7fELF") and data[4:6]==b"\x02\x01"
                    and struct.unpack_from("<H",data,18)[0]==183,
                    name+" not ARM64 ELF64")
            token=b"ANativeActivity_onCreate\x00" if "bootstrap" in name else b"JNI_OnLoad\x00"
            require(token in data,name+" missing native entry-point symbol")
            bootstrap[name]={"sha256":actual,"size_bytes":len(data),
                             "entrypoint_string":token.rstrip(b"\x00").decode()}
        common={name for row in profiles.values() for name in (x["module"] for x in row)}
        unique_files={member for member in members if
                      member.startswith(ROOT+"native/blobs/") and member.endswith(".so")}
        require(native_blobs==unique_files,"unreferenced or absent native .so members")
        compared={}
        if reference:
            old=reference.get("libraries",reference.get("installed_libraries",{}))
            for game in GAMES:
                differences=[];identical=[]
                for row in profiles[game]:
                    oldhash=old.get(row["module"],{}).get("sha256")
                    if oldhash is None:continue
                    (identical if oldhash==row["digest"] else differences).append(row["module"])
                compared[game]={"matched_hashes":sorted(identical),
                                "different_hashes":sorted(differences)}
        return {
            "version":props,
            "scope":"Central directory, manifests and bootstrap bytes in final split ZIP; first two volumes not audited",
            "game_profiles":{name:{"modules":len(rows),
                "native_names":[r["module"] for r in rows],
                "sha256_by_module":{r["module"]:r["digest"] for r in rows},
                "dependencies":{r["module"]:r["dependencies"] for r in rows}}
                for name,rows in profiles.items()},
            "unique_native_payload_names":len(native_blobs),
            "native_module_sets":len(GAMES),
            "overlay_vr_asset_manifests":assets,
            "bootstrap_libraries":bootstrap,
            "reference_0989_comparison":compared,
            "sin_profile_present":any("sin" in game.lower() for game in profiles),
            "limitations":[
              "Manifest content hashes are declared hashes, not independently verified blob bytes",
              "Manifest presence does not prove engine ABI compatibility or a game loading on Quest 3",
              "Original SiN game code must be separately rebuilt as native ARM64 modules",
            ],
        }

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--last-zip",required=True,type=Path)
    p.add_argument("--output",required=True,type=Path)
    p.add_argument("--reference-0989-json",type=Path)
    a=p.parse_args()
    reference=json.loads(a.reference_0989_json.read_text()) if a.reference_0989_json else None
    result=inspect_sourcevr_zip(a.last_zip,reference)
    a.output.parent.mkdir(parents=True,exist_ok=True)
    a.output.write_text(json.dumps(result,indent=2)+"\n",encoding="utf-8")
    print(json.dumps({"version":result["version"].get("vrVersionName"),
       "game_module_counts":{k:v["modules"] for k,v in result["game_profiles"].items()},
       "unique_native_payload_names":result["unique_native_payload_names"],
       "sin_profile_present":result["sin_profile_present"]},indent=2))

if __name__=="__main__":main()
