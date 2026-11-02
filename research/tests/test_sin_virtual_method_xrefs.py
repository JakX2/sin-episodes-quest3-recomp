"""Original-free objdump-assisted x86 function call/string cross-reference test."""
import pathlib
import shutil
import struct
import sys
import tempfile
import unittest

sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/"tools"))
from verify_gameplay_entity_registry import PEImage
from audit_sin_virtual_method_xrefs import inspect_method,read_string
from test_sin_rtti_vtables import make_rtti

BASE=0x22000000

@unittest.skipUnless(shutil.which("objdump"),"GNU objdump not installed")
class SyntheticMethodXrefs(unittest.TestCase):
    def test_real_instruction_boundaries_not_random_e8_bytes(self):
        image,_,_=make_rtti()
        b=bytearray(image)
        func=BASE+0x1000
        target=BASE+0x10B0
        string_va=BASE+0x3d5000
        b[string_va-BASE:string_va-BASE+len(b"PhoneBook\x00")]=b"PhoneBook\x00"
        # push literal; call relative target; ret; int3 align
        instructions=(b"\x68"+struct.pack("<I",string_va)+b"\xe8"+
                      struct.pack("<i",target-(func+10))+b"\xc3\xcc")
        b[func-BASE:func-BASE+len(instructions)]=instructions
        with tempfile.TemporaryDirectory() as tmp:
            path=pathlib.Path(tmp)/"synthetic-server.dll"
            path.write_bytes(b)
            report=inspect_method(path,PEImage(bytes(b)),func)
        self.assertTrue(report["ret_then_int3_end_found"])
        self.assertEqual(report["decoded_instructions"],3)
        self.assertEqual(report["direct_calls"],[{"callee_va":f"0x{target:08X}","count":1}])
        self.assertIn({"string_va":f"0x{string_va:08X}","literal":"PhoneBook"},
                      report["referenced_strings"])
        self.assertFalse(report["body_may_be_truncated"])

    def test_out_of_section_string_is_not_claimed(self):
        image,_,_=make_rtti()
        pe=PEImage(image)
        self.assertIsNone(read_string(pe,BASE+0x1200))
        self.assertIsNone(read_string(pe,BASE+0x4d7000))

if __name__=="__main__":unittest.main()
