#!/usr/bin/env python3
"""Owner-only, SHA-pinned original 2006 SiN client stats ID audit.

Only metadata is emitted. No original binary/disassembly is stored.
Use --self-test in public CI, or pass original client.dll and optionally the
original script file when working privately with the owner's game data.
"""
import argparse
import hashlib
import json
import re
import struct
from pathlib import Path

ORIGINAL_SHA = "6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d"
SCRIPT_SHA = "a18afd2e8fba54653a9bd173c3eacf440fe016b3659b74578f2ecd1f37a156a8"
NAMES_SHA = "8913a9a523ffcfaf6baf621f6b3d0c82786346c510afa8d56e6b2fe30909a1ce"
POINTER_SHA = "0c91373338d5d085702cc9dfce22a03151d41c26cd86660a34034548180ae0ec"
TABLE_RVA = 0x31E428
ENTRY_COUNT = 66
IMAGE_BASE = 0x24000000
UNSCRIPTED = {
    "STAT_NUM_DD_TRIGGERS", "STAT_NUM_ITEMS_PICKED_UP",
    "STAT_NUM_ITEMS_USED_GENERIC", "STAT_NUM_FLAILS",
}
SYNTHETIC_NAMES = (
    "STAT_TOTAL_TIME",
    "STAT_ABSOLUTE_TIME",
    "STAT_EXPECTED_TIME",
    "STAT_TOTAL_TIME_IN_COMBAT",
    "STAT_AVG_TIME_LENGTH_IN_COMBAT",
    "STAT_TOTAL_SHOTS",
    "STAT_TOTAL_SHOTS_IN_COMBAT",
    "STAT_TOTAL_PRIMARY_SHOTS",
    "STAT_TOTAL_SECONDARY_SHOTS",
    "STAT_TOTAL_WEAPON_ACCURACY",
    "STAT_TOTAL_WEAPON_ACCURACY_IN_COMBAT",
    "STAT_NUM_GRENADES_THROWN",
    "STAT_NUM_MELEES",
    "STAT_TOTAL_RELOADS",
    "STAT_TOTAL_HEADSHOTS",
    "STAT_TOTAL_HEADSHOTS_IN_COMBAT",
    "STAT_TOTAL_DAMAGE_TAKEN",
    "STAT_TOTAL_HEALTH_GAINED",
    "STAT_AVG_HEALTH",
    "STAT_AVG_HEALTH_IN_COMBAT",
    "STAT_TOTAL_DEATHS_PER_MINUTE",
    "STAT_TOTAL_DEATHS_PER_HOUR",
    "STAT_TOTAL_DEATHS",
    "STAT_TOTAL_REBIRTHS",
    "STAT_TOTAL_TIME_AT_FULL_HEALTH",
    "STAT_TOTAL_TIME_BELOW_25_HEALTH",
    "STAT_TOTAL_NPC_KILLS",
    "STAT_TOTAL_KILLS_PER_MINUTE",
    "STAT_TOTAL_KILLS_PER_MINUTE_IN_COMBAT",
    "STAT_TOTAL_DPS",
    "STAT_TOTAL_DPS_IN_COMBAT",
    "STAT_DPS_TAKEN_IN_COMBAT",
    "STAT_TOTAL_JUMPS",
    "STAT_TOTAL_JUMPS_IN_COMBAT",
    "STAT_TOTAL_DUCKS",
    "STAT_TOTAL_DUCKS_IN_COMBAT",
    "STAT_TOTAL_SAVES",
    "STAT_TOTAL_LOADS",
    "STAT_NUM_FOOT_STEPS",
    "STAT_NUM_PAUSES",
    "STAT_NUM_ITEMS_USED",
    "STAT_NUM_NPCS_CAUGHT_ON_FIRE",
    "STAT_NUM_NPC_HELMETS_SHOT_OFF",
    "STAT_NUM_NPCS_DECAPITATED",
    "STAT_NUM_CONTAINERS_EXPLODED",
    "STAT_MOST_KILLS_BEFORE_DEATH",
    "STAT_NUM_SECRETS",
    "STAT_NUM_ITEM_CRATES_BROKEN",
    "STAT_CHALLENGE_RATING",
    "STAT_INSTANT_CHALLENGE_RATING",
    "STAT_MOST_KILLS_BEFORE_DAMAGE",
    "STAT_NUM_DD_TRIGGERS",
    "STAT_NUM_ITEMS_PICKED_UP",
    "STAT_NUM_ITEMS_USED_GENERIC",
    "STAT_NUM_FLAILS",
    "STAT_WEAPON_ACCURACY",
    "STAT_WEAPON_HEADSHOTS",
    "STAT_WEAPON_PRIMARY_SHOTS",
    "STAT_WEAPON_SECONDARY_SHOTS",
    "STAT_WEAPON_ACCURACY_IN_COMBAT",
    "STAT_NPC_KILLS",
    "STAT_WEAPON_NPC_KILLS",
    "STAT_LEVEL_KILLS",
    "STAT_LEVEL_DEATHS",
    "STAT_LEVEL_SECRETS",
    "STAT_GAME_PLAY_VAR_AVERAGES",
)


