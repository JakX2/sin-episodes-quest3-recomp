#!/usr/bin/env python3
"""Owner-only line-level check of Count/Jessica script and VCD time fields.

This is an independent sanity check, not the reconstructed VCD parser or
an audio-duration oracle. It never extracts or publishes private files.
"""
import argparse
import hashlib
import math
import re
from pathlib import Path
from zipfile import ZipFile

SCRIPT_SHA = "517d5b72f23cd6e1164e7c43d72eef97d4c80285365734747c8ced9199e6985d"
SCENE_SHA = "b3bc833210dabb7fb30ffde46be1ca7136f40e09ae97b42713f48b5b22e5739e"


def sha256(path):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def text(archive, member):
    info = archive.getinfo(member)
    if info.file_size > 4 * 1024 * 1024:
        raise ValueError("oversized input member: " + member)
    return archive.read(info).decode("latin1")


def refs(archive, actor):
    source = text(archive, f"scripts/talker/{actor}.txt")
    source = re.sub(r"//[^\r\n]*", "", source)
    return [m.group(1).strip('"').replace("\\", "/").lower()
            for m in re.finditer(r"(?im)^\s*scene\s+(\S+)", source)]


def event_maximum(source):
    pairs = [(float(a), float(b)) for a, b in re.findall(
        r"(?im)^\s*time\s+([-+.\deE]+)\s+([-+.\deE]+)\s*$", source)]
    if not pairs or any(not math.isfinite(t) for pair in pairs for t in pair):
        raise ValueError("missing or non-finite event time")
    return max([0.0] + [start if end == -1 else end for start, end in pairs])


def audit(scripts, scenes):
    if sha256(scripts) != SCRIPT_SHA or sha256(scenes) != SCENE_SHA:
        raise ValueError("archive SHA256 differs from audited retail input")
    with ZipFile(scripts) as script_zip, ZipFile(scenes) as scene_zip:
        paths = {}
        for member in scene_zip.namelist():
            if member.lower().endswith(".vcd"):
                key = member.lower()
                if key in paths:
                    raise ValueError("ambiguous case-insensitive VCD path")
                paths[key] = member
        if len(paths) != 261:
            raise ValueError("unexpected VCD inventory")
        for actor, count in (("npc_count", 6), ("npc_jessica", 18)):
            targets = sorted(set(refs(script_zip, actor)))
            if actor == "npc_jessica":
                targets = [p for p in targets if "/pain_jessica" in p
                           or "/lookatjessica_breasts" in p]
            times = [event_maximum(text(scene_zip, paths[p])) for p in targets]
            if len(times) != count or any(t <= 0 for t in times):
                raise ValueError("unexpected selection target set: " + actor)
            print(f"{actor}: {len(times)}/{count} positive event maxima; "
                  f"range {min(times):.6f}..{max(times):.6f} s")
        print("PASS 24/24 retail targets; 261 unique VCD paths")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("scripts", type=Path)
    parser.add_argument("scenes", type=Path)
    args = parser.parse_args()
    audit(args.scripts, args.scenes)
