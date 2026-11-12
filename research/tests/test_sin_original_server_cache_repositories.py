"""Original-free Phase188 scene.cache/modelsounds.cache native mutation tests."""
from __future__ import annotations
import hashlib
import pathlib
import struct
import sys
import unittest
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/'tools'))
import audit_sin_original_server_cache_repositories as mod

class FakePE:
    base=0x22000000
    def __init__(self):self.memory={}
    def at(self,va,n):return bytes(self.memory.get(va+i,0xcc) for i in range(n))
    def put(self,va,raw):
        for i,v in enumerate(raw):
            addr=va+i
            if addr in self.memory and self.memory[addr]!=v:
                raise AssertionError('conflicting x86 witness at %#x'%addr)
            self.memory[addr]=v
    def corrupt(self,va):self.memory[va]=self.memory.get(va,0xcc)^1
    def section(self,va):
        if any(a<=va<a+len(v) for a,v in mod.STRINGS.items()):return '.rdata'
        if 0x224d6000<=va<0x225a1000:return '.data'
        return '.text'

def fixture():
    pe=FakePE()
    for base in mod.INIT_ADDRESSES:
        for off,hexcode in mod.INIT_WITNESSES.items():pe.put(base+off,bytes.fromhex(hexcode))
        for off,target in mod.INIT_CALL_TARGETS[base].items():
            pe.put(base+off,mod._rel32(0xe8,base+off,target))
    for base in mod.RELOAD_ADDRESSES:
        for off,hexcode in mod.RELOAD_WITNESSES.items():pe.put(base+off,bytes.fromhex(hexcode))
        for off,target in mod.RELOAD_CALL_TARGETS[base].items():
            pe.put(base+off,mod._rel32(0xe8,base+off,target))
        for off in mod.RELOAD_TAIL_JMP_OFFSETS:
            pe.put(base+off,mod._rel32(0xe9,base+off,mod.INIT_ADDRESSES[mod.RELOAD_ADDRESSES.index(base)]))
    for addr,(path_addr,version,path) in mod.OBJECTS.items():
        pe.put(addr+0x34,struct.pack('<I',path_addr))
        pe.put(addr+0x38,struct.pack('<I',version))
    for addr,s in mod.STRINGS.items():pe.put(addr,s)
    return pe

class CacheRepositoryTests(unittest.TestCase):
    def test_original_free_fixtures_pass(self):
        result=mod.verify_native(fixture(),whole_hash=False)
        self.assertEqual(result['sparse_opcode_witnesses'],98)
        self.assertEqual(result['direct_relative_calls'],24)
        self.assertEqual(result['reload_tail_jumps'],4)
    def test_region_count_and_exact_lengths(self):
        self.assertEqual(len(mod.REGIONS),4)
        self.assertEqual(sum(n for _,n,sha in mod.REGIONS.values()),1312)
        self.assertEqual([v[1] for v in mod.REGIONS.values()],[576,576,80,80])
    def test_mutate_every_initialization_witness(self):
        for base in mod.INIT_ADDRESSES:
            for off in mod.INIT_WITNESSES:
                with self.subTest(base=hex(base),off=hex(off)):
                    f=fixture();f.corrupt(base+off)
                    with self.assertRaisesRegex(mod.VerificationError,'Init opcode mismatch'):
                        mod.verify_native(f,whole_hash=False)
    def test_mutate_every_reload_witness(self):
        for base in mod.RELOAD_ADDRESSES:
            for off in mod.RELOAD_WITNESSES:
                with self.subTest(base=hex(base),off=hex(off)):
                    f=fixture();f.corrupt(base+off)
                    with self.assertRaisesRegex(mod.VerificationError,'Reload opcode mismatch'):
                        mod.verify_native(f,whole_hash=False)
    def test_mutate_each_init_call(self):
        for base,edges in mod.INIT_CALL_TARGETS.items():
            for off in edges:
                with self.subTest(base=hex(base),off=hex(off)):
                    f=fixture();f.corrupt(base+off+1)
                    with self.assertRaisesRegex(mod.VerificationError,'Init CALL mismatch'):
                        mod.verify_native(f,whole_hash=False)
    def test_mutate_each_reload_call(self):
        for base,edges in mod.RELOAD_CALL_TARGETS.items():
            for off in edges:
                with self.subTest(base=hex(base),off=hex(off)):
                    f=fixture();f.corrupt(base+off+1)
                    with self.assertRaisesRegex(mod.VerificationError,'Reload CALL mismatch'):
                        mod.verify_native(f,whole_hash=False)
    def test_mutate_each_reload_tail_jump(self):
        for base in mod.RELOAD_ADDRESSES:
            for off in mod.RELOAD_TAIL_JMP_OFFSETS:
                with self.subTest(base=hex(base),off=hex(off)):
                    f=fixture();f.corrupt(base+off+1)
                    with self.assertRaisesRegex(mod.VerificationError,'tail JMP mismatch'):
                        mod.verify_native(f,whole_hash=False)
    def test_mutate_original_singleton_path_pointers(self):
        for addr in mod.OBJECTS:
            f=fixture();f.corrupt(addr+0x34)
            with self.assertRaisesRegex(mod.VerificationError,'path pointer mismatch'):
                mod.verify_native(f,whole_hash=False)
    def test_mutate_original_repository_versions(self):
        for addr in mod.OBJECTS:
            f=fixture();f.corrupt(addr+0x38)
            with self.assertRaisesRegex(mod.VerificationError,'repository version mismatch'):
                mod.verify_native(f,whole_hash=False)
    def test_original_diagnostics_are_exact_and_mutable(self):
        for addr in mod.STRINGS:
            with self.subTest(addr=hex(addr)):
                f=fixture();f.corrupt(addr)
                with self.assertRaisesRegex(mod.VerificationError,'diagnostic/string|pathname bytes'):
                    mod.verify_native(f,whole_hash=False)
    def test_four_full_hashes_reject_synthetic_fixture(self):
        f=fixture()
        with self.assertRaisesRegex(mod.VerificationError,'whole native function SHA'):
            mod.verify_native(f,whole_hash=True)
    def test_original_binary_hash_gate_rejects_fake_binaries(self):
        with self.assertRaisesRegex(mod.VerificationError,'full server/client DLL SHA256'):
            mod.audit(b'fake-server',b'fake-client')
    def test_original_names_version_and_sibling_callbacks_differ(self):
        self.assertEqual(mod.OBJECTS[0x22503EE0],(0x2242ED68,3,'scene.cache'))
        self.assertEqual(mod.OBJECTS[0x224F0388],(0x2240BC9C,2,'modelsounds.cache'))
        self.assertNotEqual(mod.INIT_CALL_TARGETS[mod.INIT_ADDRESSES[0]][0x17E],
                            mod.INIT_CALL_TARGETS[mod.INIT_ADDRESSES[1]][0x17E])
        self.assertEqual(len(mod.NORMALIZED_SHA),2)

if __name__=='__main__':unittest.main()