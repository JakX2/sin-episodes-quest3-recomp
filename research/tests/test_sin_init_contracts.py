"""Original DLL-free synthetic mutation tests for SiN client/server early Init."""
import struct
import sys
from pathlib import Path
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"tools"))
from audit_sin_init_contracts import INIT,LATE,audit_init_one,VerificationError,audit

class FakePE:
    def __init__(self):self.memory={};self.names={}
    def at(self,va,n):return bytes(self.memory.get(va+i,0xcc) for i in range(n))
    def put(self,va,data):
        for i,b in enumerate(data):
            key=va+i
            if key in self.memory and self.memory[key]!=b:
                raise AssertionError("overlapping synthetic fixture at %#x"%key)
            self.memory[key]=b
    def cstring(self,va):return self.names[va]
    def corrupt(self,va):self.memory[va]^=0x10

def fixture(module):
    spec=INIT[module];late=LATE[module];p=FakePE()
    for idx,(name,site,reg) in enumerate(spec["requirements"]):
        push=site-(10 if module=="client.dll" and idx==0 else 5)
        ptr=0x23000000+idx*256
        p.names[ptr]=name
        p.put(push-2,b'\x6a\x00')
        p.put(push,b'\x68'+struct.pack('<I',ptr))
        p.put(site,b'\xff'+(b'\xd6' if reg=="esi" else b'\xd7'))
        p.put(site+2,b'\x83\xc4'+(b'\x28' if module=="client.dll" and idx==0 else b'\x08'))
        p.put(site+5,b'\x85\xc0')
        p.put(site+7,b'\xa3'+struct.pack('<I',0x24300000+idx*4))
        branch=site+12
        p.put(branch,b'\x0f\x84'+struct.pack('<i',spec["fail"]-branch-6))
    p.put(late["gate_virtual"],b'\xff'+(b'\x12' if module=="client.dll" else b'\x10'))
    p.put(late["gate_virtual"]+2,b'\x84\xc0')
    branch=late["gate_branch"]
    p.put(branch,b'\x0f\x84'+struct.pack('<i',spec["fail"]-branch-6))
    p.put(late["return_true"],b'\xb8\x01\x00\x00\x00' if module=="client.dll" else b'\x5f\xb0\x01')
    p.put(late["return_false"],b'\x33\xc0' if module=="client.dll" else b'\x5f\x32\xc0')
    ret=0x240fe59b if module=="client.dll" else 0x221e79a8
    p.put(ret,b'\xc2'+struct.pack('<H',late["ret_bytes"]))
    p.put(late["optional_query"],b'\xff\xd6')
    if module=="server.dll":p.put(0x221e7743,b'\x8b\x7c\x24\x20')
    else:
        p.put(0x240fe105,b'\x8b\x74\x24\x28')
        p.put(0x240fe101,b'\x8b\x44\x24\x30')
    return p

class TestInitHandshake(unittest.TestCase):
    def test_expected_order_and_stack_cleanup(self):
        for mod,count in (("client.dll",21),("server.dll",16)):
            result=audit_init_one(fixture(mod),mod,False)
            self.assertEqual(result["required_core_count"],count)
            self.assertEqual(result["observed_callee_stack_cleanup_bytes"],12 if mod=="client.dll" else 16)
            self.assertEqual(result["ordered_required_factory_calls"][-1]["name"],
                             "VSoundEmitter001" if mod=="client.dll" else "ISERVERPLUGINHELPERS001")
    def test_all_37_names_mutated(self):
        for mod in INIT:
            for idx,(_,site,_) in enumerate(INIT[mod]["requirements"]):
                with self.subTest(mod=mod,index=idx):
                    p=fixture(mod);p.names[0x23000000+idx*256]="NOT_ORIGINAL"
                    with self.assertRaises(VerificationError):audit_init_one(p,mod,False)
    def test_all_37_call_instructions_mutated(self):
        for mod in INIT:
            for _,site,_ in INIT[mod]["requirements"]:
                with self.subTest(mod=mod,site=hex(site)):
                    p=fixture(mod);p.corrupt(site)
                    with self.assertRaises(VerificationError):audit_init_one(p,mod,False)
    def test_all_37_global_stores_mutated(self):
        for mod in INIT:
            for _,site,_ in INIT[mod]["requirements"]:
                with self.subTest(mod=mod,site=hex(site)):
                    p=fixture(mod);p.corrupt(site+7)
                    with self.assertRaises(VerificationError):audit_init_one(p,mod,False)
    def test_all_37_missing_result_branches_mutated(self):
        for mod in INIT:
            for _,site,_ in INIT[mod]["requirements"]:
                with self.subTest(mod=mod,site=hex(site)):
                    p=fixture(mod);p.corrupt(site+14)
                    with self.assertRaises(VerificationError):audit_init_one(p,mod,False)
    def test_third_factory_argument_protected(self):
        p=fixture("server.dll");p.corrupt(0x221e774f)
        with self.assertRaises(VerificationError):audit_init_one(p,"server.dll",False)
    def test_emitter_vtable_0_guard(self):
        for mod in INIT:
            for site in (LATE[mod]["gate_virtual"],LATE[mod]["gate_branch"]):
                p=fixture(mod);p.corrupt(site)
                with self.assertRaises(VerificationError):audit_init_one(p,mod,False)
    def test_original_returns_pinned(self):
        for mod,site in (("client.dll",0x240fe59c),("server.dll",0x221e79a9)):
            p=fixture(mod);p.corrupt(site)
            with self.assertRaises(VerificationError):audit_init_one(p,mod,False)
    def test_exact_original_function_hashes_mandatory(self):
        for mod in INIT:
            with self.assertRaisesRegex(VerificationError,"SHA-256"):
                audit_init_one(fixture(mod),mod,True)
    def test_original_Dll_fingerprints_mandatory(self):
        with self.assertRaisesRegex(VerificationError,"SHA-256"):
            audit(b"fake client",b"fake server")
if __name__=="__main__":
    unittest.main()
