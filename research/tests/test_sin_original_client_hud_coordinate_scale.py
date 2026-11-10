"""Synthetic original-free x86 evidence tests for Phase156 HUD scale helper."""
import pathlib
import struct
import sys
import unittest

sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/"tools"))
from audit_sin_original_client_hud_coordinate_scale import (
    REGIONS, OPCODES, CALLS, HUD_SCALE_CALLS, REFERENCE_CANVAS,
    PANEL_CALLS, verify_original_hud_scale, VerificationError, audit,
)

class FakePE:
    base=0x24000000
    def __init__(self):self.memory={}
    def put(self,va,raw):
        for i,byte in enumerate(raw):
            if va+i in self.memory and self.memory[va+i]!=byte:
                raise AssertionError("x86 witness fixture conflict at %#x"%(va+i))
            self.memory[va+i]=byte
    def at(self,va,n):
        return bytes(self.memory.get(va+i,0xCC) for i in range(n))
    def corrupt(self,va):
        self.memory[va]^=0x10

def fixture():
    p=FakePE()
    for va,hexcode in OPCODES.items():
        p.put(va,bytes.fromhex(hexcode))
    for va,target in CALLS.items():
        p.put(va,b"\xe8"+struct.pack("<i",target-va-5))
    for va,value in REFERENCE_CANVAS.items():
        p.put(va,struct.pack("<f",value))
    return p

class HudCoordinateScaleTests(unittest.TestCase):
    def test_all_hud_call_sites_and_canvas_constants_are_crosswalked(self):
        self.assertEqual(len(HUD_SCALE_CALLS),12)
        self.assertEqual(len(CALLS),13)
        self.assertEqual(len(set(HUD_SCALE_CALLS)),len(HUD_SCALE_CALLS))
        self.assertEqual(set(PANEL_CALLS.values()).issubset(HUD_SCALE_CALLS),True)
        self.assertEqual(REFERENCE_CANVAS[0x2429B69C],1600.0)
        self.assertEqual(REFERENCE_CANVAS[0x2429B6A0],1200.0)
        report=verify_original_hud_scale(fixture(),check_hashes=False)
        self.assertEqual(report["verified_full_function_regions"],0)
        self.assertEqual(report["all_original_hud_scaler_direct_calls"],12)
        self.assertEqual(report["verified_instruction_anchors"],len(OPCODES))
        self.assertEqual(report["verified_direct_calls"],13)

    def test_mutate_every_original_scaler_instruction_anchor(self):
        for va in OPCODES:
            with self.subTest(va=hex(va)):
                p=fixture()
                p.corrupt(va)
                with self.assertRaises(VerificationError):
                    verify_original_hud_scale(p,check_hashes=False)

    def test_mutate_every_original_call_edge(self):
        for va in CALLS:
            with self.subTest(va=hex(va)):
                p=fixture()
                p.corrupt(va+1)
                with self.assertRaises(VerificationError):
                    verify_original_hud_scale(p,check_hashes=False)

    def test_mutate_both_original_reference_canvas_constants(self):
        for va in REFERENCE_CANVAS:
            with self.subTest(va=hex(va)):
                p=fixture()
                p.corrupt(va)
                with self.assertRaises(VerificationError):
                    verify_original_hud_scale(p,check_hashes=False)

    def test_complete_function_hashes_not_satisfied_by_synthetic_fixture(self):
        self.assertEqual(len(REGIONS),3)
        for name,(va,n,digest) in REGIONS.items():
            with self.subTest(name=name):
                self.assertGreater(n,0)
                self.assertEqual(len(bytes.fromhex(digest)),32)
        with self.assertRaisesRegex(VerificationError,"full SHA256 mismatch"):
            verify_original_hud_scale(fixture(),check_hashes=True)
        with self.assertRaises((ValueError,VerificationError)):
            audit(b"not original client",b"not original server")

    def test_wrong_image_base_rejected(self):
        p=fixture()
        p.base=0x22000000
        with self.assertRaisesRegex(VerificationError,"image base"):
            verify_original_hud_scale(p,check_hashes=False)

if __name__=="__main__":
    unittest.main()
