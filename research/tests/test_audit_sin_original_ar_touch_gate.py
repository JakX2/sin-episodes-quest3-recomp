"""Synthetic-only mutation checks for recovered original SiN grenade touch CFG.

The real original PE32 game image is private; fabricated instruction windows
in this test exercise the independent branch/field/virtual-call verifier.
"""
import struct
from pathlib import Path
import sys
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"tools"))
import audit_sin_original_ar_touch_gate as original

class FakePE:
    def __init__(self):
        self.memory={}
        self.put(0x223142f7,bytes.fromhex("83 b9 1c 01 00 00 00"))
        self.put(0x223142fe,b"\x74\x50")
        self.put(0x22314300,bytes.fromhex("f6 81 20 01 00 00 04"))
        self.put(0x22314307,b"\x75\x47")
        self.put(0x22314310,bytes.fromhex("ff 90 fc 00 00 00"))
        self.put_call(0x2231431c,b"\x8b\xcf\xe8",0x222b2730)
        self.put(0x22314323,b"\x3b\xc6")
        self.put(0x22314325,b"\x74\x15")
        self.put(0x22314332,bytes.fromhex("c6 44 24 10 01"))
        self.put_call(0x22314337,b"\xe8",0x222b2b40)
        self.put(0x2231433d,bytes.fromhex("80 bf 95 07 00 00 00"))
        self.put(0x22314344,b"\x74\x0a")
        self.put(0x2231434a,bytes.fromhex("ff 92 e8 03 00 00"))
        self.put(0x22314350,bytes.fromhex("5f c2 04 00"))
        self.put(0x22314c4d,bytes.fromhex("80 be 95 07 00 00 00"))
        self.put(0x22314c73,bytes.fromhex("c6 86 95 07 00 00 00"))

    def put(self,addr,raw):
        for i,v in enumerate(raw):
            self.memory[addr+i]=v

    def put_call(self,site,prefix,target):
        self.put(site,prefix+struct.pack("<i",target-site-len(prefix)-4))

    def at(self,addr,length):
        return bytes(self.memory.get(addr+i,0) for i in range(length))

class TouchGateTests(unittest.TestCase):
    def test_fabricated_windows_have_full_original_branch_shape(self):
        found=original.check_touch(FakePE())
        self.assertEqual(found["detonate_virtual_slot"],250)
        self.assertEqual(found["contact_flag_field_offset"],"0x120")
        self.assertEqual(found["grenade_original_flag_offset"],"0x795")
        self.assertFalse(found["sourcevr_entity_integration_proven"])

    def test_fake_cannot_pass_real_full_function_sha(self):
        with self.assertRaisesRegex(original.AuditError,"fingerprint"):
            original.check_touch(FakePE(),strict_touch_hash=True)

    def test_contact_state_guard(self):
        f=FakePE();f.put(0x223142f7,b"\x90")
        with self.assertRaisesRegex(original.AuditError,"contact"):
            original.check_touch(f)

    def test_zero_state_branch_target(self):
        f=FakePE();f.put(0x223142ff,b"\x10")
        with self.assertRaisesRegex(original.AuditError,"zero"):
            original.check_touch(f)

    def test_other_bit4_filter(self):
        f=FakePE();f.put(0x22314306,b"\x08")
        with self.assertRaisesRegex(original.AuditError,"filter"):
            original.check_touch(f)

    def test_contact_flag_branch_target(self):
        f=FakePE();f.put(0x22314308,b"\x01")
        with self.assertRaisesRegex(original.AuditError,"contact mask"):
            original.check_touch(f)

    def test_other_virtual_slot_changed(self):
        f=FakePE();f.put(0x22314312,b"\xf8")
        with self.assertRaisesRegex(original.AuditError,"virtual method"):
            original.check_touch(f)

    def test_related_helper_call_changed(self):
        f=FakePE();f.put_call(0x2231431c,b"\x8b\xcf\xe8",0x222b2700)
        with self.assertRaisesRegex(original.AuditError,"related object helper"):
            original.check_touch(f)

    def test_identity_comparison_changed(self):
        f=FakePE();f.put(0x22314323,b"\x90")
        with self.assertRaisesRegex(original.AuditError,"comparison"):
            original.check_touch(f)

    def test_flag_assignment_byte_changed(self):
        f=FakePE();f.put(0x22314336,b"\x00")
        with self.assertRaisesRegex(original.AuditError,"setter no longer receives"):
            original.check_touch(f)

    def test_flag_setter_call_changed(self):
        f=FakePE();f.put_call(0x22314337,b"\xe8",0x222b2b50)
        with self.assertRaisesRegex(original.AuditError,"setter helper"):
            original.check_touch(f)

    def test_flag_test_offset_changed(self):
        f=FakePE();f.put(0x2231433f,b"\x94")
        with self.assertRaisesRegex(original.AuditError,"flag"):
            original.check_touch(f)

    def test_virtual_detonate_method_index_changed(self):
        f=FakePE();f.put(0x2231434c,b"\xe4")
        with self.assertRaisesRegex(original.AuditError,"Detonate"):
            original.check_touch(f)

    def test_original_spawn_no_longer_clears_flag(self):
        f=FakePE();f.put(0x22314c79,b"\x01")
        with self.assertRaisesRegex(original.AuditError,"spawn no longer clears"):
            original.check_touch(f)

    def test_original_image_sha_enforced(self):
        with self.assertRaisesRegex(original.AuditError,"SHA256 mismatch"):
            original.audit(Path(__file__)) # synthetic Python source, NOT DLL

if __name__=="__main__":
    unittest.main()
