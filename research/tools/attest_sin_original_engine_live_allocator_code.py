#!/usr/bin/env python3
"""Original-free verifier for a few SHA-grounded *loaded* engine.dll opcodes.

The original 2006 PE32 engine was separately authenticated in the original
decomp/edict retire/reuse audits. This module checks read-only remote code
witnesses *in addition* to that file hash. It cannot prove the game cannot
patch its code immediately afterward, nor is it a physical edict owner pin.

Live consumer: observe_sin_original_engine_edict_owner.Win32Reader, which
opens only the explicitly selected original game PID with read/query rights.
Self-test: invented byte arrays and no commercial executable or process.
"""
from dataclasses import dataclass
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys

from audit_sin_original_engine_gameents_consumer import (
    ENGINE_SHA, IMAGE_BASE, PE32)

# Original x86 instruction VA, opcode prefix, optional absolute operand VA.
# Short windows independently SHA-backed by:
#   audit_sin_original_engine_gameents_edict_reuse.py
#   audit_sin_original_engine_gameents_edict_retire.py
# Each absolute 32-bit operand is relocated by Windows when the module moves.
# No instruction body, game function or pointer is followed or executed.
@dataclass(frozen=True)
class Witness:
    name: str
    va: int
    opcode: bytes
    target_va: int = 0

    @property
    def length(self):
        return len(self.opcode) + (4 if self.target_va else 0)


WITNESSES = (
    Witness("select_protected_last", 0x20110ECC, bytes.fromhex("a1"),
            0x2079DA64),
    Witness("select_edict_base", 0x20110ED1, bytes.fromhex("8b15"),
            0x2079DB04),
    Witness("select_used_count", 0x20110EDE, bytes.fromhex("8b15"),
            0x2079DAFC),
    Witness("select_capacity", 0x20110F36, bytes.fromhex("a1"),
            0x2079DB00),
    Witness("reuse_subtract_base", 0x20110FBB, bytes.fromhex("2b0d"),
            0x2079DB04),
    Witness("index_compare_capacity", 0x20111024, bytes.fromhex("3b35"),
            0x2079DB00),
    Witness("reuse_free_callback_vslot_0c", 0x20110FE8,
            bytes.fromhex("ff520c")),
    Witness("reuse_set_active_bit", 0x20110FEB,
            bytes.fromhex("830e02")),
    Witness("reuse_increment_serial", 0x20111000,
            bytes.fromhex("83460401")),
    Witness("reuse_store_float_timestamp", 0x20111004,
            bytes.fromhex("d95e10")),
)
GLOBAL_VAS = (0x2079DA64, 0x2079DAFC, 0x2079DB00, 0x2079DB04)
MIN_IMAGE_SIZE = 1 + max(
    [va - IMAGE_BASE + 3 for va in GLOBAL_VAS]
    + [w.va - IMAGE_BASE + w.length - 1 for w in WITNESSES]
)


@dataclass(frozen=True)
class CodeAttestation:
    accepted: bool
    code_windows_checked: int
    failed_witness: str
    status: str

    def summary(self):
        return (f"{self.status}: {self.failed_witness} "
                f"(checked={self.code_windows_checked})")


def rebase(original_va, loaded_module_base):
    """Refuse any PE32 address overflow, not merely negative values."""
    address = loaded_module_base + original_va - IMAGE_BASE
    if not (0 < address <= 0xFFFFFFFF):
        raise ValueError("mapped original PE32 VA outside 32-bit address space")
    return address


def expected_instruction(witness, loaded_module_base):
    raw = witness.opcode
    if witness.target_va:
        raw += rebase(witness.target_va, loaded_module_base).to_bytes(
            4, "little")
    return raw


