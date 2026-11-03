"""Synthetic, original-game-DLL-free Phase92 verifier regressions."""
from __future__ import annotations
import struct
import sys
import unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from audit_sin_cache_io_hud_lifecycle import (
    CLIENT,SERVER,CLIENT_PATTERNS,SERVER_OPCODE_ANCHORS,SERVER_CALLS,
    verify_client,verify_server,audit,VerificationError)

class FakePE:
    def __init__(self):self.memory={}
    def at(self,va,n):return bytes(self.memory.get(va+i,0x90) for i in range(n))
    def put(self,va,raw):
        for i,b in enumerate(raw):
            addr=va+i
            if addr in self.memory and self.memory[addr]!=b:
                raise AssertionError('fixture overlap at %#x'%addr)
            self.memory[addr]=b
    def corrupt(self,va):self.memory[va]^=0x40

def fixture_client():
    p=FakePE()
    for _,start,length,_,shape in CLIENT:
        p.put(start+length,b'\xcc')
        for off,raw in CLIENT_PATTERNS[shape]:p.put(start+off,bytes.fromhex(raw))
    return p

def fixture_server():
    p=FakePE()
    for _,_,start,length,_ in SERVER:p.put(start+length,b'\xcc')
    for site,raw in SERVER_OPCODE_ANCHORS.items():p.put(site,bytes.fromhex(raw))
    for site,target in SERVER_CALLS.items():
        p.put(site,b'\xe8'+struct.pack('<i',target-site-5))
    return p

class Phase92Tests(unittest.TestCase):
    def test_both_synthetic_verifiers_pass(self):
        self.assertEqual(verify_client(fixture_client(),False)['original_function_count'],8)
        self.assertEqual(len(verify_server(fixture_server(),False)['methods']),4)
    def test_full_sha256_is_enforced(self):
        for func,fixture in ((verify_client,fixture_client),(verify_server,fixture_server)):
            with self.assertRaisesRegex(VerificationError,'SHA mismatch'):
                func(fixture(),True)
    def test_exactly_three_hud_function_shapes(self):
        r=verify_client(fixture_client(),False)
        self.assertEqual(r['distinct_binary_implementations'],3)
        shapes=[entry['shape'] for entry in r['methods']]
        self.assertEqual(shapes.count('single'),5)
        self.assertEqual(shapes.count('double'),1)
        self.assertEqual(shapes.count('nulling'),2)
    def test_all_client_original_dispatch_mutations(self):
        for name,start,length,digest,shape in CLIENT:
            for offset,raw in CLIENT_PATTERNS[shape]:
                with self.subTest(name=name,offset=offset):
                    p=fixture_client();p.corrupt(start+offset)
                    with self.assertRaises(VerificationError):verify_client(p,False)
    def test_all_client_method_end_boundaries(self):
        for _,start,length,_,_ in CLIENT:
            p=fixture_client();p.corrupt(start+length)
            with self.assertRaises(VerificationError):verify_client(p,False)
    def test_all_12_original_server_call_targets(self):
        self.assertEqual(len(SERVER_CALLS),12)
        for site in SERVER_CALLS:
            with self.subTest(site=hex(site)):
                p=fixture_server();p.corrupt(site+1)
                with self.assertRaises(VerificationError):verify_server(p,False)
    def test_all_20_original_server_opcode_anchors(self):
        self.assertEqual(len(SERVER_OPCODE_ANCHORS),20)
        for site in SERVER_OPCODE_ANCHORS:
            with self.subTest(site=hex(site)):
                p=fixture_server();p.corrupt(site)
                with self.assertRaises(VerificationError):verify_server(p,False)
    def test_all_four_server_function_boundaries(self):
        for _,_,start,length,_ in SERVER:
            p=fixture_server();p.corrupt(start+length)
            with self.assertRaises(VerificationError):verify_server(p,False)
    def test_float_prefix_is_scene_only(self):
        r=verify_server(fixture_server(),False)
        self.assertTrue(r['scene_has_float_prefix'])
        self.assertFalse(r['model_sound_has_float_prefix'])
    def test_count_temporary_buffer_and_unresolved_sentinel(self):
        r=verify_server(fixture_server(),False)
        self.assertEqual(r['item_count_format'],'unsigned 16-bit')
        self.assertEqual(r['temporary_buffer_capacity'],512)
        self.assertEqual(r['unresolved_key_sentinel'],0xffffffff)
    def test_original_client_and_server_class_associations(self):
        self.assertEqual(CLIENT[4][0],'CLoadingDisc')
        self.assertEqual(CLIENT[5][0],'CCenterPrint')
        self.assertEqual(SERVER[0][0],'CSceneCache')
        self.assertEqual(SERVER[3][0],'CModelSoundCache')
    def test_prior_SHA_pins_mandatory(self):
        with self.assertRaisesRegex(VerificationError,'SHA-256'):
            audit(b'fake client',b'fake server')

if __name__=='__main__':unittest.main()
