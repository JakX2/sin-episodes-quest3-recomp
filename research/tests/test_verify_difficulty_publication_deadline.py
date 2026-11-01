"""Synthetic PE32 fixtures only; the original game binary is never included."""
import hashlib
from pathlib import Path
import struct
import sys
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[2]/"scripts"))
import verify_difficulty_publication_deadline as audit

def fabricated():
    data=bytearray(0x3400)
    data[:2]=b"MZ"
    struct.pack_into("<I",data,0x3c,0x80)
    data[0x80:0x84]=b"PE\0\0"
    struct.pack_into("<HHIIIHH",data,0x84,0x14c,1,0,0,0,224,0)
    struct.pack_into("<H",data,0x98,0x10b)
    struct.pack_into("<I",data,0x98+28,0x22000000)
    off=0x98+224
    data[off:off+5]=b".text"
    struct.pack_into("<IIII",data,off+8,0x3000,0x2ba000,0x3000,0x400)
    def pos(va):return 0x400+va-0x222ba000
    for va,code in audit.INSTRUCTIONS.items():
        raw=bytes.fromhex(code)
        data[pos(va):pos(va)+len(raw)]=raw
    return bytes(data),pos

def check(binary):
    return audit.verify(binary,sha=hashlib.sha256(binary).hexdigest())

class PublicationEvidenceTests(unittest.TestCase):
    def test_fake_original_free_fixture(self):
        raw,_=fabricated()
        report=check(raw)
        self.assertEqual(report["publication_writer_va"],"0x222bc475")
        self.assertEqual(report["downstream_wrapper_va"],"0x222bc0e0")
        self.assertFalse(report["complete_stats_feedback_logic_recovered"])
    def test_exact_original_hash_required(self):
        raw,_=fabricated()
        with self.assertRaisesRegex(audit.EvidenceError,"SHA256 mismatch"):
            audit.verify(raw)
    def test_active_flag_offset(self):
        raw,pos=fabricated();out=bytearray(raw)
        out[pos(0x222bc3e3)+2]=0x05
        with self.assertRaisesRegex(audit.EvidenceError,"instruction mismatch"):
            check(bytes(out))
    def test_flag_write_corruption(self):
        raw,pos=fabricated();out=bytearray(raw)
        out[pos(0x222bc475)+3]=0x00
        with self.assertRaisesRegex(audit.EvidenceError,"instruction mismatch"):
            check(bytes(out))
    def test_deadline_offset_corruption(self):
        raw,pos=fabricated();out=bytearray(raw)
        out[pos(0x222bc463)+2]=0x70
        with self.assertRaisesRegex(audit.EvidenceError,"instruction mismatch"):
            check(bytes(out))
    def test_interval_corruption(self):
        raw,pos=fabricated();out=bytearray(raw)
        out[pos(0x222bc479)+2]=0x64
        with self.assertRaisesRegex(audit.EvidenceError,"instruction mismatch"):
            check(bytes(out))
    def test_global_enable_offset_corruption(self):
        raw,pos=fabricated();out=bytearray(raw)
        out[pos(0x222bc485)+2]=0x28
        with self.assertRaisesRegex(audit.EvidenceError,"instruction mismatch"):
            check(bytes(out))
    def test_downstream_call_corruption(self):
        raw,pos=fabricated();out=bytearray(raw)
        out[pos(0x222bc48e)+1]=0x00
        with self.assertRaisesRegex(audit.EvidenceError,"instruction mismatch"):
            check(bytes(out))
    def test_consumer_clearing_flag_corruption(self):
        raw,pos=fabricated();out=bytearray(raw)
        out[pos(0x222ba460)+3]=0x01
        with self.assertRaisesRegex(audit.EvidenceError,"instruction mismatch"):
            check(bytes(out))
    def test_invalid_pe(self):
        with self.assertRaises(audit.EvidenceError):
            audit.read_pe_va(b"not a PE",0x222bc475,4)

if __name__=="__main__":
    unittest.main()
