"""Phase165: original-free ScatterGun original x86 evidence and state tests."""
import itertools
import pathlib
import struct
import sys
import unittest
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/'tools'))
from audit_sin_original_server_scattergun_ammo import (
    REGIONS,OPCODES,CALL_EDGES,TAIL_JUMPS,ORIGINAL_VTABLE,ORIGINAL_COL,
    ORIGINAL_RTTI_DESCRIPTOR,TYPE_NAME,VTABLE_SLOTS,
    verify_original_scattergun,VerificationError,audit
)

class FakePE:
    base=0x22000000
    def __init__(self):self.bytes={}
    def put(self,va,raw):
        for i,c in enumerate(raw):
            if va+i in self.bytes and self.bytes[va+i]!=c:
                raise AssertionError('overlapping original-free fixture at %#x'%(va+i))
            self.bytes[va+i]=c
    def at(self,va,n):
        return bytes(self.bytes.get(va+i,0xcc) for i in range(n))
    def corrupt(self,va):self.bytes[va]^=0x10

def fixture():
    pe=FakePE()
    pe.put(ORIGINAL_VTABLE-4,struct.pack('<I',ORIGINAL_COL))
    pe.put(ORIGINAL_COL+12,struct.pack('<I',ORIGINAL_RTTI_DESCRIPTOR))
    pe.put(ORIGINAL_RTTI_DESCRIPTOR+8,TYPE_NAME)
    for slot,target in VTABLE_SLOTS.items():
        pe.put(ORIGINAL_VTABLE+4*slot,struct.pack('<I',target))
    for va,hextext in OPCODES.items():
        pe.put(va,bytes.fromhex(hextext))
    for calls,opcode in ((CALL_EDGES,0xe8),(TAIL_JUMPS,0xe9)):
        for site,target in calls.items():
            pe.put(site,bytes((opcode,))+struct.pack('<i',target-site-5))
    return pe

def independent_x86_snapshot(old_a,a,old_b,b,track,latched):
    if old_a!=a:
        if old_a!=0xffffffff:latched=1
        old_a=a
    if old_b!=b and track:
        if old_b!=0xffffffff:latched=1
        old_b=b
    return old_a,old_b,latched,bool(latched)

def independent_x86_consume(value,mode,flagbyte,flagptr):
    if mode:flagbyte=1
    elif flagptr is not None:flagptr |= 1
    return ((value-1)&0xffffffff,flagbyte,flagptr)

class OriginalScatterGunTests(unittest.TestCase):
    def test_original_class_and_vtable_match(self):
        out=verify_original_scattergun(fixture(),check_hashes=False)
        self.assertEqual(out['original_class'],'CWeaponScatterGun')
        self.assertEqual(out['verified_vtable_slots'],2)
        self.assertEqual(out['sparse_x86_witnesses'],34)
        self.assertEqual(out['direct_CALL_edges'],2)
        self.assertEqual(out['tail_JMP_edges'],1)
        self.assertEqual(VTABLE_SLOTS[216],0x2231c3e0)
        self.assertEqual(VTABLE_SLOTS[227],0x2231c810)

    def test_each_original_opcode_witness_rejects_mutations(self):
        for va in OPCODES:
            with self.subTest(va=hex(va)):
                pe=fixture();pe.corrupt(va)
                with self.assertRaises(VerificationError):
                    verify_original_scattergun(pe,check_hashes=False)

    def test_each_native_CALL_and_tail_JMP_rejects_mutations(self):
        for calls in (CALL_EDGES,TAIL_JUMPS):
            for va in calls:
                with self.subTest(call=hex(va)):
                    pe=fixture();pe.corrupt(va+1)
                    with self.assertRaises(VerificationError):
                        verify_original_scattergun(pe,check_hashes=False)

    def test_rtti_constructor_owner_cannot_be_reassigned(self):
        locations=[ORIGINAL_VTABLE-4,ORIGINAL_COL+12,
                   ORIGINAL_RTTI_DESCRIPTOR+8]
        locations += [ORIGINAL_VTABLE+slot*4 for slot in VTABLE_SLOTS]
        for va in locations:
            with self.subTest(rtti=hex(va)):
                pe=fixture();pe.corrupt(va)
                with self.assertRaises(VerificationError):
                    verify_original_scattergun(pe,check_hashes=False)

    def test_original_complete_function_hashes_required(self):
        self.assertEqual(REGIONS['scattergun_virtual_ammo_snapshot_update'][1],104)
        self.assertEqual(REGIONS['scattergun_secondary_clip_decrement'][1],51)
        for entry,n,digest in REGIONS.values():
            self.assertEqual(len(bytes.fromhex(digest)),32)
        with self.assertRaisesRegex(VerificationError,'full-method SHA'):
            verify_original_scattergun(fixture(),check_hashes=True)
        with self.assertRaisesRegex(VerificationError,'wrong original client'):
            audit(b'fake-client',b'fake-server')

    def test_independent_secondary_ammo_dword_wrap_and_two_event_paths(self):
        for value,mode,ptr in itertools.product(
            (0,1,2,0xffffffff,0x80000000),(0,1,2,255),
            (None,0,1,0x54,0xffffffff)):
            out=independent_x86_consume(value,mode,42,ptr)
            self.assertEqual(out[0],(value-1)&0xffffffff)
            self.assertEqual(out[1],1 if mode else 42)
            self.assertEqual(out[2],ptr if mode or ptr is None else ptr|1)

    def test_independent_ammo_snapshot_matrix(self):
        combos=itertools.product(
            (0xffffffff,0,1,7),(0,1,7),
            (0xffffffff,0,2),(0,2,3),(0,1,255),(0,1,42))
        total=0
        for old_a,a,old_b,b,enabled,pending in combos:
            result=independent_x86_snapshot(
                old_a,a,old_b,b,enabled,pending)
            self.assertEqual(result[0],a)
            self.assertEqual(result[1],b if enabled else old_b)
            expected=bool(pending or
                (old_a not in (0xffffffff,a)) or
                (enabled and old_b not in (0xffffffff,b)))
            self.assertEqual(result[3],expected)
            total+=1
        self.assertEqual(total,972)

    def test_wrong_server_PE_base_rejected(self):
        pe=fixture();pe.base=0x24000000
        with self.assertRaisesRegex(VerificationError,'image base'):
            verify_original_scattergun(pe,check_hashes=False)

if __name__=='__main__':unittest.main()
