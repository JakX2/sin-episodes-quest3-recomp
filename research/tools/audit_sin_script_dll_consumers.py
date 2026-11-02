#!/usr/bin/env python3
"""Read original SiN KeyValues scripts and verify their 2006 x86 DLL consumers.

Read-only, no game execution. Original proprietary DLLs/scripts are input-only;
report describes original data, x86 references and evidence gaps.
"""
from __future__ import annotations

import argparse
from collections import Counter
import hashlib
import io
import json
from pathlib import Path
import re
import struct
import zipfile

from verify_gameplay_entity_registry import (
    CLIENT_SHA256, SERVER_SHA256, PEImage, VerificationError,
)
from audit_full_sin_game_dlls import scan_registrations, rtti_names

WEAPON_PATH = re.compile(r"^scripts/(weapon_[a-z0-9_]+)\.txt$", re.I)
REQUIRED_SCRIPTS = (
    "scripts/rules_combat.txt", "scripts/phonebook.txt",
    "scripts/game_sounds_phonebook.txt",
    "scripts/vehicles/sincar.txt", "scripts/vehicles/sincar_pit.txt",
)
KEYS = (
    "clip_size", "clip2_size", "default_clip", "default_clip2",
    "primary_ammo", "secondary_ammo", "ZoomFOV", "MeleeWeapon",
    "HUDColor", "kickMin", "kickRampSecs", "kickRampMin", "kickRampMax",
    "kickRampStartScale", "kickRampEndScale", "viewmodelOriginOffset",
    "class_names", "weapon_class_names", "target_class_names", "PhoneBook",
)
WEAPON_VALUES = (
    "printname", "viewmodel", "playermodel", "clip_size", "clip2_size",
    "default_clip", "default_clip2", "primary_ammo", "secondary_ammo",
    "ZoomFOV", "MeleeWeapon", "HUDColor", "kickMin", "kickRampMin",
    "kickRampMax", "kickRampSecs", "kickRampStartScale", "kickRampEndScale",
    "viewmodelOriginOffset", "ITEM_FLAG_NOPLAYERPICKUP",
)
MAX_SCRIPT = 1024 * 1024
MAX_TOKENS = 100000
MAX_DEPTH = 40


class ScriptError(ValueError):
    pass


def tokens(source: str) -> list[str]:
    out = []
    pos = 0
    while pos < len(source):
        c = source[pos]
        if c.isspace():
            pos += 1
            continue
        if source.startswith("//", pos):
            newline = source.find("\n", pos+2)
            pos = len(source) if newline < 0 else newline+1
            continue
        if c in "{}":
            out.append(c)
            pos += 1
        elif c == '"':
            pos += 1
            data = []
            while pos < len(source):
                c = source[pos]
                pos += 1
                if c == '"':
                    break
                if c == "\\" and pos < len(source) and source[pos] in ('"', "\\"):
                    c = source[pos]
                    pos += 1
                data.append(c)
            else:
                raise ScriptError("unterminated quoted KeyValues string")
            out.append("".join(data))
        else:
            end = pos
            while (end < len(source) and not source[end].isspace() and
                   source[end] not in '{}"' and not source.startswith("//", end)):
                end += 1
            if end == pos:
                raise ScriptError(f"unexpected KeyValues byte at {pos}")
            out.append(source[pos:end])
            pos = end
        if len(out) > MAX_TOKENS:
            raise ScriptError("too many KeyValues tokens")
    return out


def parse_kv(raw: bytes) -> list[tuple[str, object]]:
    if len(raw) > MAX_SCRIPT:
        raise ScriptError("script exceeds one MiB")
    source = raw.decode("utf-8-sig", "replace")
    seq = tokens(source)
    index = 0

    def group(depth: int, bracketed: bool):
        nonlocal index
        if depth > MAX_DEPTH:
            raise ScriptError("KeyValues nesting exceeds limit")
        result = []
        while index < len(seq):
            key = seq[index]
            index += 1
            if key == "}":
                if not bracketed:
                    raise ScriptError("unexpected closing brace")
                return result
            if key == "{":
                raise ScriptError("unexpected opening brace")
            if index >= len(seq):
                raise ScriptError(f"missing value for {key}")
            value = seq[index]
            index += 1
            if value == "{":
                value = group(depth+1, True)
            elif value == "}":
                raise ScriptError(f"unexpected closing brace after {key}")
            result.append((key, value))
        if bracketed:
            raise ScriptError("unclosed KeyValues block")
        return result

    return group(0, False)


