#!/usr/bin/env python3
"""Private-archive proof of SourceVR ARM64 filesystem game-module loading.

This is a READ-ONLY bytecode/ELF audit of the owner's SourceVR 0.5.5 ZIP.
It does not load the proprietary libraries, modify an APK or expose its bytes.
"""
import argparse
import hashlib
import json
import struct
import zipfile
from pathlib import Path

from audit_sourcevr_native_game_factories import (
    AuditError, NativeELF, decode_add, decode_adrp, decode_bl, require,
)
from audit_sourcevr_engine_module_loader import ENGINE_WITNESSES

FILESYSTEM_SHA256 = "6b2fa7b745cd338ccfde04b6d3f152cbcf8c07cf1073ee424584d7296a5ad30e"
HL2_LAUNCHER_SHA256 = "84a9b779eabc5eef2e8ad33a217f0669d005990f085402b9a3766df66a64f18b"
FS_TYPEINFO = 0x993a0
FS_TYPE_NAME = 0x9f28
FS_VTABLE = 0x98e38
FS_LOADER = 0x2dd90
FS_LOAD_BY_NAME = 0x6efc0
FS_DLOPEN_WRAPPER = 0x6edc0


def qword(elf, address):
    return struct.unpack("<Q", elf.at(address, 8))[0]


def verify_text_argument(elf, address, register, text):
    va = decode_adrp(address, elf.word(address), register)
    va += decode_add(elf.word(address + 4), register)
    require(elf.at(va, len(text) + 1) == text.encode() + b"\x00",
            f"failed to trace {text} string at {address:#x}")
    return va


