"""Tests original Source condition registration witnesses with synthetic bytes only."""
import importlib.util
from pathlib import Path
import re
import struct
import sys
import unittest

TOOLS = Path(__file__).resolve().parents[1] / "tools"
sys.path.insert(0, str(TOOLS))
spec = importlib.util.spec_from_file_location(
    "condition_registry", TOOLS / "audit_sin_assault_condition_registry.py"
)
a = importlib.util.module_from_spec(spec)
spec.loader.exec_module(a)


class FakePE:
    def __init__(self, memory):
        self.memory = memory

    def at(self, address, size):
        for base, data in self.memory.items():
            if base <= address and address + size <= base + len(data):
                offset = address - base
                return data[offset:offset + size]
        raise a.VerificationError(f"synthetic original bytes missing at 0x{address:08X}")


def direct_call(source, destination):
    return b"\xe8" + struct.pack("<i", destination - source - 5)


def fixture():
    memory = {}
    for name, cond_id, text_va, push_va, name_va, call_va in a.SOURCE_CONDITIONS:
        memory[text_va] = name.encode("ascii") + b"\0"
        memory[push_va - 10] = (
            b"\x68" + struct.pack("<I", a.REGISTRY_SOURCE_FILE_VA) +
            b"\x68" + struct.pack("<I", a.REGISTRY_CONTEXT_STRING_VA))
        memory[push_va] = bytes((0x6A, cond_id))
        memory[name_va] = b"\x68" + struct.pack("<I", text_va)
        memory[name_va + 5] = b"\xB9" + struct.pack("<I", a.REGISTRY_THIS_VA)
        memory[call_va] = direct_call(call_va, a.REGISTRY_FUNCTION_VA)
    for _, push_va, cond_id, call_va, destination in a.ASSAULT_USE_SITES:
        memory[push_va] = bytes((0x6A, cond_id))
        memory[call_va] = direct_call(call_va, destination)
    return memory


class OriginalAssaultConditionRegistryTests(unittest.TestCase):
    def test_every_registered_name_and_actual_callsite(self):
        result = a.audit_original(FakePE(fixture()))
        self.assertEqual(result["verified_registration_count"], 9)
        self.assertEqual(result["verified_assault_callsites"], 7)
        mapping = {x["original_numeric_id"]: x["symbol"]
                   for x in result["original_conditions"]}
        self.assertEqual(mapping["0x18"], "COND_PROVOKED")
        self.assertEqual(mapping["0x33"], "COND_HEAR_COMBAT")
        self.assertEqual(mapping["0x3B"], "COND_RECEIVED_ORDERS")
        self.assertEqual(mapping["0x0A"], "COND_SEE_ENEMY")
        self.assertEqual(mapping["0x19"], "COND_NEW_ENEMY")
        by_role = {x["site"]: x["condition"]
                   for x in result["original_assault_condition_use_sites"]}
        self.assertEqual(by_role[
            "ai_goal_assault.BeginAssault signals COND_PROVOKED"], "COND_PROVOKED")
        self.assertEqual(by_role[
            "CAI_AssaultBehavior gunfire cue queries COND_HEAR_COMBAT"],
            "COND_HEAR_COMBAT")
        self.assertEqual(by_role[
            "CAI_AssaultBehavior diversion queries COND_RECEIVED_ORDERS"],
            "COND_RECEIVED_ORDERS")

    def test_wrong_registration_id_rejected(self):
        memory = fixture()
        memory[0x2205AA4D] = b"\x6a\x3b"  # provoked->received orders substitution
        with self.assertRaises(a.VerificationError):
            a.audit_original(FakePE(memory))

    def test_wrong_symbol_pointer_rejected(self):
        memory = fixture()
        memory[0x2205AE00] = b"\x68" + struct.pack("<I", 0x223E0960)
        with self.assertRaises(a.VerificationError):
            a.audit_original(FakePE(memory))

    def test_wrong_original_symbol_text_rejected(self):
        memory = fixture()
        memory[0x223E0714] = b"COND_HEAR_DANGER\0"
        with self.assertRaises(a.VerificationError):
            a.audit_original(FakePE(memory))

    def test_wrong_registry_receiver_rejected(self):
        memory = fixture()
        memory[0x2205AA54] = b"\xb9" + struct.pack("<I", 0x22547608)
        with self.assertRaises(a.VerificationError):
            a.audit_original(FakePE(memory))

    def test_wrong_registry_call_rejected(self):
        memory = fixture()
        memory[0x2205AE0A] = direct_call(0x2205AE0A, 0x22073770)
        with self.assertRaises(a.VerificationError):
            a.audit_original(FakePE(memory))

    def test_wrong_real_goal_input_condition_rejected(self):
        memory = fixture()
        memory[0x220CFFAA] = b"\x6a\x3b"
        with self.assertRaises(a.VerificationError):
            a.audit_original(FakePE(memory))

    def test_wrong_gunfire_condition_call_rejected(self):
        memory = fixture()
        memory[0x220CEAB5] = direct_call(0x220CEAB5, 0x22039760)
        with self.assertRaises(a.VerificationError):
            a.audit_original(FakePE(memory))

    def test_wrong_schedule_cleanup_condition_rejected(self):
        memory = fixture()
        memory[0x220CED99] = b"\x6a\x18"
        with self.assertRaises(a.VerificationError):
            a.audit_original(FakePE(memory))

    def test_portable_cpp_mapping_cannot_diverge(self):
        header = (Path(__file__).resolve().parents[2] /
                  "include/sin_compat/original_assault_condition_ids.hpp").read_text()
        cpp = re.findall(
            r'\{"(COND_[A-Z0-9_]+)",\s*LegacyAssaultConditionId\{(0x[0-9A-Fa-f]+)\},\s*OriginalAssaultCondition::\w+\}',
            header)
        expected = [(name, cond_id)
                    for name, cond_id, *_ in a.SOURCE_CONDITIONS]
        self.assertEqual([(name, int(number, 16)) for name, number in cpp],
                         expected)

    def test_nonoriginal_dll_is_rejected(self):
        with self.assertRaises(a.VerificationError):
            a.audit(b"some other game dll")


if __name__ == "__main__":
    unittest.main()
