#!/usr/bin/env python3
"""Version-pinned, read-only DEX startup environment contract audit.

Uses the user's private SourceVR classes.dex. No APK modification or extraction.
The instruction checks are narrowly pinned; this is not a full DEX disassembler.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys

EXPECTED_SHA256 = "0dea2c61aa36557f49d125bd91607228e522fb25dbe023c1867b54d7ae7ec74a"
MAIN = "Lcom/sourcevrport/hl2vr/MainActivity;"
LAUNCHER = "Lcom/sourcevrport/hl2vr/LauncherActivity;"
PROFILES = "hl2,portal,portal2,ep1,ep2,lostcoast,entropyzero"


def decode_uleb(data, position):
    result = 0
    for index in range(5):
        if position >= len(data):
            raise ValueError("truncated DEX ULEB128")
        value = data[position]
        position += 1
        result |= (value & 127) << (index * 7)
        if value < 128:
            return result, position
    raise ValueError("invalid DEX ULEB128")


class Dex:
    def __init__(self, data):
        self.data = data
        if len(data) < 112 or data[:4] != b"dex\n" or data[7] != 0:
            raise ValueError("invalid DEX file")
        if struct.unpack_from("<I", data, 32)[0] != len(data):
            raise ValueError("DEX file size disagrees with header")
        self.strings = []
        string_count, string_off = self.table(56, 4)
        for i in range(string_count):
            (off,) = struct.unpack_from("<I", data, string_off + i * 4)
            _, start = decode_uleb(data, off)
            end = data.find(b"\0", start)
            if end < 0:
                raise ValueError("unterminated DEX string")
            self.strings.append(data[start:end].decode("utf-8", "replace"))
        type_count, type_off = self.table(64, 4)
        self.types = [self.strings[struct.unpack_from("<I", data, type_off + i*4)[0]]
                      for i in range(type_count)]
        field_count, field_off = self.table(80, 8)
        self.fields = [self.field_name(*struct.unpack_from("<HHI", data, field_off + i*8))
                       for i in range(field_count)]
        method_count, method_off = self.table(88, 8)
        self.methods = [self.method_name(*struct.unpack_from("<HHI", data, method_off + i*8))
                        for i in range(method_count)]
        class_count, class_off = self.table(96, 32)
        self.code = {}
        for i in range(class_count):
            row = struct.unpack_from("<IIIIIIII", data, class_off + i*32)
            if not row[6]:
                continue
            p = row[6]
            counts = []
            for _ in range(4):
                n, p = decode_uleb(data, p)
                counts.append(n)
            for _ in range(counts[0] + counts[1]):
                _, p = decode_uleb(data, p)
                _, p = decode_uleb(data, p)
            for count in counts[2:]:
                method_id = 0
                for _ in range(count):
                    delta, p = decode_uleb(data, p)
                    _, p = decode_uleb(data, p)
                    offset, p = decode_uleb(data, p)
                    method_id += delta
                    if offset:
                        if offset + 16 > len(data) or method_id >= len(self.methods):
                            raise ValueError("out-of-range DEX method")
                        instructions = struct.unpack_from("<I", data, offset + 12)[0]
                        if offset + 16 + instructions * 2 > len(data):
                            raise ValueError("out-of-range DEX code")
                        self.code[self.methods[method_id]] = list(
                            struct.unpack_from("<" + "H" * instructions, data, offset + 16))

    def table(self, header_offset, stride):
        count, offset = struct.unpack_from("<II", self.data, header_offset)
        if offset + count * stride > len(self.data):
            raise ValueError("invalid DEX table")
        return count, offset

    def field_name(self, cls, typ, name):
        return self.types[cls] + "->" + self.strings[name]

    def method_name(self, cls, proto, name):
        return self.types[cls] + "->" + self.strings[name]

    def method(self, name):
        try:
            return self.code[name]
        except KeyError:
            raise ValueError(f"missing required DEX method: {name}") from None


def require(c, explanation):
    if not c:
        raise ValueError("unsupported bytecode layout: " + explanation)


def audit(data, enforce_sha=True):
    digest = hashlib.sha256(data).hexdigest()
    if enforce_sha and digest != EXPECTED_SHA256:
        raise ValueError(f"unsupported SourceVR classes.dex SHA256: {digest}")
    dex = Dex(data)
    setup = dex.method(MAIN + "->setupEngineEnvironment")
    setter = dex.method(MAIN + "->setenv")
    profiles = dex.method(LAUNCHER + "->supportedProfiles")
    on_create = dex.method(MAIN + "->onCreate")
    require(len(setup) > 46 and len(setter) > 3 and len(profiles) > 92,
            "startup method too short")
    require(len(on_create) > 201 and (on_create[171] & 255) == 0x71 and
            dex.methods[on_create[172]] == MAIN + "->setupEngineEnvironment" and
            (on_create[200] & 255) == 0x6f and
            dex.methods[on_create[201]] == "Landroid/app/NativeActivity;->onCreate",
            "expected environment setup before NativeActivity.onCreate")
    require((setup[40] & 255) == 0x54, "expected iget-object before APP_LIB_PATH")
    require(dex.fields[setup[41]] ==
            "Landroid/content/pm/ApplicationInfo;->nativeLibraryDir",
            "expected ApplicationInfo.nativeLibraryDir field")
    require((setup[42] & 255) == 0x1a and
            dex.strings[setup[43]] == "APP_LIB_PATH",
            "expected APP_LIB_PATH const-string at dex PC 42")
    require((setup[44] & 255) == 0x71 and
            dex.methods[setup[45]] == MAIN + "->setenv",
            "expected MainActivity.setenv invocation")
    require((setter[1] & 255) == 0x71 and
            dex.methods[setter[2]] == "Landroid/system/Os;->setenv",
            "expected android.system.Os.setenv in wrapper")
    require((profiles[91] & 255) == 0x1a and
            dex.strings[profiles[92]] ==
            "Invalid SOURCEVR_SUPPORTED_PROFILES: " + PROFILES,
            "expected fixed supported profile diagnostics")
    require(PROFILES in dex.strings, "missing fixed supported profile list")

    return {
        "schema": 1,
        "dex_sha256": digest,
        "source": "user-supplied SourceVR APK classes.dex",
        "startup_method": MAIN + "->setupEngineEnvironment",
        "on_create_order": {
            "setupEngineEnvironment_pc": 171,
            "NativeActivity_onCreate_pc": 200,
            "interpretation": "normal startup code orders env initialization before native activity initialization",
        },
        "startup_pc": {
            "nativeLibraryDir_iget": 40,
            "APP_LIB_PATH_const_string": 42,
            "setenv_call": 44,
        },
        "setter": MAIN + "->setenv -> Landroid/system/Os;->setenv",
        "app_mod_lib_literal_in_dex": "APP_MOD_LIB" in dex.strings,
        "profile_method": LAUNCHER + "->supportedProfiles",
        "supported_profiles": PROFILES.split(","),
        "meaning": (
            "Java supplies APP_LIB_PATH from ApplicationInfo.nativeLibraryDir "
            "via android.system.Os.setenv prior to the native engine. "
            "Java contains no APP_MOD_LIB literal in this DEX. "
            "Does not establish whether an external override exists."
        ),
        "next_gate": "An isolated variant's launch environment must be controllable before testing native module selection.",
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("dex", type=Path)
    parser.add_argument("--output", type=Path)
    options = parser.parse_args()
    try:
        report = audit(options.dex.read_bytes())
    except (ValueError, OSError, IndexError, struct.error) as error:
        print(f"SourceVR DEX audit failed: {error}", file=sys.stderr)
        return 1
    formatted = json.dumps(report, indent=2, sort_keys=True) + "\n"
    if options.output:
        options.output.write_text(formatted, encoding="utf-8")
        print(f"Wrote {options.output}")
    else:
        print(formatted)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
