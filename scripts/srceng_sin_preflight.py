#!/usr/bin/env python3
"""Read-only SiN content preflight for experimental srceng Android rendering.

Only bounded format headers, counts and source-relative file names are emitted.
Does not extract, convert, alter or distribute any owned game resources and
does not load an engine or claim gameplay compatibility.
"""
import argparse
from collections import Counter
import json
import os
from pathlib import Path
import re
import struct
import sys

MAX_FILES = 250_000
MAX_SAMPLES = 24
BSP_HEADER_SIZE = 8 + 64 * 16 + 4
RESOURCE_SUFFIXES = {
    ".bsp", ".mdl", ".vtx", ".vvd", ".vmt", ".vtf",
    ".vpk", ".wav", ".mp3", ".ani", ".vtx", ".pcf",
}
ADVERTISED_BSP = (19, 21)
ADVERTISED_MDL = (46, 49)


def read_prefix(path, size):
    with path.open("rb") as stream:
        return stream.read(size)


def advertised_range(version, bounds):
    if version is None:
        return "unknown"
    return ("within_advertised_version_range_not_proven_compatible"
            if bounds[0] <= version <= bounds[1]
            else "outside_advertised_version_range_not_proven_incompatible")


def bsp_header(path, file_size):
    data = read_prefix(path, BSP_HEADER_SIZE)
    if len(data) < BSP_HEADER_SIZE:
        return {"status": "truncated_header", "size_bytes": file_size}
    if data[:4] != b"VBSP":
        return {"status": "unexpected_signature", "size_bytes": file_size}
    version = struct.unpack_from("<I", data, 4)[0]
    invalid_lumps = 0
    populated_lumps = 0
    for index in range(64):
        offset, length, lump_version, fourcc = struct.unpack_from(
            "<IIII", data, 8 + index * 16
        )
        if length:
            populated_lumps += 1
            if offset > file_size or length > file_size - offset:
                invalid_lumps += 1
    map_revision = struct.unpack_from("<I", data, 8 + 64 * 16)[0]
    return {
        "status": "header_parsed",
        "size_bytes": file_size,
        "version": version,
        "srceng_advertised_bsp_range": advertised_range(version, ADVERTISED_BSP),
        "populated_lumps": populated_lumps,
        "out_of_bounds_lumps": invalid_lumps,
        "map_revision": map_revision,
    }


def mdl_header(path, file_size):
    data = read_prefix(path, 8)
    if len(data) != 8:
        return {"status": "truncated_header", "size_bytes": file_size}
    if data[:4] not in (b"IDST", b"IDAG"):
        return {"status": "unexpected_signature", "size_bytes": file_size}
    version = struct.unpack_from("<I", data, 4)[0]
    return {
        "status": "header_parsed",
        "size_bytes": file_size,
        "version": version,
        "srceng_advertised_mdl_range": advertised_range(version, ADVERTISED_MDL),
    }


def vtf_header(path, file_size):
    data = read_prefix(path, 16)
    if len(data) < 16:
        return {"status": "truncated_header", "size_bytes": file_size}
    if data[:4] != b"VTF\x00":
        return {"status": "unexpected_signature", "size_bytes": file_size}
    major, minor, header_size = struct.unpack_from("<III", data, 4)
    return {
        "status": "header_parsed",
        "size_bytes": file_size,
        "version": "%d.%d" % (major, minor),
        "header_size": header_size,
        "header_size_sane": 16 <= header_size <= file_size,
        "srceng_compatibility": "unknown_requires_runtime_check",
    }


def vpk_header(path, file_size):
    data = read_prefix(path, 12)
    if len(data) < 12:
        return {"status": "truncated_header", "size_bytes": file_size}
    signature, major, tree_size = struct.unpack_from("<III", data, 0)
    if signature != 0x55AA1234:
        return {"status": "unexpected_signature", "size_bytes": file_size}
    # The .vpk archive format uses a u32 major version in this position.
    return {
        "status": "header_parsed",
        "size_bytes": file_size,
        "version": major,
        "declared_tree_size": tree_size,
        "srceng_compatibility": "unknown_requires_runtime_check",
    }


def gameinfo_hint(path):
    # Only check for common Source keys; gameinfo formats vary. No parsed
    # SearchPaths or proprietary gameinfo contents are ever serialized.
    data = read_prefix(path, 256 * 1024)
    lowered = data.lower()
    return {
        "present": True,
        "size_capped_read_bytes": len(data),
        "contains_gameinfo_token": b"gameinfo" in lowered,
        "contains_searchpaths_token": b"searchpaths" in lowered,
        "searchpaths_not_resolved": True,
    }


