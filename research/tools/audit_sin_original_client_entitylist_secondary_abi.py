#!/usr/bin/env python3
"""Phase 1656: cross-authenticate the original VClientEntityList003 subobject ABI.

This is NOT a fresh native-method decompilation. Phases 226, 228, 229,
230 and 231 independently established the factory, constructor, vtables and
small virtual methods. This audit joins those previously published anchors
into one bounded nine-slot secondary-interface contract.

--server + --client: owner-local original 2006 PE32 files; mandatory complete
SHA-256 provenance, then read-only original on-disk checks. No DLL loaded.
--self-test: invented byte windows; no original binaries or game execution.
Nothing in this file registers the interface in a replacement client.dll.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import struct

from audit_sin_original_dll_interface_abi import (
    ORIGINALS, PEImage, VerificationError,
)
from audit_sin_original_core_factory_thunks import (
    FACTORIES, pe_highlow_relocations,
)
from audit_sin_original_embedded_interface_initializers import (
    CHAINS, VPTR_WRITES, check_module as check_initializer,
)
from audit_sin_original_embedded_interface_virtual_accessors import (
    METHODS as ACCESSORS, CLIENT_VTABLE_VA, check_module as check_accessors,
)
from audit_sin_original_client_entitylist_handle_dispatch import (
    VIRTUAL_SLOTS as HANDLE_SLOTS, check_native as check_handles,
)
from audit_sin_original_client_entitylist_parent_lifecycle import (
    PARENT_VTABLE_VA, VIRTUAL_SLOTS as PARENT_SLOTS,
    verify_native as check_parent_lifecycle,
)

IMAGE_BASE = 0x24000000
INTERFACE = "VClientEntityList003"
PARENT_VA = 0x2433FDC8
SECONDARY_OFFSET = 0x10014
SECONDARY_VA = PARENT_VA + SECONDARY_OFFSET
FACTORY_VA = 0x241056B0
PRIMARY_VTABLE_VA = 0x242973B8
SECONDARY_VTABLE_VA = 0x24297390
SECONDARY_VIRTUAL_COUNT = 9
PRIMARY_VIRTUAL_COUNT = 3
NEXT_TABLE_GAP_BYTES = 4
# Each index was already independently bound to an original relocated
# .rdata pointer, not inferred from sequential addresses in the code.
SECONDARY_TARGETS = (
    0x241056F0,  # 0: indexed dword, one DWORD argument, RET 4
    0x24105BB0,  # 1: handle->entity virtual wrapper, RET 4
    0x241074D0,  # 2: serial-checked EHandle resolver, RET 4
    0x24105AF0,  # 3: indexed entity virtual wrapper, RET 4
    0x24105BE0,  # 4: second handle->entity wrapper, RET 4
    0x24105700,  # 5: optional base offset, RET 4
    0x24105740,  # 6: dword getter +0x10, RET
    0x24105720,  # 7: dword setter +0x08, RET 4
    0x24105730,  # 8: dword getter +0x08, RET
)
PRIMARY_TARGETS = (0x241089D0, 0x24108A70, 0x241082A0)
# Slot 2 of the *primary* parent is a deleting destructor. It is not
# part of the secondary nine-slot entity-list interface.
CALLEE_STACK_POP_BYTES = (4, 4, 4, 4, 4, 4, 0, 4, 0)
PRIMARY_VPTR_WRITE_SITE = 0x241081A2
SECONDARY_VPTR_WRITE_SITE = 0x241081A8


def require(condition: bool, why: str) -> None:
    if not condition:
        raise VerificationError("Phase1656 VClientEntityList003: " + why)


def pack(value: int) -> bytes:
    return struct.pack("<I", value)


def expected_window(values: tuple[int, ...]) -> bytes:
    return b"".join(pack(value) for value in values)


def verify_pointer_window(window: bytes, expected: tuple[int, ...],
                          label: str) -> int:
    require(len(window) == len(expected)*4, label + " table size differs")
    for slot, pointer in enumerate(expected):
        require(struct.unpack_from("<I", window, slot*4)[0] == pointer,
                label + " vtable pointer differs at slot " + str(slot))
    return len(expected)


def verify_factory_thunk(code: bytes, return_va: int) -> None:
    require(len(code) == 6, "factory thunk length differs")
    require(code[0:1] == b"\xB8" and code[5:] == b"\xC3",
            "expected original MOV EAX, immediate; RET factory shape")
    require(struct.unpack_from("<I", code, 1)[0] == return_va,
            "factory points at primary object rather than embedded interface")


def verify_constructor_vptr(code: bytes, offset: int, vtable_va: int,
                            primary: bool) -> None:
    expected = (b"\xC7\x06" if primary else
                b"\xC7\x86" + pack(offset))
    require(code == expected + pack(vtable_va),
            "constructor does not install the pinned vtable pointer")


def check_prior_phase_contract() -> None:
    client = CHAINS["client"]
    require((client[0], client[3], client[4], client[5],
             client[10], client[11]) ==
            (INTERFACE, PARENT_VA, SECONDARY_OFFSET, FACTORY_VA,
             SECONDARY_VTABLE_VA, SECONDARY_TARGETS[0]),
            "Phase228 embedded constructor/factory contract changed")
    records = {row[0]: row for row in FACTORIES["client"]}
    require(INTERFACE in records and
            records[INTERFACE][1] == FACTORY_VA and
            records[INTERFACE][2] == SECONDARY_VA,
            "Phase226 singleton factory contract changed")
    require(CLIENT_VTABLE_VA == SECONDARY_VTABLE_VA,
            "Phase229 secondary vtable base changed")
    require(PARENT_VTABLE_VA == PRIMARY_VTABLE_VA,
            "Phase231 primary vtable base changed")
    require(SECONDARY_VIRTUAL_COUNT == len(SECONDARY_TARGETS) == 9,
            "secondary method count must remain nine")
    require(PRIMARY_VIRTUAL_COUNT == len(PRIMARY_TARGETS) == 3,
            "primary method count must remain three")
    require(SECONDARY_VA == 0x2434FDDC and
            PRIMARY_VTABLE_VA-SECONDARY_VTABLE_VA ==
            4*SECONDARY_VIRTUAL_COUNT + NEXT_TABLE_GAP_BYTES,
            "embedded-object offset or adjacent vtable boundary changed")
    slot_map = {index:va for index,va in HANDLE_SLOTS}
    for _,va,_,_,_,slot,_,_ in ACCESSORS["client"]:
        require(slot not in slot_map, "prior phases overlap a secondary slot")
        slot_map[slot] = va
    require(set(slot_map) == set(range(SECONDARY_VIRTUAL_COUNT)),
            "prior phase audits do not cover every secondary slot")
    require(tuple(slot_map[i] for i in range(SECONDARY_VIRTUAL_COUNT)) ==
            SECONDARY_TARGETS, "original Phase229/230 secondary slot order changed")
    require(tuple(target for _,target in sorted(PARENT_SLOTS)) ==
            PRIMARY_TARGETS, "Phase231 parent virtual order changed")
    require(tuple(row[0] for row in VPTR_WRITES["client"][-2:]) ==
            (PRIMARY_VPTR_WRITE_SITE, SECONDARY_VPTR_WRITE_SITE),
            "original constructor final primary/secondary writes changed")


def verify_original(client_bytes: bytes, server_bytes: bytes) -> dict:
    check_prior_phase_contract()
    for module, raw in (("client",client_bytes),("server",server_bytes)):
        require(hashlib.sha256(raw).hexdigest() == ORIGINALS[module]["sha256"],
                "entire 2006 original " + module + ".dll SHA-256 differs")
    pe = PEImage(client_bytes)
    require(pe.base == IMAGE_BASE, "original client PE32 image base differs")
    relocated = pe_highlow_relocations(pe)
    # Reuse the WHOLE-native-method hash checks and real HIGHLOW checks from
    # the original prior investigations, not merely their Python constants.
    check_initializer(pe, "client", relocated_sites=relocated)
    check_accessors(pe, "client", relocated_sites=relocated)
    check_handles(pe, relocations=relocated)
    check_parent_lifecycle(pe, relocated_sites=relocated)
    require(pe.section_of(FACTORY_VA) == ".text",
            "original factory not located in .text")
    verify_factory_thunk(pe.at(FACTORY_VA, 6), SECONDARY_VA)
    require(FACTORY_VA + 1 in relocated,
            "original factory embedded address lacks HIGHLOW relocation")
    for is_primary,site,vtable,offset,size in (
        (True, PRIMARY_VPTR_WRITE_SITE, PRIMARY_VTABLE_VA, 0, 6),
        (False, SECONDARY_VPTR_WRITE_SITE,
         SECONDARY_VTABLE_VA, SECONDARY_OFFSET, 10),
    ):
        require(pe.section_of(site) == ".text",
                "final constructor pointer write outside .text")
        verify_constructor_vptr(pe.at(site, size), offset, vtable, is_primary)
        require(site + (2 if is_primary else 6) in relocated,
                "original constructor pointer immediate lacks HIGHLOW relocation")
    for label, base, expected in (
        ("secondary", SECONDARY_VTABLE_VA, SECONDARY_TARGETS),
        ("primary", PRIMARY_VTABLE_VA, PRIMARY_TARGETS),
    ):
        require(pe.section_of(base) == ".rdata" and
                pe.section_of(base + 4*len(expected) - 1) == ".rdata",
                label + " vtable outside .rdata")
        verify_pointer_window(pe.at(base, 4*len(expected)), expected, label)
        for i,target in enumerate(expected):
            require(base+4*i in relocated,
                    label + " vtable missing relocation slot "+str(i))
            require(pe.section_of(target) == ".text",
                    label + " vtable target outside original .text")
    # The intervening DWORD at 0x242973B4 is NOT a tenth virtual.
    # Do not guess its RTTI role or contents without an independent audit.
    gap_va = SECONDARY_VTABLE_VA+4*SECONDARY_VIRTUAL_COUNT
    require(gap_va+NEXT_TABLE_GAP_BYTES == PRIMARY_VTABLE_VA and
            pe.section_of(gap_va) == ".rdata",
            "inter-table boundary differs")
    # The original singleton exists in the virtual/unbacked .data image;
    # reading its on-disk vptr would be an invalid fabricated runtime state.
    require(pe.section_of(PARENT_VA) == ".data" and
            pe.section_of(SECONDARY_VA) == ".data",
            "runtime primary and secondary objects not in image .data")
    return {
        "phase": 1656,
        "provenance": "both original retail DLLs matched full SHA-256",
        "original_interface": INTERFACE,
        "primary_object_va": hex(PARENT_VA),
        "factory_returns_secondary_va": hex(SECONDARY_VA),
        "secondary_this_adjustment_bytes": SECONDARY_OFFSET,
        "secondary_vtable_va": hex(SECONDARY_VTABLE_VA),
        "secondary_virtual_slots_verified": SECONDARY_VIRTUAL_COUNT,
        "secondary_slot_target_vas": [hex(x) for x in SECONDARY_TARGETS],
        "secondary_slot_ret_pop_bytes": list(CALLEE_STACK_POP_BYTES),
        "secondary_table_sha256": hashlib.sha256(
            pe.at(SECONDARY_VTABLE_VA, 4*SECONDARY_VIRTUAL_COUNT)
        ).hexdigest(),
        "separate_parent_vtable_va": hex(PRIMARY_VTABLE_VA),
        "separate_parent_virtual_slots_verified": PRIMARY_VIRTUAL_COUNT,
        "inter_table_gap_bytes": NEXT_TABLE_GAP_BYTES,
        "gap_dword_role_authenticated": False,
        "new_original_native_functions_recovered": 0,
        "original_native_methods_executed": 0,
        "independent_client_interface_registered": False,
        "retail_windows_startup_verified": False,
    }


def reject(callable_, label: str) -> None:
    try:
        callable_()
    except VerificationError:
        return
    raise AssertionError("corruption incorrectly accepted: " + label)


def self_test() -> None:
    check_prior_phase_contract()
    # Synthetic test values deliberately DIFFER from original game addresses.
    fake_secondary = tuple(0x401000 + i*0x1110 for i in range(9))
    fake_primary = tuple(0x601000 + i*0x120 for i in range(3))
    for label,values in (("secondary",fake_secondary),("primary",fake_primary)):
        window = expected_window(values)
        require(verify_pointer_window(window, values, label) == len(values),
                "synthetic table pass unexpectedly failed")
        reject(lambda:verify_pointer_window(window[:-4],values,label),
               label + " truncated")
        reject(lambda:verify_pointer_window(window+b"\x00\x00\x00\x00",
                                            values,label),label + " added slot")
        for i in range(len(window)):
            damaged = bytearray(window)
            damaged[i] ^= 0x01
            reject(lambda data=bytes(damaged):verify_pointer_window(
                data,values,label), label + " byte "+str(i))
        swapped = bytearray(window)
        swapped[:4],swapped[4:8] = swapped[4:8],swapped[:4]
        reject(lambda:verify_pointer_window(bytes(swapped),values,label),
               label + " swapped methods")
    fake_parent = 0x50000000
    fake_embedded = fake_parent + SECONDARY_OFFSET
    fake_factory = b"\xB8" + pack(fake_embedded) + b"\xC3"
    verify_factory_thunk(fake_factory, fake_embedded)
    reject(lambda:verify_factory_thunk(fake_factory, fake_parent),
           "factory returned primary instead of secondary")
    for i in range(len(fake_factory)):
        damaged = bytearray(fake_factory)
        damaged[i] ^= 1
        reject(lambda data=bytes(damaged):verify_factory_thunk(
            data, fake_embedded), "factory byte "+str(i))
    primary_store = b"\xC7\x06"+pack(0x12345000)
    secondary_store = b"\xC7\x86"+pack(SECONDARY_OFFSET)+pack(0x12346000)
    verify_constructor_vptr(primary_store,0,0x12345000,True)
    verify_constructor_vptr(secondary_store,SECONDARY_OFFSET,0x12346000,False)
    for is_primary,body,offset,vt in (
        (True,primary_store,0,0x12345000),
        (False,secondary_store,SECONDARY_OFFSET,0x12346000),
    ):
        for i in range(len(body)):
            damaged = bytearray(body)
            damaged[i] ^= 1
            reject(lambda data=bytes(damaged):verify_constructor_vptr(
                data,offset,vt,is_primary), "constructor byte "+str(i))
    reject(lambda:verify_original(b"synthetic-client",b"synthetic-server"),
           "forged original PE32 file hashes")
    print("PASS Phase1656 original-free VClientEntityList003: independent "
          "Phase226/228/229/230/231 cross-contract, exact nine secondary "
          "virtuals, separate three-slot parent, +0x10014 this-adjustment, "
          "RET4/RET0 inventory, corrupt-every-pointer/constructor/factory "
          "fixture and fake original-SHA refusal")


def main() -> int:
    p = argparse.ArgumentParser(description=__doc__)
    mode = p.add_mutually_exclusive_group(required=True)
    mode.add_argument("--self-test", action="store_true")
    mode.add_argument("--client", type=Path,
                      help="owner-private unchanged 2006 client.dll")
    p.add_argument("--server", type=Path,
                   help="owner-private unchanged 2006 server.dll")
    args = p.parse_args()
    if args.self_test:
        self_test()
        return 0
    if args.client is None or args.server is None:
        p.error("--client and --server are both required")
    report = verify_original(args.client.read_bytes(), args.server.read_bytes())
    print(json.dumps(report,indent=2,sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
