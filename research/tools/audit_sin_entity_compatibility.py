#!/usr/bin/env python3
"""Cross-check original SiN server factory registrations against original SDK FGD.

Independent evidence axes:
- 2006 server.dll factory registrations, from audit_full_sin_game_dlls.py CSV.
- Original SinEpisodes.fgd and base.fgd editor declarations, from owner's SDK ZIP.
- Optional, independently saved plaintext classname list from Valve's SE1 wiki.

An editor declaration is NOT proof that a factory was compiled; lack of a
match to the known 22-byte factory-registration idiom is NOT proof of absence.
No gameplay code, game assets or DLL bytes are emitted or redistributed.
"""
from __future__ import annotations

import argparse
import csv
import io
import json
from collections import Counter
from pathlib import Path
import re
import zipfile

FGDS = ("SinEpisodesSDK/bin/base.fgd", "SinEpisodesSDK/bin/SinEpisodes.fgd")
VDC_URL = "https://developer.valvesoftware.com/wiki/List_of_SE1_entities"
DECL = re.compile(r"(?m)^\s*@(BaseClass|PointClass|SolidClass|NPCClass|FilterClass|KeyFrameClass)\b")
ENTITY = re.compile(r"=\s*([A-Za-z_][A-Za-z0-9_]*)\b")
BASE = re.compile(r"\bbase\s*\(([^)]*)\)", re.I)
IO = re.compile(r"(?m)^[ \t]*(input|output)[ \t]+([A-Za-z_][A-Za-z0-9_]*)\s*\(", re.I)
KEY = re.compile(r"(?m)^[ \t]*([A-Za-z_][A-Za-z0-9_]*)\s*\(\s*([A-Za-z_][A-Za-z0-9_]*)\s*\)[ \t]*:")
CL = re.compile(r"^[A-Za-z_][A-Za-z0-9_]{0,95}$")
MAX_ARCHIVE = 100 * 1024 * 1024


def strip_line_comments(source: str) -> str:
    """Preserve line structure, skipping // only when outside quoted strings."""
    output = []
    quoted = False
    escaped = False
    i = 0
    while i < len(source):
        c = source[i]
        if c == '"' and not escaped:
            quoted = not quoted
        if not quoted and source[i:i+2] == "//":
            j = source.find("\n", i)
            if j < 0:
                break
            output.append("\n")
            i = j+1
            escaped = False
            continue
        output.append(c)
        if c == "\\" and quoted:
            escaped = not escaped
        else:
            escaped = False
        i += 1
    return "".join(output)


def find_square_end(source: str, start: int) -> int:
    depth = 0
    quoted = False
    escaped = False
    for i in range(start, len(source)):
        c = source[i]
        if c == '"' and not escaped:
            quoted = not quoted
        if not quoted:
            if c == "[":
                depth += 1
            elif c == "]":
                depth -= 1
                if depth == 0:
                    return i
        if c == "\\" and quoted:
            escaped = not escaped
        else:
            escaped = False
    raise ValueError("unclosed FGD definition square bracket")


def parse_fgd(raw: bytes, source_label: str) -> list[dict]:
    text = strip_line_comments(raw.decode("utf-8-sig", errors="replace"))
    found = []
    matches = list(DECL.finditer(text))
    for n, match in enumerate(matches):
        end = matches[n+1].start() if n+1 < len(matches) else len(text)
        block = text[match.end():end]
        opening = block.find("[")
        if opening < 0:
            raise ValueError(f"{source_label}: missing body for {match.group(1)}")
        header = block[:opening]
        name_match = ENTITY.search(header)
        if not name_match:
            raise ValueError(f"{source_label}: missing entity classname in {header[:100]!r}")
        closing = find_square_end(block, opening)
        body = block[opening+1:closing]
        bases = [n.strip() for item in BASE.findall(header) for n in item.split(",") if n.strip()]
        found.append({
            "classname": name_match.group(1),
            "kind": match.group(1),
            "source_fgd": source_label,
            "bases": bases,
            "direct_inputs": sorted({x.group(2) for x in IO.finditer(body) if x.group(1).lower() == "input"}),
            "direct_outputs": sorted({x.group(2) for x in IO.finditer(body) if x.group(1).lower() == "output"}),
            "direct_keyvalues": sorted({x.group(1) for x in KEY.finditer(body)
                                         if x.group(1).lower() not in ("input", "output")}),
        })
    return found


