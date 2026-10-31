#!/usr/bin/env python3
"""Fresh, independent signature verifier for recovered SiN Phase 23–43 notes.

This is a 2026 backfill verifier, NOT an original missing phase ZIP/script.
Supply your own lawful 2006 SiN bin.zip. No game DLLs are distributed here.
"""
import argparse
import hashlib
import json
import struct
import zipfile
from pathlib import Path

SHA = {
    "server.dll": "568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650",
    "client.dll": "6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d",
}

SAMPLES = {
    "server.dll": {
        "rifle_alt_function": (0x22313C60, "83 ec 34 53 56 57"),
        "scatter_bullet_attack": (0x2231C450, "83 ec 7c 56 57"),
        "scatter_flak_attack": (0x2231C810, "81 ec 80 00 00 00"),
        "scatter_bullet_count_11": (0x2231C5A9, "c7 44 24 2c 0b 00 00 00"),
        "scatter_flak_count_8": (0x2231C9B4, "bf 08 00 00 00"),
        "flak_touch_callback_install": (0x2231CF88, "c7 46 7c 70 bd 31 22"),
        "ar_grenade_touch_install": (0x22314BAB, "c7 46 7c f0 42 31 22"),
        "magnum_secondary_function": (0x22316CF0, "81 ec 88 00 00 00"),
        "magnum_secondary_clip_check": (0x22316D08, "39 9e f0 04 00 00"),
        "duag_ammo_damage_bits": (0x222D566C, "68 02 00 40 00"),
        "duag_impact_mask_test": (0x22316927, "f7 c3 00 00 40 00"),
        "target_damage_filter_call": (0x2214D0AD, "ff 90 d8 00 00 00"),
        "target_trace_attack_call": (0x2214D0CD, "ff 90 dc 00 00 00"),
        "impact_equipped_weapon_forward": (0x22245EB8, "ff a2 98 01 00 00"),
        "zoom_enter_0_25_seconds": (0x222CBB88, "68 00 00 80 3e"),
        "zoom_exit_0_15_seconds": (0x222CBBE6, "68 9a 99 19 3e"),
        "shared_weapon_zoom_getter": (0x222AC030, "66 8b 81 1c 05 00 00"),
    },
    "client.dll": {
        "zoom_input_bit": (0x2416269D, "81 ce 00 00 08 00"),
        "quick_grenade_input_bit": (0x2416271B, "81 ce 00 00 00 02"),
        "zoom_plus_command": (0x2426B29B, "68 60 fb 29 24"),
        "grenade_plus_command": (0x2426B3BB, "68 a0 fb 29 24"),
        "zooming_recv_property": (0x2421AC80, "6a 01 6a 10 68 d4 5e 2b 24"),
        "zoom_scope_recv_property": (0x2421AC99, "6a 11 68 c4 5e 2b 24"),
    },
}


class PE:
    def __init__(self, data):
        offset, = struct.unpack_from("<I", data, 0x3C)
        if data[offset:offset + 4] != b"PE\x00\x00":
            raise ValueError("Not a PE image")
        self.data = data
        self.base, = struct.unpack_from("<I", data, offset + 24 + 28)
        section_count, = struct.unpack_from("<H", data, offset + 6)
        optional_size, = struct.unpack_from("<H", data, offset + 20)
        self.sections = []
        for index in range(section_count):
            h = offset + 24 + optional_size + 40 * index
            rva, raw_size, raw_offset = struct.unpack_from("<III", data, h + 12)
            self.sections.append((self.base + rva, raw_size, raw_offset))

    def read(self, va, count):
        for begin, length, offset in self.sections:
            if begin <= va and va + count <= begin + length:
                start = offset + va - begin
                return self.data[start:start + count]
        raise ValueError(f"No raw bytes for {va:#x}")

    def u32(self, va):
        return struct.unpack("<I", self.read(va, 4))[0]


def audit(zip_path):
    with zipfile.ZipFile(zip_path) as archive:
        binaries = {name: archive.read(name) for name in SHA}

    evidence = {"sha256": {}, "signatures": {}, "shared_zoom_getters": {}}
    for name, expected_hash in SHA.items():
        data = binaries[name]
        actual_hash = hashlib.sha256(data).hexdigest()
        if actual_hash != expected_hash:
            raise ValueError(f"Wrong 2006 {name}: {actual_hash}")
        evidence["sha256"][name] = actual_hash
        pe = PE(data)
        evidence["signatures"][name] = {}
        for label, (va, instruction) in SAMPLES[name].items():
            expected = bytes.fromhex(instruction)
            if pe.read(va, len(expected)) != expected:
                raise ValueError(f"{name}: {label} differs at {va:#x}")
            evidence["signatures"][name][label] = hex(va)

    server = PE(binaries["server.dll"])
    for name, vtable in {
        "CWeaponMagnum": 0x2247875C,
        "CWeaponAssaultRifle": 0x224776AC,
        "CWeaponScatterGun": 0x2247A1E4,
    }.items():
        address = server.u32(vtable + 272 * 4)
        if address != 0x222AC030:
            raise ValueError(f"{name}: unexpected ZoomFOV getter: {address:#x}")
        evidence["shared_zoom_getters"][name] = hex(address)
    return evidence


if __name__ == "__main__":
    args = argparse.ArgumentParser()
    args.add_argument("bin_zip", type=Path)
    args.add_argument("--output", type=Path)
    opt = args.parse_args()
    report = audit(opt.bin_zip)
    if opt.output:
        opt.output.parent.mkdir(parents=True, exist_ok=True)
        opt.output.write_text(json.dumps(report, indent=2) + "\n")
    checked = sum(len(group) for group in SAMPLES.values())
    print(f"PASS: two SHA-256s, {checked} fixed x86 signatures, "
          "three shared ZoomFOV virtual slots")
