"""Phase166 original-free ScatterGun deferred timer x86 evidence tests.

Synthetic sparse opcode/call fixture does not contain any original full
function region. Original whole-region hashes are only checked with the
owner-held exact 2006 server.dll. Independent x87 status-word flag math
tests due/equals/NaN and the two-mode dirty-notification semantics.
"""
import itertools
import math
import pathlib
import struct
import sys
import unittest

sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/"tools"))
from audit_sin_original_server_scattergun_deferred import (
    REGIONS,OPCODES,CALLS,TAIL_JUMPS,verify_original_deferred,
    VerificationError,audit
)

class MockPE:
    base=0x22000000
    def __init__(self):
        self.memory={}
    def put(self,va,source):
        for i,byte in enumerate(source):
            if va+i in self.memory and self.memory[va+i]!=byte:
                raise AssertionError("sparse fixture byte conflict %#x"%(va+i))
            self.memory[va+i]=byte
    def at(self,va,n):
        return bytes(self.memory.get(va+i,0xCC) for i in range(n))
    def damage(self,va):
        self.memory[va]^=0x10

def fixture():
    pe=MockPE()
    for va,opcode in OPCODES.items():
        pe.put(va,bytes.fromhex(opcode))
    for collection,op in ((CALLS,0xE8),(TAIL_JUMPS,0xE9)):
        for va,target in collection.items():
            pe.put(va,bytes((op,))+struct.pack("<i",target-va-5))
    return pe

def independent_x87_status_ah(lhs,rhs):
    # x87 FCOMP/FUCOMPP: less -> C0, equal -> C3,
    # greater -> neither, unordered -> C0+C2+C3.
    if math.isnan(lhs) or math.isnan(rhs):
        return 0x45
    if lhs<rhs:
        return 0x01
    if lhs>rhs:
        return 0x00
    return 0x40

def due_original(lhs,rhs):
    # Original handler TEST AH,0x41 ; JE skip.
    return (independent_x87_status_ah(lhs,rhs)&0x41)!=0

def changed_original(lhs,rhs):
    # Shared setters TEST AH,0x44 ; JNP skip.
    masked=independent_x87_status_ah(lhs,rhs)&0x44
    parity_even=masked.bit_count()%2==0
    return parity_even

def independent_gate(pending,deadline,clock,clip_bits,blocked,owner_exists):
    if not pending:return "NoPending"
    if not due_original(deadline,clock):return "ScheduledForLater"
    if clip_bits==0 or (clip_bits&0x80000000):return "NonpositivePrimaryClip"
    if blocked:return "Blocked"
    if not owner_exists:return "MissingOwner"
    return "Dispatched"

def independent_float_update(old,new,mode,dirty,event_ptr):
    if not changed_original(old,new):
        return old,dirty,event_ptr,False
    if mode:
        dirty=1
    elif event_ptr is not None:
        event_ptr |= 1
    return new,dirty,event_ptr,True

