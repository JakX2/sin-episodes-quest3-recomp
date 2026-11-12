"""Original-free mutation regression tests for Phase185 2006 heap forwarding."""
from __future__ import annotations
import hashlib
import pathlib
import struct
import sys
import unittest
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/'tools'))
import audit_sin_original_client_memalloc_bridge as m

class FakePE:
    base=0x24000000
    def __init__(self):self.mem={};self.imports=dict(m.IMPORTS)
    def at(self,va,n):return bytes(self.mem.get(va+i,0xCC) for i in range(n))
    def put(self,va,data):
        for i,v in enumerate(data):
            here=va+i
            if here in self.mem and self.mem[here]!=v:
                raise AssertionError('synthetic instruction conflict at %#x'%here)
            self.mem[here]=v
    def flip(self,va):self.mem[va]=self.mem.get(va,0xCC)^0x01
    def import_iat(self):return self.imports

def fake():
    p=FakePE()
    for addr,hexcode in m.OPCODES.items():p.put(addr,bytes.fromhex(hexcode))
    for source,dest in m.CALLS.items():p.put(source,b'\xe8'+struct.pack('<i',dest-source-5))
    return p

class HeapBridgeTests(unittest.TestCase):
    def test_all_verified_opcode_and_call_witnesses(self):
        observed=m.verify_native(fake(),whole_hash=False)
        self.assertEqual(observed['native_opcode_witnesses'],37)
        self.assertEqual(observed['native_direct_call_edges'],2)
    def test_all_original_function_lengths_and_unique_addresses(self):
        self.assertEqual(len(m.REGIONS),7)
        self.assertEqual(sum(n for va,n,digest in m.REGIONS.values()),262)
        self.assertEqual(len({va for va,n,digest in m.REGIONS.values()}),7)
        self.assertTrue(all(len(bytes.fromhex(digest))==32 for va,n,digest in m.REGIONS.values()))
    def test_corrupt_every_original_opcode_witness(self):
        for va in m.OPCODES:
            with self.subTest(address=hex(va)):
                p=fake();p.flip(va)
                with self.assertRaisesRegex(m.VerificationError,'opcode witness'):
                    m.verify_native(p,whole_hash=False)
    def test_mutate_every_debug_owner_probe_call(self):
        for source in m.CALLS:
            with self.subTest(address=hex(source)):
                p=fake();p.flip(source)
                with self.assertRaisesRegex(m.VerificationError,'resolver CALL'):
                    m.verify_native(p,whole_hash=False)
    def test_corrupt_each_import_library_and_location(self):
        for key in m.IMPORTS:
            with self.subTest(key=key):
                p=fake();p.imports[key]+=4
                with self.assertRaisesRegex(m.VerificationError,'IAT mismatch'):
                    m.verify_native(p,whole_hash=False)
                p=fake();p.imports.pop(key)
                with self.assertRaisesRegex(m.VerificationError,'IAT mismatch'):
                    m.verify_native(p,whole_hash=False)
    def test_imports_are_distinct_modules_not_embedded_heap_code(self):
        self.assertEqual(m.IMPORTS[('tier0.dll','g_pMemAlloc')],0x242761CC)
        self.assertEqual(m.IMPORTS[('vstdlib.dll','KeyValuesSystem')],0x24276230)
        self.assertEqual(m.CALLS,{0x2416FD03:0x2416F7F0,0x2416FD93:0x2416F7F0})
    def test_synthetic_cannot_pretend_original_whole_body_digest(self):
        with self.assertRaisesRegex(m.VerificationError,'complete heap bridge function SHA'):
            m.verify_native(fake(),whole_hash=True)
    def test_wrong_original_image_base(self):
        p=fake();p.base=0
        with self.assertRaisesRegex(m.VerificationError,'image base'):
            m.verify_native(p,whole_hash=False)
    def test_both_original_file_sha_guards(self):
        with self.assertRaisesRegex(m.VerificationError,'length'):
            m.audit(b'x',b'y')
        with self.assertRaisesRegex(m.VerificationError,'complete DLL SHA256'):
            m.audit(bytes(3592192),bytes(6049792))
    def test_duplicate_forwarders_match_but_alloc_debug_callsite_differs(self):
        self.assertEqual(m.REGIONS['free_a'][2],m.REGIONS['free_b'][2])
        self.assertEqual(m.REGIONS['debug_alloc_direct_a'][2],m.REGIONS['debug_alloc_direct_b'][2])
        self.assertNotEqual(m.REGIONS['alloc_cached_a'][2],m.REGIONS['alloc_cached_b'][2])
    def test_full_real_original_validation(self):
        paths=[pathlib.Path('/mnt/data/sin_original/client.dll'),pathlib.Path('/mnt/data/sin_original/server.dll')]
        if not all(p.exists() for p in paths):self.skipTest('private original files not present in CI')
        result=m.audit(paths[0].read_bytes(),paths[1].read_bytes())
        self.assertEqual(result['client_allocator_bridge']['verified_native_functions'],7)
        self.assertEqual(result['client_allocator_bridge']['complete_native_region_bytes'],262)

if __name__=='__main__':unittest.main()