def require(ok, reason):
    if not ok:
        raise ValueError(reason)


class PeReader:
    def __init__(self, data):
        self.data = data
        require(data[:2] == b"MZ" and len(data) >= 512, "missing DOS header")
        pe = struct.unpack_from("<I", data, 0x3C)[0]
        require(pe+248 <= len(data) and data[pe:pe+4] == b"PE\0\0",
                "bad PE signature")
        machine, count = struct.unpack_from("<HH", data, pe+4)
        opt_size = struct.unpack_from("<H", data, pe+20)[0]
        require(machine == 0x14C and count >= 3 and opt_size == 224,
                "original Win32 PE32 header mismatch")
        require(struct.unpack_from("<H", data, pe+24)[0] == 0x10B,
                "expected PE32 optional header")
        self.base = struct.unpack_from("<I", data, pe+52)[0]
        require(self.base == IMAGE_BASE, "original image base mismatch")
        self.sections = {}
        for i in range(count):
            pos = pe+24+opt_size+40*i
            require(pos+40 <= len(data), "truncated PE section")
            name = data[pos:pos+8].split(b"\0", 1)[0].decode("ascii")
            virtual_size, rva, disk_size, disk_at = struct.unpack_from(
                "<IIII", data, pos+8)
            require(disk_at+disk_size <= len(data), "truncated section bytes")
            self.sections[name] = (rva, virtual_size, disk_at, disk_size)
        require(".text" in self.sections and ".rdata" in self.sections and
                ".data" in self.sections, "missing required game sections")

    def raw(self, rva, size):
        for begin, virt_size, offset, disk_size in self.sections.values():
            if begin <= rva and rva+size <= begin+min(virt_size,disk_size):
                return self.data[offset+rva-begin:offset+rva-begin+size]
        raise ValueError("RVA not contained in file-backed section")

    def in_section(self, name, rva):
        begin, size, _, _ = self.sections[name]
        return begin <= rva < begin+size

    def cstring(self, address):
        rva = address-self.base
        require(self.in_section(".rdata",rva),
                "stat name pointer outside original .rdata")
        data = self.raw(rva, 96)
        require(b"\0" in data, "unterminated stat name")
        result = data.split(b"\0",1)[0]
        require(result.startswith(b"STAT_") and
                all(ch in b"ABCDEFGHIJKLMNOPQRSTUVWXYZ_0123456789"
                    for ch in result), "unexpected stat identifier")
        return result.decode("ascii")


def audit(data, original=True, definitions=None):
    digest = hashlib.sha256(data).hexdigest()
    if original:
        require(digest == ORIGINAL_SHA, "original client SHA256 mismatch")
    pe = PeReader(data)
    require(pe.in_section(".data",TABLE_RVA), "table outside .data")
    block = pe.raw(TABLE_RVA,ENTRY_COUNT*4)
    if original:
        require(hashlib.sha256(block).hexdigest() == POINTER_SHA,
                "original pointer block changed")
    table_ptrs = struct.unpack("<"+"I"*ENTRY_COUNT,block)
    names = tuple(pe.cstring(ptr) for ptr in table_ptrs)
    require(len(set(names))==ENTRY_COUNT,
            "duplicate stat identifier in table")
    require(hashlib.sha256("\n".join(names).encode()).hexdigest()==NAMES_SHA,
            "compiled stat-name list does not match original")
    # The original function 0x241FFB60 loads [EBX*4+0x2431E428].
    require(pe.raw(0x1FFB70,3)==b"\x8B\x04\x9D" and
            struct.unpack("<I",pe.raw(0x1FFB73,4))[0] ==
            IMAGE_BASE+TABLE_RVA,
            "original 66-pointer array load changed")
    # Stores EBX as record identifier, increments and compares against 0x42.
    require(pe.raw(0x1FFCB3,10)==b"\x89\x5C\x16\x0C\x83\xC3\x01\x83\xFB\x42",
            "original index-store/66-iteration bound mismatch")
    outcome = {
        "status": "STATIC_ORIGINAL_STAT_IDS_AUTHENTICATED" if original
                  else "SYNTHETIC_STAT_ID_TABLE",
        "sha256": digest,
        "registered_ids": len(names),
        "name_table_rva": hex(TABLE_RVA),
        "names_joined_sha256": NAMES_SHA,
        "four_compiled_only_names": sorted(UNSCRIPTED),
        "retail_init_invoked": False,
        "full_retail_resource_binding_proven": False,
    }
    if definitions is not None:
        require(original, "owner definitions validation requires SHA-pin")
        require(hashlib.sha256(definitions).hexdigest()==SCRIPT_SHA,
                "original stats_definitions.txt SHA256 mismatch")
        decoded = definitions.decode("utf-8-sig")
        script_names = re.findall(r'"name"\s*"([A-Z0-9_]+)"',decoded)
        require(len(script_names)==62 and len(set(script_names))==62,
                "original script definition count/uniqueness mismatch")
        require(set(script_names).issubset(set(names)),
                "script references a stat ID not in original DLL")
        require(set(names)-set(script_names)==UNSCRIPTED,
                "compiled-only stat set changed")
        outcome["original_script_definitions"] = len(script_names)
        outcome["script_sha256"] = SCRIPT_SHA
        outcome["script_name_lookup_complete"] = True
    return outcome


