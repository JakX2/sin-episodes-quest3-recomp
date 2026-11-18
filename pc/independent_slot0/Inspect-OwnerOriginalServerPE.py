#!/usr/bin/env python3
"""Read-only PE32 original server.dll factory/vtable metadata authentication.

Requires the SHA-pinned original file from an owner's private installation.
Never executes, modifies, embeds or uploads the binary. Output is limited to
registration identity and method RVAs, not original code or pseudocode.
"""
import argparse
import hashlib
import json
import re
import struct
from pathlib import Path

PIN = "568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650"
REG = (
    ("ServerGameDLL003", 0x3bf140, 0x1e7630, 0x510650, 0x442efc),
    ("ServerGameEnts001", 0x3bf210, 0x1e82c0, 0x510654, 0x443030),
    ("ServerGameClients003", 0x3bf230, 0x1e8320, 0x510658, 0x443054),
)
MAIN_SLOTS = (
    0x1e7640, 0x1e7b00, 0x1ea060, 0x1e8f80, 0x1e9080, 0x1e91d0,
    0x1e7be0, 0x1e7b30, 0x1e79b0, 0x1e7a90, 0x1e7c30, 0x1e7c40,
    0x1e92b0, 0x1e7c50, 0x1ea290, 0x1ea2e0, 0x1e7c60, 0x1e7c70,
    0x1e7d10, 0x1e9a30, 0x1e7d20, 0x1e9b80, 0x1e9bc0, 0x1e9aa0,
    0x1e7b50, 0x1e7e50, 0x1e9ae0, 0x1e7e90, 0x1e7c80, 0x1e7ce0,
    0x1e7cf0,
)
CLIENTS_SLOTS = (
    0x1e63f0, 0x1e8330, 0x1ea330, 0x1ea3a0, 0x1e8340,
    0x1e9650, 0x1e8510, 0x1e8370, 0x1e9c00, 0x1ea600,
    0x1e84d0, 0x1e85b0, 0x1e8530, 0x1e8520, 0x1e85e0,
)
ENTS_SLOTS = (0x1e9360, 0x1e82d0, 0x1e9380, 0x1e82e0,
              0x1e94b0, 0x1e82f0, 0x1e94d0)

# Offset of a verified epilogue for each virtual entry, with the exact
# Windows x86 callee-cleanup byte count. These are metadata checks against
# the owner-private original, not executable method bodies.
ENTS_SLOT_ABI = (
    ("scalar_deleting_destructor", 0x1e937c, 4),
    ("SetDebugEdictBase", 0x1e82d9, 4),
    ("MarkEntitiesAsTouching", 0x1e94a8, 8),
    ("FreeContainingEntity", 0x1e82eb, 4),
    ("BaseEntityToEdict", 0x1e94c3, 4),
    ("EdictToBaseEntity", 0x1e8309, 4),
    ("CheckTransmit", 0x1e962f, 12),
)


# The first ten original ServerGameDLL003::DLLInit engineFactory requests,
# all independently recovered from the owner's SHA-authenticated server.
SERVER_DLLINIT_FIRST_TEN = (
    "VEngineServer021", "VoiceServer001", "VEngineCvar002",
    "VEngineServerStringTable001", "StaticPropMgrServer002",
    "VEngineRandom001", "IEngineSoundServer003", "SpatialPartition001",
    "VModelInfoServer001", "EngineTraceServer003",
)
SERVER_DLLINIT_FILESYSTEM = ("VFileSystem012",)
SERVER_DLLINIT_NEXT_FIVE = (
    "GAMEEVENTSMANAGER002", "VEngineCache001", "VSoundEmitter001",
    "ServerUploadGameStats001", "ISERVERPLUGINHELPERS001",
)

