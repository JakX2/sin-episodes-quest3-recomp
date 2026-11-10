"""Original-free original SiN 2006 CHudPoisonTimer binary-audit regressions."""
import pathlib
import struct
import sys
import unittest

sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/"tools"))
from audit_sin_original_client_poison_timer_hud import (
    CALLS, CONSTANTS, OPCODES, REGIONS, RTTI, STRINGS,
    TYPE_NAME, VTABLE, VerificationError, audit, verify_client,
)

class FakeClientImage:
    base=0x24000000
    def __init__(self):self.memory={}
    def put(self,va,payload):
        for offset,byte in enumerate(payload):
            address=va+offset
            if address in self.memory and self.memory[address]!=byte:
                raise AssertionError("conflicting synthetic x86 anchor at %#x" % address)
            self.memory[address]=byte
    def at(self,va,length):
        return bytes(self.memory.get(va+offset,0xCC) for offset in range(length))
    def tamper(self,va):
        self.memory[va]^=0x10

def fixture():
    pe=FakeClientImage()
    for va,target in {**RTTI,**VTABLE}.items():
        pe.put(va,struct.pack("<I",target))
    pe.put(0x24320EB0,TYPE_NAME)
    for va,hexcode in OPCODES.items():
        pe.put(va,bytes.fromhex(hexcode))
    for va,target in CALLS.items():
        pe.put(va,b"\xE8"+struct.pack("<i",target-va-5))
    for va,text in STRINGS.items():
        pe.put(va,text.encode()+b"\0")
    for va,data in CONSTANTS.items():
        pe.put(va,data)
    return pe

class PoisonHudEvidenceTests(unittest.TestCase):
    def test_real_rtti_virtual_slots_and_opcode_witnesses(self):
        proof=verify_client(fixture(),check_hashes=False)
        self.assertEqual(proof["original_class"],"CHudPoisonTimer")
        self.assertEqual(proof["verified_original_code_anchors"],len(OPCODES))
        self.assertEqual(proof["verified_calls"],len(CALLS))
        self.assertEqual(proof["verified_RTTI_and_virtual_pointers"],8)
        self.assertEqual(proof["verified_full_function_hashes"],0)
        self.assertEqual(len(REGIONS),4)

    def test_all_original_opcode_mutations_rejected(self):
        for va in OPCODES:
            with self.subTest(opcode=hex(va)):
                pe=fixture()
                pe.tamper(va)
                with self.assertRaises(VerificationError):
                    verify_client(pe,check_hashes=False)

    def test_all_original_rtti_and_vtable_mutations_rejected(self):
        for va in list(RTTI)+list(VTABLE)+[0x24320EB0]:
            with self.subTest(pointer=hex(va)):
                pe=fixture()
                pe.tamper(va)
                with self.assertRaises(VerificationError):
                    verify_client(pe,check_hashes=False)

    def test_original_callsite_mutations_rejected(self):
        for va in CALLS:
            with self.subTest(call=hex(va)):
                pe=fixture()
                pe.tamper(va+1)
                with self.assertRaises(VerificationError):
                    verify_client(pe,check_hashes=False)

    def test_original_float_and_sound_token_mutations_rejected(self):
        for va in list(CONSTANTS)+list(STRINGS):
            with self.subTest(data=hex(va)):
                pe=fixture()
                pe.tamper(va)
                with self.assertRaises(VerificationError):
                    verify_client(pe,check_hashes=False)

    def test_whole_function_hashes_not_fabricated_by_synthetic_bytes(self):
        pe=fixture()
        with self.assertRaisesRegex(VerificationError,"full x86 code SHA256"):
            verify_client(pe,check_hashes=True)
        for _,(_,size,digest) in REGIONS.items():
            self.assertGreater(size,0)
            self.assertEqual(len(bytes.fromhex(digest)),32)

    def test_original_dll_fingerprint_guard(self):
        with self.assertRaisesRegex(VerificationError,"wrong original client"):
            audit(b"not the original client DLL",b"not the original server DLL")

if __name__=="__main__":
    unittest.main()
