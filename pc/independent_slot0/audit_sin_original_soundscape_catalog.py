#!/usr/bin/env python3
"""Phase 483: private SHA-gated SiN retail soundscape graph inventory.

--scripts original/scripts.zip: validates seven SHA-pinned original files
and reports *derived counts only*. --self-test: invented text, no assets.
This does not claim 2006 engine soundscape playback or actual map loading.
"""
import argparse
import hashlib
import json
import sys
import zipfile
from pathlib import Path
from audit_sin_original_soundscape_manifest import (
    FILES_OBSERVED, MANIFEST_PATH, inspect_manifest,
)

EXPECTED = {
    "scripts/soundscapes.txt":
        "a818d932aadc67e2591912fee78325d2014c2f9f2df13d4a1a779f56bfca288d",
    "scripts/soundscapes_intro.txt":
        "cde83b02e212293e0fe75e656095a4465c08b58df7ecdcf595b1756d38d17050",
    "scripts/soundscapes_docks.txt":
        "23dcb7f7912e6f498f5492305fb71949c10824c9fa0aa4017da75d8a31c8d003",
    "scripts/soundscapes_druglab.txt":
        "5f1c9cea97cb0b601ece2b80d9b300d081d9ea760102e70210e4dd83bb11fa81",
    "scripts/soundscapes_pit.txt":
        "0423c8e74a16deadbe32a2ebeaa3204ac588f1bfc01f49e9de80a2452fd3591e",
    "scripts/soundscapes_highrise.txt":
        "ea84fd8efabca92aefde57918f5c62a753c070d9a4d1992c8978e9b1dcfdc84f",
    "scripts/soundscapes_finale.txt":
        "78f3cd52757251e948ed9af214c05ce0659234a77679d9d01a2bf0c6ef4e2e0d",
}


def tokenize(source):
    at = 0
    n = len(source)
    while at < n:
        c = source[at]
        if c.isspace():
            at += 1
            continue
        if source.startswith("//", at):
            end = source.find("\n", at + 2)
            at = n if end == -1 else end + 1
            continue
        if source.startswith("/*", at):
            end = source.find("*/", at + 2)
            if end == -1:
                raise ValueError("unterminated fabricated block comment")
            at = end + 2
            continue
        if c in "{}":
            yield c
            at += 1
            continue
        if c == '"':
            at += 1
            start = at
            while at < n and source[at] != '"':
                if source[at] in "\r\n\0":
                    raise ValueError("newline or NUL in quoted text")
                at += 1
            if at == n:
                raise ValueError("unterminated quoted text")
            yield source[start:at]
            at += 1
            continue
        start = at
        while at < n and not source[at].isspace() and source[at] not in "{}":
            at += 1
        if at == start:
            raise ValueError("bad script token")
        yield source[start:at]


def parse(script):
    toks = list(tokenize(script))
    cursor = 0
    def read(depth):
        nonlocal cursor
        if depth > 20:
            raise ValueError("script exceeded nesting depth")
        out = []
        while cursor < len(toks):
            key = toks[cursor]
            cursor += 1
            if key == "}":
                if depth == 0:
                    raise ValueError("unmatched closing brace")
                return out
            if key == "{":
                raise ValueError("unexpected nested open")
            if cursor == len(toks):
                raise ValueError("missing property value")
            val = toks[cursor]
            cursor += 1
            if val == "{":
                val = read(depth + 1)
            elif val == "}":
                raise ValueError("unexpected closing property")
            out.append((key, val))
        if depth != 0:
            raise ValueError("unterminated script block")
        return out
    roots = read(0)
    if cursor != len(toks):
        raise ValueError("unexpected trailing data")
    if not roots or any(not isinstance(value, list) for _, value in roots):
        raise ValueError("root soundscapes must be KeyValues blocks")
    return roots


