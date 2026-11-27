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
import sys

from audit_sin_original_engine_gameents_consumer import ENGINE_SHA, IMAGE_BASE

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


def self_test():
    assert len(ENGINE_SHA) == 64
    assert IMAGE_BASE == 0x20000000
    assert len(WITNESSES) == 10
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
    print("PASS original-free 2006 loaded allocator-code attestation: 10 SHA-backed original x86 witnesses, in-memory PE32 relocation, double-scan, tampered opcodes/operands, truncated/failed reads and invalid mapping refusal")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-test", action="store_true", required=True)
    args = parser.parse_args()
    if args.self_test:
        self_test()
        return 0
    return 2


if __name__ == "__main__":
    sys.exit(main())
