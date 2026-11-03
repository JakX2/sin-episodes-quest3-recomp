"""Synthetic original-DLL-free SourceVR ARM64 registration/manifest regressions."""
import io
import struct
import sys
import unittest
import zipfile
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import audit_sourcevr_native_sin_factories as a

class FakeELF:
    def __init__(self,module):
        self.module=module
        self.data=b'fake native without original copyrighted content'
        self.memory={}
        for addr,word in (a.SOUND_INST if module=='libsoundemittersystem.so' else a.VGUI_INST).items():
            self.put(addr,struct.pack('<I',word))
        if module=='libsoundemittersystem.so':
            self.put(0x2879,b'VSoundEmitter002\0')
            self.start=(0x18D50,)
            self.export=0x1A300
        else:
            self.put(0x54D4,b'VGUI_InputInternal001\0')
            self.put(0x654E,b'VGUI_System010\0')
            self.start=(0x269D0,0x3DFB0)
            self.export=0x40EA0
    def put(self,addr,raw):
        for i,b in enumerate(raw):
            if addr+i in self.memory and self.memory[addr+i]!=b:
                raise AssertionError('synthetic overlap at %#x'%(addr+i))
            self.memory[addr+i]=b
    def at(self,addr,count):
        return bytes(self.memory.get(addr+i,0xcc) for i in range(count))
    def symbol(self,name):
        if name!='CreateInterface':raise a.VerificationError('unknown symbol')
        return {'va':self.export,'size':120,'type':2}
    def init_array(self):return self.start
    def corrupt(self,addr):self.memory[addr]^=0x40

class SourceVR96Tests(unittest.TestCase):
    def test_both_native_factories(self):
        for lib,n in (('libsoundemittersystem.so',1),('libvgui2.so',2)):
            r=a.verify_native_registration(FakeELF(lib),lib,False)
            self.assertEqual(len(r['native_registration_entries']),n)
            self.assertFalse(r['abi_methods_verified'])
            self.assertTrue(r['exact_case_sensitive_name_match'])
    def test_every_native_factory_opcode_mutation(self):
        for lib,sites in (('libsoundemittersystem.so',a.SOUND_INST),
                          ('libvgui2.so',a.VGUI_INST)):
            for addr in sites:
                with self.subTest(lib=lib,addr=hex(addr)):
                    f=FakeELF(lib);f.corrupt(addr)
                    with self.assertRaisesRegex(a.VerificationError,'opcode mismatch'):
                        a.verify_native_registration(f,lib,False)
    def test_all_registered_interface_names(self):
        for lib,sites in (('libsoundemittersystem.so',(0x2879,)),
                          ('libvgui2.so',(0x54D4,0x654E))):
            for addr in sites:
                f=FakeELF(lib);f.corrupt(addr)
                with self.assertRaisesRegex(a.VerificationError,'registered name'):
                    a.verify_native_registration(f,lib,False)
    def test_createinterface_export_must_match(self):
        f=FakeELF('libsoundemittersystem.so');f.export=0xBAD
        with self.assertRaisesRegex(a.VerificationError,'exported location'):
            a.verify_native_registration(f,'libsoundemittersystem.so',False)
    def test_init_array_must_contain_registrations(self):
        f=FakeELF('libvgui2.so');f.start=(0x269D0,)
        with self.assertRaisesRegex(a.VerificationError,'initializer'):
            a.verify_native_registration(f,'libvgui2.so',False)
    def test_native_whole_sha256_required(self):
        for lib in a.NATIVE:
            with self.assertRaisesRegex(a.VerificationError,'SHA-256'):
                a.verify_native_registration(FakeELF(lib),lib,True)
    def test_elf_rejects_wrong_class_or_machine(self):
        with self.assertRaises(a.VerificationError):a.ELF64(b'')
        raw=bytearray(64);raw[:6]=b'\x7fELF\x02\x01'
        struct.pack_into('<HH',raw,16,3,3)
        with self.assertRaisesRegex(a.VerificationError,'AArch64'):
            a.ELF64(bytes(raw))
    def test_seven_native_manifests_include_both_libraries(self):
        memory=io.BytesIO()
        with zipfile.ZipFile(memory,'w') as z:
            for profile in a.PROFILES:
                lines=[f'SOURCEVR_NATIVE_V1\t{profile}']
                for lib,sha in a.NATIVE.items():
                    lines.append(f'0001\t{sha}\t{lib}\t{lib}\tsourcevr/v1/native/blobs/{sha}/{lib}\t-')
                z.writestr(a.ZIP_PREFIX+'sourcevr/v1/native/'+profile+'.manifest',
                           '\n'.join(lines)+'\n')
        memory.seek(0)
        with zipfile.ZipFile(memory) as z:
            for profile in a.PROFILES:
                rows=a.manifest_rows(z,profile)
                self.assertEqual(set(rows),set(a.NATIVE))
                for lib in a.NATIVE:self.assertEqual(rows[lib]['sha256'],a.NATIVE[lib])
    def test_invalid_and_duplicate_manifest_rows(self):
        cases=[('INVALID\n','invalid SourceVR'),
               ('SOURCEVR_NATIVE_V1\thl2\n0000\tX\tlibvgui2.so\tx\tx\tx\n'
                '0001\tX\tlibvgui2.so\tx\tx\tx\n','duplicate')]
        for lines,pattern in cases:
            m=io.BytesIO()
            with zipfile.ZipFile(m,'w') as z:
                z.writestr(a.ZIP_PREFIX+'sourcevr/v1/native/hl2.manifest',lines)
            m.seek(0)
            with zipfile.ZipFile(m) as z:
                with self.assertRaisesRegex(a.VerificationError,pattern):
                    a.manifest_rows(z,'hl2')
    def test_exact_factory_status_zero_found_one_absent(self):
        r=a.verify_native_registration(FakeELF('libsoundemittersystem.so'),
                                        'libsoundemittersystem.so',False)
        self.assertEqual(r['found_return_status'],0)
        self.assertEqual(r['missing_return_status'],1)
    def test_previously_unexpected_legacy_emitter_string_caught(self):
        f=FakeELF('libsoundemittersystem.so');f.data+=b'VSoundEmitter001\0'
        with self.assertRaisesRegex(a.VerificationError,'unexpected legacy'):
            a.verify_native_registration(f,'libsoundemittersystem.so',False)
    def test_original_PE32_provenance_required(self):
        with self.assertRaisesRegex(a.VerificationError,'SHA-256'):
            a.audit_original_sin(b'fake client',b'fake server')
if __name__=='__main__':unittest.main()
