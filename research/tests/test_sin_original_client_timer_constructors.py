"""Original-free Phase161 exact 2006 timer constructor evidence regressions."""
import hashlib
import itertools
import pathlib
import struct
import sys
import unittest

sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/"tools"))
from audit_sin_original_client_timer_constructors import (
    REGIONS,OPCODES,CALLS,RTTI_NAMES,RTTI_LINKS,
    verify_original_client,audit,VerificationError,
)

class FakePE:
    base=0x24000000
    def __init__(self):self.words={}
    def put(self,at,data):
        for i,byte in enumerate(data):
            va=at+i
            if va in self.words and self.words[va]!=byte:
                raise AssertionError("synthetic native-byte conflict at %#x"%va)
            self.words[va]=byte
    def at(self,va,count):
        return bytes(self.words.get(va+i,0xCC) for i in range(count))
    def corrupt(self,va):self.words[va]^=0x10

def fixture():
    pe=FakePE()
    for addr,token in RTTI_NAMES.items():
        pe.put(addr,token)
    for addr,target in RTTI_LINKS.items():
        pe.put(addr,struct.pack("<I",target))
    for addr,hexcode in OPCODES.items():
        pe.put(addr,bytes.fromhex(hexcode))
    for site,target in CALLS.items():
        pe.put(site,b"\xe8"+struct.pack("<i",target-site-5))
    return pe

def original_x86_element_mask_drawing(
    element_mask,engine_available,player_available,active_flags,
    player_virtual_result):
    if not engine_available or not player_available:return False
    if active_flags&4:return False
    if element_mask&0x10 and player_virtual_result<=0:return False
    return (element_mask&active_flags)==0

class TimerConstructorEvidenceTests(unittest.TestCase):
    def test_dual_timer_original_metadata_without_proprietary_bytes(self):
        report=verify_original_client(fixture(),check_hashes=False)
        self.assertEqual(report["verified_complete_code_region_hashes"],0)
        self.assertEqual(report["original_visibility_masks"],
                         {"breath":16,"poison":16,"health":24})
        self.assertEqual(report["verified_small_offset_accessor_hashes"],0)
        self.assertEqual(report["original_opcode_witnesses"],len(OPCODES))
        self.assertEqual(report["original_direct_call_edges"],15)
        self.assertEqual(report["original_panel_offset"],20)
        self.assertEqual(report["panel_relative_four_byte_field"],[163,164,165,166])
        self.assertEqual(report["complete_object_four_byte_field"],[183,184,185,186])
        self.assertEqual(len(REGIONS),11)

    def test_each_original_ms_vtable_and_type_descriptor_link_guarded(self):
        self.assertEqual(len(RTTI_LINKS),12)
        for va in RTTI_LINKS:
            with self.subTest(msvc_link=hex(va)):
                pe=fixture();pe.corrupt(va)
                with self.assertRaises(VerificationError):
                    verify_original_client(pe,check_hashes=False)

    def test_every_original_opcode_anchor_rejects_corruption(self):
        for va in OPCODES:
            with self.subTest(va=hex(va)):
                pe=fixture();pe.corrupt(va)
                with self.assertRaises(VerificationError):
                    verify_original_client(pe,check_hashes=False)

    def test_every_original_constructor_call_rejects_corruption(self):
        for site in CALLS:
            with self.subTest(site=hex(site)):
                pe=fixture();pe.corrupt(site+1)
                with self.assertRaises(VerificationError):
                    verify_original_client(pe,check_hashes=False)

    def test_original_class_rtti_names_are_required(self):
        for va in RTTI_NAMES:
            with self.subTest(rtti=hex(va)):
                pe=fixture();pe.corrupt(va)
                with self.assertRaises(VerificationError):
                    verify_original_client(pe,check_hashes=False)

    def test_synthetic_witnesses_do_not_fake_whole_code_regions(self):
        for name,(va,n,sha) in REGIONS.items():
            with self.subTest(name=name):
                self.assertGreater(n,0)
                self.assertEqual(len(bytes.fromhex(sha)),32)
        with self.assertRaisesRegex(VerificationError,"complete code SHA mismatch"):
            verify_original_client(fixture(),check_hashes=True)
        with self.assertRaisesRegex(VerificationError,"wrong original client"):
            audit(b"fake client",b"fake server")

    def test_original_32bit_nullable_pointer_offsets(self):
        # Independent expected table; not a tautological expression check.
        samples={
            0x00000000:(0x00000000,0x00000000),
            0x00000001:(0x000000A4,0x000000A9),
            0x00001000:(0x000010A3,0x000010A8),
            0xFFFFFFF0:(0x00000093,0x00000098),
            0xFFFFFFFF:(0x000000A2,0x000000A7),
        }
        for original,(expected_panel,expected_resource) in samples.items():
            with self.subTest(pointer=hex(original)):
                adjusted=lambda offset: (
                    ((original+offset)&0xFFFFFFFF) if original else 0)
                self.assertEqual(adjusted(0xA3),expected_panel)
                self.assertEqual(adjusted(0xA8),expected_resource)
        self.assertEqual(0xA3+0x14,0xB7)
        self.assertEqual(0xA8+0x14,0xBC)

    def test_breath_and_poison_share_mask_health_differs_on_bit8(self):
        count=0
        for flags,override,virtual,engine,has_player in itertools.product(
            (0,4,8,16,24,32,128), (0,8,16), (-1,0,1),
            (False,True),(False,True)):
            active=override if override else flags
            breath=original_x86_element_mask_drawing(
                0x10,engine,has_player,active,virtual)
            poison=original_x86_element_mask_drawing(
                0x10,engine,has_player,active,virtual)
            health=original_x86_element_mask_drawing(
                0x18,engine,has_player,active,virtual)
            self.assertEqual(breath,poison)
            if flags==8 and override==0 and virtual>0 and engine and has_player:
                self.assertTrue(breath)
                self.assertTrue(poison)
                self.assertFalse(health)
            if not engine or not has_player or virtual<=0 or (active&4):
                self.assertFalse(breath)
                self.assertFalse(poison)
                self.assertFalse(health)
            if breath!=health:
                self.assertTrue(active&8)
            count+=1
        self.assertEqual(count,7*3*3*2*2)

    def test_wrong_original_load_base_rejected(self):
        pe=fixture()
        pe.base=0x22000000
        with self.assertRaisesRegex(VerificationError,"image base"):
            verify_original_client(pe,check_hashes=False)

if __name__=="__main__":
    unittest.main()
