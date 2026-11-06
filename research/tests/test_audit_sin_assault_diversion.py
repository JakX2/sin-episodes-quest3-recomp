"""Self-contained original 2006 assault diversion witnesses: no proprietary DLLs."""
import importlib.util
from pathlib import Path
import struct
import sys
import unittest

TOOLS = Path(__file__).resolve().parents[1] / "tools"
sys.path.insert(0, str(TOOLS))
spec = importlib.util.spec_from_file_location(
    "assault_diversion", TOOLS / "audit_sin_assault_diversion.py")
a = importlib.util.module_from_spec(spec)
spec.loader.exec_module(a)


def call_bytes(source, target):
    return b"\xe8" + struct.pack("<i", target - source - 5)


class FakePE:
    def __init__(self, regions):
        self.regions = regions

    def at(self, va, size):
        for base, raw in self.regions.items():
            if base <= va and va + size <= base + len(raw):
                start = va - base
                return raw[start:start + size]
        raise a.VerificationError(f"no fake PE region {va:#x}+{size}")


def fixture():
    data = {address: raw for _, address, raw in a.INSTRUCTIONS}
    data.update({address: call_bytes(address, target)
                 for _, address, target in a.DIRECT_CALLS})
    data[a.VTABLE_VA - 4] = struct.pack("<I", a.COL_VA)
    data[a.COL_VA + 12] = struct.pack("<I", a.RTTI_NAME_VA - 8)
    data[a.RTTI_NAME_VA] = b".?AVCAI_AssaultBehavior@@\0"
    for _, off, target in a.VTABLE_ENTRIES:
        data[a.VTABLE_VA + off] = struct.pack("<I", target)
    for _, address, number in a.LITERAL_FLOATS:
        data[address] = struct.pack("<f", number)
    return data


FGD = b'''@PointClass = assault_assaultpoint : "Assault point" [
assaulttimeout(float) : "Assault time out" : "3.0" : "This point is cleared when no enemies are seen for this long (seconds)"
allowdiversion(choices) : "Allow diversion" : 0 : "If you come in contact with enemies while approaching the assault point, divert to kill them. Resume the assault once contact is lost."
input SetAllowDiversion(integer) : "Set the allow diversion flag."
input SetClearOnContact(integer) : "Set the clear on contact flag."
]'''


class AssaultDiversionOriginalTests(unittest.TestCase):
    def test_vtable_fpu_threshold_and_condition_calls(self):
        result = a.audit_code(FakePE(fixture()))
        self.assertEqual(result["original_cpp_class"], "CAI_AssaultBehavior")
        self.assertEqual(result["verified_literals"]["elapsed threshold"], 4.0)
        self.assertEqual(result["original_x86_assaultpoint_member_offset"], "0x3B9")
        self.assertEqual(result["observed_numeric_conditions"]["set"], [0x18])
        self.assertEqual(result["observed_numeric_conditions"]["clear_in_schedule_path"],
                         [0x19, 0x0A])
        self.assertEqual(len(result["original_virtual_methods"]), 3)
        self.assertEqual(result["verified_direct_calls"], len(a.DIRECT_CALLS))

    def test_original_sdk_describes_diversion_separately_from_timeout(self):
        record = a.corroborate_sdk_fgd(FGD)
        self.assertEqual(record["allowdiversion_default"], 0)
        self.assertIn("Resume the assault", record["allowdiversion_documented_effect"])
        self.assertEqual(record["assaulttimeout_default_seconds"], 3.0)
        with self.assertRaises(a.VerificationError):
            a.corroborate_sdk_fgd(FGD.replace(b'3.0', b'4.0'))
        with self.assertRaises(a.VerificationError):
            a.corroborate_sdk_fgd(FGD.replace(b'allowdiversion(choices)',
                                                b'allowdiversion(float)'))

    def test_corrupted_vtable_method_fails_closed(self):
        data = fixture()
        data[a.VTABLE_VA + 0x18] = struct.pack("<I", 0x220CEBF0)
        with self.assertRaises(a.VerificationError):
            a.audit_code(FakePE(data))

    def test_corrupted_condition_call_fails_closed(self):
        data = fixture()
        data[0x220CEBCE] = call_bytes(0x220CEBCE, 0x220397C0)
        with self.assertRaises(a.VerificationError):
            a.audit_code(FakePE(data))

    def test_corrupted_4_second_constant_fails_closed(self):
        data = fixture()
        data[0x223DDCF4] = struct.pack("<f", 3.0)
        with self.assertRaises(a.VerificationError):
            a.audit_code(FakePE(data))

    def test_corrupted_allowdiversion_member_offset_fails_closed(self):
        data = fixture()
        data[0x220CEB83] = b"\x38\x98\xb8\x03\x00\x00"
        with self.assertRaises(a.VerificationError):
            a.audit_code(FakePE(data))

    def test_corrupted_rtti_type_descriptor_fails_closed(self):
        data = fixture()
        data[a.COL_VA + 12] = struct.pack("<I", 0)
        with self.assertRaises(a.VerificationError):
            a.audit_code(FakePE(data))

    def test_wrong_original_server_hash_is_rejected(self):
        with self.assertRaises(a.VerificationError):
            a.audit(b"not a game DLL")


if __name__ == "__main__":
    unittest.main()
