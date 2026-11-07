#!/usr/bin/env python3
"""Verify REAL SourceVR 0.5.5 ARM64 game-server/client ABI object topology.

Read-only on the OWNER'S private rejoined SourceVR ZIP. Pins complete 47
and 78 slot vtables for all 11 distinct native game modules, each concrete
class RTTI -> base RTTI, singleton factory and constructor vptr assignment.
No engine methods are invoked and no third-party binaries are redistributed.
"""
import argparse
import hashlib
import json
import re
import struct
import zipfile
from pathlib import Path

from audit_sourcevr_native_game_factories import (
    AuditError, NativeELF, PROFILE_SHAS, WITNESSES, ORIGINAL,
    decode_add, decode_adrp, require, verify_registration,
)

# Full original SHA256 of each ELF is in keys. Value tuple:
# (vtable address, SHA256 of *all* native method pointers, constructor ADRP,
#  constructor STR, first virtual method, final virtual method).
SERVER_TABLES = {
"fdc8bb8777e044875609da75850fdf0a58155d5caf84b834a9d72486fe0a2322": (0xa11488, "b10e3eecbdfa5486c567b777ad04f57dd285bc7d5925562525105e08e4ccb03f",0x532a4c,0x532a5c,0x52cc20,0x52f100),
"a81b04bdb0164ee450891a153c055efd268562b7171181f1e83f401b64c7c226": (0xa94550, "6cac8b0b12dfa55c255eb591919bccc41c2e3d2b2064c8e98b21981430b88f4d",0x551f2c,0x551f3c,0x54c100,0x54e5e0),
"6c8b6df1db323288e3c49b635569006b6e3cfbfc7745b443c655d5ed4e31e8d8": (0x9c4818, "df1a0332e65d09d51b53f1d103b8a12c124aa60d7e51b0d933946408fd143760",0x44c6cc,0x44c6dc,0x4468a0,0x448d80),
"ecb111cb112f0388f075382fdf15cd8105c2472cac0a478aa073c31a2353f87e": (0xaf47f0, "347fa0fd215db01990dd6205a1be90095eacbb578ff5e2d0292f6517b4fd270b",0x4935ac,0x4935bc,0x48d6a0,0x48fc00),
"16e461d91ab54cc9491081363e9a801bb20e915e5c44a272a1de524e51b15c8a": (0xaa1990, "8947d253730cb517bf0aedf422d616c9020a05288e4be9727a5e8cfb85b2f2c4",0x556a8c,0x556a9c,0x550c60,0x553140),
}
CLIENT_TABLES = {
"0a8d1082e86d857cad812a65ca682695098de2498919931b08e87fb3b38bc362": (0x81fe10, "66502faf165c307a59d98924dc404d10a33806861ecc91b8ae933666ab7ebcbe",0x370808,0x370810,0x36c820,0x36f7c0),
"e4384399bd36ebeec04c678d9d757176935e4d7ca3d228bcded5536aabb50c37": (0x82d898, "df95a7d270c5434128cd6478b3c7febbb98c3e1dde190f11ead25e3fa4b24add",0x374588,0x374590,0x3705a0,0x373540),
"9ad8e9eff680ccd4a29f46d540f4d737bfb9467c3013b9ca93d123834f50c773": (0x82d898, "df95a7d270c5434128cd6478b3c7febbb98c3e1dde190f11ead25e3fa4b24add",0x374588,0x374590,0x3705a0,0x373540),
"f14083ceaeb46317ab9cd1cdf36e011a6d4013462bc7143403dcc8152ae53b9b": (0x7bdaa0, "33b846735b559c1808839673dd137ab72941240b04398b14555631a71d67db2e",0x2d1a68,0x2d1a70,0x2cdc40,0x2d0a20),
"0f555f09a2ab90c89501a8aa8c7a153ed2c7a5d6906816f8779e3ff3bf222ae2": (0x834c98, "91844061fcdc858b53afb4fb45b4de49851baa41f891c2ddf847fb7185d61331",0x2ec668,0x2ec670,0x2e8840,0x2eb620),
"720602b9643c4b2505ba9c7f6e5e0ac05fcb99270965a5b74b3c26d3e89ca1b0": (0x82ea28, "218cb0c308333a27e05083df755663eaf3fe8ec53ab16fabdfa5d76e81f6b89e",0x375568,0x375570,0x371580,0x374520),
}
TABLES = {"libserver.so": SERVER_TABLES, "libclient.so": CLIENT_TABLES}
CLASS = {
    "libserver.so": (b"14CServerGameDLL", b"14IServerGameDLL", 47),
    "libclient.so": (b"9CHLClient", b"14IBaseClientDLL", 78),
}


