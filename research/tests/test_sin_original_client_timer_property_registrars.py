"""Phase162 original-free tamper tests for native HUD registrar metadata.

Synthetic PE fixture deliberately contains sparse instruction witnesses,
NOT full proprietary code regions. Independent checks confirm one-time
flag write before callbacks and all class names/default string pointers.
"""
import pathlib
import struct
import sys
import unittest

sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/"tools"))
from audit_sin_original_client_timer_property_registrars import (
    REGISTRATION,ORIGINAL_STRINGS,CONSTRUCTOR_CALLS,STORES,
    verify_original_registrars,audit,VerificationError
)
class FakePE:
    base=0x24000000
    def __init__(self):self.memory={}
    def put(self,va,data):
        for i,value in enumerate(data):
            addr=va+i
            if addr in self.memory and self.memory[addr]!=value:
                raise AssertionError("conflicting sparse witness %#x"%addr)
            self.memory[addr]=value
    def at(self,va,n):
        return bytes(self.memory.get(va+i,0xcc) for i in range(n))
    def corrupt(self,va):self.memory[va]^=0x10
def pack(value):return struct.pack("<I",value)
def call(site,target):return b"\xe8"+struct.pack("<i",target-site-5)

def fixture():
    pe=FakePE()
    for va,text in ORIGINAL_STRINGS.items():
        pe.put(va,text.encode("ascii")+b"\0")
    for row in REGISTRATION:
        entry,sha,guard,owner,prop,member,typ,default,accessor,kind=row
        pe.put(entry+3,b"\x80\x3d"+pack(guard)+b"\0")
        pe.put(entry+0x0a,b"\x75\x50")
        pe.put(entry+0x0c,b"\x68"+pack(owner))
        pe.put(entry+0x11,b"\xc6\x05"+pack(guard)+b"\x01")
        pe.put(entry+0x18,call(entry+0x18,0x24011dc0))
        for offset,prefix,idx in STORES:
            pe.put(entry+offset,prefix+pack(row[idx]))
        pe.put(entry+0x52,b"\xc6\x44\x24\x18"+bytes((kind,)))
        pe.put(entry+0x57,call(entry+0x57,0x2400a300))
        pe.put(entry+0x5c,b"\x83\xc4\x18\xc3")
    for site,target in CONSTRUCTOR_CALLS.items():
        pe.put(site,call(site,target))
    return pe

class OriginalHudRegistrarTests(unittest.TestCase):
    def test_all_seven_metadata_records(self):
        self.assertEqual(len(REGISTRATION),7)
        self.assertEqual(len(CONSTRUCTOR_CALLS),7)
        result=verify_original_registrars(fixture(),check_hashes=False)
        self.assertEqual(result["verified_original_full_registrar_SHA256s"],0)
        self.assertEqual(result["original_direct_registry_calls"],14)
        self.assertEqual(result["original_constructor_registration_call_edges"],7)
        self.assertEqual(result["original_registration_order"],[
            "BreathTimerColor","BreathBarForeground","BreathBarBackground",
            "PoisonTimerColor","MutagenBarForeground",
            "MutagenBarBackground","MutagenBarInactive"])
        self.assertEqual(result["named_colors"]["poison"]["default"],"255 255 255 0")
        self.assertEqual(result["named_colors"]["breath"]["default"],"0 0 0 0")
        self.assertEqual(result["verified_original_metadata_string_literals"],
                         len(ORIGINAL_STRINGS))

    def test_mutate_every_original_descriptor_immediate(self):
        for row in REGISTRATION:
            entry=row[0]
            for offset,prefix,idx in STORES:
                with self.subTest(entry=hex(entry),field=idx):
                    pe=fixture();pe.corrupt(entry+offset+4)
                    with self.assertRaises(VerificationError):
                        verify_original_registrars(pe,check_hashes=False)

    def test_original_guard_set_and_call_order_mutations(self):
        for row in REGISTRATION:
            entry=row[0]
            for relative in (3,0x0a,0x0c,0x11,0x18,0x52,0x57,0x5c):
                with self.subTest(entry=hex(entry),offset=hex(relative)):
                    pe=fixture();pe.corrupt(entry+relative)
                    with self.assertRaises(VerificationError):
                        verify_original_registrars(pe,check_hashes=False)

    def test_names_defaults_and_real_constructor_calls_required(self):
        for address in ORIGINAL_STRINGS:
            with self.subTest(string=hex(address)):
                pe=fixture();pe.corrupt(address)
                with self.assertRaises(VerificationError):
                    verify_original_registrars(pe,check_hashes=False)
        for site in CONSTRUCTOR_CALLS:
            with self.subTest(call=hex(site)):
                pe=fixture();pe.corrupt(site+1)
                with self.assertRaises(VerificationError):
                    verify_original_registrars(pe,check_hashes=False)

    def test_full_original_code_sha_fails_on_synthetic_fixture(self):
        self.assertEqual(len({r[1] for r in REGISTRATION}),7)
        for row in REGISTRATION:
            self.assertEqual(len(bytes.fromhex(row[1])),32)
        with self.assertRaisesRegex(VerificationError,"full property registrar SHA256"):
            verify_original_registrars(fixture(),check_hashes=True)
        with self.assertRaises(VerificationError):
            audit(b"not the original client",b"not the original server")

    def test_wrong_original_module_load_base(self):
        pe=fixture();pe.base=0x22000000
        with self.assertRaisesRegex(VerificationError,"image base"):
            verify_original_registrars(pe,check_hashes=False)

if __name__=="__main__":unittest.main()
