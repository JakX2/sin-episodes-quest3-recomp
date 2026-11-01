"""Original-free fabricated PE32 tests for the pinned GPV caller verifier."""
import hashlib
from pathlib import Path
import struct, sys, unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[2]/'scripts'))
import verify_dynamic_difficulty_activation as audit

def fixture():
    blob=bytearray(0x26000)
    blob[:2]=b'MZ'
    struct.pack_into('<I',blob,0x3c,0x80)
    blob[0x80:0x84]=b'PE\0\0'
    struct.pack_into('<HHIIIHH',blob,0x84,0x14c,2,0,0,0,224,0)
    struct.pack_into('<H',blob,0x98,0x10b)
    struct.pack_into('<I',blob,0x98+28,0x22000000)
    sections=((b'.text',0x2b8000,0x20000,0x400),
              (b'.data',0x592000,0x1000,0x20400))
    for i,(name,rva,size,off) in enumerate(sections):
        row=0x98+224+40*i
        blob[row:row+len(name)]=name
        struct.pack_into('<IIII',blob,row+8,size,rva,size,off)
    def position(va):
        rva=va-0x22000000
        for _,start,size,off in sections:
            if start<=rva<start+size:return off+rva-start
        raise ValueError(hex(va))
    def put(va,code):blob[position(va):position(va)+len(code)]=code
    for addr,expected in audit.SIGNATURES.items():put(addr,bytes.fromhex(expected))
    for va in audit.CALLS:
        put(va,b'\xe8'+struct.pack('<i',audit.TARGET-va-5))
    put(audit.RESTORE_CALLER,b'\xe8'+struct.pack('<i',0x222b9130-audit.RESTORE_CALLER-5))
    return bytes(blob),position
def check(blob):return audit.verify(blob,sha=hashlib.sha256(blob).hexdigest())
class OriginalFreeCallerTests(unittest.TestCase):
    def test_synthetic_good(self):
        blob,_=fixture()
        result=check(blob)
        self.assertEqual(result['exact_direct_call_sites'],['0x222b9194','0x222ba3ba'])
        self.assertFalse(result['original_full_update_schedule_recovered'])
    def test_not_original_hash(self):
        blob,_=fixture()
        with self.assertRaisesRegex(audit.ActivationEvidenceError,'SHA-256 mismatch'):audit.verify(blob)
    def test_guard_toggle_corruption(self):
        blob,pos=fixture();b=bytearray(blob);b[pos(0x222b9161)]=0x74
        with self.assertRaisesRegex(audit.ActivationEvidenceError,'instruction signature'):check(bytes(b))
    def test_cvar_ptr_corruption(self):
        blob,pos=fixture();b=bytearray(blob);b[pos(0x222ba399)+2]=0x60
        with self.assertRaisesRegex(audit.ActivationEvidenceError,'instruction signature'):check(bytes(b))
    def test_slider_source_corruption(self):
        blob,pos=fixture();b=bytearray(blob);b[pos(0x222b9189)+2]=0x0c
        with self.assertRaisesRegex(audit.ActivationEvidenceError,'instruction signature'):check(bytes(b))
    def test_guarded_call_corruption(self):
        blob,pos=fixture();b=bytearray(blob);b[pos(audit.RESTORE_CALLER)+1]=0
        with self.assertRaisesRegex(audit.ActivationEvidenceError,'guarded restore call target'):check(bytes(b))
    def test_interpolation_call_corruption(self):
        blob,pos=fixture();b=bytearray(blob);b[pos(audit.CALLS[1])+1]=0
        with self.assertRaisesRegex(audit.ActivationEvidenceError,'interpolation call target'):check(bytes(b))
    def test_unexpected_third_call(self):
        blob,pos=fixture();b=bytearray(blob);va=0x222bf000
        b[pos(va):pos(va)+5]=b'\xe8'+struct.pack('<i',audit.TARGET-va-5)
        with self.assertRaisesRegex(audit.ActivationEvidenceError,'caller inventory changed'):check(bytes(b))
    def test_short_pe(self):
        with self.assertRaises(audit.ActivationEvidenceError):audit.pe_sections(b'not PE')
if __name__=='__main__':unittest.main()