def one_block(nodes: list[tuple[str, object]], name: str) -> list[tuple[str, object]]:
    matches = [v for k,v in nodes if k.lower() == name.lower()]
    if len(matches) != 1 or not isinstance(matches[0], list):
        raise ScriptError(f"expected one {name} KeyValues block")
    return matches[0]


def scalar(nodes: list[tuple[str,object]]) -> dict[str,str]:
    result = {}
    for k,v in nodes:
        if isinstance(v,str):
            if k.lower() in result:
                raise ScriptError(f"duplicate scalar KeyValues key: {k}")
            result[k.lower()] = v
    return result


def read_zip_member(archive: zipfile.ZipFile, name: str) -> bytes:
    info = archive.getinfo(name)
    if info.file_size > MAX_SCRIPT:
        raise ScriptError(f"{name} exceeds maximum size")
    return archive.read(info)


def inspect_scripts(script_zip: bytes, registered: set[str]) -> dict:
    if len(script_zip) > 32 * 1024 * 1024:
        raise ScriptError("scripts archive exceeds 32 MiB")
    with zipfile.ZipFile(io.BytesIO(script_zip)) as z:
        names = z.namelist()
        if len(names) != len(set(names)):
            raise ScriptError("duplicate ZIP member names")
        for name in REQUIRED_SCRIPTS:
            if name not in names:
                raise ScriptError(f"missing original script: {name}")
        paths = sorted(n for n in names if WEAPON_PATH.fullmatch(n))
        if not paths or len(paths) > 64:
            raise ScriptError("unexpected weapon script inventory")
        weapons = []
        for path in paths:
            root = one_block(parse_kv(read_zip_member(z,path)), "WeaponData")
            values = scalar(root)
            sounds = []
            textures = []
            for key, value in root:
                if key.lower() == "sounddata" and isinstance(value,list):
                    sounds.extend(k for k,v in value if isinstance(v,str))
                if key.lower() == "texturedata" and isinstance(value,list):
                    textures.extend(k for k,v in value if isinstance(v,list))
            name = WEAPON_PATH.fullmatch(path).group(1).lower()
            weapons.append({
                "classname": name,
                "server_entity_factory_22byte_pattern": name in registered,
                "script_sha256": hashlib.sha256(z.read(path)).hexdigest(),
                "specified_fields": {key:values[key.lower()] for key in WEAPON_VALUES
                                     if key.lower() in values},
                "sound_event_keys": sounds,
                "client_texture_categories": textures,
            })
        combat = one_block(parse_kv(read_zip_member(z,"scripts/rules_combat.txt")), "AttackSpreadRules")
        classnames = scalar(one_block(combat, "class_names"))
        spreads = {}
        for key, val in combat:
            if not isinstance(val,list) or key.lower() == "class_names":
                continue
            try:
                rules = one_block(val,"weapon_class_names")
            except ScriptError:
                continue
            mapping = scalar(rules)
            spreads[key] = {
                "weapon_rules": mapping,
                "target_spreads": {},
            }
            for weapon_rule in set(mapping.values()):
                try:
                    definition = one_block(val, weapon_rule)
                    targets = scalar(one_block(definition, "target_class_names"))
                    spreads[key]["target_spreads"][weapon_rule] = {
                        "target_rules": targets,
                        "literal_values": {
                            label: scalar(definition).get(label.lower())
                            for label in sorted(set(targets.values()))
                        },
                    }
                except ScriptError:
                    continue
        phone = scalar(one_block(parse_kv(read_zip_member(z,"scripts/phonebook.txt")), "PhoneBook"))
        phone_sounds = {k for k,v in parse_kv(read_zip_member(z,"scripts/game_sounds_phonebook.txt"))
                        if isinstance(v,list)}
        vehicles = []
        for path in ("scripts/vehicles/sincar.txt", "scripts/vehicles/sincar_pit.txt"):
            blocks = parse_kv(read_zip_member(z,path))
            car = one_block(blocks,"vehicle")
            engine = one_block(car,"engine")
            vehicles.append({
                "path":path,
                "sha256":hashlib.sha256(z.read(path)).hexdigest(),
                "engine_settings":{k:v for k,v in scalar(engine).items() if k in (
                    "horsepower","maxrpm","maxspeed","maxreversespeed","autotransmission","axleratio")},
                "gear_entries":sum(k.lower()=="gear" for k,_ in engine),
                "axle_entries":sum(k.lower()=="axle" for k,_ in car),
            })
        return {
            "total_script_zip_entries":len(names),
            "weapon_count":len(weapons),
            "weapons":weapons,
            "combat": {"class_to_rule":classnames,
                       "class_count":len(classnames),
                       "rules":spreads,
                       "classes_without_recognized_22byte_factory":
                           sorted(k for k in classnames if k not in registered)},
            "phonebook":{
                "number_count":len(phone),
                "cue_names":phone,
                "phonebook_sound_cues_in_soundfile":len(phone_sounds),
                "sound_references_in_dedicated_file":
                    sorted({v for v in phone.values() if v in phone_sounds}),
                "not_in_dedicated_phonebook_soundfile":
                    sorted({v for v in phone.values() if v not in phone_sounds}),
                "note":"Unmatched cues may exist in other sound manifests; not evidence of a missing asset.",
            },
            "vehicles":vehicles,
        }


