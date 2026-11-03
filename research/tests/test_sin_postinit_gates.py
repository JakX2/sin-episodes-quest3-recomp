"""Private-original-free mutation regressions for client/server late Init."""
from __future__ import annotations
import struct
import sys
from pathlib import Path
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from audit_sin_postinit_gates import POST,audit_post_one,VerificationError,audit

class FakePE:
    def __init__(self):self.data={};self.names={}
    def put(self,va,raw):
        for i,b in enumerate(raw):
            at=va+i
            if at in self.data and self.data[at]!=b:
                raise AssertionError('fixture overlap %#x'%at)
            self.data[at]=b
    def at(self,va,n):return bytes(self.data.get(va+i,0xcc) for i in range(n))
    def corrupt(self,va):self.data[va]^=0x10
    def cstring(self,va):return self.names[va]

def fixture(mod):
    pe=FakePE(); spec=POST[mod]
    for site,target,test,branch,kind in spec['gate_direct']:
        pe.put(site,b'\xe8'+struct.pack('<i',target-site-5))
        pe.put(test,b'\x84\xc0')
        offset=spec['fail']-branch-(6 if kind=='near' else 2)
        pe.put(branch,b'\x0f\x84'+struct.pack('<i',offset) if kind=='near'
               else b'\x74'+struct.pack('<b',offset))
    for site,encoding,test,branch,kind,name,push in spec['gate_indirect']:
        ptr=0x22800000+(push&0xfff)*16
        pe.names[ptr]=name
        pe.put(push,b'\x68'+struct.pack('<I',ptr))
        pe.put(site,bytes.fromhex(encoding))
        pe.put(test,b'\x85\xc0')
        offset=spec['fail']-branch-(6 if kind=='near' else 2)
        pe.put(branch,b'\x0f\x84'+struct.pack('<i',offset) if kind=='near'
               else b'\x74'+struct.pack('<b',offset))
    if mod=='client.dll':
        for push,name in ((0x240fe349,'VMaterialSystemStub001'),(0x240fe332,'-textmode')):
            ptr=0x22700000+(push&0xfff)*16
            pe.names[ptr]=name;pe.put(push,b'\x68'+struct.pack('<I',ptr))
        for va,raw in ((0x240fe34e,'ffd6'),(0x240fe35b,'a3a0f63324'),
                       (0x240fe339,'ff5228'),(0x240fe33c,'85c07407'),
                       (0x240fe340,'c605fcf6332401')):
            pe.put(va,bytes.fromhex(raw))
    else:
        for push,name in ((0x221e783a,'resource/gameevents.res'),
                          (0x221e791f,'resource/ModEvents.res'),
                          (0x221e7847,'sv_cheats'),
                          (0x221e7944,'VDebugOverlay003')):
            ptr=0x22900000+(push&0xfff)*16
            pe.names[ptr]=name;pe.put(push,b'\x68'+struct.pack('<I',ptr))
        pe.put(0x221e783f,bytes.fromhex('ff5204'))
        pe.put(0x221e7924,bytes.fromhex('ff5204'))
        pe.put(0x221e784c,b'\xe8'+struct.pack('<i',0x2225bc00-0x221e784c-5))
        for va,raw in ((0x221e7854,'85c0'),(0x221e7856,'a3685e5722'),
                       (0x221e7949,'ffd6'),(0x221e7950,'a3cc215722'),
                       (0x221e794b,'6810050000'),(0x221e795d,'85c07421'),
                       (0x221e7978,'5fb001'),(0x221e7997,'5fb001'),
                       (0x221e7988,'c70588b8542200000000')):
            pe.put(va,bytes.fromhex(raw))
        branch=0x221e785b
        pe.put(branch,b'\x0f\x84'+struct.pack('<i',spec['fail']-branch-6))
        pe.put(0x221e7955,b'\xe8'+struct.pack('<i',0x2226e790-0x221e7955-5))
    return pe

class TestLateInit(unittest.TestCase):
    def test_exact_gate_inventory(self):
        c=audit_post_one(fixture('client.dll'),'client.dll',False)
        s=audit_post_one(fixture('server.dll'),'server.dll',False)
        self.assertEqual(c['late_required_check_count'],7)
        self.assertEqual(s['late_required_check_count'],4)
        self.assertEqual(s['resource_registration_paths'],
                         ['resource/gameevents.res','resource/ModEvents.res'])
    def test_all_direct_calls_reject_modified_target(self):
        for module,group in POST.items():
            for site,*rest in group['gate_direct']:
                with self.subTest(module=module,site=hex(site)):
                    p=fixture(module);p.corrupt(site+1)
                    with self.assertRaises(VerificationError):audit_post_one(p,module,False)
    def test_all_helper_success_guards_reject_mutation(self):
        for module,group in POST.items():
            for site,target,test,branch,kind in group['gate_direct']:
                with self.subTest(module=module,site=hex(branch)):
                    p=fixture(module);p.corrupt(branch+1 if kind=='near' else branch)
                    with self.assertRaises(VerificationError):audit_post_one(p,module,False)
    def test_all_virtual_queries_and_names_are_pinned(self):
        for site,encoding,test,branch,kind,name,push in POST['client.dll']['gate_indirect']:
            p=fixture('client.dll');p.corrupt(site)
            with self.assertRaises(VerificationError):audit_post_one(p,'client.dll',False)
            p=fixture('client.dll');p.names.clear()
            with self.assertRaises((KeyError,VerificationError)):
                audit_post_one(p,'client.dll',False)
    def test_client_textmode_not_a_required_false_gate(self):
        p=fixture('client.dll');p.corrupt(0x240fe340)
        with self.assertRaises(VerificationError):audit_post_one(p,'client.dll',False)
    def test_client_nonfatal_factory_pinned(self):
        p=fixture('client.dll');p.corrupt(0x240fe34e)
        with self.assertRaises(VerificationError):audit_post_one(p,'client.dll',False)
    def test_server_event_registration_sites_pinned(self):
        for site in (0x221e783f,0x221e7924):
            p=fixture('server.dll');p.corrupt(site)
            with self.assertRaises(VerificationError):audit_post_one(p,'server.dll',False)
    def test_server_sv_cheats_is_required(self):
        for site in (0x221e784c,0x221e7856,0x221e785b):
            p=fixture('server.dll');p.corrupt(site)
            with self.assertRaises(VerificationError):audit_post_one(p,'server.dll',False)
    def test_server_optional_factory_and_allocation_fallback_pinned(self):
        for site in (0x221e7949,0x221e7955,0x221e795d,0x221e7978,0x221e7997):
            p=fixture('server.dll');p.corrupt(site)
            with self.assertRaises(VerificationError):audit_post_one(p,'server.dll',False)
    def test_both_original_region_hashes_required(self):
        for module in POST:
            with self.assertRaisesRegex(VerificationError,'SHA-256'):
                audit_post_one(fixture(module),module,True)
    def test_dual_original_dll_sha_pins_required(self):
        with self.assertRaisesRegex(VerificationError,'SHA-256'):
            audit(b'not original client',b'not original server')

if __name__=='__main__':unittest.main()