def pointer_refs(section_data, base_va, value):
    return [base_va + p for p in range(0, len(section_data) - 7, 8)
            if struct.unpack_from("<Q", section_data, p)[0] == value]


def method_table(elf, kind):
    class_name, base_name, count = CLASS[kind]
    ro = elf.sections[".rodata"]
    rel = elf.sections[".data.rel.ro"]
    text = elf.sections[".text"]
    strings = elf.data[ro[4]:ro[4] + ro[5]]
    native_data = elf.data[rel[4]:rel[4] + rel[5]]
    for name in (class_name, base_name):
        require(strings.count(name + b"\x00") == 1,
                f"native RTTI class {name!r} missing or ambiguous")
    class_name_va = ro[3] + strings.index(class_name + b"\x00")
    base_name_va = ro[3] + strings.index(base_name + b"\x00")
    concrete_refs = pointer_refs(native_data, rel[3], class_name_va)
    base_refs = pointer_refs(native_data, rel[3], base_name_va)
    require(len(concrete_refs) == 1 and len(base_refs) == 1,
            "native concrete/base RTTI references changed")
    concrete_typeinfo = concrete_refs[0] - 8
    base_typeinfo = base_refs[0] - 8
    require(struct.unpack("<Q", elf.at(concrete_typeinfo+16, 8))[0]
            == base_typeinfo, "concrete class RTTI no longer derives from expected base")
    candidates = []
    for ref in pointer_refs(native_data, rel[3], concrete_typeinfo):
        candidate = ref + 8
        if candidate < rel[3] + 16 or candidate + count * 8 + 8 > rel[3] + rel[5]:
            continue
        try:
            first = struct.unpack("<Q", elf.at(candidate, 8))[0]
        except AuditError:
            continue
        if text[3] <= first < text[3] + text[5]:
            candidates.append(candidate)
    require(len(candidates) == 1, "native concrete game-module vtable ambiguous")
    start = candidates[0]
    require(struct.unpack("<Q", elf.at(start-16, 8))[0] == 0,
            "unexpected secondary vtable offset-to-top")
    require(struct.unpack("<Q", elf.at(start-8, 8))[0] == concrete_typeinfo,
            "vtable not attached to expected class RTTI")
    raw = elf.at(start, count * 8)
    methods = struct.unpack("<%dQ" % count, raw)
    require(all(text[3] <= m < text[3] + text[5] for m in methods),
            "native game-method vtable has a non-executable slot")
    require(struct.unpack("<Q", elf.at(start+count*8, 8))[0] == 0,
            "native game method count or table terminator changed")
    return start, methods, hashlib.sha256(raw).hexdigest(), concrete_typeinfo


