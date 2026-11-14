#!/usr/bin/env python3
"""Phase224 original-free EHandle native opcode/CALL mutation regressions."""
from __future__ import annotations

import sys
import unittest
from pathlib import Path

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"tools"))
import audit_sin_original_client_ehandle_specialists as native


class SyntheticPE:
    """Sparse instruction witnesses only: NEVER original retail code bodies."""
    def __init__(self):
        self.base=native.BASE["client"]
        self.memory={}
        self.rdata=set()
        self.data={native.GLOBAL_ENTITY_LIST_PTR}
        for va,n,_sha,pad,caller,pop,witnesses in native.METHODS.values():
            for off,hx in witnesses.items():
                self.write(va+off,bytes.fromhex(hx))
            self.write(va+n-3,b"\xc2"+pop.to_bytes(2,"little"))
            self.write(va+n,b"\xcc"*pad)
            self.write(caller,native.rel_call(caller,va))
        for site,dest in native.NESTED_CALLS:
            self.write(site,native.rel_call(site,dest))
        for va,value in native.FORMAT_LITERALS.items():
            raw=value.encode("ascii")+b"\0"
            self.write(va,raw)
            self.rdata.update(range(va,va+len(raw)))
        for va,head in native.FOLLOWING_ENTRIES.items():
            self.write(va,head)

    def write(self,va,raw):
        for off,byte in enumerate(raw):
            old=self.memory.get(va+off,byte)
            if old!=byte:
                raise AssertionError("synthetic opcode collision at "+hex(va+off))
            self.memory[va+off]=byte

    def at(self,va,n):
        return bytes(self.memory.get(va+off,0) for off in range(n))

    def section_of(self,va):
        if va in self.rdata:return ".rdata"
        if va in self.data:return ".data"
        return ".text"

    def flip(self,va):
        self.memory[va]=self.memory.get(va,0)^1


class Phase224Tests(unittest.TestCase):
    @staticmethod
    def verify(fake):
        return native.verify_native(fake,check_hash=False)

    def test_synthetic_does_not_claim_authentic_native_byte_count(self):
        report=self.verify(SyntheticPE())
        self.assertEqual(report["new_whole_SHA_verified_original_client_methods"],0)
        self.assertEqual(report["new_complete_original_native_x86_bytes"],0)
        self.assertEqual(report["sparse_original_instruction_witnesses"],81)
        self.assertEqual(report["verified_original_router_direct_calls"],4)
        self.assertEqual(report["verified_original_nested_formatter_and_engine_calls"],11)
        self.assertEqual(report["native_format_literals"],3)
        self.assertEqual(report["original_ehandle_slot_index_mask"],"0xFFF")
        self.assertEqual(report["original_ehandle_serial_shift"],12)
        with self.assertRaises(native.VerificationError):
            native.verify_native(SyntheticPE(),check_hash=True)
        with self.assertRaises(native.VerificationError):
            native.audit(b"MZ",b"MZ")

    def test_each_original_native_instruction_witness_mutation(self):
        for name,(va,n,sha,pad,caller,pop,witnesses) in native.METHODS.items():
            for off in witnesses:
                with self.subTest(method=name,offset=hex(off)):
                    fake=SyntheticPE();fake.flip(va+off)
                    with self.assertRaises(native.VerificationError):
                        self.verify(fake)

    def test_four_router_and_eleven_nested_calls_rejected_when_changed(self):
        edges=[(record[4],record[0]) for record in native.METHODS.values()]
        edges+=list(native.NESTED_CALLS)
        for site,target in edges:
            with self.subTest(site=hex(site),target=hex(target)):
                fake=SyntheticPE();fake.flip(site+1)
                with self.assertRaises(native.VerificationError):
                    self.verify(fake)

    def test_all_original_return_arity_and_padding_bytes(self):
        for name,(va,n,sha,pad,caller,pop,witnesses) in native.METHODS.items():
            with self.subTest(method=name,kind="ret"):
                fake=SyntheticPE();fake.flip(va+n-2)
                with self.assertRaises(native.VerificationError):
                    self.verify(fake)
            for index in range(pad):
                with self.subTest(method=name,kind="int3",index=index):
                    fake=SyntheticPE();fake.flip(va+n+index)
                    with self.assertRaises(native.VerificationError):
                        self.verify(fake)

    def test_original_literal_strings_and_neighboring_code_entries(self):
        for va,word in native.FORMAT_LITERALS.items():
            for offset in (0,len(word)):
                with self.subTest(string=hex(va),offset=offset):
                    fake=SyntheticPE();fake.flip(va+offset)
                    with self.assertRaises(native.VerificationError):
                        self.verify(fake)
        for va,head in native.FOLLOWING_ENTRIES.items():
            for offset in range(len(head)):
                with self.subTest(next_function=hex(va),offset=offset):
                    fake=SyntheticPE();fake.flip(va+offset)
                    with self.assertRaises(native.VerificationError):
                        self.verify(fake)

    def test_original_global_entity_list_base_and_code_sections(self):
        fake=SyntheticPE();fake.data.clear()
        with self.assertRaises(native.VerificationError):
            self.verify(fake)
        fake=SyntheticPE();fake.section_of=lambda _addr:".data"
        with self.assertRaises(native.VerificationError):
            self.verify(fake)
        fake=SyntheticPE();fake.base+=0x1000
        with self.assertRaises(native.VerificationError):
            self.verify(fake)

    def test_original_method_lengths_and_type_case(self):
        self.assertEqual(sum(row[1] for row in native.METHODS.values()),672)
        self.assertEqual({row[0] for row in native.METHODS.values()},
                         {0x24199060,0x24197E90,0x241994C0,0x241995E0})
        self.assertEqual({row[5] for row in native.METHODS.values()},{12,16})
        self.assertEqual([row[3] for row in native.METHODS.values()],[0,15,0,1])
        self.assertEqual(native.TYPE_CASES[13],11)
        self.assertEqual(native.CASE_DESTINATIONS[11],0x24199A8E)


if __name__=="__main__":
    unittest.main()