def verify_loaded_code(read_bytes, module_base, module_size):
    """Compare small remote instruction windows twice; NEVER execute them.

    read_bytes(address, number_of_bytes) may only be the Win32Reader's
    PROCESS_VM_READ-only callback in live mode, or a fake fixture in CI.
    Returns error *labels*, not original instruction bytes or pointers.
    """
    if not callable(read_bytes):
        return CodeAttestation(False, 0, "reader", "invalid_reader")
    if (not isinstance(module_base, int) or
            not isinstance(module_size, int) or
            module_base <= 0 or module_base > 0xFFFFFFFF or
            module_size < MIN_IMAGE_SIZE or
            module_size > 0x100000000 - module_base):
        return CodeAttestation(False, 0, "module", "invalid_image_mapping")
    checked = 0
    for attempt in range(2):
        for witness in WITNESSES:
            try:
                address = rebase(witness.va, module_base)
                end = address + witness.length
                if end > module_base + module_size or end > 0x100000000:
                    return CodeAttestation(
                        False, checked, witness.name, "outside_image")
                actual = read_bytes(address, witness.length)
                if (not isinstance(actual, (bytes, bytearray)) or
                        len(actual) != witness.length):
                    return CodeAttestation(
                        False, checked, witness.name, "short_code_read")
                if actual != expected_instruction(witness, module_base):
                    return CodeAttestation(
                        False, checked, witness.name, "code_witness_mismatch")
            except (OSError, ValueError, OverflowError):
                return CodeAttestation(
                    False, checked, witness.name, "unreadable_code")
            checked += 1
    return CodeAttestation(True, checked, "", "double_checked_unpinned")



def parse_highlow_relocations(directory):
    """Parse independent PE32 .reloc bytes; never load or execute the DLL."""
    if not isinstance(directory, (bytes, bytearray)) or len(directory) < 8:
        raise ValueError("truncated original PE32 relocation directory")
    entries = set()
    offset = 0
    while offset < len(directory):
        if len(directory) - offset < 8:
            raise ValueError("truncated relocation block header")
        page, block_size = struct.unpack_from("<II", directory, offset)
        if (page & 0xFFF or block_size < 8 or block_size & 1 or
                offset + block_size > len(directory)):
            raise ValueError("invalid original PE32 relocation block")
        for pos in range(offset + 8, offset + block_size, 2):
            encoded = struct.unpack_from("<H", directory, pos)[0]
            reloc_type = encoded >> 12
            if reloc_type == 0:  # IMAGE_REL_BASED_ABSOLUTE padding
                continue
            if reloc_type != 3:  # IMAGE_REL_BASED_HIGHLOW
                raise ValueError("unexpected PE32 relocation kind")
            rva = page + (encoded & 0xFFF)
            if rva > 0xFFFFFFFF - 4 or rva in entries:
                raise ValueError("duplicate or overflowing HIGHLOW relocation")
            entries.add(rva)
        offset += block_size
    return entries


def require_all_allocator_relocations(entries):
    """Require relocations at the *operand* RVAs, not opcode RVAs."""
    expected = {
        w.va - IMAGE_BASE + len(w.opcode)
        for w in WITNESSES if w.target_va
    }
    if len(expected) != 6 or not expected.issubset(entries):
        raise ValueError("original allocator global operand lacks HIGHLOW relocation")
    return len(expected)


