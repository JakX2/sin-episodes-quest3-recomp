#!/usr/bin/env python3
"""Read-only AArch64 ELF provenance and SourceVR loader callsite verifier.

Checks owner-supplied binaries against known SHA-256 and ARM64 ADRP/ADD/branch
instructions. No copyrighted binaries, gamecode, or Source interfaces included.
No claim of a working SiN, custom game module, Android runtime or headset.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys

PROFILES = {
    "hl2vr-vulkan-0989": {
        "sha256": "d8ecc43d92ac5cd30f1111b0e568eab839858e5626e79c74ebba629d2e13e94a",
        "warning_adrp": 0x61e868,
        "strings": ("HL2QUEST_GAME", "GAMEBIN", "VClient017", "ClientVirtualReality001",
                    "ClientRenderTargets001", "ServerGameDLL%03d", "server_episodic.so",
                    "server_portal.so", "server_portal2.so", "client_episodic",
                    "client_portal", "client_portal2"),
    },
    "hl2vr-turbo-0989": {
        "sha256": "43ee3b08e5979ea2e923acbaefab04ed252603561e6323450c15079a27d0159a",
        "warning_adrp": 0x61e970,
        "strings": ("HL2QUEST_GAME", "GAMEBIN", "VClient017", "ClientVirtualReality001",
                    "ClientRenderTargets001", "ServerGameDLL%03d", "server_episodic.so",
                    "server_portal.so", "server_portal2.so", "client_episodic",
                    "client_portal", "client_portal2"),
    },
    "portalvr-112": {
        "sha256": "82891887a5911363fb4aacb62fbb8ad9678e8a1ced3d35ed476609552555255d",
        "warning_adrp": 0x60ee80,
        "strings": ("VClient017", "ClientVirtualReality001",
                    "ClientRenderTargets001", "ServerGameDLL%03d"),
    },
}
WARNING = "client.dll is not VR-compatible.\n"


class VerificationError(ValueError):
    pass


def sections(blob):
    if len(blob) < 64 or blob[:4] != b"\x7fELF" or blob[4:6] != b"\x02\x01":
        raise VerificationError("expected ELF64 little-endian")
    if struct.unpack_from("<H", blob, 18)[0] != 183:
        raise VerificationError("expected AArch64 ELF")
    table = struct.unpack_from("<Q", blob, 40)[0]
    entrysz, count, stridx = struct.unpack_from("<HHH", blob, 58)
    if count == 0 or entrysz < 64 or stridx >= count or table + entrysz * count > len(blob):
        raise VerificationError("unsupported or truncated section table")
    def parse(n):
        return struct.unpack_from("<IIQQQQIIQQ", blob, table + n * entrysz)
    shstrings = parse(stridx)
    off, sz = shstrings[4], shstrings[5]
    if off + sz > len(blob):
        raise VerificationError("invalid section-string table")
    names = blob[off:off + sz]
    result = {}
    for i in range(count):
        sh = parse(i)
        start = sh[0]
        if start >= len(names):
            continue
        end = names.find(b"\x00", start)
        if end < 0:
            continue
        name = names[start:end].decode("ascii", errors="replace")
        if sh[4] + sh[5] > len(blob) and sh[1] != 8:  # SHT_NOBITS
            raise VerificationError("truncated section: " + name)
        result[name] = (sh[3], sh[4], sh[5])
    if ".rodata" not in result or ".text" not in result:
        raise VerificationError("missing .rodata or .text")
    return result


def signed(value, width):
    return value - (1 << width) if value & (1 << (width - 1)) else value


def adrp_page(opcode, pc):
    if opcode & 0x9f000000 != 0x90000000:
        return None
    imm = (((opcode >> 5) & 0x7ffff) << 2) | ((opcode >> 29) & 3)
    return (pc & ~0xfff) + (signed(imm, 21) << 12)


def add_immediate(opcode):
    if opcode & 0xffc00000 != 0x91000000:
        return None
    return ((opcode >> 5) & 31, opcode & 31, (opcode >> 10) & 0xfff)


def branch_target(opcode, pc):
    if opcode & 0x7c000000 != 0x14000000:
        return None
    return pc + (signed(opcode & 0x03ffffff, 26) << 2)


def conditional_nz_target(opcode, pc):
    if opcode & 0xff00001f != 0xb5000000:
        return None
    return pc + (signed((opcode >> 5) & 0x7ffff, 19) << 2)


def find_exact(blob, address, name):
    encoded = name.encode("ascii") + b"\x00"
    pos = blob.find(encoded)
    if pos < 0:
        raise VerificationError("missing string: " + name)
    if blob.find(encoded, pos + 1) >= 0:
        raise VerificationError("nonunique string: " + name)
    return address + pos


def xrefs(blob, textaddr, rodata, roaddr, string):
    target = find_exact(rodata, roaddr, string)
    found = []
    for offset in range(0, len(blob) - 4, 4):
        op = struct.unpack_from("<I", blob, offset)[0]
        pc = textaddr + offset
        if adrp_page(op, pc) != target & ~0xfff:
            continue
        register = op & 31
        for index in range(1, 7):
            if offset + index * 4 + 4 > len(blob):
                break
            second = struct.unpack_from("<I", blob, offset + index * 4)[0]
            fields = add_immediate(second)
            if fields and fields[0] == register and fields[2] + (target & ~0xfff) == target:
                found.append((pc, pc + index * 4))
                break
    return found


def verify_bytes(blob, profile):
    info = PROFILES[profile]
    if hashlib.sha256(blob).hexdigest() != info["sha256"]:
        raise VerificationError("SHA-256 mismatch for " + profile)
    mapping = sections(blob)
    textaddr, textoff, textlen = mapping[".text"]
    roaddr, rooff, rolen = mapping[".rodata"]
    code = blob[textoff:textoff + textlen]
    rodata = blob[rooff:rooff + rolen]
    expected = info["warning_adrp"]
    refs = xrefs(code, textaddr, rodata, roaddr, WARNING)
    if not any(pc == expected and add == expected + 4 for pc, add in refs):
        raise VerificationError("VR warning ADRP/ADD callsite did not match pinned executable")
    def op(pc):
        index = pc - textaddr
        if index < 0 or index + 4 > len(code):
            raise VerificationError("branch instruction outside .text")
        return struct.unpack_from("<I", code, index)[0]
    # Null VR factory result logs the warning; the success and warning paths merge.
    warning_target = branch_target(op(expected + 12), expected + 12)
    success_target = conditional_nz_target(op(expected - 4), expected - 4)
    if warning_target is None or warning_target != success_target:
        raise VerificationError("VR warning no longer reconverges with non-null factory path")
    refs_for = {}
    for name in info["strings"]:
        hits = xrefs(code, textaddr, rodata, roaddr, name)
        if not hits:
            raise VerificationError("missing direct string reference: " + name)
        refs_for[name] = [hex(pc) for pc, _ in hits[:5]]
    return {
        "profile": profile, "sha256_verified": True,
        "engine_elf_arch": "AArch64", "warning_callsite": hex(expected),
        "warning_paths_reconverge": True,
        "warning_continuation": hex(warning_target),
        "referenced_interface_and_module_strings": refs_for,
        "interpretation": "Static instruction-level loader evidence only",
        "sourcevr_module_loading_verified": False,
        "sin_quest_runtime_verified": False,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--profile", required=True, choices=PROFILES.keys())
    parser.add_argument("--engine", required=True, type=Path)
    args = parser.parse_args()
    try:
        output = verify_bytes(args.engine.read_bytes(), args.profile)
    except (OSError, VerificationError) as exc:
        print(json.dumps({"verified": False, "error": str(exc)}))
        return 1
    print(json.dumps(output, indent=2, sort_keys=True))
    return 0


if __name__ == "__main__":
    sys.exit(main())
