"""Phase160: synthetic original-free CHudElement RTTI/function/branch regression."""
import itertools
import pathlib
import struct
import sys
import unittest
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/"tools"))
from audit_sin_original_client_hud_element_visibility import (
    RTTI,VTABLE,TYPE_VA,TYPE_NAME,REGIONS,OPCODES,CALLS,
    verify_original_hud_element,audit,VerificationError,
)
class FakePE:
    base=0x24000000
    def __init__(self):self.data={}
    def put(self,va,raw):
        for i,byte in enumerate(raw):
            if va+i in self.data and self.data[va+i]!=byte:
                raise AssertionError("synthetic byte collision at %#x"%(va+i))
            self.data[va+i]=byte
    def at(self,va,count):
        return bytes(self.data.get(va+i,0xCC) for i in range(count))
    def corrupt(self,va):self.data[va]^=0x10

def fixture():
    p=FakePE()
    for va,target in RTTI.items():p.put(va,struct.pack("<I",target))
    for slot,target in enumerate(VTABLE):
        p.put(0x2429B6EC+4*slot,struct.pack("<I",target))
    p.put(TYPE_VA,TYPE_NAME)
    for va,hexcode in OPCODES.items():p.put(va,bytes.fromhex(hexcode))
    for va,target in CALLS.items():
        p.put(va,b"\xe8"+struct.pack("<i",target-va-5))
    return p

def original_x86_gate(engine_bool,has_player,override,player_flags,
                      mask,player_virtual_return,player_byte):
    # Independent imperative interpretation of x86 TEST/JE/JNE/JLE,
    # bit-4 hard suppress and the NEG/SBB/NEG bool conversion.
    if (engine_bool&0xff)==0:return True,0
    if not has_player:return True,0
    ebx=player_flags&0xffffffff
    if override&0xffffffff:ebx=override&0xffffffff
    if ebx&4:return True,0
    calls=0
    if mask&16:
        calls=1
        if player_virtual_return<=0:return True,calls
    if mask&32 and player_byte==0:return True,calls
    scratch=ebx&mask
    carry=int(scratch!=0)
    sbb=(0-carry)&0xffffffff
    return ((0-sbb)&0xffffffff)!=0,calls

def readable_predicate(engine_bool,has_player,override,player_flags,
                       mask,player_virtual_return,player_byte):
    if not engine_bool or not has_player:return True,0
    active=override if override else player_flags
    if active&4:return True,0
    calls=int(bool(mask&16))
    if calls and player_virtual_return<=0:return True,calls
    if mask&32 and not player_byte:return True,calls
    return bool(active&mask),calls

class OriginalCHudElementTests(unittest.TestCase):
    def test_original_rtti_and_all_virtual_slots(self):
        report=verify_original_hud_element(fixture(),check_hashes=False)
        self.assertEqual(report["original_class"],"CHudElement")
        self.assertEqual(report["verified_original_vtable_slots"],13)
        self.assertEqual(report["verified_original_full_method_hashes"],0)
        self.assertEqual(report["verified_sparse_opcode_sites"],len(OPCODES))
        self.assertEqual(report["verified_original_rel32_call_edges"],4)
        self.assertEqual(len(REGIONS),8)
        self.assertEqual(VTABLE[9],0x24142A60)
        self.assertEqual(VTABLE[11],0x24141830)
        self.assertEqual(VTABLE[12],0x24141850)

    def test_mutate_each_original_rtti_and_vtable_pointer(self):
        addresses=[*RTTI,TYPE_VA]+[0x2429B6EC+4*i for i in range(13)]
        for va in addresses:
            with self.subTest(va=hex(va)):
                p=fixture();p.corrupt(va)
                with self.assertRaises(VerificationError):
                    verify_original_hud_element(p,check_hashes=False)

    def test_mutate_every_original_opcode(self):
        for va in OPCODES:
            with self.subTest(va=hex(va)):
                p=fixture();p.corrupt(va)
                with self.assertRaises(VerificationError):
                    verify_original_hud_element(p,check_hashes=False)

    def test_mutate_original_callsites(self):
        for va in CALLS:
            with self.subTest(va=hex(va)):
                p=fixture();p.corrupt(va+1)
                with self.assertRaises(VerificationError):
                    verify_original_hud_element(p,check_hashes=False)

    def test_original_full_region_hashes_required(self):
        for name,(va,length,digest) in REGIONS.items():
            with self.subTest(name=name):
                self.assertGreater(length,0)
                self.assertEqual(len(bytes.fromhex(digest)),32)
        with self.assertRaisesRegex(VerificationError,"full code SHA mismatch"):
            verify_original_hud_element(fixture(),check_hashes=True)
        with self.assertRaisesRegex(VerificationError,"wrong original 2006 client"):
            audit(b"wrong original client.dll",b"wrong original server.dll")

    def test_independent_x86_boolean_matrix(self):
        count=0
        for enabled,has_player,override,flags,mask,virtual_result,player_byte in itertools.product(
                (0,1),(False,True),(0,1,4,0x80),
                (0,1,4,16,32,0x80,0xffffffff),
                (0,1,4,16,32,0x30,0x80,0x90,0xffffffff),
                (-1,0,1,100),(0,1)):
            original=original_x86_gate(enabled,has_player,override,flags,
                                      mask,virtual_result,player_byte)
            expected=readable_predicate(enabled,has_player,override,flags,
                                        mask,virtual_result,player_byte)
            self.assertEqual(original,expected)
            count+=1
        self.assertEqual(count,2*2*4*7*9*4*2)

    def test_wrong_pe_base_rejected(self):
        p=fixture();p.base=0x22000000
        with self.assertRaisesRegex(VerificationError,"image base"):
            verify_original_hud_element(p,check_hashes=False)

if __name__=="__main__":unittest.main()
