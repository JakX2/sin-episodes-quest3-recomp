#!/usr/bin/env python3
"""Build an opt-in SIDE-BY-SIDE SourceVR diagnostic APK from a private original.

Default mode only prints preflight. --build rebuilds, signs, validates an
isolated package. It never installs an APK or modifies the source APK.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import struct
import subprocess
import sys
import zipfile
import xml.etree.ElementTree as ET

from sourcevr_java_env_audit import Dex, MAIN
from sourcevr_isolated_modlib_patch import get_source_dex, verify_original_dex, find_activity, patch_smali

ANDROID = "http://schemas.android.com/apk/res/android"
A_NAME = "{%s}name" % ANDROID
ORIGINAL_PACKAGE = "com.sourcevrport.hl2vr"
DEFAULT_PACKAGE = "com.sourcevrport.hl2vr.sinprobe"
COMPONENTS = {"activity", "activity-alias", "service", "receiver", "provider", "instrumentation"}


def validate_arm64_probe(data: bytes) -> str:
    if len(data) < 128 or data[:6] != b"\x7fELF\x02\x01" or struct.unpack_from("<H", data, 18)[0] != 183:
        raise ValueError("probe is not a little-endian ARM64 ELF64 shared library")
    if b"CreateInterface\x00" not in data or b"SinFactoryProbeRequestCount\x00" not in data:
        raise ValueError("probe missing the expected factory diagnostic symbol strings")
    return hashlib.sha256(data).hexdigest()


def fqcn(package: str, value: str) -> str:
    if value.startswith("."):
        return package + value
    if "." not in value:
        return package + "." + value
    return value


def change_manifest(source: str, new_package: str) -> str:
    if not re.fullmatch(r"[A-Za-z_][A-Za-z_0-9]*(?:\.[A-Za-z_][A-Za-z_0-9]*)+", new_package):
        raise ValueError("invalid debug package name")
    if new_package == ORIGINAL_PACKAGE:
        raise ValueError("debug package must differ from original")
    ET.register_namespace("android", ANDROID)
    try:
        root = ET.fromstring(source)
    except ET.ParseError as exc:
        raise ValueError("decoded AndroidManifest.xml is not readable XML") from exc
    if root.tag != "manifest" or root.get("package") != ORIGINAL_PACKAGE:
        raise ValueError("unsupported original manifest package; refusing side-by-side conversion")
    root.set("package", new_package)
    for item in root.iter():
        local = item.tag.rsplit("}", 1)[-1]
        if local in COMPONENTS or local == "application":
            name = item.get(A_NAME)
            if name:
                item.set(A_NAME, fqcn(ORIGINAL_PACKAGE, name))
        if local == "activity-alias":
            target = item.get("{%s}targetActivity" % ANDROID)
            if target:
                item.set("{%s}targetActivity" % ANDROID, fqcn(ORIGINAL_PACKAGE, target))
        if local == "provider":
            auth = item.get("{%s}authorities" % ANDROID)
            if auth:
                item.set("{%s}authorities" % ANDROID, auth.replace(ORIGINAL_PACKAGE, new_package))
        if local in ("permission", "uses-permission"):
            name = item.get(A_NAME)
            if name and name.startswith(ORIGINAL_PACKAGE + "."):
                item.set(A_NAME, new_package + name[len(ORIGINAL_PACKAGE):])
    app = root.find("application")
    if app is None:
        raise ValueError("missing manifest application")
    if root.get("{%s}sharedUserId" % ANDROID):
        raise ValueError("sharedUserId requires original certificate; side-by-side signing unsafe")
    # Engine uses stat()+dlopen(path), not transparent APK!/lib loading.
    app.set("{%s}extractNativeLibs" % ANDROID, "true")
    app.set("{%s}label" % ANDROID, "SourceVR SiN Loader Probe")
    return ET.tostring(root, encoding="unicode") + "\n"


def cmd(argv: list[str]) -> None:
    print("+", " ".join(argv))
    subprocess.run(argv, check=True)


def validate_signed_apk(path: Path, debug_package: str) -> dict:
    if not path.is_file() or path.stat().st_size < 10000:
        raise ValueError("missing/empty signed APK")
    with zipfile.ZipFile(path) as archive:
        names = set(archive.namelist())
        if "classes.dex" not in names or "lib/arm64-v8a/libclient.so" not in names:
            raise ValueError("signed APK missing DEX or diagnostic client")
        data = archive.read("lib/arm64-v8a/libclient.so")
        if b"SinFactoryProbeRequestCount\x00" not in data:
            raise ValueError("signed APK does not contain the factory diagnostic")
        dex = Dex(archive.read("classes.dex"))
        setup = dex.method(MAIN + "->setupEngineEnvironment")
        values = [
            dex.strings[setup[i+1]] for i in range(len(setup)-1)
            if (setup[i] & 255) == 0x1a and setup[i+1] < len(dex.strings)
        ]
        if values.count("APP_MOD_LIB") != 1 or values.count("APP_LIB_PATH") != 1:
            raise ValueError("rebuilt DEX missing or duplicating native library configuration")
    return {"apk": str(path), "sha256": hashlib.sha256(path.read_bytes()).hexdigest(),
            "package": debug_package, "size_bytes": path.stat().st_size}


def build(apk: Path, probe: Path, output: Path, package: str,
          apktool: str, zipalign: str, apksigner: str, keytool: str, aapt: str = "aapt") -> dict:
    # Read/validate everything we can before creating any output directory.
    if not apk.is_file() or not probe.is_file():
        raise ValueError("original APK and diagnostic libclient.so must exist")
    if output.exists():
        raise ValueError("output directory already exists; refusing to overwrite")
    with zipfile.ZipFile(apk) as archive:
        verify_original_dex(archive.read("classes.dex"))
    probe_hash = validate_arm64_probe(probe.read_bytes())
    for tool in (apktool, zipalign, apksigner, keytool, aapt):
        if shutil.which(tool) is None and not Path(tool).is_file():
            raise ValueError(f"required executable missing: {tool}")
    change_manifest('<manifest xmlns:android="%s" package="%s"><application/></manifest>' %
                    (ANDROID, ORIGINAL_PACKAGE), package)
    output.mkdir(parents=True, exist_ok=False)
    decoded = output / "decoded"
    cmd([apktool, "d", "--force", "--output", str(decoded), str(apk)])
    activity = find_activity(decoded)
    original_smali = activity.read_text(encoding="utf-8")
    changed_smali, summary = patch_smali(original_smali)
    activity.write_text(changed_smali, encoding="utf-8")
    (output / "MainActivity.original.smali.txt").write_text(original_smali, encoding="utf-8")
    manifest = decoded / "AndroidManifest.xml"
    manifest.write_text(
        change_manifest(manifest.read_text(encoding="utf-8"), package), encoding="utf-8")
    dest = decoded / "lib/arm64-v8a/libclient.so"
    dest.parent.mkdir(parents=True, exist_ok=True)
    if dest.exists():
        (output / "original_libclient.sha256.txt").write_text(
            hashlib.sha256(dest.read_bytes()).hexdigest())
    shutil.copyfile(probe, dest)
    unsigned, aligned = output / "unsigned.apk", output / "aligned.apk"
    signed = output / "sin-sourcevr-loader-probe.apk"
    cmd([apktool, "b", str(decoded), "--output", str(unsigned)])
    cmd([zipalign, "-f", "-p", "4", str(unsigned), str(aligned)])
    keystore = output / "ephemeral-debug.keystore"
    cmd([keytool, "-genkeypair", "-noprompt", "-keystore", str(keystore),
         "-storepass", "android", "-keypass", "android", "-alias", "sinprobe-debug",
         "-dname", "CN=SiN SourceVR Probe,O=Local Development", "-keyalg", "RSA",
         "-keysize", "2048", "-validity", "3650"])
    cmd([apksigner, "sign", "--ks", str(keystore), "--ks-key-alias", "sinprobe-debug",
         "--ks-pass", "pass:android", "--key-pass", "pass:android",
         "--out", str(signed), str(aligned)])
    cmd([apksigner, "verify", "--verbose", str(signed)])
    # Verify ACTUAL compiled manifest identity, not just our decoded XML.
    manifest_dump = subprocess.run([aapt, "dump", "badging", str(signed)],
                                   capture_output=True, text=True, check=True).stdout
    match = re.search(r"(?m)^package:\s+name='([^']+)'", manifest_dump)
    if not match or match.group(1) != package:
        raise ValueError("signed APK package mismatch; refusing unverified side-by-side identity")
    result = validate_signed_apk(signed, package)
    result.update({"source_apk_sha256": hashlib.sha256(apk.read_bytes()).hexdigest(),
                   "probe_sha256": probe_hash, "modlib_patch": summary,
                   "limitations": [
                       "No actual Android startup or native module selection tested",
                       "The diagnostic returns null from CreateInterface; no gameplay is possible",
                       "Changed package identity may affect content staging; inspect logcat",
                   ]})
    (output / "build-report.json").write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    return result


def main() -> int:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--apk", type=Path, required=True, help="Complete private original SourceVR APK")
    p.add_argument("--probe", type=Path, required=True, help="CI-built ARM64 diagnostic libclient.so")
    p.add_argument("--output", type=Path, required=True, help="New directory; must not already exist")
    p.add_argument("--package", default=DEFAULT_PACKAGE)
    p.add_argument("--apktool", default="apktool")
    p.add_argument("--zipalign", default="zipalign")
    p.add_argument("--apksigner", default="apksigner")
    p.add_argument("--keytool", default="keytool")
    p.add_argument("--aapt", default="aapt", help="Required to verify the signed Android package identity")
    p.add_argument("--build", action="store_true", help="Actually create a separate signed APK")
    args = p.parse_args()
    try:
        dex = get_source_dex(apk=args.apk, dex=None)
        digest = verify_original_dex(dex)
        probe_hash = validate_arm64_probe(args.probe.read_bytes())
        if not args.build:
            print(json.dumps({"status": "PLAN ONLY (no changes)",
                              "source_dex_sha256": digest,
                              "probe_sha256": probe_hash,
                              "package": args.package, "output": str(args.output),
                              "next": "Pass --build to create the private signed test APK"}, indent=2))
            return 0
        result = build(args.apk, args.probe, args.output, args.package,
                       args.apktool, args.zipalign, args.apksigner, args.keytool, args.aapt)
        print(json.dumps(result, indent=2))
        return 0
    except (OSError, ValueError, zipfile.BadZipFile, KeyError, subprocess.CalledProcessError) as exc:
        print(f"ISOLATED APK PRECHECK/BUILD FAILED: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
