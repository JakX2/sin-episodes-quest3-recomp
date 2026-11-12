"""Phase184 original-free native witness mutation tests."""
from __future__ import annotations
import pathlib
import struct
import sys
import unittest

sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/'tools'))
import audit_sin_original_client_keyvalues_lifetime as mod

class FakePE:
    base=0x24000000
    def __init__(self):self.memory={}
    def at(self,va,n):return bytes(self.memory.get(va+i,0xcc) for i in range(n))
    def put(self,va,raw):
        for i,v in enumerate(raw):
            addr=va+i
            if addr in self.memory and self.memory[addr]!=v:
                raise AssertionError('overlapping inconsistent native fixture')
            self.memory[addr]=v
    def corrupt(self,va):self.memory[va]=self.memory.get(va,0xcc)^0x80
    def section_of(self,va):return '.text'

def fixture():
    pe=FakePE()
    for addr,hexcode in mod.OPCODES.items():pe.put(addr,bytes.fromhex(hexcode))
    for addr,dest in {**mod.DIRECT_CALLS,**mod.INCOMING_CALLS}.items():
        pe.put(addr,b'\xe8'+struct.pack('<i',dest-addr-5))
    return pe

class NativeLifetimeTests(unittest.TestCase):
    def test_three_exact_native_function_regions(self):
        self.assertEqual(len(mod.REGIONS),3)
        self.assertEqual(sum(n for _,n,_ in mod.REGIONS.values()),231)
        self.assertEqual(len({va for va,_,_ in mod.REGIONS.values()}),3)
        self.assertTrue(all(len(bytes.fromhex(s))==32 for _,_,s in mod.REGIONS.values()))
    def test_synthetic_opcode_and_edges_pass_without_hash(self):
        data=mod.verify_native(fixture(),check_hash=False)
        self.assertEqual(data['opcode_witnesses'],40)
        self.assertEqual(data['original_direct_calls'],5)
        self.assertEqual(data['incoming_original_calls'],2)
    def test_mutate_all_40_independent_opcode_fragments(self):
        for address in mod.OPCODES:
            with self.subTest(address=hex(address)):
                pe=fixture();pe.corrupt(address)
                with self.assertRaisesRegex(mod.VerificationError,'opcode mismatch'):
                    mod.verify_native(pe,check_hash=False)
    def test_mutate_every_direct_and_incoming_native_call(self):
        for address in {**mod.DIRECT_CALLS,**mod.INCOMING_CALLS}:
            with self.subTest(address=hex(address)):
                pe=fixture();pe.corrupt(address)
                with self.assertRaisesRegex(mod.VerificationError,'CALL target mismatch'):
                    mod.verify_native(pe,check_hash=False)
    def test_whole_function_sha_rejects_synthetic_evidence(self):
        with self.assertRaisesRegex(mod.VerificationError,'SHA256 mismatch'):
            mod.verify_native(fixture(),check_hash=True)
    def test_exact_original_binary_provenance_rejected_for_fake_dlls(self):
        with self.assertRaisesRegex(mod.VerificationError,'full DLL SHA256 mismatch'):
            mod.audit(b'fakeclient',b'fakeserver')
    def test_original_client_image_base_is_required(self):
        pe=fixture();pe.base=0
        with self.assertRaisesRegex(mod.VerificationError,'client base'):
            mod.verify_native(pe,check_hash=False)
    def test_unexpected_executable_section_is_rejected(self):
        pe=fixture();pe.section_of=lambda _:'.rdata'
        with self.assertRaisesRegex(mod.VerificationError,'not executable'):
            mod.verify_native(pe,check_hash=False)
    def test_native_layout_and_reference_consistency(self):
        offsets=mod.verify_native(fixture(),check_hash=False)['native_layout']
        self.assertEqual(offsets['peer'],24)
        self.assertEqual(offsets['child'],28)
        self.assertEqual(offsets['fallback'],32)
        self.assertEqual(offsets['escape_byte'],36)
        self.assertIn(0x24165033,mod.OPCODES)
        self.assertIn(0x24166621,mod.OPCODES)
    def test_malformed_pe_rejected(self):
        with self.assertRaisesRegex(mod.VerificationError,'MZ PE32'):
            mod.PEImage(b'not a PE')
        with self.assertRaisesRegex(mod.VerificationError,'PE signature'):
            mod.PEImage(b'MZ'+b'\x00'*0x1fe)
    def test_original_calls_target_correct_historical_routines(self):
        self.assertEqual(mod.DIRECT_CALLS[0x24165053],0x24166620)
        self.assertEqual(mod.DIRECT_CALLS[0x2416663f],0x24166620)
        self.assertEqual(mod.DIRECT_CALLS[0x24166670],0x24166620)
        self.assertEqual(mod.INCOMING_CALLS[0x241927bb],0x24165010)
        self.assertEqual(mod.INCOMING_CALLS[0x24166057],0x24166620)

if __name__=='__main__':unittest.main()