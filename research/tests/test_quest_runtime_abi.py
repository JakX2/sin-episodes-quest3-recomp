"""Synthetic ELF64/AArch64 fixtures; commercial game files not required."""
import io
from pathlib import Path
import struct
import tempfile
import unittest
import zipfile
from audit_quest_runtime_abi import AuditError, REQUIRED, analyze, inspect_elf

def fake_elf(*,machine=183,factory=True):
    labels=b'\x00.shstrtab\x00.dynstr\x00.dynsym\x00.dynamic\x00'
    strings=b'\x00CreateInterface\x00liblog.so\x00libmock.so\x00'
    names=[labels.index(n) for n in (b'.shstrtab',b'.dynstr',b'.dynsym',b'.dynamic')]
    sym=b'\x00'*24+struct.pack('<IBBHQQ',1,0x12,0,1 if factory else 0,0,120)
    dyn=struct.pack('<qQ',1,strings.index(b'liblog.so'))
    dyn+=struct.pack('<qQ',14,strings.index(b'libmock.so'))
    dyn+=struct.pack('<qQ',0,0)
    chunks=(labels,strings,sym,dyn)
    result=bytearray(64);headers=[(0,0,0,0,0,0,0,0,0,0)]
    for i,chunk in enumerate(chunks):
        result+=bytes((-len(result))%8)
        offset=len(result);result+=chunk
        headers.append((names[i],(3,3,11,6)[i],0,0,offset,
                       len(chunk),2 if i in (2,3) else 0,0,8,(0,0,24,16)[i]))
    result+=bytes((-len(result))%8)
    off=len(result)
    for h in headers:result+=struct.pack('<IIQQQQIIQQ',*h)
    ident=b'\x7fELF'+bytes((2,1,1))+bytes(9)
    result[:64]=struct.pack('<16sHHIQQQIHHHHHH',ident,3,machine,1,
                             0,0,off,0,64,0,0,64,5,1)
    return bytes(result)

def apk(*,omit=None,omit_factory=None):
    data=io.BytesIO()
    with zipfile.ZipFile(data,'w') as z:
        for name in REQUIRED:
            if name==omit:continue
            z.writestr('lib/arm64-v8a/'+name,
                       fake_elf(factory=name!=omit_factory))
        z.writestr('assets/hl2quest_turbo/arm64-v8a/libclient.so',fake_elf())
        z.writestr('assets/hl2quest_turbo/arm64-v8a/libengine.so',fake_elf())
    return data.getvalue()

class QuestRuntimeTests(unittest.TestCase):
    def test_parse_factory_and_dependency(self):
        x=inspect_elf(fake_elf())
        self.assertEqual(x['architecture'],'AArch64 ELF64')
        self.assertIn('liblog.so',x['needed'])
        self.assertTrue(x['create_interface_exported'])
    def test_reject_wrong_arch_and_invalid_header(self):
        with self.assertRaisesRegex(AuditError,'AArch64'):
            inspect_elf(fake_elf(machine=62))
        with self.assertRaisesRegex(AuditError,'ELF64'):
            inspect_elf(b'not ELF')
    def test_parse_apk(self):
        with tempfile.TemporaryDirectory() as temp:
            p=Path(temp)/'fixture.apk';p.write_bytes(apk())
            x=analyze(p)
        self.assertEqual(x['installed_count'],len(REQUIRED))
        self.assertEqual(x['container'],'APK')
        self.assertTrue(x['no_sin_named_modules'])
        self.assertIn('libclient.so',x['matching_variants'])
    def test_zip_holding_single_apk(self):
        with tempfile.TemporaryDirectory() as temp:
            p=Path(temp)/'outer.zip'
            with zipfile.ZipFile(p,'w') as z:z.writestr('HL2VR/test.apk',apk())
            x=analyze(p)
        self.assertEqual(x['container'],'ZIP containing APK')
        self.assertEqual(x['inner_apk_name'],'HL2VR/test.apk')
    def test_missing_essential_module_rejected(self):
        with tempfile.TemporaryDirectory() as temp:
            p=Path(temp)/'bad.apk';p.write_bytes(apk(omit='libserver.so'))
            with self.assertRaisesRegex(AuditError,'libserver.so'):analyze(p)
    def test_missing_engine_factory_rejected(self):
        with tempfile.TemporaryDirectory() as temp:
            p=Path(temp)/'bad.apk';p.write_bytes(apk(omit_factory='libengine.so'))
            with self.assertRaisesRegex(AuditError,'CreateInterface'):analyze(p)

if __name__=='__main__':unittest.main()
