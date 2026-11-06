"""Synthetic original x86 task-dispatch evidence; no proprietary SiN DLL."""
import importlib.util
from pathlib import Path
import struct
import sys
import unittest

TOOLS = Path(__file__).resolve().parents[1] / "tools"
sys.path.insert(0, str(TOOLS))
spec = importlib.util.spec_from_file_location(
    "assault_task_execution", TOOLS / "audit_sin_assault_task_execution.py")
a = importlib.util.module_from_spec(spec)
spec.loader.exec_module(a)


class FakePE:
    def __init__(self, pieces):
        self.pieces = pieces

    def at(self, va, size):
        for base, raw in self.pieces.items():
            if base <= va and va + size <= base + len(raw):
                return raw[va - base:va - base + size]
        raise a.VerificationError(f"no original synthetic bytes at 0x{va:08X}")


def rel32_call(va, target):
    return b"\xe8" + struct.pack("<i", target - va - 5)


def original_synthetic_fixture():
    pieces = {va: bytes.fromhex(opcode) for va, opcode, _ in a.WITNESSES}
    pieces[a.BEHAVIOR_VTABLE + a.START_TASK_VTABLE_OFFSET] = struct.pack(
        "<I", a.START_TASK_VA)
    pieces[a.BEHAVIOR_VTABLE + a.RUN_TASK_VTABLE_OFFSET] = struct.pack(
        "<I", a.RUN_TASK_VA)
    pieces[a.JUMP_TABLE_VA] = struct.pack(
        "<6I", *(target for _, _, target in a.JUMP_TABLE))
    for callsite, target, _ in a.CALLS:
        pieces[callsite] = rel32_call(callsite, target)
    return pieces


class OriginalAssaultTaskExecutionTests(unittest.TestCase):
    def test_all_byte_witnesses_and_original_dispatch_targets(self):
        result = a.audit_code(FakePE(original_synthetic_fixture()))
        self.assertEqual(result["verified_instruction_witnesses"],
                         len(a.WITNESSES))
        self.assertEqual(len(result["verified_direct_call_targets"]), 8)
        self.assertEqual(result["original_task_jump_table"][3]["case_va"],
                         "0x220CF600")
        self.assertEqual(result["original_task_jump_table"][4]["case_va"],
                         "0x220CF740")
        self.assertEqual(result["recovered_behaviors"]["TASK_HIT_RALLY_POINT"]["arrival_flag_field"],
                         "m_bHitRallyPoint +0x24")
        self.assertEqual(result["recovered_behaviors"]["TASK_HIT_ASSAULT_POINT"]["event_fire_call"],
                         "0x220CF467")
        self.assertEqual(result["recovered_behaviors"]["TASK_AWAIT_ASSAULT_TIMEOUT"]["float_source"],
                         "CAssaultPoint.m_flAssaultTimeout +0x3B4")

    def test_refuses_changed_rally_arrival_output_offset(self):
        data = original_synthetic_fixture()
        data[0x220CF550] = b"\x81\xc1\xbc\x03\x00\x00"
        with self.assertRaises(a.VerificationError):
            a.audit_code(FakePE(data))

    def test_refuses_changed_assault_arrival_flag(self):
        data = original_synthetic_fixture()
        data[0x220CF44A] = b"\xc6\x46\x24\x01"
        with self.assertRaises(a.VerificationError):
            a.audit_code(FakePE(data))

    def test_refuses_changed_configured_rally_delay_offset(self):
        data = original_synthetic_fixture()
        data[0x220CF617] = b"\x8b\x88\x98\x03\x00\x00"
        with self.assertRaises(a.VerificationError):
            a.audit_code(FakePE(data))

    def test_refuses_changed_assault_timeout_offset(self):
        data = original_synthetic_fixture()
        data[0x220CF663] = b"\x8b\x88\x9c\x03\x00\x00"
        with self.assertRaises(a.VerificationError):
            a.audit_code(FakePE(data))

    def test_refuses_routing_unregistered_id_to_real_task(self):
        data = original_synthetic_fixture()
        targets = [target for _, _, target in a.JUMP_TABLE]
        targets[4] = 0x220CF582
        data[a.JUMP_TABLE_VA] = struct.pack("<6I", *targets)
        with self.assertRaises(a.VerificationError):
            a.audit_code(FakePE(data))

    def test_refuses_diversion_in_time_defer_scheduling(self):
        data = original_synthetic_fixture()
        data[0x220CF593] = b"\xd9\x5e\x28"
        with self.assertRaises(a.VerificationError):
            a.audit_code(FakePE(data))

    def test_refuses_corrupted_next_point_advance_call(self):
        data = original_synthetic_fixture()
        data[0x220CF5E9] = rel32_call(0x220CF5E9, 0x220CE960)
        with self.assertRaises(a.VerificationError):
            a.audit_code(FakePE(data))

    def test_refuses_corrupted_run_task_vtable(self):
        data = original_synthetic_fixture()
        data[a.BEHAVIOR_VTABLE + a.RUN_TASK_VTABLE_OFFSET] = struct.pack(
            "<I", a.START_TASK_VA)
        with self.assertRaises(a.VerificationError):
            a.audit_code(FakePE(data))

    def test_rejects_nonoriginal_DLL(self):
        with self.assertRaises(a.VerificationError):
            a.audit(b"not the original 2006 SiN server.dll")


if __name__ == "__main__":
    unittest.main()