def read_sdk_fgds(sdk_zip: Path) -> list[dict]:
    out = []
    with zipfile.ZipFile(sdk_zip) as archive:
        for name in FGDS:
            if name not in archive.namelist():
                raise ValueError(f"original SDK ZIP missing {name}")
            info = archive.getinfo(name)
            if info.file_size > MAX_ARCHIVE:
                raise ValueError("FGD exceeds size limit")
            out.extend(parse_fgd(archive.read(name), name.split("/")[-1]))
    return out


def read_factory_csv(path: Path) -> list[dict]:
    with path.open(encoding="utf-8-sig", newline="") as f:
        rows = list(csv.DictReader(f))
    expected = {"classname", "register_va", "factory_va"}
    if not rows or not expected.issubset(rows[0]):
        raise ValueError("expected original verified factory CSV columns")
    by_name = set()
    for row in rows:
        name = row["classname"]
        if not CL.fullmatch(name) or name in by_name:
            raise ValueError(f"malformed or duplicate binary classname: {name}")
        by_name.add(name)
        for key in ("register_va", "factory_va"):
            if not re.fullmatch(r"0x[0-9a-fA-F]{6,8}", row[key]):
                raise ValueError(f"invalid original address: {key}")
    return rows


def read_wiki_plaintext(path: Path | None) -> set[str] | None:
    """Only parse a user-supplied plaintext list, not scraped HTML or guesses."""
    if path is None:
        return None
    lines = path.read_text(encoding="utf-8-sig").splitlines()
    names = set()
    for n, raw in enumerate(lines, 1):
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        if not CL.fullmatch(line):
            raise ValueError(f"wiki list line {n}: expected one classname per line")
        names.add(line)
    return names


