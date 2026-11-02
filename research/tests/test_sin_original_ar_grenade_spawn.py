"""Original-free synthetic x86 direct-call and SiN AR grenade state tests."""
from __future__ import annotations

import pathlib
import struct
import sys
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1] / "tools"))
from audit_sin_original_ar_grenade_spawn import (
    SPAWN_CALL_EDGES, FLOAT_LITERALS, audit, decode_call_target,
    VerificationError,
)
from test_gameplay_entity_registry import fake_registered_server

BASE = 0x22000000


def fake_armed_server() -> bytes:
    data = bytearray(fake_registered_server())
    for site, target in SPAWN_CALL_EDGES.items():
        relative = target - site - 5
        data[site-BASE:site-BASE+5] = b"\xE8" + struct.pack("<i", relative)
    def put(address: int, value: bytes):
        data[address-BASE:address-BASE+len(value)] = value
    put(0x22313E25, b"\x68" + struct.pack("<I", 0x22477230))
    put(0x22477230, b"grenade_ar\x00")
    put(0x2245F43C, b"ARNADE\x00")
    for address, value in FLOAT_LITERALS.items():
        put(address, struct.pack("<f", value))
    return bytes(data)


class AssaultGrenadeTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.blob = fake_armed_server()

    def test_structural_fake_matches_all_known_original_edges(self):
        result = audit(self.blob, check_hash=False)
        path = result["actual_projectile_path"]
        self.assertEqual(len(path["verified_direct_call_edges"]), 7)
        self.assertEqual(path["classname"], "grenade_ar")
        self.assertEqual(path["related_ammo_literal"], "ARNADE")
        self.assertEqual(path["original_float_constants"]["0x224771F0"], 2000.0)
        self.assertEqual(path["original_float_constants"]["0x223D9778"], 0.5)

    def test_wrong_instructions_fail_not_just_a_string_match(self):
        data = bytearray(self.blob)
        data[0x22313E2A-BASE] = 0x90
        with self.assertRaisesRegex(VerificationError, "expected x86"):
            audit(bytes(data), check_hash=False)

    def test_wrong_branch_target_fails(self):
        data = bytearray(self.blob)
        data[0x22146C48-BASE+1] ^= 0x10
        with self.assertRaisesRegex(VerificationError, "code edge"):
            audit(bytes(data), check_hash=False)

    def test_wrong_float_or_classname_rejected(self):
        data = bytearray(self.blob)
        data[0x224771F0-BASE] ^= 1
        with self.assertRaisesRegex(VerificationError, "numeric literal"):
            audit(bytes(data), check_hash=False)
        data = bytearray(self.blob)
        data[0x22477230-BASE] = ord("X")
        with self.assertRaisesRegex(VerificationError, "classname"):
            audit(bytes(data), check_hash=False)

    def test_unpinned_fake_rejected(self):
        with self.assertRaisesRegex(VerificationError, "SHA-256"):
            audit(self.blob)

    def test_asm_rel32_decoder_is_strict(self):
        code = b"\xE8" + struct.pack("<i", 0x22146C40 - 0x22313E2A - 5)
        self.assertEqual(decode_call_target(0x22313E2A, code), 0x22146C40)
        with self.assertRaisesRegex(VerificationError, "expected x86"):
            decode_call_target(0x22313E2A, b"\x90\x00\x00\x00\x00")


if __name__ == "__main__":
    unittest.main()
