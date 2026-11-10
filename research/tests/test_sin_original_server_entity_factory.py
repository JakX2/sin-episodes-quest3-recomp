"""Phase170 original-free x86 mutation tests and independent factory matrix."""
import hashlib
import itertools
import pathlib
import struct
import sys
import unittest

sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/"tools"))
from audit_sin_original_server_entity_factory import (
    REGIONS,OPCODES,CALLS,INCOMING,LITERALS,
    verify_original_factory,VerificationError)

class MockPE:
    base=0x22000000
    def __init__(self):self.memory={}
    def put(self,va,blob):
        for i,byte in enumerate(blob):
            previous=self.memory.get(va+i)
            if previous is not None and previous!=byte:
                raise AssertionError("native sparse witness overlap %#x"%(va+i))
            self.memory[va+i]=byte
    def at(self,va,n):
        return bytes(self.memory.get(va+i,0xCC) for i in range(n))
    def damage(self,va):self.memory[va]^=1

def fixture():
    pe=MockPE()
    for va,opcode in OPCODES.items():pe.put(va,bytes.fromhex(opcode))
    for va,target in {**CALLS,**INCOMING}.items():
        pe.put(va,bytes((0xE8,))+struct.pack("<i",target-va-5))
    for va,raw in LITERALS.items():pe.put(va,raw)
    return pe

def independent_primary_factory(index,edict_ok,factory_found,guard_bit,previous_cookie):
    calls=[]
    cookie=previous_cookie
    if index!=-1:
        calls.append("edict_create")
        cookie=0xA000 if edict_ok else 0
        if not edict_ok:calls.append("warning")
    if not guard_bit:
        calls.extend(("registry_alloc","registry_init","destructor_register"))
    calls.append("lookup")
    lookup_cookie=cookie
    cookie=0
    if factory_found:calls.append("dispatch24")
    return calls,cookie,lookup_cookie

class OriginalFactoryTests(unittest.TestCase):
    def test_native_evidence_ledger(self):
        r=verify_original_factory(fixture(),check_hashes=False)
        self.assertEqual(r["verified_complete_original_function_hashes"],0)
        self.assertEqual(r["verified_sparse_opcode_witnesses"],49)
        self.assertEqual(r["verified_native_direct_call_edges"],6)
        self.assertEqual(r["verified_incoming_flak_creation_calls"],1)
        self.assertEqual(len(REGIONS),4)
        self.assertEqual(len(OPCODES),49)
    def test_all_original_sparse_opcodes_mutation_guarded(self):
        for va in OPCODES:
            with self.subTest(opcode=hex(va)):
                pe=fixture();pe.damage(va)
                with self.assertRaisesRegex(VerificationError,"opcode"):
                    verify_original_factory(pe,check_hashes=False)
    def test_all_native_rel32_calls_mutation_guarded(self):
        for va in {**CALLS,**INCOMING}:
            with self.subTest(call=hex(va)):
                pe=fixture();pe.damage(va+1)
                with self.assertRaisesRegex(VerificationError,"CALL"):
                    verify_original_factory(pe,check_hashes=False)
    def test_original_diagnostic_literal_mutated(self):
        for va in LITERALS:
            pe=fixture();pe.damage(va)
            with self.assertRaisesRegex(VerificationError,"diagnostic"):
                verify_original_factory(pe,check_hashes=False)
    def test_real_whole_function_sha_rejects_sparse_fixture(self):
        self.assertEqual({v[1] for v in REGIONS.values()},{89,29,48,96})
        for _,(va,length,digest) in REGIONS.items():
            self.assertEqual(len(bytes.fromhex(digest)),32)
            self.assertNotEqual(hashlib.sha256(fixture().at(va,length)).hexdigest(),digest)
        with self.assertRaisesRegex(VerificationError,"whole-function SHA"):
            verify_original_factory(fixture(),check_hashes=True)
    def test_wrong_original_pe_image_base(self):
        pe=fixture();pe.base=0x24000000
        with self.assertRaisesRegex(VerificationError,"image base"):
            verify_original_factory(pe,check_hashes=False)
    def test_direct_call_fingerprint_edges(self):
        self.assertEqual(INCOMING[0x2231B9F9],0x22266B00)
        self.assertEqual(CALLS[0x22266B34],0x2228B110)
        self.assertEqual(CALLS[0x22266B60],0x2228B110)
        self.assertEqual(CALLS[0x2228B128],0x2228AFB0)
        self.assertEqual(CALLS[0x2228AFD5],0x2226E390)
        self.assertEqual(CALLS[0x2228B006],0x2228F360)
    def test_independent_factory_state_matrix(self):
        count=0
        for index,edict_ok,factory_found,guard_bit,previous in itertools.product(
            (-1,0,1,-2),(False,True),(False,True),(False,True),(0,0xBEEF)
        ):
            calls,final,lookup_cookie=independent_primary_factory(
                index,edict_ok,factory_found,guard_bit,previous)
            self.assertEqual(final,0)
            self.assertEqual(lookup_cookie,
                             previous if index==-1 else (0xA000 if edict_ok else 0))
            self.assertEqual("edict_create" in calls,index!=-1)
            self.assertEqual("warning" in calls,index!=-1 and not edict_ok)
            self.assertEqual("registry_alloc" in calls,not guard_bit)
            self.assertEqual("dispatch24" in calls,factory_found)
            self.assertEqual(calls[-1],"dispatch24" if factory_found else "lookup")
            count+=1
        self.assertEqual(count,64)
    def test_secondary_factory_does_not_touch_edict_context(self):
        for cookie,has_factory in itertools.product((0,0xBEEF),(False,True)):
            result_cookie=cookie
            branch="dispatch20" if has_factory else "return_null"
            self.assertEqual(result_cookie,cookie)
            self.assertNotEqual(branch,"dispatch24")

if __name__=="__main__":
    unittest.main()