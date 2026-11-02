#!/usr/bin/env python3
"""Opt-in APP_MOD_LIB setter injection for a DECODED, PRIVATE SourceVR APK.

Default dry-run; --apply changes exactly one MainActivity.smali file.
The original APK and its installed counterpart are never touched.
Requires the exact original classes.dex fingerprint. Does not build/sign APKs.
"""
from __future__ import annotations

import argparse
import hashlib
from pathlib import Path
import re
import sys
import tempfile
import zipfile

EXPECTED_DEX_SHA256 = "0dea2c61aa36557f49d125bd91607228e522fb25dbe023c1867b54d7ae7ec74a"
EXPECTED_CLASS = "Lcom/sourcevrport/hl2vr/MainActivity;"
SETENV = EXPECTED_CLASS + "->setenv(Ljava/lang/String;Ljava/lang/String;)V"

# Verified from the pinned original classes.dex method proto. This method takes
# two lists and a NativeGraph, NOT zero arguments.
SETUP_DESCRIPTOR = (
    "(Ljava/util/List;Ljava/util/List;"
    "Lcom/sourcevrport/hl2vr/ProfileAssetStage$NativeGraph;)V"
)
METHOD = re.compile(
    r"(?ms)^(?P<header>\.method[^\n]*\bsetupEngineEnvironment"
    + re.escape(SETUP_DESCRIPTOR) + r"[ \t]*\r?\n)"
    r"(?P<body>.*?)(?P<footer>^\.end method[ \t]*$)"
)
# Matches a short window of real smali, without assuming register numbers or
# arbitrary whitespace/line-number directives from the decompiler.
KEY = re.compile(
    r"(?m)^(?P<indent>[ \t]*)const-string(?:/jumbo)?[ \t]+"
    r"(?P<keyreg>[vp]\d+),[ \t]*\"APP_LIB_PATH\"[ \t]*$"
)
CALL = re.compile(
    r"(?m)^[ \t]*invoke-static[ \t]+"
    r"\{[ \t]*(?P<keyreg>[vp]\d+)[ \t]*,[ \t]*(?P<pathreg>[vp]\d+)[ \t]*\}"
    r",[ \t]*" + re.escape(SETENV) + r"[ \t]*$"
)
PATH = re.compile(
    r"(?m)^[ \t]*iget-object[ \t]+(?P<dst>[vp]\d+),[ \t]*[vp]\d+"
    r",[ \t]*Landroid/content/pm/ApplicationInfo;->nativeLibraryDir:Ljava/lang/String;[ \t]*$"
)
COMMENT_OR_BLANK = re.compile(r"(?m)^[ \t]*(?:#[^\n]*)?$")

def verify_original_dex(data: bytes) -> str:
    digest = hashlib.sha256(data).hexdigest()
    if digest != EXPECTED_DEX_SHA256:
        raise ValueError(
            f"Unsupported classes.dex SHA256: {digest}; expected {EXPECTED_DEX_SHA256}. "
            "Refusing to modify an unreviewed SourceVR build."
        )
    return digest


def get_source_dex(*, apk: Path | None, dex: Path | None) -> bytes:
    if (apk is None) == (dex is None):
        raise ValueError("Specify exactly one of --original-apk or --original-dex")
    if apk is not None:
        with zipfile.ZipFile(apk) as z:
            return z.read("classes.dex")
    assert dex is not None
    return dex.read_bytes()


def find_activity(root: Path) -> Path:
    if not root.is_dir():
        raise ValueError(f"decoded APK directory not found: {root}")
    candidates = sorted(
        p for p in root.glob("smali*/com/sourcevrport/hl2vr/MainActivity.smali")
        if p.is_file()
    )
    if len(candidates) != 1:
        raise ValueError(f"expected exactly one MainActivity.smali; found {len(candidates)}")
    return candidates[0]