def verify_constructor_vptr(elf, vtable, singleton, site, store):
    instruction = elf.word(site)
    reg = instruction & 31
    require((instruction & 0x9f000000) == 0x90000000,
            "constructor no longer uses ADRP for native vtable")
    target = decode_adrp(site, instruction, reg)
    target += decode_add(elf.word(site+4), reg)
    require(target == vtable, "constructor installs a different game vtable")
    instruction = elf.word(store)
    require((instruction & 0xffc0001f) == (0xf9000000 | reg),
            "constructor no longer STR-writes the vtable pointer")
    base = (instruction >> 5) & 31
    offset = ((instruction >> 10) & 4095) * 8
    valid_base = False
    for before in range(store-40, store, 4):
        w = elf.word(before)
        if (w & 0x9f00001f) != (0x90000000 | base):
            continue
        address = decode_adrp(before, w, base)
        if before + 4 < store:
            try:
                address += decode_add(elf.word(before + 4), base)
            except AuditError:
                pass
        if address + offset == singleton:
            valid_base = True
    require(valid_base, "constructor vptr store does not target factory singleton")
    return {"constructor_vtable_adrp_va": hex(site),
            "constructor_vptr_store_va": hex(store),
            "factory_singleton_va": hex(singleton)}


def verify_module(elf, sha):
    require(sha in WITNESSES, "native game module is not in prior factory audit")
    kind = WITNESSES[sha][0]
    require(kind in TABLES and sha in TABLES[kind],
            "game-module vtable has no pinned witness")
    expected_va, expected_hash, constructor_site, store_site, first, last = TABLES[kind][sha]
    factory = verify_registration(elf, kind, *WITNESSES[sha][1:])
    vtable, pointers, table_hash, typeinfo = method_table(elf, kind)
    require(vtable == expected_va and table_hash == expected_hash,
            "game-module ABI vtable VA or full pointer hash changed")
    require(pointers[0] == first and pointers[-1] == last,
            "native game module method boundary changed")
    constructor = verify_constructor_vptr(
        elf, vtable, int(factory["singleton_va"], 16),
        constructor_site, store_site)
    return {
        "module": kind,
        "registered_factory": factory["registered_name"],
        "class_rtti": CLASS[kind][0].decode("ascii"),
        "base_rtti": CLASS[kind][1].decode("ascii"),
        "method_slots": len(pointers),
        "vtable_va": hex(vtable),
        "vtable_sha256": table_hash,
        "concrete_rtti_va": hex(typeinfo),
        "first_method_va": hex(pointers[0]),
        "last_method_va": hex(pointers[-1]),
        "method_pointer_size": 8,
        **constructor,
        "methods_callable_from_SiN_2006_without_adapter": False,
    }



# Independently verified original SiN Episodes 2006 PE32/MSVC RTTI.
# (TypeDescriptor.name, native CompleteObjectLocator VA, x86 vtable VA,
#  method count, full SHA256 of *every* 32-bit virtual function pointer,
#  first method VA, last method VA).
ORIGINAL_VTABLES = {
    "server.dll": (
        b".?AVCServerGameDLL@@", 0x224c51e8, 0x22442efc, 31,
        "97e42f1905b28e6aad0e3de21473d12725c49b25980be4a5db285333774a8299",
        0x221e7640, 0x221e7cf0,
    ),
    "client.dll": (
        b".?AVCHLClient@@", 0x242e5818, 0x24296c6c, 48,
        "9d28eccf91315f7a6ecc2394386d7b4077919d8649d34297a951eb7a1cafa664",
        0x240fe0e0, 0x240fe670,
    ),
}

