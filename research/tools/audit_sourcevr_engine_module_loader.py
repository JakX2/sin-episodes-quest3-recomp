#!/usr/bin/env python3
"""Read-only proof of seven SourceVR ARM64 engine client/server loader paths.

Input is the user's PRIVATE, joined SourceVR 0.5.5 multipart ZIP. It is
never copied or embedded in reports/CI. Only SHA-pinned ELF metadata and
exact AArch64 code witnesses are emitted.
"""
import argparse
import hashlib
import json
import re
import struct
import zipfile
from pathlib import Path

from audit_sourcevr_native_game_factories import (
    AuditError, NativeELF, ORIGINAL, PROFILE_SHAS,
    decode_add, decode_adrp, decode_bl, require,
)

# Real native libengine.so for each supplied game profile, NOT interchangeable.
# (sha256, server_format_ADRP_VA, client_factory_ADRP_VA, dlsym_helper_VA)
ENGINE_WITNESSES = {
    "hl2": (
        "265c675d6f0d7a57a841bf6a0daf1bb7708a3c4c1216d800299f6d448ce75830",
        0x2f395c, 0x327ff0, 0x53f5c0,
    ),
    "ep1": (
        "8891aced91eab526f1c9c2d6d74c808bb7ce7db5ef8c59670c8a1b7d1bbd19d5",
        0x2f395c, 0x327ff0, 0x53f5c0,
    ),
    "ep2": (
        "f3acb5b5461ce59f4d5b5b7124ce36d160768de2e4ac1b2f8fd0d3bbf7cab8f1",
        0x2f395c, 0x327ff0, 0x53f5c0,
    ),
    "lostcoast": (
        "0ad9c190944389487571c62240577c5505619221c5e339e8dba19bb0a21a4ed6",
        0x2f395c, 0x327ff0, 0x53f5c0,
    ),
    "portal": (
        "fb0daa545f48fe2ae721b8ef28188da389f6c153c37e22c02bcd26c2a98a8e83",
        0x2f313c, 0x3277d0, 0x53e1a0,
    ),
    "portal2": (
        "e675c7dac606fba362805c063146f405f1346b1b1e1f2d0f9d0c3dc5feeca67d",
        0x2f6f7c, 0x32b610, 0x5503c0,
    ),
    "entropyzero": (
        "81c6041998d4dcace9739bae16ebfcff62d3a66f8079de59872a103c7f5f7cee",
        0x2f395c, 0x327ff0, 0x53f5c0,
    ),
}


def expect_word(elf, address, word, description):
    require(elf.word(address) == word,
            f"{description} changed at {address:#x}")


def string_pair(elf, address, register, expected):
    va = (decode_adrp(address, elf.word(address), register)
          + decode_add(elf.word(address + 4), register))
    require(elf.at(va, len(expected) + 1) == expected.encode("ascii") + b"\x00",
            f"{expected!r} is not referenced from x{register} at {address:#x}")
    return va


def decode_b(address, instruction):
    require(instruction & 0xfc000000 == 0x14000000,
            f"unconditional B changed at {address:#x}")
    relative = instruction & 0x3ffffff
    if relative & (1 << 25):
        relative -= 1 << 26
    return address + relative * 4