class PE32:
    def __init__(self, data):
        self.data = data
        if len(data) < 256 or data[:2] != b"MZ":
            raise ValueError("Not an MZ image")
        pe = struct.unpack_from("<I", data, 0x3c)[0]
        if pe + 0x78 > len(data) or data[pe:pe+4] != b"PE\x00\x00":
            raise ValueError("Invalid PE header")
        if struct.unpack_from("<H", data, pe+4)[0] != 0x14c:
            raise ValueError("Not x86 PE")
        n = struct.unpack_from("<H", data, pe+6)[0]
        opt_size = struct.unpack_from("<H", data, pe+20)[0]
        opt = pe + 24
        if not 1 <= n <= 96 or opt_size < 0x60 or opt+opt_size+n*40 > len(data):
            raise ValueError("Bad PE section table")
        if struct.unpack_from("<H", data, opt)[0] != 0x10b:
            raise ValueError("Not PE32 optional header")
        self.base = struct.unpack_from("<I", data, opt+28)[0]
        self.sections = []
        for i in range(n):
            o = opt+opt_size+i*40
            name = data[o:o+8].rstrip(b"\0").decode("ascii")
            vsize, rva, rawsize, fileoff = struct.unpack_from("<IIII", data, o+8)
            if fileoff+rawsize > len(data):
                raise ValueError("Truncated section")
            self.sections.append((name, rva, vsize, rawsize, fileoff))

    def off(self, rva, size=1):
        for name, start, vsize, rawsize, fileoff in self.sections:
            if start <= rva and rva + size <= start + rawsize:
                return fileoff + rva - start
        raise ValueError("Unmapped PE RVA: " + hex(rva))

    def raw(self, rva, size):
        p = self.off(rva, size)
        return self.data[p:p+size]

    def u32(self, rva):
        return struct.unpack("<I", self.raw(rva, 4))[0]

    def rva(self, va):
        if va < self.base:
            raise ValueError("Invalid VA")
        return va - self.base

    def string(self, rva):
        o = self.off(rva)
        end = self.data.find(b"\x00", o, o+128)
        if end < 0:
            raise ValueError("Unterminated factory string")
        return self.data[o:end].decode("ascii")

def direct_call_target(pe, call_rva):
    # Small bounded PE32 relative-call decoder, not a general disassembler.
    call = pe.raw(call_rva, 5)
    if call[0] != 0xE8:
        raise ValueError("Expected x86 relative call at RVA " + hex(call_rva))
    rel = struct.unpack_from("<i", call, 1)[0]
    target = call_rva + 5 + rel
    pe.off(target)  # the target must exist in a mapped PE section
    return target