def build_matrix(factories: list[dict], declarations: list[dict],
                 wiki_names: set[str] | None = None) -> dict:
    by_binary = {x["classname"]: x for x in factories}
    by_fgd: dict[str,list[dict]] = {}
    bases: dict[str,dict] = {}
    for declaration in declarations:
        target = bases if declaration["kind"].lower() == "baseclass" else by_fgd
        if target is bases:
            if declaration["classname"] in bases:
                raise ValueError("duplicate BaseClass " + declaration["classname"])
            bases[declaration["classname"]] = declaration
        else:
            target.setdefault(declaration["classname"], []).append(declaration)

    def inherited(decl: dict) -> tuple[list[str],list[str],list[str]]:
        keys = set(decl["direct_keyvalues"])
        inputs = set(decl["direct_inputs"])
        outputs = set(decl["direct_outputs"])
        def visit(name: str, visiting: set[str]) -> None:
            if name in visiting:
                raise ValueError("cyclic FGD base inheritance at " + name)
            parent = bases.get(name)
            if parent is None:
                return
            keys.update(parent["direct_keyvalues"])
            inputs.update(parent["direct_inputs"])
            outputs.update(parent["direct_outputs"])
            for parent_name in parent["bases"]:
                visit(parent_name, visiting | {name})
        for base in decl["bases"]:
            visit(base, set())
        return sorted(keys), sorted(inputs), sorted(outputs)

    output = []
    all_names = sorted(set(by_binary) | set(by_fgd) | (wiki_names or set()))
    for name in all_names:
        binary = by_binary.get(name)
        editor = by_fgd.get(name, [])
        base_defs = [x for x in editor if x["source_fgd"] == "base.fgd"]
        sin_defs = [x for x in editor if x["source_fgd"] == "SinEpisodes.fgd"]
        if binary and sin_defs:
            state = "binary_and_sin_fgd"
        elif binary and base_defs:
            state = "binary_and_base_fgd"
        elif binary:
            state = "binary_only_no_sdk_declaration"
        elif sin_defs:
            state = "sin_fgd_no_recognized_binary_registration"
        elif base_defs:
            state = "base_fgd_no_recognized_binary_registration"
        else:
            state = "wiki_only_unconfirmed_by_sdk_binary"
        keys: set[str] = set()
        inputs: set[str] = set()
        outputs: set[str] = set()
        for declaration in editor:
            k,i,o = inherited(declaration)
            keys.update(k);inputs.update(i);outputs.update(o)
        output.append({
            "classname": name,
            "evidence_class": state,
            "binary_factory_match": binary is not None,
            "original_registration_va": binary["register_va"] if binary else None,
            "original_factory_object_va": binary["factory_va"] if binary else None,
            "fgd_sources": [x["source_fgd"] for x in editor],
            "fgd_declaration_kinds": [x["kind"] for x in editor],
            "fgd_base_names": sorted({n for x in editor for n in x["bases"]}),
            "fgd_keyvalues_including_inherited": sorted(keys),
            "fgd_inputs_including_inherited": sorted(inputs),
            "fgd_outputs_including_inherited": sorted(outputs),
            "on_wiki_list": None if wiki_names is None else name in wiki_names,
            "implementation_status": "not_implemented_in_quest_port",
        })
    counts = Counter(x["evidence_class"] for x in output)
    return {
        "scope": "2006 server entity factory metadata vs original SinEpisodes SDK Hammer FGD; wiki optional",
        "sources": {"original_factory_count": len(factories),
                    "original_fgd_declaration_count_excluding_bases": sum(
                        x["kind"].lower() != "baseclass" for x in declarations),
                    "original_fgd_base_definition_count": len(bases),
                    "wiki_url": VDC_URL,
                    "wiki_status": "user_verified_plaintext_list" if wiki_names is not None
                                   else "not_retrieved_403_do_not_infer_entries"},
        "counts": dict(sorted(counts.items())),
        "total_unique_classnames": len(output),
        "entities": output,
        "limitations": [
            "The SE1 wiki page was inaccessible during this run; when absent, wiki membership is UNKNOWN, not false.",
            "A DLL factory proves registration in this original server build, not functioning Quest gameplay.",
            "FGD is Hammer/editor metadata and may describe shared Source or SiN maps without corresponding code.",
            "No factory match only means none in the 510 previously recognized 22-byte x86 routines.",
            "base.fgd membership does not prove SourceVR ARM64 host support.",
            "Inheritance is resolved for FGD bases only; game C++ inheritance and runtime I/O are unverified.",
        ],
    }


def main() -> int:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--factory-csv", required=True, type=Path)
    p.add_argument("--sdk-zip", required=True, type=Path)
    p.add_argument("--wiki-plain", type=Path, help="Optional user-confirmed UTF-8 one-classname-per-line VDC list")
    p.add_argument("--output-json", type=Path, required=True)
    p.add_argument("--output-csv", type=Path, required=True)
    args = p.parse_args()
    try:
        matrix = build_matrix(read_factory_csv(args.factory_csv),
                              read_sdk_fgds(args.sdk_zip),
                              read_wiki_plaintext(args.wiki_plain))
        if args.output_csv.resolve() == args.output_json.resolve():
            raise ValueError("JSON and CSV output paths must differ")
        args.output_json.write_text(json.dumps(matrix, indent=2)+"\n", encoding="utf-8")
        fields = ["classname", "evidence_class", "binary_factory_match",
                  "original_registration_va", "fgd_sources", "fgd_declaration_kinds",
                  "fgd_base_names", "fgd_keyvalues_including_inherited",
                  "fgd_inputs_including_inherited", "fgd_outputs_including_inherited",
                  "on_wiki_list", "implementation_status"]
        with args.output_csv.open("w",encoding="utf-8",newline="") as dst:
            writer = csv.DictWriter(dst, fieldnames=fields)
            writer.writeheader()
            for entity in matrix["entities"]:
                writer.writerow({key: "|".join(entity[key]) if isinstance(entity[key],list)
                                 else entity[key] for key in fields})
    except (OSError, ValueError, zipfile.BadZipFile) as e:
        p.exit(1, "SIN ENTITY MATRIX FAILED: " + str(e) + "\n")
    print(json.dumps({"status":"complete","counts":matrix["counts"],
                      "total_unique_classnames":matrix["total_unique_classnames"],
                      "wiki_status":matrix["sources"]["wiki_status"]},indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