def inspect_original_pe_game_table(blob, name):
    """Read-only PE32 RTTI COL -> primary vftable, no DLL execution."""
    require(name in ORIGINAL_VTABLES, "unknown original SiN game module")
    needle, col_va, expected_vtable, count, digest, first, last = ORIGINAL_VTABLES[name]
    require(hashlib.sha256(blob).hexdigest() == ORIGINAL[name][0],
            "owner's original x86 game DLL SHA256 changed")
    require(blob[:2] == b"MZ", "original game module is not PE")
    require(len(blob) >= 0x100, "truncated PE DOS header")
    header = struct.unpack_from("<I", blob, 0x3c)[0]
    require(header+24 < len(blob) and blob[header:header+4] == b"PE\x00\x00",
            "original PE signature missing")
    require(struct.unpack_from("<H",blob,header+4)[0] == 0x14c,
            "original PE must be x86")
    num_sections = struct.unpack_from("<H",blob,header+6)[0]
    option_len = struct.unpack_from("<H",blob,header+20)[0]
    option = header+24
    require(option+option_len <= len(blob) and
            struct.unpack_from("<H",blob,option)[0] == 0x10b,
            "original module must be PE32")
    image_base = struct.unpack_from("<I",blob,option+28)[0]
    section_table = option+option_len
    require(2 <= num_sections <= 64 and
            section_table+40*num_sections <= len(blob),
            "invalid original PE section table")
    sections = {}
    for idx in range(num_sections):
        at = section_table+40*idx
        label = blob[at:at+8].split(b"\x00",1)[0].decode("ascii","replace")
        vsize, rva, rsize, raw = struct.unpack_from("<IIII",blob,at+8)
        require(raw+rsize <= len(blob),"invalid original section raw extent")
        sections[label]=(image_base+rva,vsize,raw,rsize)
    require(all(x in sections for x in (".text",".rdata",".data")),
            "original game section missing")
    def address_of(file_offset):
        for va,vsz,raw,rsize in sections.values():
            if raw <= file_offset < raw+rsize:
                return va + file_offset-raw
        raise AuditError("unmapped original PE offset")
    def offset_of(va):
        for begin, vsize, raw, rsize in sections.values():
            if begin <= va < begin+min(vsize,rsize):
                return raw + va-begin
        raise AuditError("unmapped original PE virtual address")
    require(blob.count(needle+b"\x00") == 1,
            "original game concrete MSVC RTTI name ambiguous")
    rtti_name_offset = blob.index(needle+b"\x00")
    original_type = address_of(rtti_name_offset-8)
    col = offset_of(col_va)
    require(struct.unpack_from("<III",blob,col) == (0,0,0),
            "original MSVC complete object locator header changed")
    require(struct.unpack_from("<I",blob,col+12)[0] == original_type,
            "original locator no longer points to concrete TypeDescriptor")
    hierarchy = struct.unpack_from("<I",blob,col+16)[0]
    require(sections[".rdata"][0] <= hierarchy <
            sections[".rdata"][0]+sections[".rdata"][1],
            "original MSVC class hierarchy not in .rdata")
    rstart, _, roff, rsize = sections[".rdata"]
    native_rdata = blob[roff:roff+rsize]
    col_refs = [rstart+i for i in range(0,len(native_rdata)-3,4)
                if struct.unpack_from("<I",native_rdata,i)[0] == col_va]
    require(len(col_refs) == 1,"original game vtable locator reference ambiguous")
    vtable = col_refs[0]+4
    require(vtable == expected_vtable,"original game x86 vtable address changed")
    start = offset_of(vtable)
    raw_table = blob[start:start+count*4]
    require(len(raw_table)==count*4,"truncated original game vtable")
    methods = struct.unpack("<%dI" % count,raw_table)
    text_va,text_len,_,_ = sections[".text"]
    require(all(text_va <= p < text_va+text_len for p in methods),
            "original vtable contains non-text pointer")
    require(methods[0] == first and methods[-1] == last,
            "original x86 game method boundary differs")
    require(hashlib.sha256(raw_table).hexdigest() == digest,
            "original game pointer table SHA256 mismatch")
    after = struct.unpack_from("<I",blob,start+count*4)[0]
    require(not (text_va <= after < text_va+text_len),
            "original x86 game vtable unexpectedly extends beyond pinned slots")
    return {
        "module": name,
        "required_factory": ORIGINAL[name][1],
        "original_sha256": ORIGINAL[name][0],
        "concrete_rtti": needle.decode("ascii"),
        "complete_object_locator_va": hex(col_va),
        "class_hierarchy_va": hex(hierarchy),
        "vtable_va": hex(vtable),
        "method_slots":count,
        "pointer_bits":32,
        "pointer_table_sha256":digest,
        "first_method_va":hex(first),
        "last_method_va":hex(last),
    }