def reference_pointers(dll: bytes, literal: str) -> list[dict]:
    pe = PEImage(dll)
    text = next((s for s in pe.sections if s[0]==".text"), None)
    if text is None:
        raise VerificationError("missing x86 .text section")
    _, rva, _, raw, size = text
    code = dll[raw:raw+size]
    result = []
    for name,sec_rva,_,sec_raw,sec_size in pe.sections:
        if name != ".rdata":
            continue
        strings = dll[sec_raw:sec_raw+sec_size]
        needle = literal.encode("ascii")+b"\x00"
        for m in re.finditer(re.escape(needle), strings):
            offset = m.start()
            if offset and (chr(strings[offset-1]).isalnum() or strings[offset-1] == 95):
                continue
            string_va = pe.base + sec_rva + offset
            insn = b"\x68" + struct.pack("<I",string_va)
            for use in re.finditer(re.escape(insn),code):
                result.append({
                    "literal_va":f"0x{string_va:08X}",
                    "push_imm32_va":f"0x{pe.base+rva+use.start():08X}",
                })
    return result


def analyze(client: bytes, server: bytes, script_zip: bytes,
            *, check_hash: bool=True) -> dict:
    client_hash=hashlib.sha256(client).hexdigest()
    server_hash=hashlib.sha256(server).hexdigest()
    if check_hash and (client_hash!=CLIENT_SHA256 or server_hash!=SERVER_SHA256):
        raise VerificationError("original 2006 SiN client/server DLL SHA-256 mismatch")
    factory_names={r["classname"] for r in scan_registrations(server,check_hash=check_hash)}
    script_data=inspect_scripts(script_zip,factory_names)
    consumer_refs={}
    for key in KEYS:
        consumer_refs[key] = {
            "client": reference_pointers(client,key),
            "server": reference_pointers(server,key),
        }
    return {
        "scope":"Actual 2006 SiN script values and qualified original x86 DLL push-immediate string references",
        "sha256":{
            "client.dll":client_hash,
            "server.dll":server_hash,
            "scripts.zip":hashlib.sha256(script_zip).hexdigest(),
        },
        "scripts":script_data,
        "dll_literal_push_xrefs":consumer_refs,
        "limits":[
            "push imm32 of a script key is a machine-code reference, not proof of the exact parsed semantics.",
            "No push xref does not prove a field is unused; compiler may use a different instruction, data table or indirect pointer.",
            "Recognized server factories are limited to one validated 22-byte x86 registration pattern.",
            "Clip and ZoomFOV values are screen-game source data, not inferred VR control or physical gun offsets.",
            "This tool neither loads scripts into SourceVR nor reconstructs original client/server functions.",
        ],
    }


def main() -> int:
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--client",required=True,type=Path)
    p.add_argument("--server",required=True,type=Path)
    p.add_argument("--scripts-zip",required=True,type=Path)
    p.add_argument("--output",required=True,type=Path)
    args=p.parse_args()
    try:
        result=analyze(args.client.read_bytes(),args.server.read_bytes(),
                       args.scripts_zip.read_bytes())
        args.output.write_text(json.dumps(result,indent=2)+"\n",encoding="utf-8")
    except (OSError,ValueError,struct.error,zipfile.BadZipFile) as exc:
        p.exit(1,f"SIN SCRIPT/DLL AUDIT FAILED: {exc}\n")
    print(json.dumps({
        "result":"pass",
        "weapons":result["scripts"]["weapon_count"],
        "combat_class_rules":result["scripts"]["combat"]["class_count"],
        "phonebook_numbers":result["scripts"]["phonebook"]["number_count"],
        "output":str(args.output),
    },indent=2))
    return 0


if __name__=="__main__":
    raise SystemExit(main())
