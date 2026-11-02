"""Synthetic PE32 regression tests for original SiN CGrenadeAR Detonate evidence."""
from __future__ import annotations

import pathlib
import struct
import sys
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1] / "tools"))

from audit_sin_ar_detonation import (
    audit, WITNESSES, CALLS, FLOATS, DOUBLES, ORIGINAL_STRINGS,
    VerificationError,
)
from test_gameplay_entity_registry import fake_registered_server

BASE = 0x22000000


def synthetic_detonation_pe() -> bytes:
    data = bytearray(fake_registered_server())

    def put(address: int, value: bytes) -> None:
        start = address - BASE
        data[start:start + len(value)] = value

    for address, raw in WITNESSES.items():
        put(address, raw)
    for site, destination in CALLS.items():
        put(site, b"\xE8" + struct.pack("<i", destination - site - 5))
    for address, value in FLOATS.items():
        put(address, struct.pack("<f", value))
    for address, value in DOUBLES.items():
        put(address, struct.pack("<d", value))
    for address, text in ORIGINAL_STRINGS.items():
        put(address, text)
    return bytes(data)


class GrenadeDetonationTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.fake = synthetic_detonation_pe()

    def test_original_free_fixture_recovers_reproducible_graph(self):
        result = audit(self.fake, check_hash=False)
        self.assertEqual(len(result["verified_original_direct_calls"]), len(CALLS))
        self.assertEqual(result["method"]["virtual_slot"], 250)
        self.assertEqual(result["state"]["think_requeue_seconds"],
                         struct.unpack("<f", struct.pack("<f", .05))[0])
        self.assertEqual(result["state"]["think_time_comparison_delta_seconds"],
                         struct.unpack("<f", struct.pack("<f", .1))[0])
        self.assertEqual(result["effects"]["decals"], ["Scorch", "SmallScorch"])
        self.assertEqual(result["effects"]["trace_vector_scale"], 60.0)
        self.assertEqual(result["effects"]["feedback_helper_original_amplitude_clamp"], 16.0)
        self.assertEqual(result["effects"]["feedback_call_literal_float_args"],
                         [750.0, 1.0, 150.0, 25.0])
        self.assertEqual(result["state"]["evolving_field_increment"], 15.0)
        self.assertEqual(result["state"]["original_initial_damage_candidate"]["value"], 100.0)
        self.assertEqual(result["state"]["original_initial_blast_radius_candidate"]["value"], 250.0)
        self.assertEqual(result["state"]["blast_damage_type_bitmask"], "0x40")
        self.assertEqual(result["state"]["evolving_field_upper_comparison"], 300.0)

    def test_strict_binary_sha_rejects_synthetic_fixtures(self):
        with self.assertRaisesRegex(VerificationError, "SHA-256"):
            audit(self.fake)

    def test_modified_detonation_flag_guard_rejected(self):
        corrupted = bytearray(self.fake)
        corrupted[0x22314579-BASE] = 0x90
        with self.assertRaisesRegex(VerificationError, "instruction mismatch"):
            audit(bytes(corrupted), check_hash=False)

    def test_modified_direct_call_destination_rejected(self):
        corrupted = bytearray(self.fake)
        corrupted[0x223148FB-BASE+1] ^= 0x10
        with self.assertRaisesRegex(VerificationError, "wrong original call target"):
            audit(bytes(corrupted), check_hash=False)

    def test_modified_decal_or_effect_float_rejected(self):
        corrupted = bytearray(self.fake)
        corrupted[0x22407CB0-BASE] = ord("X")
        with self.assertRaisesRegex(VerificationError, "decal literal"):
            audit(bytes(corrupted), check_hash=False)
        corrupted = bytearray(self.fake)
        corrupted[0x223DDDCC-BASE] ^= 0x10
        with self.assertRaisesRegex(VerificationError, "float evidence"):
            audit(bytes(corrupted), check_hash=False)

    def test_corrupted_double_threshold_increment_rejected(self):
        corrupted = bytearray(self.fake)
        corrupted[0x22467448-BASE] ^= 0x40
        with self.assertRaisesRegex(VerificationError, "double evidence"):
            audit(bytes(corrupted), check_hash=False)

    def test_byte_guard_for_both_virtual_detonation_call_sites(self):
        for address in (0x2231434A, 0x22314444):
            corrupted = bytearray(self.fake)
            corrupted[address-BASE+2] ^= 0x10
            with self.assertRaisesRegex(VerificationError, "instruction mismatch"):
                audit(bytes(corrupted), check_hash=False)


if __name__ == "__main__":
    unittest.main()
