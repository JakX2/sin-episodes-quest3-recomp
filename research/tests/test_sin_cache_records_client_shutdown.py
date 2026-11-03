"""Synthetic original-free x86 mutation tests for Phase90."""
import struct
import sys
from pathlib import Path
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"tools"))
from audit_sin_cache_records_client_shutdown import (
    REGIONS,CLIENT_SITES,SERVER_SITES,CALLS,LATE_CLIENT_GLOBALS,
    verify_one,audit,VerificationError)

class FakePE:
    def __init__(self):self.memory={}
    def put(self,va,raw):
        for i,b in enumerate(raw):
            site=va+i
            if site in self.memory and self.memory[site]!=b:
                raise AssertionError("synthetic overlap at %#x"%site)
            self.memory[site]=b
    def at(self,va,n):return bytes(self.memory.get(va+i,0xcc) for i in range(n))
    def corrupt(self,va):self.memory[va]^=0x40

def fixture(module):
    pe=FakePE()
    sites=CLIENT_SITES if module=="client.dll" else SERVER_SITES
    for va,raw in sites.items():pe.put(va,bytes.fromhex(raw))
    for va,dest in CALLS[module].items():
        pe.put(va,b"\xe8"+struct.pack("<i",dest-va-5))
    return pe

class OriginalCacheRecordTests(unittest.TestCase):
    def test_both_metadata_contracts(self):
        client=verify_one(fixture("client.dll"),"client.dll",False)
        server=verify_one(fixture("server.dll"),"server.dll",False)
        self.assertEqual(len(client["eight_global_virtual_calls"]),8)
        self.assertEqual(server["in_memory_record_stride_bytes"],24)
        self.assertEqual(server["not_found_index"],65535)
    def test_every_opcode_tamper_is_caught(self):
        for module,sites in (("client.dll",CLIENT_SITES),("server.dll",SERVER_SITES)):
            for va in sites:
                with self.subTest(module=module,site=hex(va)):
                    pe=fixture(module);pe.corrupt(va)
                    with self.assertRaises(VerificationError):
                        verify_one(pe,module,False)
    def test_every_call_edge_tamper_is_caught(self):
        for module,calls in CALLS.items():
            for va in calls:
                with self.subTest(module=module,site=hex(va)):
                    pe=fixture(module);pe.corrupt(va+1)
                    with self.assertRaises(VerificationError):
                        verify_one(pe,module,False)
    def test_four_code_hashes_guarded(self):
        self.assertEqual(sum(map(len,REGIONS.values())),4)
        for module in REGIONS:
            with self.assertRaisesRegex(VerificationError,"SHA-256"):
                verify_one(fixture(module),module,True)
    def test_original_dual_dll_hashes_mandatory(self):
        with self.assertRaisesRegex(VerificationError,"SHA-256"):
            audit(b"fake client",b"fake server")
    def test_correct_client_virtual_slots(self):
        r=verify_one(fixture("client.dll"),"client.dll",False)
        self.assertEqual([x["slot_byte_offset"] for x in r["eight_global_virtual_calls"]],
                         [4,4,4,4,4,28,4,4])
        self.assertEqual(r["final_virtual_byte_offset"],20)
    def test_client_original_globals_preserved(self):
        self.assertEqual(len(set(LATE_CLIENT_GLOBALS)),8)
        self.assertEqual(LATE_CLIENT_GLOBALS[0][0],0x2431dc90)
        self.assertEqual(LATE_CLIENT_GLOBALS[-1][0],0x2431eebc)
    def test_scene_model_sounds_aux_sizes_distinct(self):
        r=verify_one(fixture("server.dll"),"server.dll",False)
        self.assertEqual(r["scene_allocated_helper_bytes"],28)
        self.assertEqual(r["modelsounds_allocated_helper_bytes"],24)
    def test_no_disk_record_schema_claim(self):
        r=verify_one(fixture("server.dll"),"server.dll",False)
        self.assertIn("NOT recovered",r["disk_record_schema"])
    def test_record_index_width_and_offsets(self):
        r=verify_one(fixture("server.dll"),"server.dll",False)
        self.assertEqual(r["record_index_width_bits"],16)
        self.assertEqual(r["record_reader_view_offset"],8)
        self.assertEqual(r["record_output_pointer_offset_from_array_base"],12)
        self.assertEqual(r["record_handler_index_offset_from_array_base"],20)
        self.assertEqual(r["record_insert_handler_store_offset_from_array_base"],20)

if __name__=="__main__":
    unittest.main()
