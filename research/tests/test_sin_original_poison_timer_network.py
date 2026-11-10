"""Original-free tamper tests for dual-DLL poison-timer network offset recovery."""
import pathlib
import struct
import sys
import unittest
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/"tools"))
from audit_sin_original_poison_timer_network import (
    REGIONS,OPCODES,CALLS,STRINGS,verify_orig_properties,
    VerificationError,audit,
)

class MockPE:
    def __init__(self,base):
        self.base=base
        self.words={}
    def put(self,va,data):
        for i,byte in enumerate(data):
            if va+i in self.words and self.words[va+i]!=byte:
                raise AssertionError("overlapping original-free fixture at %#x"%(va+i))
            self.words[va+i]=byte
    def at(self,va,length):
        return bytes(self.words.get(va+i,0xcc) for i in range(length))
    def tamper(self,va):
        self.words[va]^=0x10

def fixture():
    images={"client":MockPE(0x24000000),"server":MockPE(0x22000000)}
    for side,addresses in OPCODES.items():
        for va,hexcode in addresses.items():
            images[side].put(va,bytes.fromhex(hexcode))
    for side,addresses in CALLS.items():
        for va,target in addresses.items():
            images[side].put(va,b"\xe8"+struct.pack("<i",target-va-5))
    for side,addresses in STRINGS.items():
        for va,value in addresses.items():
            images[side].put(va,value.encode()+b"\0")
    return images

class PoisonTimerNetworkTests(unittest.TestCase):
    def test_synthetic_nested_property_relationship(self):
        report=verify_orig_properties(fixture(),check_hashes=False)
        self.assertEqual(report["network_table"],"DT_HL2Local")
        self.assertEqual(report["nested_property"],"m_poisonTimer")
        self.assertEqual(report["client_original_x86_offsets"]["absolute"],
                         report["client_original_x86_offsets"]["parent"]+
                         report["client_original_x86_offsets"]["nested"])
        self.assertEqual(report["server_original_x86_offsets"]["absolute"],
                         report["server_original_x86_offsets"]["parent"]+
                         report["server_original_x86_offsets"]["nested"])
        self.assertEqual(report["verified_call_edges"],4)
        self.assertEqual(report["verified_property_opcode_sites"],10)

    def test_all_original_instruction_offsets_are_guarded(self):
        for side,addresses in OPCODES.items():
            for va in addresses:
                with self.subTest(side=side,opcode=hex(va)):
                    images=fixture()
                    images[side].tamper(va)
                    with self.assertRaises(VerificationError):
                        verify_orig_properties(images,check_hashes=False)

    def test_all_original_parent_and_child_ctor_calls_guarded(self):
        for side,addresses in CALLS.items():
            for va in addresses:
                with self.subTest(side=side,call=hex(va)):
                    images=fixture()
                    images[side].tamper(va+1)
                    with self.assertRaises(VerificationError):
                        verify_orig_properties(images,check_hashes=False)

    def test_all_real_property_names_guarded(self):
        for side,addresses in STRINGS.items():
            for va in addresses:
                with self.subTest(side=side,string=hex(va)):
                    images=fixture()
                    images[side].tamper(va)
                    with self.assertRaises(VerificationError):
                        verify_orig_properties(images,check_hashes=False)

    def test_code_sha_pins_and_wrong_original_dlls(self):
        self.assertEqual(len(REGIONS),4)
        for _,(_,_,size,sha) in REGIONS.items():
            self.assertGreater(size,0)
            self.assertEqual(len(bytes.fromhex(sha)),32)
        with self.assertRaisesRegex(VerificationError,"bounded code SHA256"):
            verify_orig_properties(fixture(),check_hashes=True)
        with self.assertRaises(VerificationError):
            audit(b"non-game client",b"non-game server")

if __name__=="__main__":
    unittest.main()
