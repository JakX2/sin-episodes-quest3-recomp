"""Synthetic (original DLL-free) PE32 static factory registration regressions."""
from __future__ import annotations
import hashlib
from pathlib import Path
import struct
import sys
import unittest
from unittest.mock import patch
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import audit_sin_original_factories as audit

class SyntheticPE:
    def __init__(self,module):
        info=audit.ORIGINAL[module]
        self.base=info['base']
        self.factory=info['export']
        self.ctor=info['ctor']
        self.head=info['head']
        start=self.base+(0x330000 if module=='client.dll' else 0x570000)
        self.sections=[('.data',start,0x100000,0,0)]
        self.memory={}
        self.names={}
        self.text_va=self.base+0x1000
        n=info['count']
        self.labels=list(info['required'])+[f'SyntheticFactory{i:03d}' for i in range(n-len(info['required']))]
        self.text=bytearray(b'\x90'*(n*20+20))
        self.callback=self.text_va+n*20
        self.text[-20:-14]=b'\xb8'+struct.pack('<I',self.base+0x30000)+b'\xc3'
        self.put(self.ctor,b'\x8b\x54\x24\x04\x8b\xc1\x8b\x4c\x24\x08'
                 b'\x89\x48\x04\x89\x10\x8b\x0d'+struct.pack('<I',self.head)
                 +b'\x89\x48\x08\xa3'+struct.pack('<I',self.head)+b'\xc2\x08\x00')
        for off,raw in ((0x11,b'\x8b\x47\x04'),(0x1a,b'\x3a\x16'),
                        (0x43,b'\x8b\x7f\x08'),(0x52,b'\xc7\x00\x01\x00\x00\x00'),
                        (0x66,b'\xc7\x00\x00\x00\x00\x00'),(0x6c,b'\xff\x17')):
            self.put(self.factory+off,raw)
        for i,label in enumerate(self.labels):
            site=self.text_va+i*20
            nameptr=self.base+0x280000+i*256
            node=start+i*16
            self.names[nameptr]=label
            self.text[i*20:i*20+20]=(b'\x68'+struct.pack('<I',nameptr)
                +b'\x68'+struct.pack('<I',self.callback)
                +b'\xb9'+struct.pack('<I',node)
                +b'\xe8'+struct.pack('<i',self.ctor-site-20))
    def exports(self):return {'CreateInterface':self.factory}
    def code(self):return self.text_va,bytes(self.text)
    def at(self,va,n):
        if self.text_va<=va and va+n<=self.text_va+len(self.text):
            off=va-self.text_va
            return bytes(self.text[off:off+n])
        return bytes(self.memory.get(va+i,0xcc) for i in range(n))
    def put(self,va,raw):
        for i,b in enumerate(raw):self.memory[va+i]=b
    def cstring(self,va):
        if va not in self.names:raise audit.VerificationError('unknown interface string pointer')
        return self.names[va]
    def corrupt(self,va):self.memory[va]^=0x40

class OriginalFactoryTests(unittest.TestCase):
    def analyze(self,module,p):
        with patch.object(audit,'PE32',return_value=p),patch.object(audit,'verify_vtables',return_value={}):
            return audit.inspect_module(b'synthetic',module,require_original=False)
    def test_expected_both_modules(self):
        for module,info in audit.ORIGINAL.items():
            p=SyntheticPE(module);r=self.analyze(module,p)
            self.assertEqual(r['static_registration_count'],info['count'])
            self.assertEqual(r['status_found'],0)
            self.assertEqual(r['status_missing'],1)
            self.assertTrue(set(info['required']).issubset({x['name'] for x in r['registrations']}))
    def test_changed_export_rejected(self):
        p=SyntheticPE('client.dll');p.factory+=4
        with self.assertRaisesRegex(audit.VerificationError,'export'):
            self.analyze('client.dll',p)
    def test_modified_ctor_rejected(self):
        p=SyntheticPE('server.dll');p.corrupt(p.ctor)
        with self.assertRaisesRegex(audit.VerificationError,'constructor'):
            self.analyze('server.dll',p)
    def test_each_factory_opcode_is_pinned(self):
        for offset in (0x11,0x1a,0x43,0x52,0x66,0x6c):
            with self.subTest(offset=offset):
                p=SyntheticPE('client.dll');p.corrupt(p.factory+offset)
                with self.assertRaisesRegex(audit.VerificationError,'lookup opcode'):
                    self.analyze('client.dll',p)
    def test_changed_registration_target_rejected(self):
        p=SyntheticPE('server.dll');p.text[16]^=0x10
        with self.assertRaisesRegex(audit.VerificationError,'registration count'):
            self.analyze('server.dll',p)
    def test_factory_callback_outside_text_rejected(self):
        p=SyntheticPE('client.dll');p.text[6:10]=struct.pack('<I',0x12345678)
        with self.assertRaisesRegex(audit.VerificationError,'callback'):
            self.analyze('client.dll',p)
    def test_node_outside_data_rejected(self):
        p=SyntheticPE('server.dll');p.text[11:15]=struct.pack('<I',1)
        with self.assertRaisesRegex(audit.VerificationError,'node'):
            self.analyze('server.dll',p)
    def test_duplicate_registered_name_rejected(self):
        p=SyntheticPE('server.dll')
        p.names[p.base+0x280000+256]=p.labels[0]
        with self.assertRaisesRegex(audit.VerificationError,'duplicate'):
            self.analyze('server.dll',p)
    def test_missing_mandatory_interface_rejected(self):
        p=SyntheticPE('client.dll');p.names[p.base+0x280000]='UnknownGameInterface'
        with self.assertRaisesRegex(audit.VerificationError,'missing required'):
            self.analyze('client.dll',p)
    def test_original_sha256_is_required(self):
        with self.assertRaisesRegex(audit.VerificationError,'SHA-256'):
            audit.inspect_module(b'synthetic','server.dll')
    def test_vtable_hash_and_slot_guard(self):
        p=SyntheticPE('client.dll')
        address=p.base+0x280f00
        obj=p.base+0x300000
        slots=struct.pack('<3I',p.callback,p.callback,p.callback)
        p.put(obj,struct.pack('<I',address))
        p.put(address,slots+b'\0\0\0\0')
        rows=[{'name':'VClient011','singleton_return_va':hex(obj)}]
        metadata={'VClient011':(address,3,hashlib.sha256(slots).hexdigest())}
        with patch.dict(audit.VTABLES,{'client.dll':metadata}):
            result=audit.verify_vtables(p,'client.dll',rows)
            self.assertEqual(result['VClient011']['verified_consecutive_slot_count'],3)
            p.corrupt(address)
            with self.assertRaisesRegex(audit.VerificationError,'SHA-256'):
                audit.verify_vtables(p,'client.dll',rows)

if __name__=='__main__':unittest.main()