def audit_original_private_engine(data):
    """Owner-only: authenticate actual original PE32 file; return metadata."""
    if hashlib.sha256(data).hexdigest() != ENGINE_SHA:
        raise ValueError("original 2006 engine.dll whole-file SHA mismatch")
    pe = PE32(data)
    if pe.base != IMAGE_BASE:
        raise ValueError("wrong original 2006 engine image base")
    original_size = struct.unpack_from(
        "<I", data, struct.unpack_from("<I", data, 0x3c)[0] + 24 + 56)[0]
    if original_size < MIN_IMAGE_SIZE:
        raise ValueError("original image cannot contain allocator globals")
    pe_offset = struct.unpack_from("<I", data, 0x3c)[0]
    opt_start = pe_offset + 24
    opt_size = struct.unpack_from("<H", data, pe_offset + 20)[0]
    dir_offset = opt_start + 96 + 5 * 8  # IMAGE_DIRECTORY_ENTRY_BASERELOC
    if opt_size < 96 + 6 * 8 or dir_offset + 8 > len(data):
        raise ValueError("original relocation directory entry unavailable")
    relocation_rva, relocation_size = struct.unpack_from(
        "<II", data, dir_offset)
    if not relocation_rva or not 8 <= relocation_size <= 4 * 1024 * 1024:
        raise ValueError("original PE32 relocation directory missing")
    relocation_bytes = pe.raw(IMAGE_BASE + relocation_rva, relocation_size)
    entries = parse_highlow_relocations(relocation_bytes)
    relocated_operands = require_all_allocator_relocations(entries)
    for witness in WITNESSES:
        if pe.raw(witness.va, witness.length) != expected_instruction(
                witness, IMAGE_BASE):
            raise ValueError("original on-disk allocator instruction mismatch: "
                             + witness.name)
    return {
        "original_engine_sha256_verified": True,
        "original_pe32_image_base": hex(IMAGE_BASE),
        "original_pe32_image_size": hex(original_size),
        "original_highlow_relocations_count": len(entries),
        "allocator_absolute_operand_relocations_verified": relocated_operands,
        "original_allocator_code_witnesses_verified": len(WITNESSES),
        "owner_private_image_read_only": True,
        "original_executable_bytes_exported": False,
        "live_retail_process_attestation_verified": False,
        "continuous_owner_allocation_pin_verified": False,
    }


def exercise_synthetic_relocation_directory():
    expected = sorted({
        w.va - IMAGE_BASE + len(w.opcode)
        for w in WITNESSES if w.target_va
    })
    pages = {}
    for value in expected:
        pages.setdefault(value & ~0xFFF, []).append(value & 0xFFF)
    blocks = []
    for page, low_offsets in sorted(pages.items()):
        words = [(3 << 12) | x for x in low_offsets]
        if len(words) & 1:
            words.append(0)
        blocks.append(
            struct.pack("<II", page, 8 + len(words) * 2) +
            b"".join(struct.pack("<H", x) for x in words))
    synthetic = b"".join(blocks)
    actual = parse_highlow_relocations(synthetic)
    assert set(expected) == actual
    assert require_all_allocator_relocations(actual) == 6
    for target in expected:
        try:
            require_all_allocator_relocations(actual - {target})
        except ValueError:
            pass
        else:
            raise AssertionError("missing allocator relocation accepted")
    for corrupt in (
        b"", synthetic[:7], synthetic[:-1],
        b"\x00" * len(synthetic),
        synthetic[:4] + b"\x07\x00\x00\x00" + synthetic[8:],
    ):
        try:
            parse_highlow_relocations(corrupt)
        except ValueError:
            pass
        else:
            raise AssertionError("truncated/invalid relocation accepted")
    # Unsupported relocation type must refuse, rather than silently
    # claiming a LOW/HIGH/other relocation can rebase a full DWORD.
    wrong_type = bytearray(synthetic)
    initial = struct.unpack_from("<H", wrong_type, 8)[0]
    struct.pack_into("<H", wrong_type, 8, (2 << 12) | (initial & 0xFFF))
    try:
        parse_highlow_relocations(wrong_type)
    except ValueError:
        pass
    else:
        raise AssertionError("non-HIGHLOW relocation accepted")
    # A duplicate original PE32 operand relocation is ambiguous.
    duplicate = synthetic + synthetic[:struct.unpack_from("<I", synthetic, 4)[0]]
    try:
        parse_highlow_relocations(duplicate)
    except ValueError:
        pass
    else:
        raise AssertionError("duplicate HIGHLOW relocation accepted")
    try:
        audit_original_private_engine(b"SYNTHETIC not game code")
    except ValueError:
        pass
    else:
        raise AssertionError("fabricated game binary accepted")


