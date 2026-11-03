"""Synthetic original-file-free Phase 87 client/server startup mutation tests."""
import pathlib
import struct
import sys
import unittest
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/'tools'))
from audit_sin_subsystem_startup import (
    SITES,CALLS,TAIL_JUMPS,STRINGS,REGIONS,verify_subsystem,
    VerificationError,audit)

class SyntheticPE:
    def __init__(self): self.memory={}
    def at(self,va,count):return bytes(self.memory.get(va+i,0xcc) for i in range(count))
    def put(self,va,raw):
        for i,b in enumerate(raw):
            address=va+i
            if address in self.memory and self.memory[address]!=b:
                raise AssertionError('synthetic overlap at %#x'%address)
            self.memory[address]=b
    def corrupt(self,va):self.memory[va]^=0x20

def fixture(module):
    p=SyntheticPE()
    for va,raw in SITES[module].items():p.put(va,bytes.fromhex(raw))
    for va,target in CALLS[module].items():
        p.put(va,b'\xe8'+struct.pack('<i',target-va-5))
    for va,target in TAIL_JUMPS[module].items():
        p.put(va,b'\xe9'+struct.pack('<i',target-va-5))
    for va,raw in STRINGS[module].items():p.put(va,raw)
    return p

class StartupTest(unittest.TestCase):
    def test_both_modules_pass_synthetic_evidence(self):
        for module in SITES:
            r=verify_subsystem(fixture(module),module,False)
            self.assertEqual(r['opcode_sites'],len(SITES[module]))
            self.assertEqual(r['call_edges'],len(CALLS[module]))
    def test_client_rollback_contract(self):
        r=verify_subsystem(fixture('client.dll'),'client.dll',False)
        self.assertEqual(r['registry_x86_fields']['teardown'],'+0x08')
        self.assertTrue(r['rollback_on_required_failure'])
    def test_server_registry_contract(self):
        r=verify_subsystem(fixture('server.dll'),'server.dll',False)
        self.assertEqual(r['initialized_flag_va'],'0x22572CB8')
        self.assertTrue(r['initialized_flag_set_before_callbacks'])
        self.assertEqual(len(r['repository_wrappers']),2)
    def test_every_instruction_rejects_mutation(self):
        self.assertEqual(sum(map(len,SITES.values())),73)
        for module in SITES:
            for va in SITES[module]:
                with self.subTest(module=module,va=hex(va)):
                    p=fixture(module);p.corrupt(va)
                    with self.assertRaises(VerificationError):
                        verify_subsystem(p,module,False)
    def test_every_call_edge_rejects_mutation(self):
        self.assertEqual(sum(map(len,CALLS.values())),16)
        for module in CALLS:
            for va in CALLS[module]:
                with self.subTest(module=module,va=hex(va)):
                    p=fixture(module);p.corrupt(va+1)
                    with self.assertRaises(VerificationError):
                        verify_subsystem(p,module,False)
    def test_both_repository_tail_jumps_reject_mutation(self):
        self.assertEqual(len(TAIL_JUMPS['server.dll']),2)
        for va in TAIL_JUMPS['server.dll']:
            p=fixture('server.dll');p.corrupt(va+1)
            with self.assertRaises(VerificationError):
                verify_subsystem(p,'server.dll',False)
    def test_all_original_strings_reject_mutation(self):
        for module in STRINGS:
            for va in STRINGS[module]:
                with self.subTest(module=module,va=hex(va)):
                    p=fixture(module);p.corrupt(va)
                    with self.assertRaises(VerificationError):
                        verify_subsystem(p,module,False)
    def test_all_nine_region_hashes_required(self):
        self.assertEqual(sum(map(len,REGIONS.values())),9)
        for module in REGIONS:
            with self.assertRaisesRegex(VerificationError,'SHA-256'):
                verify_subsystem(fixture(module),module,True)
    def test_original_SHA256_both_DLLs_required(self):
        with self.assertRaisesRegex(VerificationError,'SHA-256'):
            audit(b'synthetic client',b'synthetic server')
    def test_client_vgui_and_particle_identifiers(self):
        r=verify_subsystem(fixture('client.dll'),'client.dll',False)
        self.assertEqual(r['vgui_interface_names'],
                         ['VGUI_InputInternal001','VGUI_System010'])
        self.assertEqual(r['particle_bucket_allocation_bytes'],48)
        self.assertEqual(r['particle_bucket_ctor_argument'],96)
    def test_server_repositories_use_distinct_singletons(self):
        r=verify_subsystem(fixture('server.dll'),'server.dll',False)
        self.assertEqual({x['singleton'] for x in r['repository_wrappers']},
                         {'0x22503EE0','0x224F0388'})
if __name__=='__main__':unittest.main()