def dlsym_plt_va(elf):
    """Derive PLT slot from actual AArch64 R_JUMP_SLOT relocation, not guesses."""
    for name in (".dynsym", ".dynstr", ".rela.plt", ".plt"):
        require(name in elf.sections, f"ELF {name} missing")
    dynsym, dynstr, reloc, plt = (
        elf.sections[key] for key in (".dynsym", ".dynstr", ".rela.plt", ".plt"))
    d = elf.data
    require(dynsym[9] == 24 and reloc[9] == 24 and reloc[5] % 24 == 0,
            "unexpected ELF dynsym or PLT relocation entry size")
    strings = d[dynstr[4]:dynstr[4]+dynstr[5]]
    slot = None
    for n in range(reloc[5] // 24):
        r_offset, r_info, _ = struct.unpack_from("<QQq", d, reloc[4] + n * 24)
        symbol = r_info >> 32
        r_type = r_info & 0xffffffff
        require(symbol * 24 + 24 <= dynsym[5], "PLT relocation symbol OOB")
        name_offset = struct.unpack_from("<I", d, dynsym[4] + symbol * 24)[0]
        require(name_offset < len(strings), "ELF dynamic string offset OOB")
        end = strings.find(b"\x00", name_offset)
        require(end >= 0, "unterminated ELF dynamic symbol")
        symname = strings[name_offset:end]
        if symname == b"dlsym":
            require(r_type == 0x402, "dlsym is not R_AARCH64_JUMP_SLOT")
            require(slot is None, "multiple dlsym PLT slots")
            slot = n
    require(slot is not None, "no dynamically imported dlsym in engine")
    va = plt[3] + 32 + slot * 16  # AArch64: 32-byte PLT0, 16-byte stubs
    require(plt[3] <= va < plt[3] + plt[5], "dlsym PLT slot out of bounds")
    return va


def verify_engine(elf, server, client, helper):
    # Server module selected by engine virtual LoadModule-like slot +0xC8.
    # No statement about the precise inherited virtual METHOD NAME.
    server_file = string_pair(elf, server - 0x50, 1, "server.so")
    server_search = string_pair(elf, server - 0x48, 2, "GAMEBIN")
    expect_word(elf, server - 0x40, 0xf9400008, "server loader vtable read")
    expect_word(elf, server - 0x38, 0xf9406508, "server loader +0xC8 slot")
    expect_word(elf, server - 0x34, 0xd63f0100, "server indirect load")
    require(decode_bl(server - 0x1c, elf.word(server - 0x1c)) == helper,
            "server factory extraction helper call differs")
    expect_word(elf, server + 0x24, 0xd63f0100,
                "server game factory invocation")
    version_string = string_pair(elf, server, 1, "ServerGameDLL%03d")
    expect_word(elf, server - 4, 0x52800148, "first server DLL version 10")
    expect_word(elf, server + 0x48, 0x51000502, "server version decrement")
    expect_word(elf, server + 0x50, 0x7100111f, "minimum server version check 4")
    expect_word(elf, server + 0x54, 0x5400066d, "server <=4 branch")
    require(server + 0x54 + ((elf.word(server + 0x54) >> 5) & 0x7ffff) * 4 ==
            server + 0x120, "server stop branch no longer excludes version 003")
    expect_word(elf, server + 0x70, 0xd63f0100, "server retry factory invocation")

    # Client path loads another name, but uses the SAME file search group,
    # loader vtable slot and dynamically extracted CreateInterface callback.
    client_file = string_pair(elf, client - 0x48, 1, "client")
    client_search = string_pair(elf, client - 0x3c, 2, "GAMEBIN")
    require(client_search == server_search, "server/client GAMEBIN differs")
    expect_word(elf, client - 0x30, 0xf9400100, "client loader object read")
    expect_word(elf, client - 0x2c, 0xf9400008, "client loader vtable read")
    expect_word(elf, client - 0x28, 0xf9406508, "client loader +0xC8 slot")
    expect_word(elf, client - 0x24, 0xd63f0100, "client indirect load")
    require(decode_bl(client - 0x10, elf.word(client - 0x10)) == helper,
            "client factory extraction helper call differs")
    client_version = string_pair(elf, client, 0, "VClient017")
    expect_word(elf, client + 0xc, 0xd63f0100,
                "client game factory invocation")

    # Same helper converts the loader's module handle into its exported
    # CreateInterface pointer by resolving actual dlsym@PLT relocation.
    expect_word(elf, helper, 0xb4000080, "factory helper handle guard")
    ci_symbol = string_pair(elf, helper + 4, 1, "CreateInterface")
    target = decode_b(helper + 12, elf.word(helper + 12))
    plt = dlsym_plt_va(elf)
    require(target == plt, "factory resolver not routed through dlsym PLT")

    return {
        "server_module": "server.so",
        "client_module": "client",
        "search_path": "GAMEBIN",
        "server_version_first": 10,
        "server_version_last": 4,
        "client_factory_requested": "VClient017",
        "server_load_site": f"0x{server-0x40:x}",
        "client_load_site": f"0x{client-0x30:x}",
        "server_version_site": f"0x{server:x}",
        "client_version_site": f"0x{client:x}",
        "module_factory_dlsym_helper": f"0x{helper:x}",
        "dlsym_plt_va": f"0x{plt:x}",
        "server_name_va": f"0x{server_file:x}",
        "client_name_va": f"0x{client_file:x}",
        "gamebin_va": f"0x{server_search:x}",
        "createinterface_symbol_va": f"0x{ci_symbol:x}",
        "server_version_format_va": f"0x{version_string:x}",
        "client_version_va": f"0x{client_version:x}",
    }


def audit(sourcevr_zip, sin_bin_zip=None):
    records = []
    with zipfile.ZipFile(sourcevr_zip, "r") as archive:
        for profile, (expected_sha, server, client, helper) in ENGINE_WITNESSES.items():
            manifest_name = f"source vr/assets/sourcevr/v1/native/{profile}.manifest"
            manifest = archive.read(manifest_name).decode("utf-8")
            rows = [
                line.split("\t") for line in manifest.splitlines()
                if line and len(line.split("\t")) >= 5
                and line.split("\t")[2] in ("libengine.so", "libserver.so", "libclient.so")
            ]
            selected = {}
            for cells in rows:
                kind = cells[2]
                require(kind not in selected, f"{profile}: duplicate {kind}")
                require(re.fullmatch(r"[0-9a-f]{64}", cells[1]) is not None,
                        "unexpected manifest module hash")
                path = "source vr/assets/" + cells[4]
                require(path.endswith("/" + kind), "unexpected module path")
                selected[kind] = (cells[1], path)
            require(set(selected) == {"libengine.so", "libserver.so", "libclient.so"},
                    f"{profile}: missing one native game module")
            require(selected["libengine.so"][0] == expected_sha,
                    f"{profile}: changed libengine.so manifest SHA")
            actual_game = PROFILE_SHAS[profile]
            require(selected["libserver.so"][0] == actual_game[0] and
                    selected["libclient.so"][0] == actual_game[1],
                    f"{profile}: client/server pair not matching pinned native game profile")
            payload = archive.read(selected["libengine.so"][1])
            require(hashlib.sha256(payload).hexdigest() == expected_sha,
                    f"{profile}: actual engine binary SHA256 mismatch")
            evidence = verify_engine(NativeELF(payload), server, client, helper)
            records.append({"profile": profile, "engine_sha256": expected_sha,
                            "server_sha256": actual_game[0],
                            "client_sha256": actual_game[1], **evidence})
    original_verified = False
    if sin_bin_zip is not None:
        with zipfile.ZipFile(sin_bin_zip, "r") as archive:
            for dll, (sha, name) in ORIGINAL.items():
                payload = archive.read(dll)
                require(hashlib.sha256(payload).hexdigest() == sha,
                        f"original {dll} SHA256 mismatch")
                require(name.encode() + b"\x00" in payload,
                        f"original {dll} factory string missing")
        original_verified = True
    return {
        "original_sin_factories": {name: info[1] for name, info in ORIGINAL.items()},
        "original_binary_hashes_verified": original_verified,
        "sourcevr_engine_profiles": records,
        "scope": (
            "Verified selected native libengine.so branch paths and exact C++ "
            "virtual slot +0xC8 used to request game module by GAMEBIN group, "
            "actual dlsym(CreateInterface) PLT, server 010..004 negotiation "
            "and client VClient017 lookup. Not proof of custom-module installation, "
            "virtual slot method names, SourceVR game-object ABI, or Quest execution."
        )
    }


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--sourcevr-zip", type=Path, required=True,
                   help="Private rejoined multipart SourceVR ZIP")
    p.add_argument("--sin-bin-zip", type=Path,
                   help="Optional original user-owned SiN 2006 bin.zip")
    a = p.parse_args()
    try:
        print(json.dumps(audit(a.sourcevr_zip, a.sin_bin_zip), indent=2))
    except (AuditError, zipfile.BadZipFile, KeyError,
            OSError, struct.error, UnicodeDecodeError) as exc:
        p.exit(2, f"SourceVR native module-loader audit FAILED: {exc}\n")


if __name__ == "__main__":
    main()
