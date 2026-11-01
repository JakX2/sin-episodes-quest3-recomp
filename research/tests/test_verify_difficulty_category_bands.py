"""Fabricated PE32 category-band verifier fixtures, zero proprietary bytes."""
import hashlib
from pathlib import Path
import struct
import sys
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[2]/'scripts'))
import verify_difficulty_category_bands as audit

def fixture():
    blob=bytearray(0xe500)
    blob[:2]=b'MZ'
    struct.pack_into('<I',blob,0x3c,0x80)
    blob[0x80:0x84]=b'PE\0\0'
    struct.pack_into('<HHIIIHH',blob,0x84,0x14c,4,0,0,0,224,0)
    struct.pack_into('<H',blob,0x98,0x10b)
    struct.pack_into('<I',blob,0x98+28,0x22000000)
    sections=((b'.text',0x2b9000,0x8000,0x400),
              (b'.rdata',0x3d9000,0x2000,0x8400),
              (b'.rdata2',0x45c000,0x2000,0xa400),
              (b'.data',0x521000,0x2000,0xc400))
    for i,(name,rva,size,off) in enumerate(sections):
        p=0x98+224+i*40
        blob[p:p+len(name)]=name
        struct.pack_into('<IIII',blob,p+8,size,rva,size,off)
    def location(va,n=1):
        rva=va-0x22000000
        for _,start,size,off in sections:
            if start<=rva and rva+n<=start+size:return off+rva-start
        raise ValueError(hex(va))
    def put(va,value):
        p=location(va,len(value))
        blob[p:p+len(value)]=value
    for va,code in audit.SIGNATURES.items():put(va,bytes.fromhex(code))
    put(audit.LOWER,bytes.fromhex('cdcc4c3d'))
    put(audit.UPPER,struct.pack('<f',2.0))
    put(audit.LOG_FILENAME,b'dd_info.log\0')
    for i in range(6):put(audit.TABLE+i*16,struct.pack('<4f',1,1,1,1))
    return bytes(blob),location

def check(blob):return audit.verify(blob,sha=hashlib.sha256(blob).hexdigest())

class CategoryBandBinaryTests(unittest.TestCase):
    def test_positive_synthetic(self):
        blob,_=fixture()
        result=check(blob)
        self.assertEqual(result['category_count'],6)
        self.assertEqual(result['verified_instruction_signatures'],40)
        self.assertTrue(result['category_collector_helper_is_diagnostic_logging'])
        self.assertFalse(result['original_feedback_weight_sign_logic_recovered'])
    def test_exact_hash_required(self):
        blob,_=fixture()
        with self.assertRaisesRegex(audit.EvidenceError,'SHA mismatch'):audit.verify(blob)
    def test_mutated_low_selector(self):
        blob,p=fixture();out=bytearray(blob);out[p(0x222c074c)+4]=0x03
        with self.assertRaisesRegex(audit.EvidenceError,'instruction mismatch'):check(bytes(out))
    def test_mutated_high_store(self):
        blob,p=fixture();out=bytearray(blob);out[p(0x222c085c)+2]=0x04
        with self.assertRaisesRegex(audit.EvidenceError,'instruction mismatch'):check(bytes(out))
    def test_mutated_reset_source(self):
        blob,p=fixture();out=bytearray(blob);out[p(0x222c077c)+2]=0x08
        with self.assertRaisesRegex(audit.EvidenceError,'instruction mismatch'):check(bytes(out))
    def test_mutated_additive_callback(self):
        blob,p=fixture();out=bytearray(blob);out[p(0x222baef8)+1]=0
        with self.assertRaisesRegex(audit.EvidenceError,'instruction mismatch'):check(bytes(out))
    def test_low_cap_corruption(self):
        blob,p=fixture();out=bytearray(blob)
        struct.pack_into('<f',out,p(audit.LOWER),0.1)
        with self.assertRaisesRegex(audit.EvidenceError,'clamp limits mismatch'):check(bytes(out))
    def test_high_cap_corruption(self):
        blob,p=fixture();out=bytearray(blob)
        struct.pack_into('<f',out,p(audit.UPPER),3.0)
        with self.assertRaisesRegex(audit.EvidenceError,'clamp limits mismatch'):check(bytes(out))
    def test_original_table_corruption(self):
        blob,p=fixture();out=bytearray(blob)
        struct.pack_into('<f',out,p(audit.TABLE+4*16+8),1.25)
        with self.assertRaisesRegex(audit.EvidenceError,'initial category factors'):check(bytes(out))
    def test_log_name_corruption(self):
        blob,p=fixture();out=bytearray(blob)
        out[p(audit.LOG_FILENAME)]=ord('x')
        with self.assertRaisesRegex(audit.EvidenceError,'logging filename'):check(bytes(out))
    def test_invalid_pe(self):
        with self.assertRaises(audit.EvidenceError):
            audit.read_pe_va(b'no binary',0x222c07c0,2)
if __name__=='__main__':unittest.main()
