#!/usr/bin/env python3
"""Read-only inventory of an owner-held SourceVR APK, extracted tree, or TXT listing.

No third-party packages, unpacking, subprocess execution or game/engine files
are required. Only small manifest previews and ELF identification headers are
read. Never commit APKs, extracted proprietary libraries or generated reports.
"""
import argparse
from collections import Counter
import json
from pathlib import Path
import re
import sys
import zipfile

SOURCEVR_ROOT = "assets/sourcevr/v1/"
NATIVE_BLOBS = SOURCEVR_ROOT + "native/blobs/"
SHA_BLOBS = SOURCEVR_ROOT + "blobs/sha256/"
APK_LIBS = "lib/arm64-v8a/"
MAX_PREVIEW = 512
HEX_64 = re.compile(r"^[a-fA-F0-9]{64}$")
MANIFEST = re.compile(
    r"^assets/sourcevr/v1/(native|overlay|vr)/[^/]+\.(manifest|load)$"
)
ELF_MACHINES = {3: "x86", 40: "ARM", 62: "x86_64", 183: "AArch64"}


def safe_name(name):
    """Normalize paths without ever extracting a ZIP archive."""
    normalized = str(name).replace("\\", "/")
    if normalized.startswith("/") or re.match(r"^[A-Za-z]:", normalized):
        return None
    parts = normalized.split("/")
    if any(part in ("", ".", "..") for part in parts):
        return None
    return "/".join(parts)


class PackageIndex:
    def __init__(self, source):
        self.source = Path(source)
        self.entries = {}
        self.kind = ""
        self._zip = None
        if self.source.is_dir():
            self.kind = "extracted_directory"
            for file in self.source.rglob("*"):
                if not file.is_file():
                    continue
                name = safe_name(file.relative_to(self.source).as_posix())
                if name is not None:
                    self.entries[name] = file.stat().st_size
        elif self.source.is_file() and zipfile.is_zipfile(self.source):
            self.kind = "zip_or_apk"
            self._zip = zipfile.ZipFile(self.source)
            for info in self._zip.infolist():
                if info.is_dir():
                    continue
                name = safe_name(info.filename)
                if name is not None:
                    self.entries[name] = info.file_size
        elif self.source.is_file() and self.source.suffix.lower() == ".txt":
            self.kind = "path_listing_only"
            for line in self.source.read_text(encoding="utf-8-sig").splitlines():
                line = line.strip()
                if not line:
                    continue
                if " bytes\t" in line:
                    line = line.split(" bytes\t", 1)[1]
                name = safe_name(line)
                if name is not None:
                    self.entries[name] = None
        else:
            raise ValueError(
                "Supply an extracted APK directory, a ZIP/APK, or a TXT path "
                "listing. For RAR/7z, point to the already extracted folder."
            )

    def close(self):
        if self._zip is not None:
            self._zip.close()

    def read_prefix(self, name, length):
        if self.kind == "path_listing_only":
            return None
        length = min(max(0, int(length)), MAX_PREVIEW)
        if self._zip is not None:
            with self._zip.open(name) as stream:
                return stream.read(length)
        with (self.source / name).open("rb") as stream:
            return stream.read(length)


def elf_identity(data):
    if data is None:
        return "not_checked_listing_only"
    if len(data) < 20 or data[:4] != b"\x7fELF":
        return "not_elf_or_unreadable"
    elf_class = {1: "ELF32", 2: "ELF64"}.get(data[4], "unknown_class")
    endian = {1: "little", 2: "big"}.get(data[5])
    if endian is None:
        return "elf_unknown_endian"
    machine = int.from_bytes(data[18:20], endian)
    return "%s_%s" % (elf_class, ELF_MACHINES.get(machine, "machine_%d" % machine))


def manifest_preview(data):
    if data is None:
        return {"kind": "not_checked_listing_only"}
    if not data:
        return {"kind": "empty"}
    try:
        text = data.decode("utf-8-sig")
    except UnicodeError:
        return {"kind": "binary", "hex_prefix": data[:32].hex()}
    if "\x00" in text or sum(
        char.isprintable() or char in "\r\n\t" for char in text
    ) / len(text) < 0.90:
        return {"kind": "binary", "hex_prefix": data[:32].hex()}
    return {"kind": "text", "preview": text[:MAX_PREVIEW]}


