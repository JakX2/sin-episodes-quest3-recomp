#!/usr/bin/env python3
"""Reconstruct an unsigned, local APK container from extracted SourceVR contents.

Do not use the reconstructed container as an installable APK: it has no valid
original signature. The outer builder must decode, rebuild and sign a SEPARATE
debug application. No game bytes are committed to GitHub.
"""
from __future__ import annotations

from dataclasses import dataclass
import hashlib
import argparse
import json
import sys
from pathlib import Path
import re
import stat
import zipfile

REQUIRED = (
    "AndroidManifest.xml", "resources.arsc", "classes.dex",
    "lib/arm64-v8a/libsourcevrbootstrap.so",
    "lib/arm64-v8a/libhl2vrbridge.so",
)
_DEX = re.compile(r"classes([2-9][0-9]*)?\.dex$")
_SIGNED_META = re.compile(r"(?i)^META-INF/[^/]+\.(?:RSA|DSA|EC|SF|MF)$")
_EXPECTED_DEX = "0dea2c61aa36557f49d125bd91607228e522fb25dbe023c1867b54d7ae7ec74a"


@dataclass(frozen=True)
class SourceInventory:
    root: Path
    files: tuple[tuple[str, Path], ...]
    bytes_total: int
    dex_files: tuple[str, ...]
    dex_sha256: str
    native_load_counts: tuple[tuple[str, int], ...]


def inventory(root: Path, *, pin_dex: bool = True) -> SourceInventory:
    root = root.resolve(strict=True)
    if not root.is_dir():
        raise ValueError(f"expected extracted APK directory: {root}")
    members: list[tuple[str, Path]] = []
    seen: set[str] = set()
    bytes_total = 0
    for path in sorted(root.rglob("*")):
        if path.is_symlink():
            raise ValueError(f"symlink in input is forbidden: {path}")
        mode = path.stat(follow_symlinks=False).st_mode
        if stat.S_ISDIR(mode):
            continue
        if not stat.S_ISREG(mode):
            raise ValueError(f"non-regular extracted entry: {path}")
        rel = path.relative_to(root).as_posix()
        if not rel or rel.startswith("/") or ".." in rel.split("/") or "\\" in rel:
            raise ValueError(f"unsafe APK entry path: {rel}")
        folded = rel.casefold()
        if folded in seen:
            raise ValueError(f"duplicate case-insensitive APK entry: {rel}")
        seen.add(folded)
        if _SIGNED_META.fullmatch(rel):
            # Old signature files are invalid when we reconstruct the container.
            continue
        members.append((rel, path))
        bytes_total += path.stat().st_size
    names = {x[0] for x in members}
    missing = set(REQUIRED) - names
    if missing:
        raise ValueError("extracted APK missing: " + ", ".join(sorted(missing)))
    if len(names) < 20:
        raise ValueError(f"extracted APK incomplete: only {len(names)} members")
    with (root / "AndroidManifest.xml").open("rb") as handle:
        if handle.read(2) != b"\x03\x00":
            raise ValueError("AndroidManifest.xml is not compiled binary XML")
    with (root / "resources.arsc").open("rb") as handle:
        if handle.read(2) != b"\x02\x00":
            raise ValueError("resources.arsc is not a compiled resource table")
    dex_names = sorted((n for n in names if _DEX.fullmatch(n)), key=lambda s:
                       int(_DEX.fullmatch(s).group(1) or "1"))
    if dex_names != ["classes.dex"] + [f"classes{i}.dex" for i in range(2, len(dex_names)+1)]:
        raise ValueError(f"invalid / missing multidex sequence: {dex_names}")
    digest = hashlib.sha256((root / "classes.dex").read_bytes()).hexdigest()
    native_load_counts: list[tuple[str, int]] = []
    if pin_dex:
        if digest != _EXPECTED_DEX:
            raise ValueError(f"unrecognized SourceVR classes.dex: {digest}")
        if dex_names != [f"classes{i}.dex" if i > 1 else "classes.dex" for i in range(1, 6)]:
            raise ValueError(f"expected five DEX files from this SourceVR build, got: {dex_names}")
        for profile in ("hl2", "entropyzero"):
            load_name = f"assets/sourcevr/v1/native/{profile}.load"
            if load_name not in names:
                raise ValueError(f"missing required native profile graph: {load_name}")
            lines = (root / load_name).read_text(encoding="utf-8").splitlines()
            if len(lines) != 31 or len(set(lines)) != len(lines):
                raise ValueError(f"unexpected {profile} native .load graph (expected 31 distinct entries)")
            for member in lines:
                if not re.fullmatch(
                    r"sourcevr/v1/native/blobs/[0-9a-f]{64}/lib[A-Za-z0-9_+.-]+\.so",
                    member
                ):
                    raise ValueError(f"unexpected native blob name in {profile}.load: {member}")
                if "assets/" + member not in names:
                    raise ValueError(f"missing {profile} native blob: {member}")
            native_load_counts.append((profile, len(lines)))
    return SourceInventory(root, tuple(members), bytes_total, tuple(dex_names),
                           digest, tuple(native_load_counts))


