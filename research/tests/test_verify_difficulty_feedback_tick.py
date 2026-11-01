"""Original-free, fabricated PE32 fixture for the pinned 2006 tick verifier."""
import hashlib
from pathlib import Path
import struct
import sys
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[2]/'scripts'))
import verify_difficulty_feedback_tick as original

def fixture():
    raw=bytearray(0x5600)
    raw[:2]=b'MZ'
    struct.pack_into('<I',raw,0x3c,0x80)
    raw[0x80:0x84]=b'PE\0\0'
    struct.pack_into('<HHIIIHH',raw,0x84,0x14c,2,0,0,0,224,0)
    struct.pack_into('<H',raw,0x98,0x10b)
    struct.pack_into('<I',raw,0x98+28,0x22000000)
    blocks=((b'.text',0x2b9000,0x4000,0x400),
            (b'.rdata',0x3d9000,0x1000,0x4400))
    for i,(name,rva,size,off) in enumerate(blocks):
        row=0x98+224+40*i
        raw[row:row+len(name)]=name
        struct.pack_into('<IIII',raw,row+8,size,rva,size,off)
    def pos(va):
        rva=va-0x22000000
        for _,start,size,off in blocks:
            if start<=rva<start+size:return off+rva-start
        raise ValueError(hex(va))
    def put(va,data):
        at=pos(va)
        raw[at:at+len(data)]=data
    for va,code in original.SIGNATURES.items():
        put(va,bytes.fromhex(code))
    put(0x223d9778,struct.pack('<f',0.5))
    def call(va,dest):
        put(va,b'\xe8'+struct.pack('<i',dest-va-5))
    call(0x222ba455,original.COLLECT_VA)
    call(0x222bc0e8,original.TICK_VA)
    for va,dest in zip(original.FOLLOW_UP_CALLS,original.EXPECTED_FOLLOW_UP_TARGETS):
        call(va,dest)
    return bytes(raw),pos

def check(blob):
    return original.verify(blob,sha=hashlib.sha256(blob).hexdigest())

class TickEvidenceFixtureTests(unittest.TestCase):
    def test_valid_fake_binary(self):
        binary,_=fixture()
        r=check(binary)
        self.assertEqual(r['collector_category_slots'],6)
        self.assertEqual(r['instruction_spans'],21)
        self.assertFalse(r['player_performance_weights_recovered'])
    def test_original_hash_required(self):
        binary,_=fixture()
        with self.assertRaisesRegex(original.EvidenceError,'SHA mismatch'):
            original.verify(binary)
    def test_bad_time_field(self):
        binary,pos=fixture();b=bytearray(binary)
        b[pos(0x222ba423)+2]=0x64
        with self.assertRaisesRegex(original.EvidenceError,'instruction mismatch'):check(bytes(b))
    def test_bad_counter_branch(self):
        binary,pos=fixture();b=bytearray(binary)
        b[pos(0x222ba439)]=0x7f
        with self.assertRaisesRegex(original.EvidenceError,'instruction mismatch'):check(bytes(b))
    def test_bad_half_interval_constant(self):
        binary,pos=fixture();b=bytearray(binary)
        struct.pack_into('<f',b,pos(0x223d9778),0.25)
        with self.assertRaisesRegex(original.EvidenceError,'half interval constant'):check(bytes(b))
    def test_bad_collector_call(self):
        binary,pos=fixture();b=bytearray(binary)
        b[pos(0x222ba455)+1]=0
        with self.assertRaisesRegex(original.EvidenceError,'collector target'):check(bytes(b))
    def test_bad_downstream_call(self):
        binary,pos=fixture();b=bytearray(binary)
        b[pos(original.FOLLOW_UP_CALLS[3])+1]=0
        with self.assertRaisesRegex(original.EvidenceError,'downstream call target'):check(bytes(b))
    def test_unexpected_direct_caller(self):
        binary,pos=fixture();b=bytearray(binary)
        va=0x222bb400
        b[pos(va):pos(va)+5]=b'\xe8'+struct.pack('<i',original.TICK_VA-va-5)
        with self.assertRaisesRegex(original.EvidenceError,'caller inventory changed'):check(bytes(b))
    def test_bad_pe_header(self):
        with self.assertRaises(original.EvidenceError):original.sections(b'not an executable')
if __name__=='__main__':unittest.main()
