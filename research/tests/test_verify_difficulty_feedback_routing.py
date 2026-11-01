"""Synthetic PE32 fixtures only. No original SiN DLL bytes or assets."""
from pathlib import Path
import hashlib
import struct
import sys
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[2]/"scripts"))
import verify_difficulty_feedback_routing as audit

def fabricated_binary():
    raw=bytearray(0x16400)
    raw[:2]=b"MZ"
    struct.pack_into("<I",raw,0x3c,0x80)
    raw[0x80:0x84]=b"PE\0\0"
    struct.pack_into("<HHIIIHH",raw,0x84,0x14c,3,0,0,0,224,0)
    struct.pack_into("<H",raw,0x98,0x10b)
    struct.pack_into("<I",raw,0x98+28,0x22000000)
    sections=((b".text",0x2b9000,0x10000,0x400),
              (b".rdata",0x400000,0x4000,0x10400),
              (b".data",0x521000,0x2000,0x14400))
    for i,(name,rva,size,off) in enumerate(sections):
        row=0x98+224+40*i
        raw[row:row+len(name)]=name
        struct.pack_into("<IIII",raw,row+8,size,rva,size,off)
    def pos(va,n=1):
        rva=va-0x22000000
        for _,start,size,off in sections:
            if start<=rva and rva+n<=start+size:return off+rva-start
        raise ValueError(hex(va))
    def put(va,data):
        off=pos(va,len(data))
        raw[off:off+len(data)]=data
    for va,code in audit.CODE.items():
        put(va,bytes.fromhex(code))
    for table,names,base in ((audit.LEVEL_TABLE,audit.LEVELS,0x22401000),
                             (audit.CATEGORY_TABLE,audit.CATEGORIES,0x22402000)):
        for i,name in enumerate(names):
            pointer=base+i*64
            put(table+4*i,struct.pack("<I",pointer))
            put(pointer,name.encode("ascii")+b"\0")
    return bytes(raw),pos

def check(binary):
    return audit.verify(binary,sha=hashlib.sha256(binary).hexdigest())

class FeedbackRoutingFixtures(unittest.TestCase):
    def test_fabricated_complete(self):
        raw,_=fabricated_binary()
        result=check(raw)
        self.assertEqual(result["category_count"],6)
        self.assertEqual(result["level_count"],5)
        self.assertEqual(result["global_override_bypassed_slots"],[4,5])
        self.assertEqual(result["code_signatures_verified"],len(audit.CODE))
        self.assertFalse(result["original_category_stat_weight_calculations_recovered"])
    def test_original_hash_required(self):
        raw,_=fabricated_binary()
        with self.assertRaisesRegex(audit.EvidenceError,"SHA256 mismatch"):
            audit.verify(raw)
    def test_global_field_tamper(self):
        raw,pos=fabricated_binary()
        changed=bytearray(raw);changed[pos(0x222b9e9f)+2]=0x44
        with self.assertRaisesRegex(audit.EvidenceError,"instruction mismatch"):
            check(bytes(changed))
    def test_category_level_field_tamper(self):
        raw,pos=fabricated_binary()
        changed=bytearray(raw);changed[pos(0x222b9e95)+2]=0x24
        with self.assertRaisesRegex(audit.EvidenceError,"instruction mismatch"):
            check(bytes(changed))
    def test_bypass_branch_tamper(self):
        raw,pos=fabricated_binary()
        changed=bytearray(raw);changed[pos(0x222b9e98)]=0x75
        with self.assertRaisesRegex(audit.EvidenceError,"instruction mismatch"):
            check(bytes(changed))
    def test_override_value_tamper(self):
        raw,pos=fabricated_binary()
        changed=bytearray(raw);changed[pos(0x222b9ec8)+1]=0x03
        with self.assertRaisesRegex(audit.EvidenceError,"instruction mismatch"):
            check(bytes(changed))
    def test_six_slot_loop_tamper(self):
        raw,pos=fabricated_binary()
        changed=bytearray(raw);changed[pos(0x222b9f28)+2]=0x05
        with self.assertRaisesRegex(audit.EvidenceError,"instruction mismatch"):
            check(bytes(changed))
    def test_category_name_pointer_tamper(self):
        raw,pos=fabricated_binary()
        changed=bytearray(raw)
        struct.pack_into("<I",changed,pos(audit.CATEGORY_TABLE+4*3),0x22402000)
        with self.assertRaisesRegex(audit.EvidenceError,"category label mismatch"):
            check(bytes(changed))
    def test_difficulty_name_tamper(self):
        raw,pos=fabricated_binary()
        changed=bytearray(raw)
        changed[pos(0x22401000)]=ord("X")
        with self.assertRaisesRegex(audit.EvidenceError,"level label mismatch"):
            check(bytes(changed))
    def test_invalid_pe(self):
        with self.assertRaises(audit.EvidenceError):
            audit.read_pe_va(b"no executable",0x222b9e90,3)

if __name__=="__main__":
    unittest.main()
