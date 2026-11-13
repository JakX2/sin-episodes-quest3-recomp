#!/usr/bin/env python3
"""Original-free Phase197 corruption tests for SiN cache balancer evidence."""
from __future__ import annotations
import struct
import sys
import unittest
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from audit_sin_original_server_cache_balancers import (
    ENTRY, LENGTH, ADDRESS_OFFSETS, ADDRESS_DELTAS, CALL_TARGETS,
    WITNESSES, INCOMING, rel32, verify_native, audit, VerificationError,
)

class FakePE:
    base = 0x22000000
    def __init__(self) -> None:
        self.mem: dict[int, int] = {}
        for i, (va, static, _) in enumerate(ENTRY.values()):
            self.put(va, bytes([0x90]) * LENGTH)
            for off, word in WITNESSES.items():
                self.put(va + off, bytes.fromhex(word), replace=True)
            for off, delta in zip(ADDRESS_OFFSETS, ADDRESS_DELTAS):
                self.put(va + off, struct.pack("<I", static+delta), replace=True)
            for off, targets in CALL_TARGETS.items():
                self.put(va+off, rel32(va+off, targets[i]), replace=True)
            self.put(va+LENGTH, b"\xcc"*5)
        for src, dest in INCOMING.items():
            self.put(src, rel32(src,dest))
    def put(self, va:int, content:bytes, *, replace:bool=False) -> None:
        for offset, value in enumerate(content):
            pos=va+offset
            if pos in self.mem and not replace and self.mem[pos] != value:
                raise AssertionError("fixture overlap mismatch")
            self.mem[pos]=value
    def at(self, va:int, n:int) -> bytes:
        return bytes(self.mem.get(va+i,0) for i in range(n))
    def section(self, va:int) -> str:
        return ".data" if 0x22500000 <= va < 0x22600000 else ".text"
    def mutate(self, va:int) -> None:
        self.mem[va] = self.mem.get(va,0)^1

class Tests(unittest.TestCase):
    def test_artificial_witnesses_do_not_claim_original_functions(self):
        a=verify_native(FakePE(), check_hash=False)
        self.assertEqual(a["new_sha_verified_original_functions"],0)
        self.assertEqual(a["exact_static_address_operands"],170)
        self.assertEqual(a["exact_outgoing_direct_calls"],16)
        self.assertEqual(a["exact_incoming_direct_calls"],6)
        self.assertEqual(a["sparse_opcode_witnesses"],30)
    def test_mutate_all_sparse_witnesses(self):
        for va, _, _ in ENTRY.values():
            for off in WITNESSES:
                with self.subTest(va=va,off=off):
                    f=FakePE();f.mutate(va+off)
                    with self.assertRaises(VerificationError):
                        verify_native(f,check_hash=False)
    def test_mutate_all_170_absolute_operands(self):
        for va, _, _ in ENTRY.values():
            for off in ADDRESS_OFFSETS:
                with self.subTest(va=va,off=off):
                    f=FakePE();f.mutate(va+off)
                    with self.assertRaises(VerificationError):
                        verify_native(f,check_hash=False)
    def test_mutate_all_16_outgoing_calls(self):
        for va, _, _ in ENTRY.values():
            for off in CALL_TARGETS:
                with self.subTest(va=va,off=off):
                    f=FakePE();f.mutate(va+off+1)
                    with self.assertRaises(VerificationError):
                        verify_native(f,check_hash=False)
    def test_mutate_all_six_incoming_calls(self):
        for src in INCOMING:
            with self.subTest(src=src):
                f=FakePE();f.mutate(src+1)
                with self.assertRaises(VerificationError):
                    verify_native(f,check_hash=False)
    def test_reject_unwitnessed_code_divergence(self):
        va=ENTRY["scene"][0]
        f=FakePE();f.mutate(va+0x100)
        with self.assertRaises(VerificationError):
            verify_native(f,check_hash=False)
    def test_reject_bad_ret_and_boundary(self):
        for va,_,_ in ENTRY.values():
            f=FakePE();f.mutate(va+LENGTH)
            with self.assertRaises(VerificationError):
                verify_native(f,check_hash=False)
    def test_reject_nonoriginal_dll_hashes(self):
        with self.assertRaises(VerificationError):
            audit(b"MZ", b"MZ")
        with self.assertRaises(VerificationError):
            verify_native(FakePE(), check_hash=True)
    def test_reject_image_base_and_section_drift(self):
        f=FakePE();f.base=0x24000000
        with self.assertRaises(VerificationError):
            verify_native(f,check_hash=False)
        f=FakePE();f.section=lambda _: ".text"
        with self.assertRaises(VerificationError):
            verify_native(f,check_hash=False)

if __name__ == "__main__":
    unittest.main()