def inventory(named_scripts):
    names = []
    refs = []
    stats = dict(documents=len(named_scripts), definitions=0, property_nodes=0,
                 looping_blocks=0, random_blocks=0, random_wave_blocks=0,
                 wave_properties=0, nested_soundscape_blocks=0)
    def visit(nodes, owner, path):
        for key, value in nodes:
            stats["property_nodes"] += 1
            low = key.lower()
            if not isinstance(value, list):
                if low == "wave":
                    stats["wave_properties"] += 1
                continue
            if low == "playlooping":
                stats["looping_blocks"] += 1
            elif low == "playrandom":
                stats["random_blocks"] += 1
            elif low == "rndwave":
                stats["random_wave_blocks"] += 1
            elif low == "playsoundscape":
                stats["nested_soundscape_blocks"] += 1
                for nested_key, nested_value in value:
                    if nested_key.lower() == "name" and isinstance(nested_value, str):
                        refs.append((owner, nested_value, path))
                        break
            visit(value, owner, path)
    for path, text in named_scripts:
        roots = parse(text)
        for name, body in roots:
            if not name:
                raise ValueError("soundscape without top-level name")
            names.append(name)
            visit(body, name, path)
    normalized = {name.lower() for name in names}
    if len(normalized) != len(names):
        raise ValueError("duplicate soundscape names")
    missing = [(owner, target, path) for owner, target, path in refs
               if target.lower() not in normalized]
    stats.update(definitions=len(names), nested_references=len(refs),
                 unresolved_references=len(missing),
                 unresolved_unique_targets=sorted(set(
                     target for _, target, _ in missing)))
    return stats


def real_assets(path):
    with zipfile.ZipFile(path) as z:
        names = set(z.namelist())
        inspect_manifest(z.read(MANIFEST_PATH), names)
        files = []
        for name in FILES_OBSERVED:
            data = z.read(name)
            if hashlib.sha256(data).hexdigest() != EXPECTED[name]:
                raise ValueError("original soundscape script SHA mismatch: " + name)
            files.append((name, data.decode("latin1")))
    output = inventory(files)
    expected_counts = dict(
        documents=7, definitions=301, property_nodes=4874,
        looping_blocks=281, random_blocks=150, random_wave_blocks=150,
        wave_properties=1224, nested_soundscape_blocks=242,
        nested_references=242, unresolved_references=3)
    for key, wanted in expected_counts.items():
        if output[key] != wanted:
            raise ValueError("authentic script inventory mismatch: " + key)
    if output["unresolved_unique_targets"] != [
        "d2_depot_cellblock_medium", "druglab.dudeattacked"]:
        raise ValueError("authentic unresolved target set changed")
    output.update(original_scripts_sha_verified=7,
                  manifest_sha_verified=True,
                  original_soundscape_data_exported=False,
                  actual_engine_audio_tested=False)
    return output


def synthetic():
    source = """
        "room1" {
            "playlooping" { "wave" "a.wav" }
            "playsoundscape" { "name" "room2" }
            "playsoundscape" { "name" "unknown" }
        }
        "room2" { "playrandom" { "rndwave" { "wave" "b.wav" } } }
    """
    r = inventory([("scripts/example.txt", source)])
    assert r["definitions"] == 2
    assert r["looping_blocks"] == 1
    assert r["random_blocks"] == 1
    assert r["wave_properties"] == 2
    assert r["nested_references"] == 2
    assert r["unresolved_references"] == 1
    for invalid in ('"bad" { "file" "value"', '"bad" { } }',
                    '"bad" "scalar"'):
        try:
            inventory([("synthetic", invalid)])
        except ValueError:
            pass
        else:
            raise ValueError("bad synthetic soundscape document accepted")
    return {"synthetic_soundscape_graph": "PASS",
            "original_assets_used": False}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--scripts", type=Path)
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    if args.self_test:
        result = synthetic()
    elif args.scripts:
        result = real_assets(args.scripts)
    else:
        parser.error("provide --scripts or --self-test")
    print(json.dumps(result, sort_keys=True))


if __name__ == "__main__":
    try:
        main()
    except (ValueError, AssertionError, UnicodeError,
            FileNotFoundError, zipfile.BadZipFile, KeyError) as exc:
        print("FAIL original soundscape catalog SHA audit: " + str(exc),
              file=sys.stderr)
        sys.exit(1)
