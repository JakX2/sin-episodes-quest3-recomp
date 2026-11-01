"""Original-free, self-authored synthetic PE32 handler-registration fixtures."""
from pathlib import Path
import hashlib
import struct
import sys
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[2]/'scripts'))
import verify_info_challenge_handlers as verify

FGD = b'''@PointClass base(Targetname, Parentname) size(-4 -4 -4, 4 4 4) = info_challenge : "Fixture"
[
input GetInitialChallenge(void) : "Fixture"
input GetCurrentChallenge(void) : "Fixture"
input GetChallengeDifference(void) : "Fixture"
output OnInitialChallenge(float) : "Fixture"
output OnCurrentChallenge(float) : "Fixture"
output OnChallengeDifference(float) : "Fixture"
]'''

def fixture():
    blob=bytearray(0x3000)
    blob[:2]=b'MZ';struct.pack_into('<I',blob,0x3c,0x80)
    blob[0x80:0x84]=b'PE\0\0';struct.pack_into('<HHIIIHH',blob,0x84,0x14c,4,0,0,0,224,0)
    opt=0x98;struct.pack_into('<H',blob,opt,0x10b)
    struct.pack_into('<I',blob,opt+28,0x22000000)
    sections=[(0x2f4000,0x1000,0x400),(0x468000,0x800,0x1400),
              (0x527d00,0x200,0x1c00),(0x3d6300,0x100,0x1e00)]
    for i,(rva,size,off) in enumerate(sections):
        struct.pack_into('<IIII',blob,opt+224+40*i+8,size,rva,size,off)
    def put(va,data):
        rva=va-0x22000000
        for start,size,off in sections:
            if start<=rva and rva+len(data)<=start+size:
                blob[off+rva-start:off+rva-start+len(data)]=data;return
        raise ValueError(hex(va))
    for name,(d,ptr,fn,_) in verify.HANDLERS.items():
        put(ptr,name.encode()+b'\0')
        put(d,struct.pack('<I',ptr))
        put(d+8,struct.pack('<I',fn))
    for va,sig in verify.INSTRUCTIONS.items():put(va,bytes.fromhex(sig))
    put(0x223d6314,struct.pack('<f',100.0))
    return bytes(blob)

def check(blob,fgd=FGD):
    return verify.verify(blob,fgd,server_sha=hashlib.sha256(blob).hexdigest(),
                         fgd_sha=hashlib.sha256(fgd).hexdigest())

class TestOriginalFreePE(unittest.TestCase):
    def test_synthetic_success(self):
        report=check(fixture())
        self.assertEqual(report['verified_instruction_spans'],10)
        self.assertEqual(report['difference_scale'],100)
        self.assertFalse(report['real_challenge_formula_verified'])
    def test_provenance_is_pinned(self):
        with self.assertRaisesRegex(verify.EvidenceError,'SHA mismatch'):
            verify.verify(fixture(),FGD)
    def test_name_pointer_tampering(self):
        b=bytearray(fixture())
        struct.pack_into('<I',b,0x1c40,0x22468380)
        with self.assertRaisesRegex(verify.EvidenceError,'handler pointer mismatch'):
            check(bytes(b))
    def test_wrong_instruction(self):
        b=bytearray(fixture())
        b[0x400+(0x222f43e8-0x222f4000)]=0x90
        with self.assertRaisesRegex(verify.EvidenceError,'instruction mismatch'):
            check(bytes(b))
    def test_multiplier_changed(self):
        b=bytearray(fixture())
        struct.pack_into('<f',b,0x1e00+0x14,99.0)
        with self.assertRaisesRegex(verify.EvidenceError,'multiplier'):
            check(bytes(b))
    def test_missing_fgd_output(self):
        bad=FGD.replace(b'OnChallengeDifference(float)',b'OnChallengeDifference(string)')
        with self.assertRaisesRegex(verify.EvidenceError,'FGD float output missing'):
            check(fixture(),bad)
    def test_short_pe(self):
        with self.assertRaises(verify.EvidenceError):
            verify.pe_read(b'broken',0x222f4390,2)

if __name__=='__main__':unittest.main()