def synthetic():
    # Construct original-FREE in-memory PE matching only metadata layout.
    data = bytearray(0x325000)
    data[:2] = b"MZ"
    pe = 0xF8
    struct.pack_into("<I",data,0x3C,pe)
    data[pe:pe+4] = b"PE\0\0"
    struct.pack_into("<HH",data,pe+4,0x14C,4)
    struct.pack_into("<H",data,pe+20,224)
    struct.pack_into("<H",data,pe+24,0x10B)
    struct.pack_into("<I",data,pe+52,IMAGE_BASE)
    sections = [
        (".text",0x1000,0x275000,0x1000,0x275000),
        (".rdata",0x276000,0x79000,0x276000,0x79000),
        (".data",0x2F3000,0x34000,0x2F3000,0x31000),
        (".reloc",0x390000,0x1000,0x324000,0x1000),
    ]
    for i,(name,rva,vs,offset,size) in enumerate(sections):
        pos=pe+24+224+40*i
        data[pos:pos+len(name)] = name.encode()
        struct.pack_into("<IIII",data,pos+8,vs,rva,size,offset)
    cursor=0x2AE000
    for i,name in enumerate(SYNTHETIC_NAMES):
        val=name.encode()+b"\0"
        data[cursor:cursor+len(val)]=val
        struct.pack_into("<I",data,TABLE_RVA+4*i,IMAGE_BASE+cursor)
        cursor+=len(val)
    data[0x1FFB70:0x1FFB77]=b"\x8B\x04\x9D"+struct.pack(
        "<I",IMAGE_BASE+TABLE_RVA)
    data[0x1FFCB3:0x1FFCBD]=b"\x89\x5C\x16\x0C\x83\xC3\x01\x83\xFB\x42"
    return bytes(data)


def self_test():
    good=synthetic()
    require(audit(good,original=False)["registered_ids"]==66,
            "synthetic canonical 66 IDs refused")
    mutations={
        "table-pointer":TABLE_RVA,
        "second-table-pointer":TABLE_RVA+4,
        "identifier-string":0x2AE000,
        "array-load-opcode":0x1FFB70,
        "array-address":0x1FFB73,
        "index-write":0x1FFCB3,
        "loop-limit":0x1FFCBC,
    }
    for label,at in mutations.items():
        mutated=bytearray(good)
        mutated[at]^=0x31
        try:
            audit(bytes(mutated),original=False)
        except ValueError:
            continue
        raise AssertionError(f"corrupted metadata accepted: {label}")
    try:
        audit(good,original=True)
    except ValueError as exc:
        require("SHA256" in str(exc), "synthetic file bypassed SHA gate")
    else:
        raise AssertionError("untrusted synthetic file authenticated")
    return {"self_test":"PASS","synthetic_mutations_refused":len(mutations),
            "private_game_binary_used":False}


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("dll", nargs="?")
    parser.add_argument("--definitions")
    parser.add_argument("--self-test",action="store_true")
    args=parser.parse_args()
    if args.self_test:
        print(json.dumps(self_test(),sort_keys=True))
        return
    if not args.dll:
        parser.error("specify original client.dll or --self-test")
    source=Path(args.definitions).read_bytes() if args.definitions else None
    print(json.dumps(audit(Path(args.dll).read_bytes(),
                           definitions=source),indent=2,sort_keys=True))


if __name__ == "__main__":
    main()
