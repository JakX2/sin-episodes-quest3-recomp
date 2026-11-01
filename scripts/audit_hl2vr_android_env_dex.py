#!/usr/bin/env python3
"""Static DEX references for HL2VR native library environment setup.

For privately held HL2VR 0.989 APK only. This does not execute DEX or establish
actual process environment, Android linker access, or external module loading.
"""
import argparse
import hashlib
import json
import struct
import zipfile
from pathlib import Path


class DexAuditError(ValueError):
    pass


def read_uleb(buf, pos):
    value = 0
    for shift in range(0, 35, 7):
        if pos >= len(buf):
            raise DexAuditError("truncated ULEB128")
        byte = buf[pos]
        pos += 1
        value |= (byte & 127) << shift
        if byte < 128:
            return value, pos
    raise DexAuditError("overlong ULEB128")


def dex_strings(buf):
    if len(buf) < 112 or not buf.startswith(b"dex\n"):
        raise DexAuditError("not a DEX file")
    count, offset = struct.unpack_from("<II", buf, 56)
    if count > 200000 or offset + count * 4 > len(buf):
        raise DexAuditError("invalid DEX string table")
    strings = []
    for i in range(count):
        pos = struct.unpack_from("<I", buf, offset + i * 4)[0]
        if pos >= len(buf):
            raise DexAuditError("invalid DEX string data")
        _, pos = read_uleb(buf, pos)
        end = buf.find(b"\0", pos)
        if end < 0:
            raise DexAuditError("unterminated DEX string")
        strings.append(buf[pos:end].decode("utf-8", "replace"))
    return strings


def audit(dex):
    expected_hash = "7b4f4d909cb1858c30bd42c07c44962b604776653a24d623d48236e28cc42b95"
    if hashlib.sha256(dex).hexdigest() != expected_hash:
        raise DexAuditError("HL2VR 0.989 classes.dex SHA-256 mismatch")
    strings = dex_strings(dex)
    names = ("APP_DATA_PATH", "APP_LIB_PATH", "APP_MOD_LIB", "nativeLibraryDir")
    if any(strings.count(name) != 1 for name in names):
        raise DexAuditError("expected unique environment/path strings")
    positions = {}
    for name in names[:3]:
        index = strings.index(name)
        op = b"\x1a\x07" + struct.pack("<H", index)
        matches = [pos for pos in range(0, len(dex) - 4, 2) if dex.startswith(op, pos)]
        if len(matches) != 1:
            raise DexAuditError("unexpected DEX const-string count: " + name)
        positions[name] = matches[0]
    if positions != {"APP_DATA_PATH": 0xd2fc, "APP_LIB_PATH": 0xd30a, "APP_MOD_LIB": 0xd218}:
        raise DexAuditError("native env setup callsites changed")
    # APP_LIB_PATH's const-string is followed by iget-object and invoke-static
    # of the native library directory selector, then invoke-static setenv.
    lib = positions["APP_LIB_PATH"]
    if dex[lib + 4] != 0x54 or dex[lib + 8] != 0x71 or dex[lib + 16] != 0x71:
        raise DexAuditError("APP_LIB_PATH flow signature changed")
    return {
        "build": "HL2VR-Standalone-0.989",
        "classes_dex_sha256_verified": True,
        "native_initialization_method": "initNatives",
        "APP_DATA_PATH_reference": hex(positions["APP_DATA_PATH"]),
        "APP_LIB_PATH_reference": hex(lib),
        "APP_MOD_LIB_reference": hex(positions["APP_MOD_LIB"]),
        "APP_LIB_PATH_source": "selectedNativeLibraryDirectory() return in observed bytecode",
        "APP_MOD_LIB_source": "separate conditional path, exact runtime value unverified",
        "runtime_paths_verified": False,
        "external_native_library_loaded": False,
        "quest_device_tested": False
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("apk", type=Path)
    args = parser.parse_args()
    try:
        with zipfile.ZipFile(args.apk) as archive:
            result = audit(archive.read("classes.dex"))
    except (OSError, KeyError, ValueError, zipfile.BadZipFile) as error:
        parser.exit(1, "DEX AUDIT FAILED: " + str(error) + "\n")
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