def inspect(data):
    if hashlib.sha256(data).hexdigest() != PIN:
        raise ValueError("Original server.dll SHA256 mismatch")
    pe = PE32(data)
    if pe.base != 0x22000000:
        raise ValueError("Unexpected original server image base")
    records = []
    for name, reg, factory, obj, vtable in REG:
        wrapper = pe.raw(reg, 15)
        # Registration wrapper immediate operands: name and factory VA.
        if wrapper[0] != 0x68 or wrapper[5] != 0x68 or wrapper[10] != 0xb9:
            raise ValueError(name + " registration wrapper mismatch")
        if pe.string(pe.rva(struct.unpack_from("<I", wrapper, 1)[0])) != name:
            raise ValueError(name + " registration name mismatch")
        if pe.rva(struct.unpack_from("<I", wrapper, 6)[0]) != factory:
            raise ValueError(name + " registration factory mismatch")
        callback = pe.raw(factory, 6)
        if callback[0] != 0xb8 or callback[5] != 0xc3:
            raise ValueError(name + " singleton callback shape mismatch")
        if pe.rva(struct.unpack_from("<I", callback, 1)[0]) != obj:
            raise ValueError(name + " singleton pointer mismatch")
        if pe.rva(pe.u32(obj)) != vtable:
            raise ValueError(name + " singleton vtable mismatch")
        result = {
            "name": name,
            "registration_rva": hex(reg),
            "factory_callback_rva": hex(factory),
            "object_rva": hex(obj),
            "vtable_rva": hex(vtable),
        }
        expected_slots = {
            "ServerGameDLL003": MAIN_SLOTS,
            "ServerGameEnts001": ENTS_SLOTS,
            "ServerGameClients003": CLIENTS_SLOTS,
        }[name]
        actual_slots = tuple(pe.rva(pe.u32(vtable+4*i))
                             for i in range(len(expected_slots)))
        if actual_slots != expected_slots:
            raise ValueError(name + " original table entries mismatch")
        def executable_rva(rva):
            return any(section == ".text" and start <= rva < start+size
                       for section, start, size, _, _ in pe.sections)
        if not all(executable_rva(slot) for slot in actual_slots):
            raise ValueError(name + " contains a non-code entry")
        next_word = pe.u32(vtable+4*len(expected_slots))
        if next_word >= pe.base and executable_rva(next_word-pe.base):
            raise ValueError(name + " unexpectedly has another contiguous code entry")
        result["vtable_slot_count"] = len(actual_slots)
        result["vtable_slot_rvas"] = [hex(slot) for slot in actual_slots]
        if name == "ServerGameEnts001":
            slots = tuple(pe.rva(pe.u32(vtable+4*i))
                          for i in range(len(ENTS_SLOTS)))
            if slots != ENTS_SLOTS or pe.u32(vtable+4*len(slots)) != 0:
                raise ValueError("ServerGameEnts001 must have seven original slots")
            for slot in slots:
                if not any(s == ".text" and start <= slot < start+vsize
                           for s, start, vsize, rawsize, fileoff in pe.sections):
                    raise ValueError("Non-code entry in server entity vtable")
            for index, (method, end_rva, callee_bytes) in enumerate(ENTS_SLOT_ABI):
                # Reject stale/nonmatching originals, without disassembling
                # arbitrary or untrusted PE bytes.
                if not slots[index] <= end_rva < slots[index] + 0x1000:
                    raise ValueError("Method epilogue outside its slot")
                if pe.raw(end_rva, 3) != b"\xc2" + struct.pack("<H", callee_bytes):
                    raise ValueError(method + " x86 stack cleanup mismatch")
            result["vtable_slot_rvas"] = [hex(i) for i in slots]
            result["slot_signatures"] = [
                {"method": method, "callee_cleanup_bytes": callee_bytes}
                for method, _end_rva, callee_bytes in ENTS_SLOT_ABI
            ]
        records.append(result)
    # Entity FreeContainingEntity's slot 3 delegates to the shared helper;
    # helper then invokes three distinct cleanup functions, in order.
    # These are *addresses only*, not original method code.
    if direct_call_target(pe, 0x1e82e5) != 0x266e60:
        raise ValueError("ServerGameEnts001 release wrapper target mismatch")
    cleanup_edges = (
        (0x266e8a, 0x21ffd0),
        (0x266e90, 0x220190),
        (0x266e96, 0x286c10),
    )
    for source, expected in cleanup_edges:
        if direct_call_target(pe, source) != expected:
            raise ValueError("FreeContainingEntity cleanup target mismatch")

    # The original ServerGameDLL003 vtable's first method is native DLLInit.
    # Only its first ten sequential calls to the first engineFactory are
    # reported; the next request uses the filesystem factory and is out
    # of this deliberately bounded dependency probe.
    if pe.rva(pe.u32(0x442efc)) != 0x1e7640:
        raise ValueError("Original ServerGameDLL003 DLLInit slot mismatch")
    start_rva = 0x1e7640
    code = pe.raw(start_rva, 0x103)
    calls = list(re.finditer(rb"\x6a\x00\x68(.{4})\xff\xd6", code, re.DOTALL))
    versions = tuple(
        pe.string(pe.rva(struct.unpack("<I", match.group(1))[0]))
        for match in calls
    )
    if versions != SERVER_DLLINIT_FIRST_TEN:
        raise ValueError("Original server DLLInit first-ten query order mismatch")

    # The original implementation switches from engineFactory (ESI) to
    # fileSystemFactory (EDI), then back to ESI for five more mandatory
    # queries. Inspect only those three bounded call regions and assert
    # the exact versioned names, not arbitrary nearby code.
    def query_names(start_rva, byte_count, factory_call_bytes):
        matches = list(re.finditer(
            rb"\x6a\x00\x68(.{4})" + factory_call_bytes,
            pe.raw(start_rva, byte_count), re.DOTALL))
        return tuple(pe.string(pe.rva(struct.unpack("<I", match.group(1))[0]))
                     for match in matches)

    filesystem_versions = query_names(0x1e7743, 0x1d, rb"\xff\xd7")
    if filesystem_versions != SERVER_DLLINIT_FILESYSTEM:
        raise ValueError("Original DLLInit filesystem factory query mismatch")
    additional_versions = query_names(0x1e7760, 0x7d, rb"\xff\xd6")
    if additional_versions != SERVER_DLLINIT_NEXT_FIVE:
        raise ValueError("Original DLLInit post-filesystem engine query mismatch")
    # Tick interval method is the original main table's slot 9. Its
    # command-line key, constants and vstdlib call slot indices were
    # observed in the bounded original method at RVA 0x1E7A90.
    if MAIN_SLOTS[9] != 0x1e7a90 or pe.string(0x442a68) != "-tickrate":
        raise ValueError("Original tick interval identity mismatch")
    default_interval = struct.unpack("<f", pe.raw(0x1e7a9e, 4))[0]
    minimum_rate = struct.unpack("<f", pe.raw(0x3d8f14, 4))[0]
    numerator = struct.unpack("<f", pe.raw(0x3d22f8, 4))[0]
    if abs(default_interval - 0.015) > 1e-9 or minimum_rate != 10 or numerator != 1:
        raise ValueError("Original server tick interval constants mismatch")

    # Retail-observed pre-initialization callback, main server slot 11.
    # Its wrapper jumps to a game-rules-aware helper; the no-rules
    # branch returns an immutable string. Validate just these metadata.
    wrapper_rva = MAIN_SLOTS[11]
    jump = pe.raw(wrapper_rva, 5)
    if jump[0] != 0xe9:
        raise ValueError("Original description wrapper no longer jumps")
    helper_rva = wrapper_rva + 5 + struct.unpack_from("<i", jump, 1)[0]
    if helper_rva != 0x2f0cc0:
        raise ValueError("Original description helper changed")
    fallback = pe.raw(helper_rva + 0x12, 6)
    if fallback[0] != 0xb8 or fallback[5] != 0xc3:
        raise ValueError("Original description fallback changed")
    fallback_rva = pe.rva(struct.unpack_from("<I", fallback, 1)[0])
    game_description = pe.string(fallback_rva)
    if fallback_rva != 0x466df8 or game_description != "SiN Episodes":
        raise ValueError("Original game description changed")
    # Original DLLInit post-cvar lifecycle registration. The six
    # explicit game system calls are kept as separate identities, rather
    # than silently substituting the five SDK2004 systems.
    save_getters = (
        (0x1e7868, 0x273be0, 0x51ba84, 0x1e7874),
        (0x1e787f, 0x22eca0, 0x575288, 0x1e788b),
        (0x1e7896, 0x09df20, 0x4e250c, 0x1e78a2),
        (0x1e78ac, 0x1c2f90, 0x50967c, 0x1e78b8),
        (0x1e78c3, 0x091790, 0x4e1e4c, 0x1e78cf),
    )
    system_getters = (
        (0x1e78d2, 0x1f3db0, 0x572ae0, 0x1e78d8),
        (0x1e78dd, 0x20d900, 0x513ae0, 0x1e78e3),
        (0x1e78e8, 0x2f0fa0, 0x595f58, 0x1e78ee),
        (0x1e78f3, 0x2b67d0, 0x521864, 0x1e78f9),
        (0x1e78fe, 0x25f8e0, 0x57ae9c, 0x1e7904),
        (0x1e7909, 0x1b4e50, 0x507558, 0x1e790f),
    )
    def verify_getter(call, getter, obj):
        if direct_call_target(pe, call) != getter:
            raise ValueError("Original startup getter edge mismatch")
        code = pe.raw(getter, 6)
        if code[0] != 0xb8 or code[5] != 0xc3 or pe.rva(struct.unpack_from("<I", code, 1)[0]) != obj:
            raise ValueError("Original startup singleton identity mismatch")
    for call, getter, obj, registration in save_getters:
        verify_getter(call, getter, obj)
        if pe.raw(registration, 3) != bytes.fromhex("ff5724"):
            raise ValueError("Original AddBlockHandler slot mismatch")
    for call, getter, obj, registration in system_getters:
        verify_getter(call, getter, obj)
        if direct_call_target(pe, registration) != 0x1f56e0:
            raise ValueError("Original Add game system edge mismatch")
    reverse_calls = (0x1e7a1d, 0x1e7a06, 0x1e79f0, 0x1e79d9, 0x1e79c2)
    for call, (_, getter, obj, _) in zip(reverse_calls, save_getters):
        verify_getter(call, getter, obj)
        if pe.raw(call+12, 3) != bytes.fromhex("ff5628"):
            raise ValueError("Original RemoveBlockHandler slot mismatch")
    if pe.raw(0x1f57fa, 2) != bytes.fromhex("ff12") or \
       pe.raw(0x1f53a0, 3) != bytes.fromhex("ff5214") or \
       pe.raw(0x1f53af, 3) != bytes.fromhex("ff5018"):
        raise ValueError("Original system Init or cache-lock dispatch slot mismatch")
    mod_event_name = pe.string(0x4428cc)
    if mod_event_name != "resource/ModEvents.res" or pe.raw(0x1e7924, 3) != bytes.fromhex("ff5204"):
        raise ValueError("Original mod-event load mismatch")
    for source, target in ((0x1e7927, 0x1f5700), (0x1e7930, 0x1ad8a0), (0x1e7939, 0x149b60)):
        if direct_call_target(pe, source) != target:
            raise ValueError("Original late initialization edge mismatch")
    return {"original_server_sha256": PIN, "format": "PE32 x86",
            "save_block_getters": [{"getter_rva":hex(g), "object_rva":hex(o)} for _,g,o,_ in save_getters],
            "explicit_game_system_getters": [{"getter_rva":hex(g), "object_rva":hex(o)} for _,g,o,_ in system_getters],
            "mod_events_path": mod_event_name,
            "status": "ABI research only, no server gameplay implementation",
            "factory_registrations": records,
            "game_description": {"slot": 11, "wrapper_rva": hex(wrapper_rva),
                                 "helper_rva": hex(helper_rva),
                                 "pre_init": game_description},
            "tick_interval": {"slot": 9, "default_seconds": default_interval,
                              "override_rate_exclusive_minimum": minimum_rate,
                              "command_line_flag": "-tickrate"},
            "server_dllinit_first_ten_engine_queries": versions,
            "server_dllinit_filesystem_queries": filesystem_versions,
            "server_dllinit_next_five_engine_queries": additional_versions,
            "entity_release_helper_rva": hex(0x266e60),
            "entity_release_cleanup_targets_rva": [hex(t) for _, t in cleanup_edges]}


