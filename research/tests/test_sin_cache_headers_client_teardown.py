"""Synthetic original-DLL-free mutation tests for Phase89 x86 evidence."""
import struct,sys,unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from audit_sin_cache_headers_client_teardown import (
 CLIENT_SITES,SERVER_SITES,CALLS,REGIONS,CACHE,
 VerificationError,verify_one,audit)

class PE:
    def __init__(self):self.memory={}
    def put(self,va,raw):
        for i,b in enumerate(raw):
            addr=va+i
            if addr in self.memory and self.memory[addr]!=b:
                raise AssertionError('fixture conflict at %#x'%addr)
            self.memory[addr]=b
    def at(self,va,n):return bytes(self.memory.get(va+i,0xcc) for i in range(n))
    def corrupt(self,va):self.memory[va]^=0x40

def fixture(module):
    p=PE()
    sites=CLIENT_SITES if module=='client.dll' else SERVER_SITES
    for va,raw in sites.items():p.put(va,bytes.fromhex(raw))
    for va,target in CALLS[module].items():
        p.put(va,b'\xe8'+struct.pack('<i',target-va-5))
    if module=='server.dll':
        for _,instance,pointer,name,version in CACHE:
            p.put(instance+0x34,struct.pack('<II',pointer,version))
            p.put(pointer,name)
    return p

class CacheHeaderAndClientCleanupTests(unittest.TestCase):
    def test_original_free_contracts(self):
        c=verify_one(fixture('client.dll'),'client.dll',False)
        s=verify_one(fixture('server.dll'),'server.dll',False)
        self.assertTrue(c['reverse_release'])
        self.assertEqual(s['header_length_bytes'],16)
        self.assertEqual(s['cache_singletons']['scene']['format_version'],3)
        self.assertEqual(s['cache_singletons']['model_sounds']['format_version'],2)
    def test_each_original_opcode_mutation(self):
        for module,sites in (('client.dll',CLIENT_SITES),('server.dll',SERVER_SITES)):
            for va in sites:
                with self.subTest(module=module,site=hex(va)):
                    p=fixture(module);p.corrupt(va)
                    with self.assertRaises(VerificationError):verify_one(p,module,False)
    def test_each_direct_call_mutation(self):
        for module,calls in CALLS.items():
            for va in calls:
                with self.subTest(module=module,site=hex(va)):
                    p=fixture(module);p.corrupt(va+1)
                    with self.assertRaises(VerificationError):verify_one(p,module,False)
    def test_both_cache_name_pointer_and_marker_mutations(self):
        for _,instance,pointer,_,_ in CACHE:
            for va in (instance+0x34,instance+0x38,pointer):
                with self.subTest(site=hex(va)):
                    p=fixture('server.dll');p.corrupt(va)
                    with self.assertRaises(VerificationError):
                        verify_one(p,'server.dll',False)
    def test_all_four_region_hashes_required(self):
        self.assertEqual(sum(map(len,REGIONS.values())),4)
        for module in REGIONS:
            with self.assertRaisesRegex(VerificationError,'SHA-256'):
                verify_one(fixture(module),module,True)
    def test_full_original_dual_DLLs_required(self):
        with self.assertRaisesRegex(VerificationError,'SHA-256'):
            audit(b'fake client',b'fake server')
    def test_neutral_source_key_not_unverified_crc(self):
        s=verify_one(fixture('server.dll'),'server.dll',False)
        self.assertIn('source key',s['four_little_endian_fields'][2]['meaning'])
    def test_signed_count_was_recovered(self):
        s=verify_one(fixture('server.dll'),'server.dll',False)
        self.assertIn('signed',s['four_little_endian_fields'][3]['meaning'])
    def test_no_full_record_schema_claim(self):
        s=verify_one(fixture('server.dll'),'server.dll',False)
        self.assertIn('not recovered',s['payload_schema'])
    def test_client_cleanup_sentinel(self):
        c=verify_one(fixture('client.dll'),'client.dll',False)
        self.assertTrue(c['heap_free_skipped_when_sentinel_is_minus_one'])

if __name__=='__main__':unittest.main()
