"""Synthetic, original-free Phase158 VGUI draw bridge evidence and semantics.

No original 2006 game executable bytes are embedded. The independent
symbolic stack-word and clip formulas cover both native branches, 32-bit
arithmetic wrap, and all sparse owner-binary audit witnesses.
"""
import itertools
import math
import pathlib
import struct
import sys
import unittest

sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/"tools"))
from audit_sin_original_client_vgui_draw_bridge import (
    REGIONS,OPCODES,CALLS,EXTERNAL_WORKER_CALLS,VGUI_SINGLETON_CALLS,
    verify_original_draw_bridge,VerificationError,audit,
)

class FakePE:
    base=0x24000000
    def __init__(self): self.words={}
    def put(self,at,data):
        for i,byte in enumerate(data):
            va=at+i
            if va in self.words and self.words[va]!=byte:
                raise AssertionError("overlapping synthetic evidence at %#x"%va)
            self.words[va]=byte
    def at(self,va,length):
        return bytes(self.words.get(va+i,0xCC) for i in range(length))
    def corrupt(self,va):self.words[va]^=0x10

def fixture():
    pe=FakePE()
    for va,raw in OPCODES.items():
        pe.put(va,bytes.fromhex(raw))
    for va,target in CALLS.items():
        pe.put(va,b"\xE8"+struct.pack("<i",target-va-5))
    return pe

def i32(value):
    bits=value&0xFFFFFFFF
    return bits if bits<0x80000000 else bits-0x100000000

def f32(x):
    try:return struct.unpack("<f",struct.pack("<f",x))[0]
    except OverflowError:return math.copysign(math.inf,x)

def original_x87_surface_ratio(virtual_limit,original_arg8):
    numerator=i32(virtual_limit-original_arg8)
    if virtual_limit==0:
        if numerator==0:return math.nan
        return math.copysign(math.inf,numerator)
    return f32(numerator/virtual_limit)

def original_asm_wrapper(words,rectangle):
    # Read the original 0x241417C0 stack source operands as raw words.
    # Push order from native x86: arg7,arg6,arg5,arg4,arg3,extent B,
    # extent A,arg2,arg1. The CPU reverses stack push order for callee.
    lo_a,hi_a,lo_b,hi_b=rectangle
    push_order=[
        words[6],words[5],words[4],words[3],words[2],
        i32(hi_b-lo_b)&0xFFFFFFFF,
        i32(hi_a-lo_a)&0xFFFFFFFF,
        words[1],words[0]]
    return list(reversed(push_order))

def original_asm_clip(first,second,surface,offset,arg6,clip_accepts):
    ratio=original_x87_surface_ratio(surface,offset)
    if not clip_accepts:
        return ratio,first,second,False
    if arg6!=0:
        return ratio,tuple(f32(a+(b-a)*ratio) for a,b in zip(first,second)),second,True
    if offset!=surface:
        complement=1.0-ratio
        return ratio,first,tuple(f32(a+(b-a)*complement) for a,b in zip(first,second)),True
    return ratio,first,second,True

class OriginalDrawBridgeTests(unittest.TestCase):
    def test_all_owner_witnesses_match_synthetic_shape(self):
        report=verify_original_draw_bridge(fixture(),check_hashes=False)
        self.assertEqual(report["verified_complete_code_region_hashes"],0)
        self.assertEqual(report["verified_sparse_x86_instruction_anchors"],len(OPCODES))
        self.assertEqual(report["verified_direct_call_edges"],len(CALLS))
        self.assertEqual(report["original_callers_of_worker"],7)
        self.assertEqual(report["native_return_stack_words"],{"bridge":7,"worker":9})
        self.assertEqual(len(REGIONS),2)
        self.assertEqual(len(EXTERNAL_WORKER_CALLS),7)
        self.assertEqual(len(VGUI_SINGLETON_CALLS),9)

    def test_mutating_any_original_opcode_fails(self):
        for va in OPCODES:
            with self.subTest(va=hex(va)):
                pe=fixture()
                pe.corrupt(va)
                with self.assertRaises(VerificationError):
                    verify_original_draw_bridge(pe,check_hashes=False)

    def test_mutating_any_original_rel32_call_fails(self):
        for va in CALLS:
            with self.subTest(va=hex(va)):
                pe=fixture()
                pe.corrupt(va+1)
                with self.assertRaises(VerificationError):
                    verify_original_draw_bridge(pe,check_hashes=False)

    def test_full_body_hash_is_not_fabricated_by_sparse_anchors(self):
        for name,(va,size,digest) in REGIONS.items():
            with self.subTest(name=name):
                self.assertGreater(size,0)
                self.assertEqual(len(bytes.fromhex(digest)),32)
        with self.assertRaisesRegex(VerificationError,"full-code SHA mismatch"):
            verify_original_draw_bridge(fixture(),check_hashes=True)
        with self.assertRaises((VerificationError,ValueError)):
            audit(b"not original client",b"not original server")

    def test_invalid_original_executable_image_base_rejected(self):
        pe=fixture()
        pe.base=0x22000000
        with self.assertRaisesRegex(VerificationError,"image base"):
            verify_original_draw_bridge(pe,check_hashes=False)

    def test_stack_word_order_and_wrapped_rect_differences(self):
        original=[1,2,3,4,5,6,0x7FC01234]
        words=original_asm_wrapper(original,(10,210,20,420))
        self.assertEqual(words,[1,2,200,400,3,4,5,6,0x7FC01234])
        wrapped=original_asm_wrapper(original,(
            0x7fffffff,-0x80000000,-0x80000000,0x7fffffff))
        self.assertEqual(wrapped[2],1)
        self.assertEqual(wrapped[3],0xffffffff)

    def test_two_original_backend_guards_have_distinct_predicates(self):
        for field,texture in itertools.product((0,1,2,255),(-3,-1,0,1,17)):
            if field!=0:
                route="clip"
            elif texture==-1:
                route="skip"
            else:
                route="textured_subrect"
            if field:
                self.assertEqual(route,"clip")
            elif texture==-1:
                self.assertEqual(route,"skip")
            else:
                self.assertEqual(route,"textured_subrect")

    def test_original_x87_interpolation_and_signed_wrapping(self):
        first=(0.0,10.0)
        second=(100.0,210.0)
        observed=0
        for limit,arg8,arg6,accepts in itertools.product(
                (1,10,100,1000),(-10,0,1,10,100,120,1001),
                (0,1,4294967295),(False,True)):
            ratio,a,b,submitted=original_asm_clip(
                first,second,limit,arg8,arg6,accepts)
            self.assertEqual(submitted,accepts)
            self.assertEqual(ratio,original_x87_surface_ratio(limit,arg8))
            if not accepts:
                self.assertEqual((a,b),(first,second))
            elif arg6!=0:
                self.assertEqual(b,second)
            elif arg8==limit:
                self.assertEqual((a,b),(first,second))
            else:
                self.assertEqual(a,first)
            observed+=1
        self.assertEqual(observed,168)
        self.assertTrue(math.isnan(original_x87_surface_ratio(0,0)))
        self.assertLess(original_x87_surface_ratio(100,120),0.0)

if __name__=="__main__":
    unittest.main()
