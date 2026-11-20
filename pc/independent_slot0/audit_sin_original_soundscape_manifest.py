#!/usr/bin/env python3
"""Phase 482: SHA-pinned, read-only SiN 2006 soundscape manifest evidence.

Private: --client original/client.dll --scripts original/scripts.zip
Public original-free CI: --self-test. No retail bytes in output/repo.
"""
import argparse
import hashlib
import json
import re
import struct
import sys
import zipfile
from pathlib import Path
from audit_sin_original_bf_read_candidate_layout import (
    EXPECTED_SHA, parse_image, region_at_va,
)

METHOD_START = 0x240F4160
METHOD_SIZE = 0x157
METHOD_SHA = "0fbb82377e54b7119c2c0e1a0b107c8958b354010993e03a481829cb2a2d661a"
MANIFEST_SHA = "d2c02a5f0dddb3cdcb92c655d222c6a549d77f5f940211d6429a1ef996391863"
MANIFEST_PATH = "scripts/soundscapes_manifest.txt"
ROOT_OBSERVED = "soundscaples_manifest"
FILES_OBSERVED = (
    "scripts/soundscapes.txt",
    "scripts/soundscapes_intro.txt",
    "scripts/soundscapes_docks.txt",
    "scripts/soundscapes_druglab.txt",
    "scripts/soundscapes_pit.txt",
    "scripts/soundscapes_highrise.txt",
    "scripts/soundscapes_finale.txt",
)
# Original directly called targets, without exporting their implementation.
CALLS = (
    (0x240F416F, 0x241560A0),
    (0x240F4191, 0x24164C60),
    (0x240F41A4, 0x24165010),
    (0x240F41C9, 0x24164A50),
    (0x240F41D8, 0x24164B90),
    (0x240F41F2, 0x24164770),
    (0x240F41FD, 0x2425F81B),
    (0x240F4211, 0x241667C0),
    (0x240F4219, 0x240F4070),
    (0x240F422B, 0x241667C0),
    (0x240F4232, 0x2425F81B),
    (0x240F4242, 0x24164770),
    (0x240F4259, 0x24164BA0),
    (0x240F4281, 0x240F4070),
    (0x240F4288, 0x24166050),
    (0x240F42AA, 0x24166050),
)
# The observed initialization writes this+0xCC = 0 and ends with
# return true at either the regular or missing-manifest cleanup exit.
MARKERS = (
    (0x240F4169, "89b5cc000000"),
    (0x240F41CE, "84c0"),
    (0x240F41D0, "0f84bf000000"),
    (0x240F421E, "85ff"),
    (0x240F4268, "85ff"),
    (0x240F4277, "ff5228"),
    (0x240F4291, "b001"),
    (0x240F42B2, "b001"),
)


def inspect_method(data, check_digest=True):
    if len(data) != METHOD_SIZE:
        raise ValueError("wrong original soundscape Init length")
    if check_digest and hashlib.sha256(data).hexdigest() != METHOD_SHA:
        raise ValueError("original C_SoundscapeSystem Init SHA mismatch")
    for va, target in CALLS:
        at = va - METHOD_START
        expected = b"\xe8" + struct.pack("<i", target - va - 5)
        if data[at:at+5] != expected:
            raise ValueError("soundscape original CALL mismatch at %#x" % va)
    for va, hex_bytes in MARKERS:
        opcode = bytes.fromhex(hex_bytes)
        at = va - METHOD_START
        if data[at:at+len(opcode)] != opcode:
            raise ValueError("soundscape original field/branch mismatch at %#x" % va)
    return {"full_original_method_SHA_verified": bool(check_digest),
            "direct_CALL_edges": len(CALLS),
            "field_and_branch_witnesses": len(MARKERS),
            "returns_true_on_both_observed_exits": True}


def inspect_manifest(data, available):
    if hashlib.sha256(data).hexdigest() != MANIFEST_SHA:
        raise ValueError("original soundscapes_manifest SHA mismatch")
    text = data.decode("ascii")
    root = re.search(r"^\s*([A-Za-z_][A-Za-z_0-9]*)\s*\{", text)
    if not root or root.group(1) != ROOT_OBSERVED:
        raise ValueError("retail root spelling differs")
    # This original-file-specific audit can use a regex after SHA
    # pinning, unlike the independent general parser in the port.
    paths = tuple(re.findall(
        r'"file"\s+"(scripts/[^"\r\n]+)"', text, re.IGNORECASE))
    if paths != FILES_OBSERVED:
        raise ValueError("original ordered soundscape entries differ")
    absent = [p for p in paths if p not in available]
    if absent:
        raise ValueError("original manifest points to absent files")
    return {"manifest_SHA_verified": True,
            "original_manifest_root": root.group(1),
            "file_entries": len(paths),
            "unique_file_entries": len(set(paths)),
            "all_referenced_scripts_present": True,
            "source_scripts_bytes_in_report": False}


def private_audit(client, scripts):
    if hashlib.sha256(client).hexdigest() != EXPECTED_SHA:
        raise ValueError("original client.dll whole-file SHA mismatch")
    pe = parse_image(client)
    if pe[0] != 0x24000000:
        raise ValueError("unexpected original image base")
    method = region_at_va(client, pe, METHOD_START, METHOD_SIZE)
    with zipfile.ZipFile(scripts) as z:
        manifest = z.read(MANIFEST_PATH)
        names = set(z.namelist())
        assets = inspect_manifest(manifest, names)
    return {"phase": 482, "private_original_client_verified": True,
            "original_method": inspect_method(method),
            "original_assets": assets,
            "actual_game_startup_verified": False}


def self_test():
    fabricated = bytearray(b"\xcc" * METHOD_SIZE)
    for va, target in CALLS:
        at = va - METHOD_START
        fabricated[at:at+5] = b"\xe8" + struct.pack(
            "<i", target-va-5)
    for va, hex_bytes in MARKERS:
        code = bytes.fromhex(hex_bytes)
        at = va - METHOD_START
        fabricated[at:at+len(code)] = code
    report = inspect_method(fabricated, check_digest=False)
    assert report["direct_CALL_edges"] == len(CALLS)
    corrupted = bytearray(fabricated)
    corrupted[0x240F41C9 - METHOD_START] ^= 1
    try:
        inspect_method(corrupted, check_digest=False)
    except ValueError:
        pass
    else:
        raise ValueError("fabricated method corrupted CALL accepted")
    corrupted = bytearray(fabricated)
    corrupted[0x240F4169 - METHOD_START] ^= 1
    try:
        inspect_method(corrupted, check_digest=False)
    except ValueError:
        pass
    else:
        raise ValueError("fabricated method corrupted state write accepted")
    return {"fabricated_soundscape_method_markers": "PASS",
            "no_original_retail_content": True}


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--self-test", action="store_true")
    p.add_argument("--client", type=Path)
    p.add_argument("--scripts", type=Path)
    args = p.parse_args()
    if args.self_test:
        result = self_test()
    else:
        if args.client is None or args.scripts is None:
            p.error("pass --self-test or both --client and --scripts")
        result = private_audit(args.client.read_bytes(), args.scripts)
    print(json.dumps(result, sort_keys=True))


if __name__ == "__main__":
    try:
        main()
    except (ValueError, AssertionError, FileNotFoundError,
            zipfile.BadZipFile, UnicodeError) as exc:
        print("FAIL private original soundscape audit: " + str(exc),
              file=sys.stderr)
        sys.exit(1)