def inspect_original_zip(original_zip):
    with zipfile.ZipFile(original_zip) as archive:
        result = [inspect_original_pe_game_table(archive.read(name),name)
                  for name in ("server.dll","client.dll")]
    return {
        "original_game_module_vtables": result,
        "original_server_slots":31, "original_client_slots":48,
        "native_server_slots":47, "native_client_slots":78,
        "method_count_delta_server":16,
        "method_count_delta_client":30,
        "warning":"The delta counts are NOT proven appended methods; method order, signatures and semantics require independent ABI verification."
    }

def audit(sourcevr_zip, sin_bin_zip=None):
    profiles = []
    results = {}
    with zipfile.ZipFile(sourcevr_zip) as z:
        for profile in PROFILE_SHAS:
            manifest = z.read(
                f"source vr/assets/sourcevr/v1/native/{profile}.manifest"
            ).decode("utf-8")
            recorded = {}
            for line in manifest.splitlines():
                parts = line.split("\t")
                if len(parts) >= 5 and parts[2] in TABLES:
                    kind = parts[2]
                    require(kind not in recorded, "duplicate native game module")
                    sha = parts[1]
                    require(re.fullmatch(r"[0-9a-f]{64}", sha) is not None,
                            "invalid native game manifest hash")
                    require(sha in TABLES[kind], "unrecognized native game revision")
                    require(parts[4].endswith("/" + kind), "unsafe game module path")
                    require(parts[4].startswith("sourcevr/v1/native/blobs/" + sha + "/"),
                            "game module path not tied to hash")
                    recorded[kind] = (sha, "source vr/assets/" + parts[4])
            require(set(recorded) == set(TABLES), "missing game module in profile")
            require((recorded["libserver.so"][0], recorded["libclient.so"][0]) ==
                    PROFILE_SHAS[profile], "game module pair changed")
            for kind, (sha, path) in recorded.items():
                if sha not in results:
                    raw = z.read(path)
                    require(hashlib.sha256(raw).hexdigest() == sha,
                            "real native module bytes do not match selected SHA")
                    results[sha] = {"sha256": sha, **verify_module(NativeELF(raw), sha)}
            profiles.append({"profile": profile,
                             "server_sha256": recorded["libserver.so"][0],
                             "client_sha256": recorded["libclient.so"][0]})
    require(set(results) == set(SERVER_TABLES) | set(CLIENT_TABLES),
            "not all eleven unique game module vtables were verified")
    original = inspect_original_zip(sin_bin_zip) if sin_bin_zip else None
    return {
        "original_game_modules": original,
        "native_game_profiles": profiles,
        "verified_unique_module_vtables": list(results.values()),
        "server_method_slots": 47,
        "client_method_slots": 78,
        "total_unique_native_method_pointers_audited":
            47*len(SERVER_TABLES) + 78*len(CLIENT_TABLES),
        "conclusion": (
            "Verified real factory singletons, CServerGameDLL/CHLClient RTTI "
            "derivation, constructor vptr stores and complete native 47/78-slot "
            "tables in all 11 unique ARM64 game modules. Signatures, behavior, "
            "compatibility with original SiN and game startup are unverified."
        ),
        "quest3_playable": False,
    }


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--sourcevr-zip", required=True, type=Path)
    p.add_argument("--sin-bin-zip", type=Path,
                   help="Optional owner-provided 2006 original client/server PE32 ZIP")
    args = p.parse_args()
    try:
        print(json.dumps(audit(args.sourcevr_zip, args.sin_bin_zip), indent=2))
    except (AuditError, KeyError, OSError, struct.error,
            UnicodeDecodeError, zipfile.BadZipFile) as exc:
        p.exit(2, f"SourceVR 0.5.5 game vtable audit FAILED: {exc}\n")


if __name__ == "__main__":
    main()
