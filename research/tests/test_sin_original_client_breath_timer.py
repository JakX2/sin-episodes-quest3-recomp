"""Original-free synthetic x86 and original x87-branch tests for Phase155."""
import itertools
import math
import pathlib
import struct
import sys
import unittest
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/"tools"))
from audit_sin_original_client_breath_timer import (
    CALLS, CONSTANTS, NETWORK_NAMES, OPCODES, REGIONS,
    RTTI, SOUND_NAMES, TYPE_NAME, TYPE_NAME_VA, VTABLE,
    verify_original_breath, VerificationError, audit,
)

class FakePE:
    def __init__(self,base):self.base=base;self.m={}
    def put(self,va,code):
        for i,byte in enumerate(code):
            if va+i in self.m and self.m[va+i]!=byte:
                raise AssertionError("synthetic witness conflict at %#x"%(va+i))
            self.m[va+i]=byte
    def at(self,va,n):
        return bytes(self.m.get(va+i,0xCC) for i in range(n))
    def damage(self,va):self.m[va]^=0x10

def fixture():
    data={"client":FakePE(0x24000000),"server":FakePE(0x22000000)}
    for va,target in {**RTTI,**VTABLE}.items():
        data["client"].put(va,struct.pack("<I",target))
    data["client"].put(TYPE_NAME_VA,TYPE_NAME)
    for va,s in SOUND_NAMES.items():
        data["client"].put(va,s.encode()+b"\0")
    for side,items in NETWORK_NAMES.items():
        for va,s in items.items():data[side].put(va,s.encode()+b"\0")
    for va,packed in CONSTANTS.items():data["client"].put(va,packed)
    for side,items in OPCODES.items():
        for va,hexcode in items.items():
            data[side].put(va,bytes.fromhex(hexcode))
    for side,items in CALLS.items():
        for va,target in items.items():
            data[side].put(va,b"\xe8"+struct.pack("<i",target-va-5))
    return data

def ah(a,b):
    # x87 FCOMP/FUCOMPP condition bits: C0 0x01, C2 0x04, C3 0x40.
    if math.isnan(a) or math.isnan(b):return 0x45
    if a<b:return 0x01
    if a==b:return 0x40
    return 0

def jp_after_test_ah(v,mask):
    return (v&mask).bit_count()%2==0

def x87_machine(raw,old,status,player_exists):
    if not player_exists:return "skip"
    # JNP early exit on ordered equality (TEST AH,44h).
    if not jp_after_test_ah(ah(raw,old),0x44):return "skip"
    # JP bypasses FadeIn when >=1 or unordered.
    if not jp_after_test_ah(ah(raw,1.0),0x05) and status==0:
        return "FadeIn"
    if not jp_after_test_ah(ah(raw,0.25),0x05):
        return "Low"
    if not jp_after_test_ah(ah(raw,0.0),0x44):
        return "Empty"
    if not jp_after_test_ah(ah(raw,1.0),0x44) and status>0:
        return "FadeOut"
    return "none"

def independent_readable(raw,old,status,player_exists):
    if not player_exists or raw==old:return "skip"
    if raw<1.0 and status==0:return "FadeIn"
    if raw<0.25:return "Low"
    if raw==0.0:return "Empty"
    if raw==1.0 and status>0:return "FadeOut"
    return "none"

class BreathTimerTests(unittest.TestCase):
    def test_synthetic_positive_and_properties(self):
        p=verify_original_breath(fixture(),check_hashes=False)
        self.assertEqual(p["original_class"],"CHudBreathTimer")
        self.assertEqual(p["verified_code_regions"],0)
        self.assertEqual(p["verified_vtable_and_RTTI_pointers"],8)
        self.assertEqual(p["original_player_offsets"],
                         {"client":"0x11F0","server":"0x13A4"})
        self.assertEqual(len(REGIONS),6)
        self.assertEqual(len(p["original_sound_tokens"]),4)

    def test_all_original_opcode_mutations(self):
        for side,items in OPCODES.items():
            for va in items:
                with self.subTest(side=side,site=hex(va)):
                    p=fixture()
                    p[side].damage(va)
                    with self.assertRaises(VerificationError):
                        verify_original_breath(p,check_hashes=False)

    def test_all_vtables_rtti_and_type_name(self):
        for va in list(RTTI)+list(VTABLE)+[TYPE_NAME_VA]:
            with self.subTest(va=hex(va)):
                p=fixture()
                p["client"].damage(va)
                with self.assertRaises(VerificationError):
                    verify_original_breath(p,check_hashes=False)

    def test_original_callsite_mutations(self):
        for side,items in CALLS.items():
            for va in items:
                with self.subTest(side=side,site=hex(va)):
                    p=fixture()
                    p[side].damage(va+1)
                    with self.assertRaises(VerificationError):
                        verify_original_breath(p,check_hashes=False)

    def test_original_sound_network_string_and_threshold_mutations(self):
        for side,items in (("client",SOUND_NAMES),):
            for va in items:
                with self.subTest(side=side,site=hex(va)):
                    p=fixture()
                    p[side].damage(va)
                    with self.assertRaises(VerificationError):
                        verify_original_breath(p,check_hashes=False)
        for side,items in NETWORK_NAMES.items():
            for va in items:
                with self.subTest(side=side,site=hex(va)):
                    p=fixture()
                    p[side].damage(va)
                    with self.assertRaises(VerificationError):
                        verify_original_breath(p,check_hashes=False)
        for va in CONSTANTS:
            p=fixture()
            p["client"].damage(va)
            with self.assertRaises(VerificationError):
                verify_original_breath(p,check_hashes=False)

    def test_all_whole_code_regions_required(self):
        with self.assertRaisesRegex(VerificationError,"SHA256 mismatch"):
            verify_original_breath(fixture(),check_hashes=True)
        for name,(_,_,n,digest) in REGIONS.items():
            with self.subTest(region=name):
                self.assertGreater(n,0)
                self.assertEqual(len(bytes.fromhex(digest)),32)
        with self.assertRaises(VerificationError):
            audit(b"wrong original client",b"wrong original server")

    def test_original_x87_branch_emulation_and_unreachable_empty_warning(self):
        samples=[-math.inf,-2.0,-0.0,0.0,0.1,0.249,0.25,0.5,
                 0.99,1.0,1.01,math.inf,float("nan")]
        old_samples=[-math.inf,-0.0,0.0,0.25,0.5,1.0,math.inf,float("nan")]
        total=0
        for raw,old,status,exists in itertools.product(
                samples,old_samples,(0,1,127,254,255),(False,True)):
            with self.subTest(raw=raw,old=old,status=status,exists=exists):
                machine=x87_machine(raw,old,status,exists)
                readable=independent_readable(raw,old,status,exists)
                self.assertEqual(machine,readable)
                self.assertNotEqual(readable,"Empty")
            total+=1
        self.assertEqual(total,1040)

if __name__=="__main__":
    unittest.main()