def analyze(index):
    names = sorted(index.entries)
    natives = Counter()
    native_paths = {}
    for name in names:
        parts = name.split("/")
        if (
            len(parts) == 7
            and parts[:5] == ["assets", "sourcevr", "v1", "native", "blobs"]
            and HEX_64.fullmatch(parts[5])
            and parts[6].startswith("lib")
            and parts[6].endswith(".so")
        ):
            natives[parts[6]] += 1
            native_paths.setdefault(parts[6], []).append(name)

    manifests = []
    for name in names:
        if MANIFEST.fullmatch(name):
            data = index.read_prefix(name, MAX_PREVIEW)
            manifests.append({
                "path": name,
                "size_bytes": index.entries[name],
                **manifest_preview(data),
            })

    game_names = sorted({
        name.rsplit("/", 1)[1].rsplit(".", 1)[0]
        for name in names
        if name.startswith(SOURCEVR_ROOT + "native/")
        and (name.endswith(".manifest") or name.endswith(".load"))
        and "/blobs/" not in name
    })
    games = [
        {
            "id": game,
            "native_manifest": SOURCEVR_ROOT + "native/" + game + ".manifest" in index.entries,
            "native_load": SOURCEVR_ROOT + "native/" + game + ".load" in index.entries,
        }
        for game in game_names
    ]

    base_libs = [name for name in names if name.startswith(APK_LIBS) and name.endswith(".so")]
    variants = [
        {
            "name": name,
            "copies": count,
            "architectures": dict(sorted(Counter(
                elf_identity(index.read_prefix(path, 20))
                for path in native_paths[name]
            ).items())),
        }
        for name, count in sorted(natives.items())
    ]
    boot = [
        {
            "name": path.removeprefix(APK_LIBS),
            "architecture": elf_identity(index.read_prefix(path, 20)),
            "size_bytes": index.entries[path],
        }
        for path in base_libs
    ]
    sha_count = sum(
        path.startswith(SHA_BLOBS) and HEX_64.fullmatch(path[len(SHA_BLOBS):]) is not None
        for path in names
    )
    warnings = [
        "Packaged filenames and ELF headers do not establish a usable game-module ABI.",
        "SourceVR documents content-only mod support; custom client/server modules are unsupported.",
        "No original game/engine binaries or payloads are included in this report.",
    ]
    if not any(name.endswith("sin.manifest") or name.endswith("sin.load") for name in names):
        warnings.append("No SiN-specific native manifest/load entry was found.")
    return {
        "format_version": 1,
        "input_kind": index.kind,
        "counts": {
            "files": len(names),
            "content_sha256_blobs": sha_count,
            "native_library_variants": sum(natives.values()),
            "distinct_native_library_names": len(natives),
        },
        "android_arm64_libraries": boot,
        "native_libraries": variants,
        "native_game_entries": games,
        "metadata": manifests,
        "warnings": warnings,
    }


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", type=Path, help="APK/ZIP, extracted folder or TXT file listing")
    parser.add_argument("-o", "--output", type=Path, default=Path("sourcevr-audit.json"))
    args = parser.parse_args(argv)
    package = None
    try:
        package = PackageIndex(args.input)
        report = analyze(package)
        args.output.write_text(json.dumps(report, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
        counts = report["counts"]
        print("SourceVR audit (%s): %d files, %d content blobs, %d native variants" %
              (report["input_kind"], counts["files"],
               counts["content_sha256_blobs"], counts["native_library_variants"]))
        print("Wrote:", args.output)
        return 0
    except (OSError, ValueError, zipfile.BadZipFile) as exc:
        print("SourceVR audit failed:", exc, file=sys.stderr)
        return 1
    finally:
        if package is not None:
            package.close()


if __name__ == "__main__":
    sys.exit(main())
