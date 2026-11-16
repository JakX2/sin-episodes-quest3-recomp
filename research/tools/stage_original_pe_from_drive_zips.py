#!/usr/bin/env python3
"""Privately stage the TWO exact owner-original SiN Episodes (2006) PE32 DLLs.

Read candidate ZIPs from the owner's Drive download in GitHub Actions,
but never extract arbitrary ZIP paths or publish original game bytes.
Identifies the correct client/server DLL by full-file SHA256, size, and
filename rather than trusting archive order or its container filename.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import stat
import tempfile
import zipfile
from pathlib import Path

EXPECTED = {
    "client.dll": (
        3_592_192,
        "6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d",
    ),
    "server.dll": (
        6_049_792,
        "568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650",
    ),
}
MAX_MEMBER_BYTES = 20 * 1024 * 1024


class OriginalBinaryError(ValueError):
    pass


def find_originals(archive_paths: list[Path],
                   expectations: dict[str, tuple[int, str]] | None = None
                   ) -> dict[str, bytes]:
    expected = EXPECTED if expectations is None else expectations
    found: dict[str, bytes] = {}
    for archive in archive_paths:
        try:
            with zipfile.ZipFile(archive) as z:
                for info in z.infolist():
                    if info.is_dir():
                        continue
                    name = Path(info.filename.replace("\\", "/")).name.lower()
                    if name not in expected:
                        continue
                    size, signature = expected[name]
                    if info.file_size > MAX_MEMBER_BYTES:
                        raise OriginalBinaryError(
                            "DLL member exceeds safe decompressed size limit")
                    content = z.read(info)
                    if len(content) != size or \
                            hashlib.sha256(content).hexdigest() != signature:
                        raise OriginalBinaryError(
                            "Owner-original SHA256 or size mismatch: " + name)
                    if name in found:
                        raise OriginalBinaryError(
                            "Duplicate original DLL in input archives: " + name)
                    found[name] = content
        except (zipfile.BadZipFile, RuntimeError, EOFError) as error:
            raise OriginalBinaryError(
                "Invalid or unavailable original ZIP; Drive download may "
                "have returned an HTML sign-in page") from error
    missing = set(expected) - set(found)
    if missing:
        raise OriginalBinaryError(
            "Missing expected retail original DLL(s): " +
            ", ".join(sorted(missing)))
    return found


def stage(archives: list[Path], output_dir: Path,
          expectations: dict[str, tuple[int, str]] | None = None) -> dict:
    matched = find_originals(archives, expectations)
    output_dir.mkdir(mode=0o700, parents=True, exist_ok=True)
    output_dir.chmod(0o700)
    summary = {}
    for name, content in matched.items():
        destination = output_dir / name
        # Fail closed rather than overwriting potentially different files.
        if destination.exists() or destination.is_symlink():
            raise OriginalBinaryError("Refusing to overwrite " + name)
        # Atomic per-file publish from an owner-only temporary location.
        fd, staging = tempfile.mkstemp(prefix="original-pe-", dir=output_dir)
        try:
            with os.fdopen(fd, "wb") as stream:
                stream.write(content)
            os.chmod(staging, stat.S_IRUSR | stat.S_IWUSR)
            os.replace(staging, destination)
        finally:
            if os.path.exists(staging):
                os.unlink(staging)
        summary[name] = {
            "size": len(content),
            "sha256": hashlib.sha256(content).hexdigest(),
            "staged": True,
        }
    return summary


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--archive", type=Path, required=True, action="append")
    parser.add_argument("--output-dir", type=Path, required=True)
    args = parser.parse_args()
    summary = stage(args.archive, args.output_dir)
    # Only fingerprints and counts, never original bytes or private download URLs.
    print(json.dumps(summary, indent=2))


if __name__ == "__main__":
    main()
