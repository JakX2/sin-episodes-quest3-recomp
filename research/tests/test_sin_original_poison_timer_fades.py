"""Phase154: synthetic original-free x87 poison timer fade-path tests.

Tests real-byte audit tampering and equivalence of original FCOMP/FUCOMPP
status-flag branches to the independent portable C++ conditions, including
NaN/unordered, equal thresholds, ±infinity and status-byte boundaries.
"""
import itertools
import math
import pathlib
import struct
import sys
import unittest

sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/"tools"))
from audit_sin_original_poison_timer_fades import (
    OPCODES, SOUND_NAMES, CONST_QUARTER_VA, FULL_CODE_SHA256,
    verify_fade_branches, VerificationError, audit,
)

class FakeClient:
    base=0x24000000
    def __init__(self):
        self.mem={}
    def put(self,va,raw):
        for i,byte in enumerate(raw):
            key=va+i
            if key in self.mem and self.mem[key]!=byte:
                raise AssertionError("conflicting synthetic anchor at %#x"%key)
            self.mem[key]=byte
    def at(self,va,n):
        return bytes(self.mem.get(va+i,0xCC) for i in range(n))
    def corrupt(self,va):
        self.mem[va]^=0x10

def fixture():
    p=FakeClient()
    for address,blob in OPCODES.items():
        p.put(address,bytes.fromhex(blob))
    for address,name in SOUND_NAMES.items():
        p.put(address,name.encode("ascii")+b"\x00")
    p.put(CONST_QUARTER_VA,struct.pack("<f",0.25))
    return p

def original_x87_ah(a,b):
    if math.isnan(a) or math.isnan(b):return 0x45
    if a<b:return 0x01
    if a==b:return 0x40
    return 0

def parity_flag(ah):
    return (ah&0xFF).bit_count()%2==0

def original_machine_decision(player,old_hud,status_byte,has_player):
    """Interpret original TEST AH masks and JP/JNP/JNE branches literally."""
    if not has_player:
        return "skip"
    if not parity_flag(original_x87_ah(player,old_hud)&0x44):
        return "skip"
    if not parity_flag(original_x87_ah(player,1.0)&0x05):
        if status_byte==0:
            return "in"
    if not parity_flag(original_x87_ah(player,0.25)&0x05):
        return "none"
    if not parity_flag(original_x87_ah(player,0.0)&0x44):
        return "none"
    if original_x87_ah(player,1.0)&0x01:
        return "none"
    if status_byte!=255:
        return "none"
    return "out"

def readable_cpp_decision(player,old_hud,status_byte,has_player):
    if not has_player or player==old_hud:return "skip"
    if player<1.0:
        return "in" if status_byte==0 else "none"
    if not (player<0.25) and player!=0.0 and player>=1.0 and status_byte==255:
        return "out"
    return "none"

class Phase154PoisonTimerTests(unittest.TestCase):
    def test_synthetic_evidence(self):
        result=verify_fade_branches(fixture(),check_hashes=False)
        self.assertEqual(result["original_class"],"CHudPoisonTimer")
        self.assertEqual(result["verified_original_branch_opcode_sites"],len(OPCODES))
        self.assertEqual(result["comparison_thresholds"],[0.0,0.25,1.0])
        self.assertEqual(result["verified_complete_code_region_bytes"],0)
        self.assertEqual(len(bytes.fromhex(FULL_CODE_SHA256)),32)

    def test_mutate_every_original_branch_instruction(self):
        for va in OPCODES:
            with self.subTest(va=hex(va)):
                p=fixture()
                p.corrupt(va)
                with self.assertRaises(VerificationError):
                    verify_fade_branches(p,check_hashes=False)

    def test_mutate_each_sound_name_and_quarter_threshold(self):
        for va in list(SOUND_NAMES)+[CONST_QUARTER_VA]:
            with self.subTest(va=hex(va)):
                p=fixture()
                p.corrupt(va)
                with self.assertRaises(VerificationError):
                    verify_fade_branches(p,check_hashes=False)

    def test_complete_original_method_hash_required(self):
        with self.assertRaisesRegex(VerificationError,"203-byte update SHA256"):
            verify_fade_branches(fixture(),check_hashes=True)
        with self.assertRaises(VerificationError):
            audit(b"invalid original client.dll",b"invalid original server.dll")

    def test_original_x87_pf_zf_decision_tree_equals_cpp_model(self):
        samples=[-math.inf,-1.0,-0.0,0.0,0.2,0.25,0.5,0.75,1.0,1.25,
                 math.inf,float("nan")]
        old_values=[-math.inf,-1.0,0.0,0.25,0.5,1.0,
                    math.inf,float("nan")]
        cases=0
        for player,old_hud,status,has_player in itertools.product(
                samples,old_values,(0,1,127,254,255),(False,True)):
            with self.subTest(player=player,old_hud=old_hud,
                              status=status,has_player=has_player):
                self.assertEqual(original_machine_decision(
                    player,old_hud,status,has_player),
                    readable_cpp_decision(player,old_hud,status,has_player))
            cases+=1
        self.assertEqual(cases,960)

if __name__=="__main__":
    unittest.main()
