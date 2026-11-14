#!/usr/bin/env python3
"""Phase220 original-free synthetic x86 mutation tests for type5 specialists."""
from __future__ import annotations
import sys
import unittest
from pathlib import Path
from unittest.mock import patch

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"tools"))
import audit_sin_original_client_type5_specialists as native


class SyntheticPE:
    """Sparse synthetic native witnesses, not any retail game machine code."""
    def __init__(self):
        self.base=native.BASE["client"]
        self.memory={}
        self.rdata=set()
        self.data={native.SYMBOL_RESOLVER_GLOBAL}
        for va,size,sha,padding,caller,pop,witnesses in native.METHODS.values():
            for off,hx in witnesses.items():
                self.write(va+off,bytes.fromhex(hx))
            self.write(va+size-3,b"\xc2"+pop.to_bytes(2,"little"))
            self.write(va+size,b"\xcc"*padding)
            self.write(caller,native.rel_call(caller,va))
        self.write(native.FOLLOWING_VA,native.FOLLOWING_ENTRY)
        for site,target in native.NESTED_DIRECT_CALLS:
            self.write(site,native.rel_call(site,target))
        for site,target in ((0x241999CB,0x24198A10),
                            (0x241999D6,0x24197B60)):
            self.write(site,native.rel_call(site,target))
        for va,string in native.FORMAT_STRINGS.items():
            data=string.encode("ascii")+b"\0"
            self.write(va,data)
            self.rdata.update(range(va,va+len(data)))

    def write(self,va,raw):
        for i,value in enumerate(raw):
            before=self.memory.get(va+i,value)
            if before!=value:
                raise AssertionError("incompatible synthetic opcode anchors")
            self.memory[va+i]=value

    def at(self,va,length):
        return bytes(self.memory.get(va+i,0) for i in range(length))

    def section_of(self,va):
        if va in self.data:return ".data"
        if va in self.rdata:return ".rdata"
        return ".text"

    def flip(self,va):
        self.memory[va]=self.memory.get(va,0)^1


class Phase220Tests(unittest.TestCase):
    @staticmethod
    def verify(fake,check_imports=False):
        return native.verify_native(fake,check_hash=False,
                                    check_imports=check_imports)

    def test_exact_synthetic_counts_and_no_original_recovery_claim(self):
        result=self.verify(SyntheticPE())
        self.assertEqual(result["new_SHA_verified_original_client_functions"],0)
        self.assertEqual(result["new_complete_native_x86_bytes"],0)
        self.assertEqual(result["sparse_original_instruction_witnesses"],44)
        self.assertEqual(result["original_direct_type5_router_calls"],4)
        self.assertEqual(result["nested_original_native_formatter_calls"],3)
        self.assertEqual(result["original_format_literals"],3)
        self.assertTrue(result["no_padding_between_preview_and_type6_preview"])
        with self.assertRaises(native.VerificationError):
            native.verify_native(SyntheticPE(),check_hash=True,
                                 check_imports=False)
        with self.assertRaises(native.VerificationError):
            native.audit(b"MZ",b"MZ")

    def test_every_sparse_native_opcode_witness_checked(self):
        for name,(va,_,_,_,_,_,anchors) in native.METHODS.items():
            for off in anchors:
                with self.subTest(name=name,offset=hex(off)):
                    fake=SyntheticPE();fake.flip(va+off)
                    with self.assertRaises(native.VerificationError):
                        self.verify(fake)

    def test_both_router_calls_both_prior_calls_and_three_nested_edges(self):
        edges=[(item[4],item[0]) for item in native.METHODS.values()]
        edges+=list(native.NESTED_DIRECT_CALLS)
        edges += [(0x241999CB,0x24198A10),(0x241999D6,0x24197B60)]
        for site,target in edges:
            with self.subTest(site=hex(site),target=hex(target)):
                fake=SyntheticPE();fake.flip(site+1)
                with self.assertRaises(native.VerificationError):
                    self.verify(fake)

    def test_each_native_RET_and_original_INT3_padding(self):
        for name,(va,size,_,padding,_,_,_) in native.METHODS.items():
            with self.subTest(method=name,kind="ret"):
                fake=SyntheticPE();fake.flip(va+size-2)
                with self.assertRaises(native.VerificationError):
                    self.verify(fake)
            for off in range(padding):
                with self.subTest(method=name,kind="int3",offset=off):
                    fake=SyntheticPE();fake.flip(va+size+off)
                    with self.assertRaises(native.VerificationError):
                        self.verify(fake)

    def test_native_zero_padding_preview_next_entry_safeguard(self):
        preview,length,*_=native.METHODS["type5_integer_symbol_aware_preview"]
        self.assertEqual(preview+length,native.FOLLOWING_VA)
        self.assertEqual(native.METHODS["type5_integer_symbol_aware_preview"][3],0)
        for i in range(len(native.FOLLOWING_ENTRY)):
            with self.subTest(i=i):
                fake=SyntheticPE();fake.flip(native.FOLLOWING_VA+i)
                with self.assertRaises(native.VerificationError):
                    self.verify(fake)

    def test_each_original_formatter_literal_checked(self):
        for va,string in native.FORMAT_STRINGS.items():
            for off in (0,len(string)):
                with self.subTest(va=hex(va),offset=off):
                    fake=SyntheticPE();fake.flip(va+off)
                    with self.assertRaises(native.VerificationError):
                        self.verify(fake)

    def test_actual_named_import_and_registry_pointer_evidence(self):
        fake=SyntheticPE()
        with patch.object(native,"named_iat",return_value=[native.Q_FILEBASE_IAT]):
            self.assertEqual(self.verify(fake,check_imports=True)[
                "original_named_Q_FileBase_IAT"],"0x24276254")
        with patch.object(native,"named_iat",return_value=[]):
            with self.assertRaises(native.VerificationError):
                self.verify(fake,check_imports=True)
        fake=SyntheticPE();fake.data.clear()
        with self.assertRaises(native.VerificationError):
            self.verify(fake)

    def test_wrong_PE_image_base_and_method_sections(self):
        fake=SyntheticPE();fake.base+=0x10000
        with self.assertRaises(native.VerificationError):
            self.verify(fake)
        fake=SyntheticPE();fake.section_of=lambda _va:".data"
        with self.assertRaises(native.VerificationError):
            self.verify(fake)

    def test_original_distinct_method_regions(self):
        self.assertEqual(sum(row[1] for row in native.METHODS.values()),269)
        self.assertEqual({row[0] for row in native.METHODS.values()},
                         {0x24197DB0,0x241983B0})
        self.assertEqual({row[5] for row in native.METHODS.values()},{12,16})


if __name__=="__main__":
    unittest.main()