def scan_owned_game(game_dir, srceng_root=None, max_files=MAX_FILES):
    root = Path(game_dir)
    if not root.is_dir():
        raise ValueError("Game directory does not exist or is not a directory")
    if root.is_symlink():
        raise ValueError("Refusing symlinked game root")
    if max_files <= 0 or max_files > MAX_FILES:
        raise ValueError("max_files must be between 1 and %d" % MAX_FILES)

    counts = Counter()
    maps = []
    models = []
    textures = []
    archives = []
    names_lower = {}
    collisions = []
    total_files = 0
    truncated = False
    gameinfo_path = None
    for parent, dirs, filenames in os.walk(root, followlinks=False):
        dirs[:] = sorted(
            name for name in dirs if not (Path(parent) / name).is_symlink()
        )
        for filename in sorted(filenames):
            path = Path(parent) / filename
            if path.is_symlink() or not path.is_file():
                continue
            if total_files >= max_files:
                truncated = True
                break
            total_files += 1
            relative = path.relative_to(root).as_posix()
            if filename.lower() == "gameinfo.txt" and gameinfo_path is None:
                gameinfo_path = path

            normalized = relative.casefold()
            previous = names_lower.get(normalized)
            if previous is not None:
                if len(collisions) < MAX_SAMPLES:
                    collisions.append({"first": previous, "second": relative})
            else:
                names_lower[normalized] = relative
            suffix = path.suffix.lower()
            if suffix in RESOURCE_SUFFIXES:
                counts[suffix] += 1
            if suffix not in (".bsp", ".mdl", ".vtf", ".vpk"):
                continue
            target = {
                ".bsp": maps, ".mdl": models,
                ".vtf": textures, ".vpk": archives
            }[suffix]
            if len(target) >= MAX_SAMPLES:
                continue
            size = path.stat().st_size
            if suffix == ".bsp":
                detail = bsp_header(path, size)
            elif suffix == ".mdl":
                detail = mdl_header(path, size)
            elif suffix == ".vtf":
                detail = vtf_header(path, size)
            else:
                detail = vpk_header(path, size)
            target.append({"relative_path": relative, **detail})
        if truncated:
            break

    srceng = None
    if srceng_root is not None:
        base = Path(srceng_root)
        srceng = {
            "root_exists": base.is_dir(),
            "platform_directory_exists": (base / "platform").is_dir(),
            "hl2_directory_exists": (base / "hl2").is_dir(),
            "content_mod_directory_expected_at_root": True,
            "files_not_copied_or_modified": True,
        }

    gameinfo = (gameinfo_hint(gameinfo_path) if gameinfo_path
                else {"present": False, "searchpaths_not_resolved": True})
    return {
        "format_version": 1,
        "scope": "read_only_header_and_layout_preflight_no_engine_test",
        "input": "owner_supplied_game_directory",
        "counts": {
            "files_scanned": total_files,
            "scan_truncated_at_limit": truncated,
            "resources_by_extension": dict(sorted(counts.items())),
            "case_insensitive_path_collision_count_sampled": len(collisions),
        },
        "gameinfo": gameinfo,
        "srceng_layout": srceng,
        "bsp_samples": maps,
        "mdl_samples": models,
        "vtf_samples": textures,
        "vpk_samples": archives,
        "case_collision_samples": collisions,
        "interpretation": [
            "The tested srceng project advertises BSP 19-21 and MDL 46-49; "
            "version range alone never proves rendering compatibility.",
            "Source engine branches can disagree on lump contents, shaders, "
            "entities and model dependencies even with matching header versions.",
            "Custom SiN client/server Windows x86 DLLs are not Android ARM64 "
            "game modules; this does not test mod-code loading.",
            "Source gameinfo SearchPaths, VPK mounts and content fallbacks were "
            "not resolved or modified by this scanner.",
            "No engine binaries, third-party source, original assets or "
            "game data are stored in this report.",
        ],
    }


def main(argv=None):
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("game_dir", type=Path,
                   help="Directory containing your legally owned SiN game content")
    p.add_argument("--srceng-root", type=Path, default=None,
                   help="Optional existing srceng content directory, checked read-only")
    p.add_argument("--max-files", type=int, default=MAX_FILES)
    p.add_argument("-o", "--output", type=Path,
                   default=Path("srceng-sin-preflight.json"))
    args = p.parse_args(argv)
    try:
        report = scan_owned_game(args.game_dir, args.srceng_root, args.max_files)
        args.output.write_text(
            json.dumps(report, indent=2, ensure_ascii=False) + "\n",
            encoding="utf-8",
        )
    except (ValueError, OSError) as exc:
        print("Preflight failed: %s" % exc, file=sys.stderr)
        return 1
    counts = report["counts"]
    print("Read-only preflight: %d files; %d BSP(s), %d MDL(s)" % (
        counts["files_scanned"],
        counts["resources_by_extension"].get(".bsp", 0),
        counts["resources_by_extension"].get(".mdl", 0),
    ))
    if counts["scan_truncated_at_limit"]:
        print("WARNING: file scan truncated; do not treat counts as complete")
    print("Report:", args.output)
    print("No compatibility or game execution has been demonstrated.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