def reconstruct_unsigned(inventory_data: SourceInventory, target: Path) -> dict:
    """Stream original extracted bytes into a standard ZIP; never modify source."""
    if target.exists():
        raise ValueError(f"refusing to overwrite existing source APK: {target}")
    target.parent.mkdir(parents=True, exist_ok=True)
    try:
        with zipfile.ZipFile(target, mode="x", allowZip64=True) as out:
            for rel, path in inventory_data.files:
                # This is temporary packaging, not final signing/alignment.
                # Store compressed archive resources and native ELF as-is.
                with out.open(zipfile.ZipInfo(rel), "w", force_zip64=True) as dest:
                    with path.open("rb") as src:
                        while True:
                            block = src.read(1024 * 1024)
                            if not block:
                                break
                            dest.write(block)
        with zipfile.ZipFile(target) as built:
            if built.testzip() is not None:
                raise ValueError("reconstructed APK ZIP integrity check failed")
            if set(built.namelist()) != {n for n, _ in inventory_data.files}:
                raise ValueError("reconstructed APK ZIP member set changed")
            if hashlib.sha256(built.read("classes.dex")).hexdigest() != inventory_data.dex_sha256:
                raise ValueError("reconstructed APK changed classes.dex")
    except Exception:
        target.unlink(missing_ok=True)
        raise
    return {
        "input": str(inventory_data.root),
        "archive": str(target),
        "entries": len(inventory_data.files),
        "dex": list(inventory_data.dex_files),
        "dex_sha256": inventory_data.dex_sha256,
        "input_bytes": inventory_data.bytes_total,
        "native_load_counts": dict(inventory_data.native_load_counts),
        "zip_bytes": target.stat().st_size,
        "note": "Unsigned transport container only; must rebuild and sign a separate debug APK.",
    }


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Read-only verification of the owner's original extracted SourceVR APK folder")
    parser.add_argument("extracted_dir", type=Path,
                        help="Folder containing AndroidManifest.xml, classes*.dex and assets")
    parser.add_argument("--report", type=Path, help="Optional JSON report output file")
    args = parser.parse_args()
    try:
        info = inventory(args.extracted_dir)
        report = {
            "status": "COMPLETE EXTRACTED SOURCEVR FOLDER PREFLIGHT PASSED",
            "source_directory": str(info.root),
            "entries": len(info.files),
            "total_bytes": info.bytes_total,
            "classes_dex_sha256": info.dex_sha256,
            "dex_files": list(info.dex_files),
            "native_load_counts": dict(info.native_load_counts),
            "next": "Run sourcevr_isolated_apk_builder.py with --extracted-dir, --probe and --build to construct an isolated private diagnostic APK.",
            "not_proven": "No reconstructed APK has been built, signed or launched by this preflight."
        }
        text = json.dumps(report, indent=2, sort_keys=True) + "\n"
        if args.report:
            if args.report.exists():
                raise ValueError("report path already exists; refusing to overwrite")
            args.report.write_text(text, encoding="utf-8")
        print(text)
        return 0
    except (OSError, ValueError) as error:
        print(f"SOURCEVR EXTRACTED PREFLIGHT FAILED: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
