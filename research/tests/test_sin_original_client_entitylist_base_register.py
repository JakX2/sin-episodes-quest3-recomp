#!/usr/bin/env python3
"""Phase248 synthetic-only PE32 mutation tests: NO authentic code credit."""
from __future__ import annotations
import sys
import unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"tools"))
import audit_sin_original_client_entitylist_base_register as native

class SyntheticPE:
    def __init__(self):
        self.base=native.ORIGINALS["client"]["base"]
        self.memory={}
        self.text=set()
        for name,va,size,sha,pad,ret,anchors in native.METHODS:
            self.write(va,bytes(size))
            self.text.update(range(va,va+size))
            self.write(va+size,b"\xCC"*pad)
            off,hx=ret
            self.write(va+off,bytes.fromhex(hx))
            for off,hx in anchors.items():
                self.write(va+off,bytes.fromhex(hx))
        for site,target in native.CALLS:
            self.write(site,native.e8(site,target))
            self.text.update(range(site,site+5))
        for site,hx in native.IMPORTED_IAT_CALLS:
            payload=bytes.fromhex(hx)
            self.write(site,payload)
            self.text.update(range(site,site+len(payload)))
    def write(self,va,data):
        for i,val in enumerate(data):
            self.memory[va+i]=val
    def at(self,va,n):
        return bytes(self.memory.get(va+i,0) for i in range(n))
    def section_of(self,va):
        return ".text" if va in self.text else ""
    def flip(self,va):
        self.memory[va]=self.memory.get(va,0)^1

class Phase248SyntheticOnlyTests(unittest.TestCase):
    @staticmethod
    def verify(pe):
        return native.verify_native(
            pe,check_hash=False,authentic_provenance=False)

    def test_synthetic_witness_never_earns_authentic_method_credit(self):
        result=self.verify(SyntheticPE())
        self.assertEqual(result["new_original_complete_SHA_methods"],0)
        self.assertEqual(result["new_original_native_code_bytes"],0)
        self.assertEqual(len(result["reviewed_original_methods"]),5)
        self.assertEqual(result["original_sparse_opcode_witnesses"],56)
        self.assertEqual(result["original_direct_E8_rel32_edges"],12)
        self.assertEqual(result["original_verified_IAT_calls"],2)
        self.assertEqual(result["initial_high_free_count"],2047)
        with self.assertRaises(native.VerificationError):
            native.verify_native(SyntheticPE(),check_hash=True)
        with self.assertRaises(native.VerificationError):
            native.audit(b"MZ",b"MZ")

    def test_all_original_sparse_native_opcode_witnesses(self):
        for name,va,size,sha,pad,ret,anchors in native.METHODS:
            for off in anchors:
                with self.subTest(name=name,offset=hex(off)):
                    fixture=SyntheticPE()
                    fixture.flip(va+off)
                    with self.assertRaises(native.VerificationError):
                        self.verify(fixture)

    def test_each_original_exit_byte_and_following_INT3(self):
        for name,va,size,sha,pad,ret,anchors in native.METHODS:
            off,hx=ret
            for byte in range(len(bytes.fromhex(hx))):
                with self.subTest(method=name,ret_byte=byte):
                    fixture=SyntheticPE()
                    fixture.flip(va+off+byte)
                    with self.assertRaises(native.VerificationError):
                        self.verify(fixture)
            for byte in range(pad):
                with self.subTest(method=name,int3_byte=byte):
                    fixture=SyntheticPE()
                    fixture.flip(va+size+byte)
                    with self.assertRaises(native.VerificationError):
                        self.verify(fixture)

    def test_all_twelve_direct_original_CALL_instructions(self):
        for site,target in native.CALLS:
            for byte in range(5):
                with self.subTest(callsite=hex(site),byte=byte):
                    fixture=SyntheticPE()
                    fixture.flip(site+byte)
                    with self.assertRaises(native.VerificationError):
                        self.verify(fixture)

    def test_native_imported_RandomInt_and_Error_call_bytes(self):
        for site,hx in native.IMPORTED_IAT_CALLS:
            for byte in range(len(bytes.fromhex(hx))):
                with self.subTest(import_site=hex(site),byte=byte):
                    fixture=SyntheticPE()
                    fixture.flip(site+byte)
                    with self.assertRaises(native.VerificationError):
                        self.verify(fixture)

    def test_original_image_base_and_complete_text_region_boundaries(self):
        fixture=SyntheticPE()
        fixture.base+=0x1000
        with self.assertRaises(native.VerificationError):
            self.verify(fixture)
        for name,va,size,*rest in native.METHODS:
            fixture=SyntheticPE()
            fixture.text.remove(va+size-1)
            with self.assertRaises(native.VerificationError):
                self.verify(fixture)

    def test_exact_original_review_inventory(self):
        self.assertEqual([m[1] for m in native.METHODS],
            [0x24127E80,0x24127F50,0x241280A0,0x241280C0,0x241A8B90])
        self.assertEqual([m[2] for m in native.METHODS],[200,106,32,181,17])
        self.assertEqual(sum(m[2] for m in native.METHODS),536)
        self.assertEqual([m[4] for m in native.METHODS],[8,6,0,11,15])
        self.assertEqual(native.CALLS[:5],(
            (0x24127EB6,0x241A8B90),
            (0x24127F7C,0x24127E20),
            (0x241280B5,0x24127F50),
            (0x24128136,0x24127E20),
            (0x24108193,0x24127E80)))
        self.assertEqual(native.METHODS[0][3],
          "3bd028ccda4ac019dbf491fbc834c2fc7713baeae2e4253e84702131e8882d3e")

if __name__=="__main__":
    unittest.main()