def self_test():
    assert len(ENGINE_SHA) == 64
    assert IMAGE_BASE == 0x20000000
    assert len(WITNESSES) == 10
    exercise_synthetic_relocation_directory()
    assert GLOBAL_VAS == (
        0x2079DA64, 0x2079DAFC, 0x2079DB00, 0x2079DB04)
    assert MIN_IMAGE_SIZE > 0x79DB04
    for loaded in (IMAGE_BASE, 0x25000000, 0x10000000):
        size = 0x900000
        memory = {
            rebase(w.va, loaded): expected_instruction(w, loaded)
            for w in WITNESSES
        }
        seen = []
        def read(addr, n):
            seen.append((addr, n))
            return memory[addr][:n]
        good = verify_loaded_code(read, loaded, size)
        assert good == CodeAttestation(
            True, 2 * len(WITNESSES), "", "double_checked_unpinned")
        assert len(seen) == 2 * len(WITNESSES)
        assert all(n <= 6 for _, n in seen)
        assert sum(n for _, n in seen) < 120
        # The relocation adjustment is necessary for every absolute
        # global operand and must reject original-base-only hardcoding.
        if loaded != IMAGE_BASE:
            for w in WITNESSES:
                if not w.target_va:
                    continue
                actual = memory[rebase(w.va, loaded)]
                assert actual != w.opcode + w.target_va.to_bytes(4, "little")

        for w in WITNESSES:
            original = memory[rebase(w.va, loaded)]
            memory[rebase(w.va, loaded)] = (
                bytes([original[0] ^ 0x01]) + original[1:])
            fail = verify_loaded_code(read, loaded, size)
            assert fail.status == "code_witness_mismatch"
            assert fail.failed_witness == w.name
            memory[rebase(w.va, loaded)] = original

        # Tamper the 32-bit target while retaining the genuine opcode.
        for w in WITNESSES:
            if not w.target_va:
                continue
            original = memory[rebase(w.va, loaded)]
            memory[rebase(w.va, loaded)] = (
                original[:-1] + bytes([original[-1] ^ 1]))
            fail = verify_loaded_code(read, loaded, size)
            assert fail.status == "code_witness_mismatch"
            assert fail.failed_witness == w.name
            memory[rebase(w.va, loaded)] = original

        # Separate passes: code can change after the first apparent
        # success, so no successful result may be inferred from a single
        # pass. No claim is made about ABA between both passes.
        first_witness = WITNESSES[0]
        address = rebase(first_witness.va, loaded)
        calls = [0]
        def changed_second_pass(addr, n):
            if addr == address:
                calls[0] += 1
                if calls[0] == 2:
                    return bytes([memory[addr][0] ^ 1]) + memory[addr][1:]
            return memory[addr]
        assert verify_loaded_code(
            changed_second_pass, loaded, size).status == (
                "code_witness_mismatch")
        assert verify_loaded_code(
            lambda addr, n: memory[addr][:-1],
            loaded, size).status == "short_code_read"
        assert verify_loaded_code(
            lambda *_: (_ for _ in ()).throw(OSError("gone")),
            loaded, size).status == "unreadable_code"
        assert verify_loaded_code(
            None, loaded, size).status == "invalid_reader"
        assert verify_loaded_code(
            read, loaded, MIN_IMAGE_SIZE - 1).status == (
            "invalid_image_mapping")
        assert verify_loaded_code(
            read, 0xFFFFF000, size).status == (
            "invalid_image_mapping")
        assert verify_loaded_code(
            read, 0, size).status == "invalid_image_mapping"
    print("PASS original-free 2006 loaded allocator-code attestation: 10 SHA-backed x86 witnesses, six HIGHLOW relocation operand entries, in-memory rebasing, double-scan, tampered opcodes/operands, truncated/failed reads and invalid mapping refusal")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    modes = parser.add_mutually_exclusive_group(required=True)
    modes.add_argument("--self-test", action="store_true")
    modes.add_argument("--engine", type=Path,
                       help="owner-private original 2006 engine.dll; audit only")
    args = parser.parse_args()
    if args.self_test:
        self_test()
        return 0
    print(json.dumps(audit_original_private_engine(
        args.engine.read_bytes()), indent=2, sort_keys=True))
    return 0


if __name__ == "__main__":
    sys.exit(main())
