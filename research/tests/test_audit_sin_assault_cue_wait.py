"""Synthetic original DLL cue/wait evidence, with no proprietary executable."""
import importlib.util
from pathlib import Path
import struct
import sys
import unittest

TOOLS = Path(__file__).resolve().parents[1] / "tools"
sys.path.insert(0, str(TOOLS))
spec = importlib.util.spec_from_file_location(
    "assault_cue_wait", TOOLS / "audit_sin_assault_cue_wait.py")
a = importlib.util.module_from_spec(spec)
spec.loader.exec_module(a)


class FakePE:
    def __init__(self, sections):
        self.sections = sections

    def at(self, va, size):
        for address, content in self.sections.items():
            if address <= va and va + size <= address + len(content):
                start = va - address
                return content[start:start + size]
        raise a.VerificationError(f"missing synthetic original address 0x{va:08X}")


def fixture():
    mapping = {va: bytes.fromhex(hex_bytes) for va, hex_bytes, _ in a.WITNESSES}
    mapping[a.CUE_JUMP_TABLE_VA] = struct.pack(
        "<5I", *(target for _, target, _ in a.CUE_TARGETS))
    source, dest = a.GUNFIRE_CONDITION_CALL
    mapping[source] = b"\xe8" + struct.pack("<i", dest - source - 5)
    mapping[a.DURATION_SCALE_VA] = struct.pack("<f", 10.0)
    mapping[a.DECISECOND_MULTIPLIER_VA] = struct.pack("<d", 0.1)
    return mapping


class OriginalAssaultCueWaitAuditTests(unittest.TestCase):
    def test_original_cue_switch_and_wait_clock_evidence(self):
        result = a.audit_code(FakePE(fixture()))
        self.assertEqual(result["verified_instruction_witnesses"], len(a.WITNESSES))
        self.assertEqual(len(result["verified_cue_cases"]), 5)
        self.assertEqual(result["verified_cue_cases"][3]["original_case_va"],
                         "0x220CEAC1")
        self.assertEqual(result["received_cue_override"]["received_value"], 4)
        self.assertEqual(result["gunfire_mode"]["npc_condition_numeric_id"], "0x33")
        self.assertEqual(result["original_wait"]["duration_scale"], 10.0)
        self.assertEqual(result["original_wait"]["duration_quantum_seconds"], 0.1)
        self.assertEqual(result["original_wait"]["npc_deadline_field_offset"], "0x9DC")

    def test_corrupt_cue_requested_mode_field(self):
        original = fixture()
        original[0x220CEA98] = b"\x8b\x41\x20"
        with self.assertRaises(a.VerificationError):
            a.audit_code(FakePE(original))

    def test_corrupt_mode_3_received_cue_write(self):
        original = fixture()
        original[0x220CEAC1] = b"\xc7\x41\x20\x04\x00\x00\x00"
        with self.assertRaises(a.VerificationError):
            a.audit_code(FakePE(original))

    def test_corrupt_mode_2_condition_call(self):
        original = fixture()
        src, dest = a.GUNFIRE_CONDITION_CALL
        original[src] = b"\xe8" + struct.pack("<i", dest + 0x10 - src - 5)
        with self.assertRaises(a.VerificationError):
            a.audit_code(FakePE(original))

    def test_corrupt_cue_switch_target(self):
        original = fixture()
        targets = [target for _, target, _ in a.CUE_TARGETS]
        targets[1] = targets[0]
        original[a.CUE_JUMP_TABLE_VA] = struct.pack("<5I", *targets)
        with self.assertRaises(a.VerificationError):
            a.audit_code(FakePE(original))

    def test_corrupt_wait_rounding_mode(self):
        original = fixture()
        original[0x220325E6] = b"\x0d\x00\x04\x00\x00" # round down, not up
        with self.assertRaises(a.VerificationError):
            a.audit_code(FakePE(original))

    def test_corrupt_deadline_field_or_timer_completion(self):
        original = fixture()
        original[0x22032624] = b"\xd9\x9e\xd8\x09\x00\x00"
        with self.assertRaises(a.VerificationError):
            a.audit_code(FakePE(original))
        original = fixture()
        original[0x220326D8] = b"\xd8\x99\xd8\x09\x00\x00"
        with self.assertRaises(a.VerificationError):
            a.audit_code(FakePE(original))

    def test_corrupt_decisecond_multiplier(self):
        original = fixture()
        original[a.DECISECOND_MULTIPLIER_VA] = struct.pack("<d", 0.2)
        with self.assertRaises(a.VerificationError):
            a.audit_code(FakePE(original))

    def test_rejects_unpinned_original_dll(self):
        with self.assertRaises(a.VerificationError):
            a.audit(b"not the original SHA256-pinned DLL")


if __name__ == "__main__":
    unittest.main()