def synthetic_test():
    """Test parsing and strict SHA rejection with an invented minimal PE."""
    b = bytearray(0x600)
    b[:2] = b"MZ"
    struct.pack_into("<I", b, 0x3c, 0x80)
    b[0x80:0x84] = b"PE\x00\x00"
    struct.pack_into("<H", b, 0x84, 0x14c)
    struct.pack_into("<H", b, 0x86, 1)
    struct.pack_into("<H", b, 0x94, 0xe0)
    struct.pack_into("<H", b, 0x98, 0x10b)
    struct.pack_into("<I", b, 0x98+28, 0x22000000)
    sec = 0x80+24+0xe0
    b[sec:sec+5] = b".text"
    struct.pack_into("<IIII", b, sec+8, 0x200, 0x1000, 0x200, 0x400)
    b[0x400:0x404] = b"TEST"
    pe = PE32(bytes(b))
    assert pe.raw(0x1000, 4) == b"TEST"
    assert pe.rva(0x22001004) == 0x1004
    struct.pack_into("<B", b, 0x408, 0xE8)
    struct.pack_into("<i", b, 0x409, 0x1014 - (0x1008 + 5))
    pe = PE32(bytes(b))
    assert direct_call_target(pe, 0x1008) == 0x1014
    try:
        direct_call_target(pe, 0x1000)
    except ValueError:
        pass
    else:
        raise AssertionError("Non-call instruction treated as call")
    try:
        pe.raw(0x1100, 0x200)
    except ValueError:
        pass
    else:
        raise AssertionError("Unmapped RVA was incorrectly accepted")
    try:
        inspect(bytes(b))
    except ValueError as exc:
        assert "SHA256 mismatch" in str(exc)
    else:
        raise AssertionError("Synthetic binary passed private SHA pin")
    print("PASS synthetic server PE32 parser and original-SHA guard")


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("original_server_dll", nargs="?", type=Path)
    ap.add_argument("--self-test", action="store_true")
    args = ap.parse_args()
    if args.self_test:
        synthetic_test()
    elif args.original_server_dll:
        print(json.dumps(inspect(args.original_server_dll.read_bytes()), indent=2))
    else:
        ap.error("Provide owner original server.dll or --self-test")


if __name__ == "__main__":
    main()