def patch_smali(source: str) -> tuple[str, str]:
    found = list(METHOD.finditer(source))
    if len(found) != 1:
        candidates = re.findall(r"(?m)^\.method[^\r\n]*setupEngineEnvironment[^\r\n]*", source)
        raise ValueError(
            "expected exactly one verified setupEngineEnvironment" + SETUP_DESCRIPTOR
            + f" method; decoded candidates: {candidates or ['none']}"
        )
    method = found[0]
    body = method.group("body")
    if '"APP_MOD_LIB"' in body:
        raise ValueError("APP_MOD_LIB already present in setupEngineEnvironment; refusing duplicate")
    keys = list(KEY.finditer(body))
    if len(keys) != 1:
        raise ValueError(f"expected exactly one APP_LIB_PATH assignment; found {len(keys)}")
    key = keys[0]

    # The DEX evidence has APP_LIB_PATH immediately followed by
    # invoke-static {key, nativeLibraryDir}, MainActivity.setenv(...)
    tail = body[key.end():]
    calls = list(CALL.finditer(tail))
    if not calls:
        raise ValueError("missing MainActivity.setenv invocation after APP_LIB_PATH")
    call = calls[0]
    intervening = tail[:call.start()]
    if len(intervening) > 240 or any(
        line.strip() and not line.lstrip().startswith((".line", "#"))
        for line in intervening.splitlines()
    ):
        raise ValueError("unexpected instructions between APP_LIB_PATH and setenv")
    if call.group("keyreg") != key.group("keyreg"):
        raise ValueError("APP_LIB_PATH register does not match setenv first argument")

    # Confirm the path register was loaded from ApplicationInfo.nativeLibraryDir,
    # not from an unknown or user-controlled file location.
    prefix = body[max(0, key.start() - 480):key.start()]
    paths = list(PATH.finditer(prefix))
    if not paths or paths[-1].group("dst") != call.group("pathreg"):
        raise ValueError("setenv second argument not verified nativeLibraryDir")
    path_between = prefix[paths[-1].end():] + body[key.start():key.end()]
    if any(
        ("const-string" in line or "move-result" in line or "iput-" in line)
        for line in path_between.splitlines()
        if "APP_LIB_PATH" not in line
    ):
        raise ValueError("possible nativeLibraryDir register overwrite before setenv")

    newline = "\r\n" if "\r\n" in source else "\n"
    indent = key.group("indent")
    name = key.group("keyreg")
    pathreg = call.group("pathreg")
    injection = (
        newline + indent + "# SourceVR private loader preflight: reuse the verified"
        " nativeLibraryDir as APP_MOD_LIB." + newline
        + indent + f'const-string {name}, "APP_MOD_LIB"' + newline
        + indent + f"invoke-static {{{name}, {pathreg}}}, {SETENV}"
    )
    # Insert directly after the original invoke to retain the path register.
    end = key.end() + call.end()
    adjusted = body[:end] + injection + body[end:]
    result = source[:method.start("body")] + adjusted + source[method.end("body"):]
    return result, f"APP_MOD_LIB <= ApplicationInfo.nativeLibraryDir via registers {name}, {pathreg}"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("decoded_apk", type=Path, help="Existing apktool decoded directory; NOT an installed APK")
    source = parser.add_mutually_exclusive_group(required=True)
    source.add_argument("--original-apk", type=Path, help="Unmodified SourceVR APK for SHA verification")
    source.add_argument("--original-dex", type=Path, help="Original SourceVR classes.dex for SHA verification")
    parser.add_argument("--apply", action="store_true", help="Write the opt-in modification to decoded smali")
    options = parser.parse_args()
    try:
        digest = verify_original_dex(get_source_dex(apk=options.original_apk, dex=options.original_dex))
        path = find_activity(options.decoded_apk)
        original = path.read_text(encoding="utf-8")
        patched, summary = patch_smali(original)
        print(f"Original DEX verified: {digest}")
        print(f"Target: {path}")
        print(summary)
        if not options.apply:
            print("DRY RUN only. Pass --apply to patch the decoded copy.")
            return 0
        # Atomic replacement with a one-time adjacent backup outside smali.
        backup = options.decoded_apk / "sourcevr_original_MainActivity.smali.backup"
        if backup.exists():
            raise ValueError(f"refusing to overwrite existing backup: {backup}")
        backup.write_text(original, encoding="utf-8")
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", dir=path.parent,
                                         delete=False) as handle:
            tmp = Path(handle.name)
            handle.write(patched)
        try:
            tmp.replace(path)
        finally:
            tmp.unlink(missing_ok=True)
        print(f"PATCHED decoded copy. Original saved at {backup}")
        print("No APK rebuilt/signed. No original installed app modified.")
        return 0
    except (OSError, ValueError, zipfile.BadZipFile, KeyError) as error:
        print(f"PRECHECK FAILED: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
