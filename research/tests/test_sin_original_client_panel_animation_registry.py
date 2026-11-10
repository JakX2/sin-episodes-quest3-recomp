"""Phase164 original-free VGUI class registry evidence regression.

Synthetic fixtures contain sparse public instruction witnesses ONLY;
full original 2006 client/server DLL code is never embedded. Independent
Q_strnicmp-prefix, class lookup and get-or-create branch checks.
"""
import itertools
import pathlib
import struct
import sys
import unittest

sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/"tools"))
from audit_sin_original_client_panel_animation_registry import (
    REGIONS,OPCODES,CALLS,NATIVE_LITERALS,verify_original_registry,
    VerificationError,audit,
)

class SyntheticPE:
    base=0x24000000
    def __init__(self):self.data={}
    def put(self,va,raw):
        for i,value in enumerate(raw):
            key=va+i
            if key in self.data and self.data[key]!=value:
                raise AssertionError("synthetic witness collision %#x"%key)
            self.data[key]=value
    def at(self,va,n):
        return bytes(self.data.get(va+i,0xCC) for i in range(n))
    def corrupt(self,va):
        self.data[va]^=0x10

def fixture():
    pe=SyntheticPE()
    for va,raw in NATIVE_LITERALS.items():pe.put(va,raw)
    for va,hextext in OPCODES.items():pe.put(va,bytes.fromhex(hextext))
    for va,dst in CALLS.items():
        pe.put(va,b"\xe8"+struct.pack("<i",dst-va-5))
    return pe

def independent_native_prefix(name):
    # Q_strnicmp(name, "vgui::", 6) is case-insensitive only for this
    # SIX-byte prefix; suffix is passed to original intern/map verbatim.
    if len(name)>=6 and name[:6].casefold()=="vgui::":
        return name[6:],True
    return name,False

def simulate_owner_lookup(input_name,owners,allow_alloc=True):
    normalized,_=independent_native_prefix(input_name)
    steps=["find"]
    if normalized in owners:
        return owners[normalized],steps
    steps.extend(("setup","allocate","insert"))
    owner=0x1000+28*len(owners) if allow_alloc else 0
    owners[normalized]=owner
    return owner,steps

class OriginalPanelRegistryTests(unittest.TestCase):
    def test_all_binary_evidence_in_synthetic_mode(self):
        result=verify_original_registry(fixture(),check_hashes=False)
        self.assertEqual(result["complete_original_method_hashes"],0)
        self.assertEqual(result["verified_original_opcode_anchors"],len(OPCODES))
        self.assertEqual(result["verified_native_call_edges"],20)
        self.assertEqual(result["original_seven_hud_registrar_owner_calls"],7)
        self.assertEqual(len(REGIONS),4)
        self.assertEqual(len(OPCODES),29)
        self.assertEqual(len(NATIVE_LITERALS),2)

    def test_all_29_original_opcode_mutations_rejected(self):
        for va in OPCODES:
            with self.subTest(instruction=hex(va)):
                pe=fixture();pe.corrupt(va)
                with self.assertRaises(VerificationError):
                    verify_original_registry(pe,check_hashes=False)

    def test_all_20_original_direct_calls_guarded(self):
        for va in CALLS:
            with self.subTest(call=hex(va)):
                pe=fixture();pe.corrupt(va+1)
                with self.assertRaises(VerificationError):
                    verify_original_registry(pe,check_hashes=False)

    def test_original_class_prefix_and_diagnostic_strings_guarded(self):
        for va in NATIVE_LITERALS:
            with self.subTest(address=hex(va)):
                pe=fixture();pe.corrupt(va)
                with self.assertRaises(VerificationError):
                    verify_original_registry(pe,check_hashes=False)
        self.assertEqual(NATIVE_LITERALS[0x242D3670],b"vgui::\0")
        self.assertIn(b"Panel Animation class",NATIVE_LITERALS[0x242D4318])

    def test_full_original_function_hashes_not_embedded(self):
        expected_sizes={0x24011D30:129,0x24011DC0:17,
                        0x24011B80:101,0x24011BF0:107}
        for entry,size,digest in REGIONS.values():
            self.assertEqual(size,expected_sizes[entry])
            self.assertEqual(len(bytes.fromhex(digest)),32)
        with self.assertRaisesRegex(VerificationError,"complete SHA256"):
            verify_original_registry(fixture(),check_hashes=True)
        with self.assertRaises((VerificationError,ValueError)):
            audit(b"not retail client",b"not retail server")

    def test_real_callers_of_owner_wrapper_are_seven_registrars(self):
        actual=tuple(sorted(va for va,dst in CALLS.items()
                            if dst==0x24011DC0))
        self.assertEqual(actual,(
            0x24230DF8,0x24230E68,0x24230ED8,
            0x24236468,0x242364D8,0x24236548,0x242365B8))
        self.assertEqual(CALLS[0x24011DC1],0x24011D30)
        self.assertEqual(CALLS[0x24011DCA],0x24011BF0)

    def test_independent_q_strnicmp_six_byte_behavior(self):
        sample={
            "vgui::Panel":("Panel",True),
            "VGUI::Panel":("Panel",True),
            "VgUi::CHudBreathTimer":("CHudBreathTimer",True),
            "vgui::":("",True),
            "vgui:":("vgui:",False),
            "vgui:::Panel":(":Panel",True),
            "prefix_vgui::Panel":("prefix_vgui::Panel",False),
            "CHudPoisonTimer":("CHudPoisonTimer",False),
            "vguI::PaneL":("PaneL",True),
        }
        for key,expected in sample.items():
            with self.subTest(name=key):
                self.assertEqual(independent_native_prefix(key),expected)
        count=0
        for chars in itertools.product(
            ("v","V"),("g","G"),("u","U"),("i","I")):
            key="".join(chars)+"::SomeClass"
            self.assertEqual(independent_native_prefix(key),
                             ("SomeClass",True))
            count+=1
        self.assertEqual(count,16)

    def test_owner_lookup_duplicate_and_missing_registration(self):
        owners={}
        owner,steps=simulate_owner_lookup("vgui::CHudPoisonTimer",owners)
        self.assertEqual(steps,["find","setup","allocate","insert"])
        self.assertEqual(owner,0x1000)
        same,steps=simulate_owner_lookup("VGUI::CHudPoisonTimer",owners)
        self.assertEqual(same,owner)
        self.assertEqual(steps,["find"])
        other,steps=simulate_owner_lookup("CHudBreathTimer",owners)
        self.assertNotEqual(other,owner)
        self.assertEqual(steps,["find","setup","allocate","insert"])
        missing,steps=simulate_owner_lookup(
            "UnavailablePanel",owners,allow_alloc=False)
        self.assertEqual(missing,0)
        self.assertEqual(steps,["find","setup","allocate","insert"])

    def test_original_PE_image_base_required(self):
        pe=fixture();pe.base=0x22000000
        with self.assertRaisesRegex(VerificationError,"image base"):
            verify_original_registry(pe,check_hashes=False)

if __name__=="__main__":
    unittest.main()
