#!/usr/bin/env python3
"""Fail-closed, original-free triage of one-click owner filesystem diagnostic ZIPs.

Reads text only, never extracts or executes archive members. This tool does NOT
authenticate retail DLLs or prove normal game initialization.
"""
import argparse
import json
import pathlib
import re
import sys
import zipfile

MAX_FILES = 64
MAX_MEMBER = 2 * 1024 * 1024
MAX_TOTAL = 8 * 1024 * 1024
TEXT_SUFFIXES = (".txt", ".log")
RESTORED = "OWNER_INDEPENDENT_RESTORATION: VERIFIED_COMPLETE"
PRESENCE = "0x53494E68"
READ = "0x53494E6B"


def inspect(path):
    problems = []
    documents = {}
    with zipfile.ZipFile(path) as archive:
        members = archive.infolist()
        if len(members) > MAX_FILES:
            raise ValueError("too many archive members")
        if sum(m.file_size for m in members) > MAX_TOTAL:
            raise ValueError("archive uncompressed size exceeds limit")
        for info in members:
            name = info.filename.replace("\\", "/")
            parts = name.split("/")
            if (info.is_dir() or name.startswith("/") or ".." in parts or
                    ":" in name or "\x00" in name or
                    info.file_size > MAX_MEMBER or
                    not name.lower().endswith(TEXT_SUFFIXES) or
                    name in documents):
                raise ValueError("unsafe, duplicate or non-text archive member: " + name)
            raw = archive.read(info)
            if len(raw) != info.file_size:
                raise ValueError("archive member size mismatch: " + name)
            documents[name] = raw.decode("utf-8-sig", errors="replace")
    def select(basename):
        matches = [v for k, v in documents.items() if k.rsplit("/", 1)[-1].lower() == basename]
        if len(matches) != 1:
            problems.append("missing or ambiguous " + basename)
            return ""
        return matches[0]
    summary = select("summary.txt")
    presence = select("presence.txt")
    read = select("read.txt") if any(k.lower().endswith("read.txt") for k in documents) else ""
    # These are report-text observations, not trusted original executable evidence.
    presence_ok = RESTORED in presence and PRESENCE.lower() in presence.lower() and "both-present" in presence.lower()
    read_ok = RESTORED in read and READ.lower() in read.lower() and bool(re.search(r"62\s*:\s*8\s*:\s*156\s*:\s*66", read))
    if not presence_ok:
        problems.append("presence evidence incomplete or restoration unverified")
    if read and not read_ok:
        problems.append("read evidence incomplete or restoration unverified")
    if not summary.strip():
        problems.append("summary absent or empty")
    return {
        "classification": "reported_read_gate" if read_ok and presence_ok else
                          "reported_presence_gate" if presence_ok else "insufficient_evidence",
        "presence_reported": presence_ok,
        "read_reported": read_ok,
        "normal_game_boot_verified": False,
        "original_method_engine_tested": False,
        "problems": problems,
        "members": sorted(documents),
    }


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("archive", type=pathlib.Path)
    args = parser.parse_args(argv)
    try:
        result = inspect(args.archive)
    except (OSError, ValueError, zipfile.BadZipFile, RuntimeError) as exc:
        print(json.dumps({"classification": "invalid_archive", "error": str(exc)}, indent=2))
        return 2
    print(json.dumps(result, indent=2, sort_keys=True))
    return 0 if not result["problems"] else 1


if __name__ == "__main__":
    sys.exit(main())
