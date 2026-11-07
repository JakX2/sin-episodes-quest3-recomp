"""Original-file-free regression tests for SourceVR ARM64 factory proof."""
import importlib.util
import io
from pathlib import Path
import sys
import unittest
import zipfile

TOOLS=Path(__file__).resolve().parents[1]/"tools"
spec=importlib.util.spec_from_file_location(
    "factory",TOOLS/"audit_sourcevr_native_game_factories.py")
a=importlib.util.module_from_spec(spec)
spec.loader.exec_module(a)

def enc_adrp(va,target,reg):
    disp=(target-(va&~4095))//4096 & 0x1fffff
    return 0x90000000 | ((disp&3)<<29) | (((disp>>2)&0x7ffff)<<5) | reg

def enc_add(reg,low):
    return 0x91000000 | ((low&4095)<<10) | (reg<<5) | reg

def enc_bl(va,target):
    return 0x94000000 | (((target-va)//4)&0x3ffffff)

def enc_ldr(dest,base,offset):
    return 0xf9400000 | ((offset//8)<<10) | (base<<5) | dest

def enc_str(src,base,offset):
    return 0xf9000000 | ((offset//8)<<10) | (base<<5) | src

class FakeELF:
    """Synthetic instruction words; never a copyrighted game binary."""
    def __init__(self):
        self.words={}
        self.exports={"CreateInterface":0x14000}
        self.name=b"ServerGameDLL010\x00"
        self.name_va=0x20000
        self.flags_map={0x20000:2,0x21000:1,0x22000:1,0x15000:4}
        site=0x10000
        registrar=0x13000
        create=0x14000
        callback=0x15000
        for va,reg,address in [
            (site-16,0,0x21000),(site-8,1,callback),(site,2,self.name_va)
        ]:
            self.words[va]=enc_adrp(va,address,reg)
            self.words[va+4]=enc_add(reg,address&4095)
        self.words[site+8]=enc_bl(site+8,registrar)
        self.words.update({
            callback:enc_adrp(callback,0x22000,0),
            callback+4:enc_add(0,0),
            callback+8:0xd65f03c0,
            registrar:enc_adrp(registrar,0x23000,8),
            registrar+4:0xa9000801,
            registrar+8:enc_ldr(9,8,0),
            registrar+12:enc_str(0,8,0),
            registrar+16:0xf9000809,
            registrar+20:0xd65f03c0,
            create+16:enc_adrp(create,0x23000,8),
            create+24:enc_ldr(21,8,0),
            create+36:0xf94006a0,
            create+40:0xaa1403e1,
            create+44:enc_bl(create+44,0x14200),
            create+48:0x34000160,
            create+52:0xf9400ab5,
            create+56:0xb5ffff75,
            create+100:0xf94002a0,
            create+116:0xd61f0000,
        })

    def word(self,va):
        return self.words.get(va,0)

    def at(self,va,n):
        if va==self.name_va:
            return self.name[:n]
        raise a.AuditError("unexpected synthetic data access")

    def flags(self,va):
        return self.flags_map.get(va,0)

class NativeFactoryTest(unittest.TestCase):
    def verify(self,elf):
        return a.verify_registration(
            elf,"libserver.so",0x20000,0x10000,0x13000,0x14000)

    def test_valid_registration(self):
        r=self.verify(FakeELF())
        self.assertEqual(r["registered_name"],"ServerGameDLL010")
        self.assertEqual(r["registry_head_va"],"0x23000")

    def test_native_factory_name_corrupted(self):
        fixture=FakeELF()
        fixture.name=b"ServerGameDLL011\x00"
        with self.assertRaisesRegex(a.AuditError,"string"):
            self.verify(fixture)

    def test_export_corruption(self):
        fixture=FakeELF()
        fixture.exports["CreateInterface"]=0x14010
        with self.assertRaisesRegex(a.AuditError,"CreateInterface"):
            self.verify(fixture)

    def test_factory_name_not_passed_to_registrar(self):
        fixture=FakeELF()
        fixture.words[0x10004]=enc_add(2,1)
        with self.assertRaisesRegex(a.AuditError,"factory name"):
            self.verify(fixture)

    def test_wrong_registrar_target(self):
        fixture=FakeELF()
        fixture.words[0x10008]=enc_bl(0x10008,0x13004)
        with self.assertRaisesRegex(a.AuditError,"registrar"):
            self.verify(fixture)

    def test_registry_node_changes(self):
        fixture=FakeELF()
        fixture.words[0x13004]=0
        with self.assertRaisesRegex(a.AuditError,"STP"):
            self.verify(fixture)

    def test_export_uses_different_head(self):
        fixture=FakeELF()
        fixture.words[0x14010]=enc_adrp(0x14010,0x24000,8)
        with self.assertRaisesRegex(a.AuditError,"another registry"):
            self.verify(fixture)

    def test_callback_return_changed(self):
        fixture=FakeELF()
        fixture.words[0x15008]=0
        with self.assertRaisesRegex(a.AuditError,"return"):
            self.verify(fixture)

    def test_callback_not_executable(self):
        fixture=FakeELF()
        fixture.flags_map[0x15000]=2
        with self.assertRaisesRegex(a.AuditError,"executable"):
            self.verify(fixture)

    def test_reject_wrong_architecture(self):
        with self.assertRaises(a.AuditError):
            a.NativeELF(b"not an ELF")

    def test_original_hash_mismatch_checked_first(self):
        buffer=io.BytesIO()
        with zipfile.ZipFile(buffer,"w") as archive:
            archive.writestr("server.dll",b"this is NOT the original DLL")
        buffer.seek(0)
        with self.assertRaisesRegex(a.AuditError,"SHA256 mismatch"):
            a.audit(buffer,io.BytesIO())

    def test_all_eleven_native_modules_pinned(self):
        self.assertEqual(len(a.WITNESSES),11)
        self.assertEqual(set(a.PROFILES),set(a.PROFILE_SHAS))
        self.assertEqual(len({value[0] for value in a.PROFILE_SHAS.values()}),5)
        self.assertEqual(len({value[1] for value in a.PROFILE_SHAS.values()}),6)

    def test_aarch64_signed_relative_addresses(self):
        for va,target in [(0x10000,0x20000),(0x20000,0x10000)]:
            self.assertEqual(
                a.decode_adrp(va,enc_adrp(va,target,2),2),target)
        self.assertEqual(
            a.decode_bl(0x10000,enc_bl(0x10000,0x13000)),0x13000)
        self.assertEqual(
            a.decode_bl(0x13000,enc_bl(0x13000,0x10000)),0x10000)

if __name__=="__main__":
    unittest.main()
