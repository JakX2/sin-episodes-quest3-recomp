"""Original-commercial-binary-free regression for SiN grenade NPC sound events."""
from __future__ import annotations
import pathlib
import struct
import sys
import unittest

sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/"tools"))
from audit_sin_grenade_ai_sounds import (
    audit,EvidenceError,INSTRUCTIONS,CALLS,FLOATS,DOUBLES,
)
from test_gameplay_entity_registry import fake_registered_server

BASE=0x22000000

def fake_sin_sound_binary()->bytes:
    b=bytearray(fake_registered_server())
    def set_va(va:int,value:bytes)->None:
        offset=va-BASE
        b[offset:offset+len(value)]=value
    for va,bytes_ in INSTRUCTIONS.items():set_va(va,bytes_)
    for site,target in CALLS.items():
        set_va(site,b"\xe8"+struct.pack("<i",target-site-5))
    for va,value in FLOATS.items():set_va(va,struct.pack("<f",value))
    for va,value in DOUBLES.items():set_va(va,struct.pack("<d",value))
    return bytes(b)

class OriginalGrenadeAiSoundTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.fake=fake_sin_sound_binary()

    def test_both_events_have_distinct_type_and_duration(self):
        result=audit(self.fake,check_hash=False)
        self.assertEqual(result["evidence"]["x86_instruction_sites"],20)
        self.assertEqual(result["evidence"]["verified_sound_insert_CALLs"],2)
        warning=result["grenade_flight_AI_warning"]
        explosion=result["blast_AI_sound_after_radius_damage"]
        self.assertEqual(warning["type_bits"],"0x00000008")
        self.assertEqual(warning["initial_radius"],100.0)
        self.assertEqual(warning["conditional_step"],15.0)
        self.assertEqual(explosion["type_bits"],"0x02000001")
        self.assertEqual(explosion["event_duration"],1.0)
        self.assertEqual(explosion["minimum_radius"],128.0)
        self.assertEqual(explosion["radius_multiplier"],1.5)

    def test_expected_original_sha_is_required(self):
        with self.assertRaisesRegex(EvidenceError,"SHA-256"):
            audit(self.fake)

    def test_wrong_flight_sound_call_target_is_rejected(self):
        b=bytearray(self.fake)
        b[0x22314559-BASE+1]^=0x10
        with self.assertRaisesRegex(EvidenceError,"CALL mismatch"):
            audit(bytes(b),check_hash=False)

    def test_wrong_blast_sound_call_opcode_is_rejected(self):
        b=bytearray(self.fake)
        b[0x222A5EC4-BASE]=0x90
        with self.assertRaisesRegex(EvidenceError,"CALL mismatch"):
            audit(bytes(b),check_hash=False)

    def test_invalid_sound_type_is_rejected(self):
        b=bytearray(self.fake)
        b[0x22314558-BASE]=0x1
        with self.assertRaisesRegex(EvidenceError,"instruction mismatch"):
            audit(bytes(b),check_hash=False)

    def test_original_specific_duration_is_not_replaceable_with_sdk_default(self):
        b=bytearray(self.fake)
        b[0x222A5EA5-BASE]^=0x10
        with self.assertRaisesRegex(EvidenceError,"instruction mismatch"):
            audit(bytes(b),check_hash=False)

    def test_original_radius_scale_and_minimum_are_verified(self):
        for va in (0x223DE5D0,0x224562D8):
            with self.subTest(va=hex(va)):
                b=bytearray(self.fake)
                b[va-BASE+6]^=0x20
                with self.assertRaisesRegex(EvidenceError,"double mismatch"):
                    audit(bytes(b),check_hash=False)

    def test_client_string_check_never_invents_network_table(self):
        result=audit(self.fake,client=self.fake,check_hash=False)
        self.assertFalse(result["client_string_crosscheck"]["DT_GrenadeAR_name_present"])
        self.assertIn("Strings alone",result["client_string_crosscheck"]["warning"])

if __name__=="__main__":
    unittest.main()