class OriginalScatterGunDeferredTests(unittest.TestCase):
    def test_native_fingerprint_summary(self):
        report=verify_original_deferred(fixture(),check_hashes=False)
        self.assertEqual(report["original_handler_entry"],"0x2231C2F0")
        self.assertEqual(report["original_handler_native_bytes"],240)
        self.assertEqual(report["verified_complete_original_function_hashes"],0)
        self.assertEqual(report["verified_original_opcode_witnesses"],59)
        self.assertEqual(report["original_direct_call_edges"],6)
        self.assertEqual(report["original_incoming_tail_jump_edges"],1)
        self.assertEqual(len(REGIONS),4)
        self.assertEqual(len(OPCODES),59)
        self.assertEqual(len(CALLS),6)

    def test_all_partial_native_instructions_are_mutation_guarded(self):
        for va in OPCODES:
            with self.subTest(opcode_va=hex(va)):
                pe=fixture();pe.damage(va)
                with self.assertRaises(VerificationError):
                    verify_original_deferred(pe,check_hashes=False)

    def test_all_call_and_jump_rel32_edges_are_guarded(self):
        for group in (CALLS,TAIL_JUMPS):
            for va in group:
                with self.subTest(branch_va=hex(va)):
                    pe=fixture();pe.damage(va+1)
                    with self.assertRaises(VerificationError):
                        verify_original_deferred(pe,check_hashes=False)

    def test_original_full_hashes_cannot_be_faked_by_sparse_fixture(self):
        lengths={0x2231C2F0:240,0x2203D190:64,
                 0x22173260:64,0x221732A0:64}
        for label,(va,n,digest) in REGIONS.items():
            with self.subTest(label=label):
                self.assertEqual(lengths[va],n)
                self.assertEqual(len(bytes.fromhex(digest)),32)
        with self.assertRaisesRegex(VerificationError,"whole-function SHA"):
            verify_original_deferred(fixture(),check_hashes=True)
        with self.assertRaises(VerificationError):
            audit(b"wrong retail client",b"wrong retail server")

    def test_gate_x87_unordered_and_signed_clip_matrix(self):
        deadlines=(-math.inf,-1.0,0.0,1.0,math.inf,math.nan)
        clocks=(-math.inf,0.0,2.0,math.inf,math.nan)
        clips=(0,1,7,0x7fffffff,0x80000000,0xffffffff)
        count=0
        for pending,deadline,clock,clip,blocked,owner in itertools.product(
            (0,1),deadlines,clocks,clips,(0,1),(False,True)
        ):
            outcome=independent_gate(pending,deadline,clock,
                                     clip,blocked,owner)
            self.assertIn(outcome,(
                "NoPending","ScheduledForLater","NonpositivePrimaryClip",
                "Blocked","MissingOwner","Dispatched"))
            if pending==0:self.assertEqual(outcome,"NoPending")
            if pending and (math.isnan(deadline) or math.isnan(clock)):
                self.assertNotEqual(outcome,"ScheduledForLater")
            if outcome=="Dispatched":
                self.assertTrue(pending and owner and not blocked)
                self.assertTrue(clip>0 and clip<0x80000000)
            count+=1
        self.assertEqual(count,1440)
        self.assertTrue(due_original(math.nan,5.0))
        self.assertTrue(due_original(5.0,math.nan))
        self.assertFalse(due_original(math.inf,5.0))
        self.assertTrue(due_original(-math.inf,5.0))

    def test_three_setters_x87_equal_and_unordered_update_policy(self):
        values=(0.0,-0.0,1.0,-1.0,math.inf,-math.inf,math.nan)
        count=0
        for old,new,mode,prior_dirty,flags in itertools.product(
            values,values,(0,1,255),(0,6),(None,0,0x100)
        ):
            result=independent_float_update(
                old,new,mode,prior_dirty,flags)
            changed=result[3]
            self.assertEqual(changed,not(old==new))
            if not changed:
                self.assertEqual(result[1:],(prior_dirty,flags,False))
            elif mode:
                self.assertEqual(result[1],1)
                self.assertEqual(result[2],flags)
            elif flags is not None:
                self.assertEqual(result[2],flags|1)
            count+=1
        self.assertEqual(count,7*7*3*2*3)
        self.assertFalse(changed_original(+0.0,-0.0))
        self.assertTrue(changed_original(math.nan,math.nan))

    def test_expected_original_runtime_addresses_are_retained(self):
        self.assertEqual(CALLS[0x2231C3AE],0x2203D190)
        self.assertEqual(CALLS[0x2231C3C0],0x22173260)
        self.assertEqual(CALLS[0x2231C3D6],0x221732A0)
        self.assertEqual(TAIL_JUMPS[0x2231C441],0x2231C2F0)

    def test_wrong_original_server_image_base_rejected(self):
        pe=fixture();pe.base=0x24000000
        with self.assertRaisesRegex(VerificationError,"image base"):
            verify_original_deferred(pe,check_hashes=False)

if __name__=="__main__":
    unittest.main()
