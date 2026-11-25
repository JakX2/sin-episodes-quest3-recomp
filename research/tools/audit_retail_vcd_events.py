#!/usr/bin/env python3
"""Owner-only VCD declaration inventory; never extracts or prints assets."""
import argparse
import hashlib
import re
from collections import Counter
from pathlib import Path
from zipfile import ZipFile

SCENE_SHA = "b3bc833210dabb7fb30ffde46be1ca7136f40e09ae97b42713f48b5b22e5739e"
EVENT = re.compile(r"^[ \t]*event[ \t]+([A-Za-z_]+)\b", re.I | re.M)
ACTOR = re.compile(r"^[ \t]*actor[ \t]+", re.I | re.M)
FLAGS = ("active", "absolutetags", "cc_noattenuate", "cctype", "event_ramp",
         "flextimingtags", "resumecondition")


def audit(path):
    with path.open("rb") as stream:
        if hashlib.file_digest(stream, "sha256").hexdigest() != SCENE_SHA:
            raise ValueError("unexpected retail scenes archive hash")
    events, files, actors, flags = Counter(), Counter(), Counter(), Counter()
    names = set()
    with ZipFile(path) as archive:
        for member in archive.infolist():
            key = member.filename.replace("\\", "/").lower()
            if not key.endswith(".vcd"):
                continue
            if key in names or member.file_size > 4 * 1024 * 1024:
                raise ValueError("duplicate or oversized VCD member")
            names.add(key)
            source = archive.read(member).decode("latin1")
            kinds = [m.lower() for m in EVENT.findall(source)]
            events.update(kinds)
            files.update(set(kinds))
            actors[len(ACTOR.findall(source))] += 1
            flags.update(flag for flag in FLAGS if re.search(
                rf"^[ \t]*{flag}\b", source, re.I | re.M))
    if len(names) != 261 or sum(events.values()) != 2994:
        raise ValueError("unexpected declaration inventory")
    print(f"VCDs: {len(names)}; event declarations: {sum(events.values())}")
    for kind, count in sorted(events.items(), key=lambda item: (-item[1], item[0])):
        print(f"{kind}: {count} declarations in {files[kind]} VCDs")
    print(f"subscene: {events['subscene']} declarations")
    print("actor declarations per VCD: " + ", ".join(f"{n}={count}" for n, count in sorted(actors.items())))
    print("flag occurrence by VCD: " + ", ".join(f"{flag}={flags[flag]}" for flag in FLAGS))
    return events, files, actors, flags


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("scenes", type=Path)
    audit(parser.parse_args().scenes)