def imported_plt(elf, name):
    """Find an actual AArch64 jump-slot, not a coincidental imported name."""
    require(all(key in elf.sections for key in
                (".dynstr", ".dynsym", ".rela.plt", ".plt")),
            "required ELF import/PLT sections missing")
    dyn = elf.sections[".dynsym"]
    strings = elf.sections[".dynstr"]
    rela = elf.sections[".rela.plt"]
    plt = elf.sections[".plt"]
    require(dyn[9] == 24 and rela[9] == 24 and rela[5] % 24 == 0,
            "unexpected dynamic relocation layout")
    strtab = elf.data[strings[4]:strings[4] + strings[5]]
    found = []
    for index in range(rela[5] // 24):
        _, info, _ = struct.unpack_from("<QQq", elf.data, rela[4] + index*24)
        sym_index, rel_kind = info >> 32, info & 0xffffffff
        require((sym_index+1)*24 <= dyn[5], "bad import symbol index")
        offset = struct.unpack_from("<I", elf.data, dyn[4]+sym_index*24)[0]
        require(offset < len(strtab), "bad imported symbol string")
        end = strtab.find(b"\x00", offset)
        require(end >= 0, "unterminated imported symbol")
        if strtab[offset:end] == name.encode():
            require(rel_kind == 0x402, f"{name} is not R_AARCH64_JUMP_SLOT")
            found.append(plt[3] + 32 + index*16)
    require(len(found) == 1, f"expected one imported {name} PLT slot")
    require(plt[3] <= found[0] < plt[3] + plt[5],
            f"{name} PLT outside section")
    return found[0]


def verify_filesystem(elf):
    # Data RELR addends store base-relative image virtual addresses.
    # An Itanium ABI vtable points eight bytes after its RTTI pointer.
    require(qword(elf, FS_TYPEINFO+8) == FS_TYPE_NAME,
            "CBaseFileSystem RTTI name pointer changed")
    require(elf.at(FS_TYPE_NAME, 18) == b"15CBaseFileSystem\x00",
            "CBaseFileSystem RTTI class changed")
    require(qword(elf, FS_VTABLE-8) == FS_TYPEINFO,
            "vtable does not reference CBaseFileSystem RTTI")
    require(qword(elf, FS_VTABLE+0xc8) == FS_LOADER,
            "filesystem virtual slot +0xC8 changed")

    require(decode_bl(0x2dec8, elf.word(0x2dec8)) == FS_LOAD_BY_NAME,
            "filesystem virtual +0xC8 handler does not call module loader")
    require(decode_bl(0x6f108, elf.word(0x6f108)) == FS_DLOPEN_WRAPPER,
            "first candidate is no longer passed to dlopen wrapper")
    require(decode_bl(0x6f1d0, elf.word(0x6f1d0)) == FS_DLOPEN_WRAPPER,
            "fallback candidate no longer uses dlopen wrapper")
    loader_plt = imported_plt(elf, "dlopen")
    require(loader_plt == 0x94c30, "filesystem dlopen PLT changed")
    require(decode_bl(0x6ee2c, elf.word(0x6ee2c)) == loader_plt,
            "filesystem wrapper no longer calls real dlopen")

    require(verify_text_argument(elf, 0x6f0c0, 0, "APP_LIB_PATH") == 0x7cf8,
            "APP_LIB_PATH was not looked up")
    require(verify_text_argument(elf, 0x6f0d0, 0, "APP_MOD_LIB") == 0x8ae1,
            "APP_MOD_LIB was not looked up")
    getenv = imported_plt(elf, "getenv")
    for site in (0x6f0c8, 0x6f0d8):
        require(decode_bl(site, elf.word(site)) == getenv,
                "native filesystem no longer calls getenv")
    require(verify_text_argument(elf, 0x6f014, 1, ".so") == 0x92e9,
            "module extension normalization changed")
    require(verify_text_argument(elf, 0x6f1a0, 2, "lib%s") == 0x8fd5,
            "missing module lib-prefix formatting")
    require(verify_text_argument(elf, 0x6ee4, 2, "") if False else True,
            "unused")  # intentionally no invented register pattern
    # Verify exact code path performing a lib prefix test, before fallback.
    for site, expected in (
        (0x6f184, 0x794803e8),
        (0x6f188, 0x528d2d8a),  # compare first two bytes against 'li'
        (0x6f190, 0x39500be9),
        (0x6f194, 0x52800c48),  # third byte 'b'
        (0x6f19c, 0x54000160),  # if already "lib", skip prefix
    ):
        require(elf.word(site) == expected,
                f"native module filename handling changed at {site:#x}")
    return {
        "shared_filesystem_sha256": FILESYSTEM_SHA256,
        "class_rtti": "CBaseFileSystem",
        "class_typeinfo_va": hex(FS_TYPEINFO),
        "class_vtable_va": hex(FS_VTABLE),
        "class_module_load_virtual_slot_offset": "0xc8",
        "class_virtual_handler_va": hex(FS_LOADER),
        "path_selector_va": hex(FS_LOAD_BY_NAME),
        "dlopen_wrapper_va": hex(FS_DLOPEN_WRAPPER),
        "real_dlopen_plt_va": hex(loader_plt),
        "environment_variables": ["APP_MOD_LIB", "APP_LIB_PATH"],
        "module_filename_rules": ["append .so if missing", "prefix lib if missing"],
    }


def verify_launcher(elf):
    require(verify_text_argument(elf, 0x37308, 0, "APP_LIB_PATH") == 0xde53,
            "launcher APP_LIB_PATH bootstrap changed")
    require(verify_text_argument(elf, 0x37368, 0, "APP_LIB_PATH") == 0xde53,
            "launcher APP_LIB_PATH setenv changed")
    require(verify_text_argument(elf, 0x3737c, 0, "APP_MOD_LIB") == 0x1149e,
            "launcher APP_MOD_LIB setenv changed")
    getenv = imported_plt(elf, "getenv")
    setenv = imported_plt(elf, "setenv")
    dladdr = imported_plt(elf, "dladdr")
    require(decode_bl(0x37310, elf.word(0x37310)) == getenv,
            "launcher APP_LIB_PATH existence probe changed")
    require(elf.word(0x37314) == 0xb50003e0,
            "launcher no longer skips bootstrap when APP_LIB_PATH exists")
    require(decode_bl(0x37328, elf.word(0x37328)) == dladdr,
            "launcher no longer derives module directory using dladdr")
    for site in (0x37378, 0x3738c):
        require(decode_bl(site, elf.word(site)) == setenv,
                "launcher module path setenv changed")
    return {
        "hl2_launcher_sha256": HL2_LAUNCHER_SHA256,
        "default_path_derived_from": "dladdr(launcher function), directory",
        "bootstrap_env": ["APP_LIB_PATH", "APP_MOD_LIB"],
        "skips_fallback_if_APP_LIB_PATH_set": True,
        "launcher_path_setup_site": "0x37308",
    }


def audit(sourcevr_zip):
    base = "source vr/assets/sourcevr/v1/native/"
    with zipfile.ZipFile(sourcevr_zip) as archive:
        fs_path = (base + "blobs/" + FILESYSTEM_SHA256 +
                   "/libfilesystem_stdio.so")
        require(hashlib.sha256(archive.read(fs_path)).hexdigest()
                == FILESYSTEM_SHA256, "filesystem ELF bytes differ")
        fs_evidence = verify_filesystem(NativeELF(archive.read(fs_path)))

        for profile in ENGINE_WITNESSES:
            lines = archive.read(base + profile + ".manifest").decode().splitlines()
            selected = [row.split("\t") for row in lines
                        if "\tlibfilesystem_stdio.so\t" in row]
            require(len(selected) == 1 and selected[0][1] == FILESYSTEM_SHA256,
                    f"{profile}: filesystem SHA differs")
        launcher_path = base + "blobs/" + HL2_LAUNCHER_SHA256 + "/liblauncher.so"
        launcher = archive.read(launcher_path)
        require(hashlib.sha256(launcher).hexdigest() == HL2_LAUNCHER_SHA256,
                "HL2 launcher ELF bytes differ")
        launcher_evidence = verify_launcher(NativeELF(launcher))
    return {
        "profiles_sharing_filesystem": list(ENGINE_WITNESSES),
        "filesystem": fs_evidence, "launcher": launcher_evidence,
        "integration_note": (
            "The engine calls a virtual loader slot +0xC8 for GAMEBIN "
            "and the shared CBaseFileSystem virtual table supplies a +0xC8 "
            "module-load implementation that reaches dlopen through "
            "APP_MOD_LIB/APP_LIB_PATH. Actual runtime object identity, "
            "game-profile authorization and compatible custom SiN "
            "ServerGameDLL010 / VClient017 implementations remain unproven."
        )
    }


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--sourcevr-zip", required=True, type=Path)
    args = p.parse_args()
    try:
        print(json.dumps(audit(args.sourcevr_zip), indent=2))
    except (AuditError, KeyError, OSError, zipfile.BadZipFile,
            struct.error, UnicodeDecodeError) as e:
        p.exit(2, f"SourceVR filesystem loader audit FAILED: {e}\n")


if __name__ == "__main__":
    main()
