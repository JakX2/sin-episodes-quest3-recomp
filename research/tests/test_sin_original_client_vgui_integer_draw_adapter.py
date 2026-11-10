"""Original-free Phase159 x87 integer-HUD draw audit and semantics.

The synthetic fixture deliberately includes only partial x86 instruction
witnesses. The 327-byte full method SHA requires owner-provided binaries.
Independently models the original x87 FILD/FSTP/FISTP stages using Python
float32 packing, modulo-32bit integer arithmetic, and truncation to zero.
"""
import itertools
import math
import pathlib
import struct
import sys
import unittest

sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/"tools"))
from audit_sin_original_client_vgui_integer_draw_adapter import (
    CALLS,GETTER_AND_WORKER_CALLS,OPCODES,ORIGINAL_EXTERNAL_CALLERS,REGIONS,
    verify_original_integer_draw, VerificationError,audit,
)

class MockPE:
    base=0x24000000
    def __init__(self):self.data={}
    def put(self,va,raw):
        for i,byte in enumerate(raw):
            if va+i in self.data and self.data[va+i]!=byte:
                raise AssertionError("conflicting partial fixture at %#x"%(va+i))
            self.data[va+i]=byte
    def at(self,va,count):
        return bytes(self.data.get(va+i,0xcc) for i in range(count))
    def damage(self,va):self.data[va]^=0x10

def fixture():
    pe=MockPE()
    for va,hexcode in OPCODES.items():
        pe.put(va,bytes.fromhex(hexcode))
    for site,target in CALLS.items():
        pe.put(site,b"\xe8"+struct.pack("<i",target-site-5))
    return pe

def native_f32(number):
    try:
        return struct.unpack("<f",struct.pack("<f",number))[0]
    except OverflowError:
        return math.copysign(math.inf,number)

def i32_wrap(n):
    bits=n&0xffffffff
    return bits if bits<0x80000000 else bits-0x100000000

def native_fistp(number):
    if not math.isfinite(number) or number>=2147483648 or number<-2147483648:
        return -2147483648
    return math.trunc(number)

def independent_x87_emulator(bounds,x,y,screen_int,denominator,opaque):
    a_lo,a_hi,b_lo,b_hi=bounds
    a=native_f32(i32_wrap(a_hi-a_lo))
    b=native_f32(i32_wrap(b_hi-b_lo))
    if math.isnan(denominator):
        ratio=math.nan
    elif denominator==0:
        ratio=(math.nan if screen_int==0 else
               math.copysign(math.inf,screen_int) * math.copysign(1.0,denominator))
    else:
        ratio=screen_int/denominator
    return [
        native_fistp(x)&0xffffffff,
        native_fistp(y)&0xffffffff,
        native_fistp(a*ratio)&0xffffffff,
        native_fistp(b*ratio)&0xffffffff,
        0,0,
        native_fistp(a)&0xffffffff,
        native_fistp(b)&0xffffffff,
        opaque&0xffffffff
    ]

class OriginalIntegerDrawTests(unittest.TestCase):
    def test_partial_witnesses_are_required(self):
        result=verify_original_integer_draw(fixture(),check_hashes=False)
        self.assertEqual(result["native_original_entry"],"0x241420D0")
        self.assertEqual(result["partial_original_opcode_witnesses"],len(OPCODES))
        self.assertEqual(result["original_direct_call_edges"],10)
        self.assertEqual(result["external_callers_of_original_function"],7)
        self.assertEqual(result["native_return_cleanup_bytes"],20)
        self.assertEqual(result["original_getters_order"],["0x240FFAE0","0x240FFAD0"])
        self.assertEqual(len(GETTER_AND_WORKER_CALLS),3)
        self.assertEqual(len(ORIGINAL_EXTERNAL_CALLERS),7)

    def test_mutation_each_original_opcode(self):
        for va in OPCODES:
            with self.subTest(va=hex(va)):
                p=fixture();p.damage(va)
                with self.assertRaises(VerificationError):
                    verify_original_integer_draw(p,check_hashes=False)

    def test_mutation_each_original_direct_call(self):
        for va in CALLS:
            with self.subTest(va=hex(va)):
                p=fixture();p.damage(va+1)
                with self.assertRaises(VerificationError):
                    verify_original_integer_draw(p,check_hashes=False)

    def test_synthetic_code_cannot_satisfy_original_function_sha(self):
        self.assertEqual(len(REGIONS),1)
        start,size,digest=next(iter(REGIONS.values()))
        self.assertEqual(start,0x241420D0)
        self.assertEqual(size,327)
        self.assertEqual(len(bytes.fromhex(digest)),32)
        with self.assertRaisesRegex(VerificationError,"SHA256 mismatch"):
            verify_original_integer_draw(fixture(),check_hashes=True)
        with self.assertRaises((VerificationError,ValueError)):
            audit(b"non-original client",b"non-original server")

    def test_image_base_is_validated(self):
        pe=fixture();pe.base=0x22000000
        with self.assertRaisesRegex(VerificationError,"image base"):
            verify_original_integer_draw(pe,check_hashes=False)

    def test_integer_conversion_rounding_and_invalids(self):
        for input_value,expected in [
            (0.0,0),(-0.0,0),(2.99,2),(-2.99,-2),
            (16777217,16777217),(2147483647,2147483647),
            (2147483648,-2147483648),(-2147483648,-2147483648),
            (-2147483649,-2147483648),(math.inf,-2147483648),
            (-math.inf,-2147483648),(math.nan,-2147483648),
        ]:
            with self.subTest(value=input_value):
                self.assertEqual(native_fistp(input_value),expected)
        self.assertEqual(native_fistp(native_f32(2147483647)),-2147483648)
        self.assertEqual(native_fistp(native_f32(16777217)),16777216)

    def test_symbolic_x87_stack_and_output_contract_matrix(self):
        # The source independently uses signed x86 ring difference,
        # float32 extent rounding and six FISTP operations.
        bounds_options=[
            (10,210,-10,390),(0,16777217,0,16777217),
            (-100,100,-200,200),(2147483647,-2147483648,0,1)
        ]
        values=[-9.99,-0.0,0.1,12.9,1000.0]
        dimensions=[-600,0,600,2147483647]
        divisors=[-1200.0,0.0,0.5,1200.0]
        checked=0
        for bounds,x,screen,divisor in itertools.product(
                bounds_options,values,dimensions,divisors):
            words=independent_x87_emulator(
                bounds,x,-3.9,screen,divisor,0x7fc01234)
            self.assertEqual(len(words),9)
            self.assertEqual(words[0],native_fistp(x)&0xffffffff)
            self.assertEqual(words[1],(-3)&0xffffffff)
            self.assertEqual(words[4:6],[0,0])
            self.assertEqual(words[8],0x7fc01234)
            self.assertTrue(all(isinstance(w,int) and
                                0<=w<=0xffffffff for w in words))
            checked+=1
        self.assertEqual(checked,320)
        self.assertEqual(
            independent_x87_emulator((10,210,-10,390),
                                     12.9,-3.9,900,1200.0,0x7fc01234),
            [12,0xfffffffd,150,300,0,0,200,400,0x7fc01234])

if __name__=="__main__":
    unittest.main()
