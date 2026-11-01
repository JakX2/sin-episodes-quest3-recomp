"""Original-free synthetic PE32 fixture for Phase93 GPV verifier."""
import hashlib
from pathlib import Path
import struct
import sys
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[2]/'scripts'))
import verify_dynamic_difficulty_gpv as audit

def fixture():
    blob=bytearray(0xd000)
    blob[:2]=b'MZ'
    struct.pack_into('<I',blob,0x3c,0x80)
    blob[0x80:0x84]=b'PE\0\0'
    struct.pack_into('<HHIIIHH',blob,0x84,0x14c,4,0,0,0,224,0)
    struct.pack_into('<H',blob,0x98,0x10b)
    struct.pack_into('<I',blob,0x98+28,0x22000000)
    sections=((0x2d3000,0x2000,0x400),(0x3d2000,0x5000,0x2400),
              (0x45f000,0x3000,0x7400),(0x523000,0x1000,0xa400))
    for i,(rva,size,off) in enumerate(sections):
        struct.pack_into('<IIII',blob,0x98+224+40*i+8,size,rva,size,off)
    def position(va,n=1):
        rva=va-0x22000000
        for start,size,off in sections:
            if start<=rva and rva+n<=start+size:return off+rva-start
        raise ValueError(hex(va))
    def put(va,data):
        offset=position(va,len(data))
        blob[offset:offset+len(data)]=data
    for va,code in audit.SIGS.items():put(va,bytes.fromhex(code))
    for va,value in ((0x223d6314,100.0),(0x223d22f8,1.0),(0x223d2590,0.0)):
        put(va,struct.pack('<f',value))
    for i,name in enumerate(audit.NAMES):
        ptr=0x2245f100+i*48
        put(audit.NAME_TABLE+4*i,struct.pack('<I',ptr))
        put(ptr,name.encode()+b'\0')
        low,high=(1.0,-1.0) if i%2 else (0.0,1.0)
        put(audit.GPV_TABLE+i*28,struct.pack('<7f',0.5,0,1,2,3,low,high))
    return bytes(blob),position

def check(blob):
    return audit.verify(blob,sha=hashlib.sha256(blob).hexdigest())

class GPVTests(unittest.TestCase):
    def test_original_free_fixture(self):
        blob,_=fixture()
        r=check(blob)
        self.assertEqual(r['gpv_table_rows'],14)
        self.assertEqual(r['verified_instruction_spans'],len(audit.SIGS))
        self.assertEqual(r['metric_values'][1]['rating_low'],1.0)
        self.assertFalse(r['actual_stat_selection_and_update_schedule_verified'])
    def test_exact_hash_required(self):
        blob,_=fixture()
        with self.assertRaisesRegex(audit.EvidenceError,'SHA mismatch'):audit.verify(blob)
    def test_bad_name_pointer(self):
        blob,pos=fixture();data=bytearray(blob)
        struct.pack_into('<I',data,pos(audit.NAME_TABLE),0x2245f100+48)
        with self.assertRaisesRegex(audit.EvidenceError,'GPV name mismatch'):check(bytes(data))
    def test_bad_interpolation_instruction(self):
        blob,pos=fixture();data=bytearray(blob)
        data[pos(0x222d349b)]=0x90
        with self.assertRaisesRegex(audit.EvidenceError,'instruction mismatch'):check(bytes(data))
    def test_bad_rating_instruction(self):
        blob,pos=fixture();data=bytearray(blob)
        data[pos(0x222d37fe)]=0x90
        with self.assertRaisesRegex(audit.EvidenceError,'instruction mismatch'):check(bytes(data))
    def test_bad_zero_width_range(self):
        blob,pos=fixture();data=bytearray(blob)
        struct.pack_into('<f',data,pos(audit.GPV_TABLE+24),0.0)
        with self.assertRaisesRegex(audit.EvidenceError,'invalid GPV float/range'):check(bytes(data))
    def test_bad_multiplier(self):
        blob,pos=fixture();data=bytearray(blob)
        struct.pack_into('<f',data,pos(0x223d6314),99.0)
        with self.assertRaisesRegex(audit.EvidenceError,'multiplier/clamp'):check(bytes(data))
    def test_bad_pe(self):
        with self.assertRaisesRegex(audit.EvidenceError,'PE'):audit.pe_read(b'bad',0x222d3490,5)
    def test_wrong_expected_name(self):
        blob,_=fixture()
        bad=('NOT_THE_FIRST',)+audit.NAMES[1:]
        with self.assertRaisesRegex(audit.EvidenceError,'name mismatch'):
            audit.verify(blob,sha=hashlib.sha256(blob).hexdigest(),names=bad)
if __name__=='__main__':unittest.main()
