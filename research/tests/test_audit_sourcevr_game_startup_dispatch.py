"""Original-asset-free fake ARM64 engine dispatch and game startup ABI tests."""
import struct
from pathlib import Path
import sys
import unittest
from unittest.mock import patch

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"tools"))
import audit_sourcevr_game_startup_dispatch as target


def adrp(pc,va,reg):
    disp=(((va & ~4095)-(pc & ~4095))//4096)&0x1fffff
    return (0x90000000 | ((disp&3)<<29) |
            (((disp>>2)&0x7ffff)<<5) | reg)


def add(reg,va):
    return 0x91000000 | ((va&4095)<<10) | (reg<<5) | reg


class FakeEngine:
    def __init__(self):
        self.i=0x328228
        self.c=0x328294
        self.s=0x2f3b1c
        self.words={}
        self.strings={0x749f8:b'server%s loaded for "%s"\n\x00',
                      0x6aef0:b'.so\x00'}
        for index,w in enumerate((
            0xf9400008,0xf9400108,0xf940e863,
            0xaa0103e2,0xd63f0100,0x35000080)):
            self.words[self.i+4*index]=w
        self.words[self.i-4]=0xf9400101
        self.words[self.i-16]=0xf9416260
        for index,w in enumerate((0xf9400008,0xf9400908,0xd63f0100)):
            self.words[self.c+4*index]=w
        for index,w in enumerate(
            (0xf9400008,0xf9403108,0xd63f0100,0xaa0003e2)):
            self.words[self.s+4*index]=w
        for pc,reg,va in (
            (self.s+16,0,0x749f8), (self.s+24,1,0x6aef0)):
            self.words[pc]=adrp(pc,va,reg)
            self.words[pc+4]=add(reg,va)

    def word(self,va):
        return self.words.get(va,0)

    def at(self,va,length):
        if va in self.strings:
            return self.strings[va][:length]
        raise target.AuditError("invalid fabricated engine memory")


class FakeMethods:
    def __init__(self,role):
        self.role=role
        self.words={}
        if role=="libclient.so":
            self.init=0x40000
            self.shutdown=0x40400
            self.methods=[self.init,0x40300,self.shutdown]+[0x40500]*75
            for off,value in (
                (0,0xd10143ff),(4,0xa9027bfd),
                (8,0xa90357f6),(12,0xa9044ff4),
                (16,0x910083fd),(20,0xaa0303f4),
                (24,0xaa0203f3),(28,0xf81f83a1),
            ):
                self.words[self.init+off]=value
            for off,value in (
                (0,0xa9bd7bfd),(4,0xf9000bf5),
                (8,0xa9024ff4),(12,0x910003fd),
            ):
                self.words[self.shutdown+off]=value
        else:
            self.init=0x50000
            self.desc=0x51000
            self.methods=[self.init]+[0x50700]*11+[self.desc]+[0x50800]*34
            for off,value in (
                (0,0xd101c3ff),(4,0xa9027bfd),
                (8,0xf9001bf9),(12,0xa9045ff8),
                (16,0xa90557f6),(20,0xa9064ff4),
                (28,0xf9000fa1),(40,0xaa0403f5),
                (44,0xaa0303f4),(48,0xaa0203f3),
            ):
                self.words[self.init+off]=value
            self.words[self.desc]=0x14000010
        self.sections={".text":(0,1,6,0x40000,0,0x20000,0,0,4,0)}

    def word(self,va):
        return self.words.get(va,0)


def mock_table(elf,role):
    if role!=elf.role:
        raise AssertionError("wrong test game module role")
    return (0,elf.methods,"synthetic",0)


class StartupABITests(unittest.TestCase):
    def test_engine_three_verified_calls(self):
        fake=FakeEngine()
        result=target.check_dispatch(fake,fake.i,fake.c,fake.s)
        self.assertEqual(result["client_init_method_index"],0)
        self.assertEqual(result["client_cleanup_method_index"],2)
        self.assertEqual(result["server_description_method_index"],12)
        self.assertEqual(len(result["client_init_argument_registers"]),4)

    def test_engine_rejects_wrong_client_slot(self):
        fake=FakeEngine()
        fake.words[fake.i+4]=0xf9400508
        with self.assertRaisesRegex(target.AuditError,"client game slot0"):
            target.check_dispatch(fake,fake.i,fake.c,fake.s)

    def test_engine_rejects_wrong_client_argument(self):
        fake=FakeEngine()
        fake.words[fake.i+8]=0xf940e864
        with self.assertRaisesRegex(target.AuditError,"x3 pointer"):
            target.check_dispatch(fake,fake.i,fake.c,fake.s)

    def test_engine_rejects_missing_w0_return_check(self):
        fake=FakeEngine()
        fake.words[fake.i+20]=0xd503201f
        with self.assertRaisesRegex(target.AuditError,"client game slot0"):
            target.check_dispatch(fake,fake.i,fake.c,fake.s)

    def test_engine_rejects_wrong_cleanup_slot(self):
        fake=FakeEngine()
        fake.words[fake.c+4]=0xf9401108
        with self.assertRaisesRegex(target.AuditError,"cleanup method slot2"):
            target.check_dispatch(fake,fake.i,fake.c,fake.s)

    def test_engine_rejects_wrong_server_desc_slot(self):
        fake=FakeEngine()
        fake.words[fake.s+4]=0xf9403908
        with self.assertRaisesRegex(target.AuditError,"description slot12"):
            target.check_dispatch(fake,fake.i,fake.c,fake.s)

    def test_engine_rejects_unproven_description_string(self):
        fake=FakeEngine()
        fake.strings[0x749f8]=b'not the description log\x00'
        with self.assertRaisesRegex(target.AuditError,"logging string changed"):
            target.check_dispatch(fake,fake.i,fake.c,fake.s)

    def test_native_callee_register_proof(self):
        cli,srv=FakeMethods("libclient.so"),FakeMethods("libserver.so")
        with patch.object(target,"method_table",side_effect=mock_table):
            result=target.check_actual_methods(cli,srv)
        self.assertEqual(result["server_startup_register_arguments"],
                         ["x1","x2","x3","x4"])
        self.assertEqual(result["client_cleanup_va"],"0x40400")
        self.assertFalse(result["startup_signature_types_fully_verified"])

    def test_native_client_x2_parameter_corruption(self):
        cli,srv=FakeMethods("libclient.so"),FakeMethods("libserver.so")
        cli.words[cli.init+0x18]=0xaa0103f3
        with patch.object(target,"method_table",side_effect=mock_table):
            with self.assertRaisesRegex(target.AuditError,"3-argument prologue"):
                target.check_actual_methods(cli,srv)

    def test_native_server_x4_parameter_corruption(self):
        cli,srv=FakeMethods("libclient.so"),FakeMethods("libserver.so")
        srv.words[srv.init+0x28]=0xaa0303f5
        with patch.object(target,"method_table",side_effect=mock_table):
            with self.assertRaisesRegex(target.AuditError,"4-argument prologue"):
                target.check_actual_methods(cli,srv)

    def test_native_server_description_branch_out_of_code(self):
        cli,srv=FakeMethods("libclient.so"),FakeMethods("libserver.so")
        srv.words[srv.desc]=0x140fffff
        with patch.object(target,"method_table",side_effect=mock_table):
            with self.assertRaisesRegex(target.AuditError,"leaves code"):
                target.check_actual_methods(cli,srv)

    def test_different_profile_addresses_pinned(self):
        self.assertEqual(len(target.DISPATCH_VAS),7)
        self.assertEqual(set(target.DISPATCH_VAS),set(target.ENGINE_WITNESSES))
        self.assertEqual(target.DISPATCH_VAS["hl2"][0],0x328228)
        self.assertEqual(target.DISPATCH_VAS["portal"][0],0x327a08)
        self.assertEqual(target.DISPATCH_VAS["portal2"][0],0x32b848)

if __name__=="__main__":
    unittest.main()
