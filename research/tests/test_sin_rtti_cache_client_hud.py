"""Synthetic original-DLL-free tests for original 2006 SiN RTTI/vtables."""
import struct
import sys
import unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from audit_sin_rtti_cache_client_hud import (
    CLIENT,SERVER,SERVER_METHOD_ANCHORS,VerificationError,
    inspect_client,inspect_server,audit)

class FakePE:
    code_bytes=bytes(0x400000)
    def __init__(self,base):self.base=base;self.memory={};self.strings={}
    def at(self,a,n):return bytes(self.memory.get(a+i,0xcc) for i in range(n))
    def code(self):return self.base,self.code_bytes
    def cstring(self,a):return self.strings[a]
    def put(self,a,data):
        for i,b in enumerate(data):
            if a+i in self.memory and self.memory[a+i]!=b:
                raise AssertionError('fixture overlap at %#x'%(a+i))
            self.memory[a+i]=b
    def word(self,a,v):self.put(a,struct.pack('<I',v))
    def tamper(self,a):self.memory[a]^=0x40

def client_fixture():
    p=FakePE(0x24000000)
    for cell,obj,vt,loc,name,methods,digest,slot in CLIENT:
        p.word(cell,obj);p.word(obj,vt);p.word(vt-4,loc)
        p.put(loc,bytes(12))
        typ=0x24400000+(cell&0x3ffff)
        p.word(loc+12,typ);p.strings[typ+8]=name
        p.put(vt,struct.pack('<%dI'%len(methods),*methods))
        p.word(vt+len(methods)*4,0)
    return p

def server_fixture():
    p=FakePE(0x22000000)
    for vt,loc,name,methods,digest,insert,alloc in SERVER:
        p.word(vt-4,loc);p.put(loc,bytes(12))
        typ=0x22600000+(vt&0xffff)
        p.word(loc+12,typ);p.strings[typ+8]=name
        p.put(vt,struct.pack('<3I',*methods));p.word(vt+12,0)
    for k,v in SERVER_METHOD_ANCHORS.items():
        p.put(int(k,16),bytes.fromhex(v))
    return p

class OriginalRttiTests(unittest.TestCase):
    def test_both_original_free_synthetic_tables(self):
        self.assertEqual(inspect_client(client_fixture())['class_count'],8)
        self.assertEqual(inspect_server(server_fixture())['class_count'],2)
    def test_client_global_cell_mutation(self):
        for cell,*_ in CLIENT:
            p=client_fixture();p.tamper(cell)
            with self.assertRaises(VerificationError):inspect_client(p)
    def test_client_object_vptr_mutation(self):
        for _,obj,*_ in CLIENT:
            p=client_fixture();p.tamper(obj)
            with self.assertRaises(VerificationError):inspect_client(p)
    def test_client_rtti_locator_mutation(self):
        for _,_,vt,*_ in CLIENT:
            p=client_fixture();p.tamper(vt-4)
            with self.assertRaises(VerificationError):inspect_client(p)
    def test_client_rtti_class_name_mutation(self):
        for cell,obj,vt,loc,*_ in CLIENT:
            p=client_fixture();type_ptr=struct.unpack('<I',p.at(loc+12,4))[0]
            p.strings[type_ptr+8]='.?AVWrongClass@@'
            with self.assertRaises(VerificationError):inspect_client(p)
    def test_client_vtable_sha_mutation(self):
        for _,_,vt,*_ in CLIENT:
            p=client_fixture();p.tamper(vt)
            with self.assertRaises(VerificationError):inspect_client(p)
    def test_server_rtti_locator_mutation(self):
        for vt,*_ in SERVER:
            p=server_fixture();p.tamper(vt-4)
            with self.assertRaises(VerificationError):inspect_server(p)
    def test_server_class_name_mutation(self):
        for vt,loc,*_ in SERVER:
            p=server_fixture();type_ptr=struct.unpack('<I',p.at(loc+12,4))[0]
            p.strings[type_ptr+8]='.?AVWrongCache@@'
            with self.assertRaises(VerificationError):inspect_server(p)
    def test_all_six_server_virtual_method_pointer_mutations(self):
        for vt,loc,name,methods,*_ in SERVER:
            for i in range(len(methods)):
                p=server_fixture();p.tamper(vt+4*i)
                with self.assertRaises(VerificationError):inspect_server(p)
    def test_all_server_method_opcode_mutations(self):
        for site in SERVER_METHOD_ANCHORS:
            p=server_fixture();p.tamper(int(site,16))
            with self.assertRaises(VerificationError):inspect_server(p)
    def test_extra_executable_vtable_slot_rejected(self):
        p=server_fixture()
        for i,b in enumerate(struct.pack('<I',0x221ad750)):
            p.memory[0x2242dca0+12+i]=b
        with self.assertRaisesRegex(VerificationError,'count incomplete'):inspect_server(p)
    def test_original_dll_sha256_chain_required(self):
        with self.assertRaisesRegex(VerificationError,'SHA-256'):
            audit(b'fake client',b'fake server')

if __name__=='__main__':unittest.main()
