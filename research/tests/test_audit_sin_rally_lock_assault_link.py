"""Synthetic original Source x86 bytes only; no proprietary DLL embedded."""
import importlib.util
from pathlib import Path
import struct
import sys
import unittest

TOOLS = Path(__file__).resolve().parents[1] / "tools"
sys.path.insert(0, str(TOOLS))
spec = importlib.util.spec_from_file_location(
    "rally_lock", TOOLS / "audit_sin_rally_lock_assault_link.py"
)
audit = importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit)


def direct_call(source, target):
    return b"\xe8" + struct.pack("<i", target - source - 5)


def fixture():
    data = {
        address: chunk for _, address, chunk in audit.PATTERNS
    }
    data.update({
        address: direct_call(address, target)
        for _, address, target in audit.CALL_EDGES
    })
    data[audit.ERROR_LITERAL_VA] = audit.ERROR_LITERAL
    data[audit.NEXT_ERROR_LITERAL_VA] = audit.NEXT_ERROR_LITERAL
    return data


class FakePE:
    def __init__(self, pieces):
        self.pieces = pieces

    def at(self, address, size):
        for base, chunk in self.pieces.items():
            if base <= address and address + size <= base + len(chunk):
                start = address - base
                return chunk[start:start + size]
        raise audit.VerificationError(f"synthetic RVA 0x{address:08X} not mapped")


class RallyLockOriginalTests(unittest.TestCase):
    def test_all_original_instruction_witnesses(self):
        result = audit.audit_instruction_evidence(FakePE(fixture()))
        self.assertEqual(result["rallypoint_lock"]["locked_by_x86_member_offset"],
                         "0x3BC")
        self.assertEqual(result["rallypoint_lock"]["release_function_va"], "0x220CE4A0")
        self.assertEqual(result["assault_state_transition"]["linked_assaultpoint_handle_original_offset"],
                         "0x14")
        self.assertEqual(result["evidence"]["verified_call_edges"], 9)
        self.assertEqual(result["next_assaultpoint_chain"]["advance_function_va"],
                         "0x220CE5B0")
        self.assertEqual(result["next_assaultpoint_chain"]["terminal_rally_lock_release_call_va"],
                         "0x220CE70F")
        self.assertEqual(result["progression_trigger_sites"]["clear_on_contact"]["queried_condition_numeric_id"], 10)
        self.assertEqual(result["progression_trigger_sites"]["rally_arrival"]["OnArrival_event_member_offset"], "0x3A4")
        self.assertEqual(result["evidence"]["verified_instruction_witnesses"],
                         len(audit.PATTERNS))

    def test_corrupt_owner_offset_fails_closed(self):
        changed = fixture()
        changed[0x220CE433] = b"\x8b\x86\xb8\x03\x00\x00"
        with self.assertRaises(audit.VerificationError):
            audit.audit_instruction_evidence(FakePE(changed))

    def test_corrupt_acquire_function_call_fails_closed(self):
        changed = fixture()
        changed[0x220CFE49] = direct_call(0x220CFE49, 0x220CE450)
        with self.assertRaises(audit.VerificationError):
            audit.audit_instruction_evidence(FakePE(changed))

    def test_wrong_assaultpoint_link_field_fails_closed(self):
        changed = fixture()
        changed[0x220CE9A7] = b"\x8b\x80\x98\x03\x00\x00"
        with self.assertRaises(audit.VerificationError):
            audit.audit_instruction_evidence(FakePE(changed))

    def test_corrupt_clear_on_contact_gate_fails_closed(self):
        changed = fixture()
        changed[0x220CF960] = b"\x80\xb8\xb9\x03\x00\x00\x00"
        with self.assertRaises(audit.VerificationError):
            audit.audit_instruction_evidence(FakePE(changed))

    def test_corrupt_onarrival_output_call_fails_closed(self):
        changed = fixture()
        changed[0x220CF556] = direct_call(0x220CF556, 0x22156910)
        with self.assertRaises(audit.VerificationError):
            audit.audit_instruction_evidence(FakePE(changed))

    def test_corrupt_terminal_unlock_target_fails_closed(self):
        changed = fixture()
        changed[0x220CE70F] = direct_call(0x220CE70F, 0x220CE430)
        with self.assertRaises(audit.VerificationError):
            audit.audit_instruction_evidence(FakePE(changed))

    def test_corrupt_next_assaultpoint_name_offset_fails_closed(self):
        changed = fixture()
        changed[0x220CE610] = b"\x8b\x89\x94\x03\x00\x00"
        with self.assertRaises(audit.VerificationError):
            audit.audit_instruction_evidence(FakePE(changed))

    def test_corrupt_missing_next_point_message_fails_closed(self):
        changed = fixture()
        changed[audit.NEXT_ERROR_LITERAL_VA] = b"unrelated diagnostic\0"
        with self.assertRaises(audit.VerificationError):
            audit.audit_instruction_evidence(FakePE(changed))

    def test_bad_original_error_literal_fails_closed(self):
        changed = fixture()
        changed[audit.ERROR_LITERAL_VA] = b"unrelated error\x00"
        with self.assertRaises(audit.VerificationError):
            audit.audit_instruction_evidence(FakePE(changed))


if __name__ == "__main__":
    unittest.main()
