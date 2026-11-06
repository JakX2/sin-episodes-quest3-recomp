"""Synthetic original SiN task registry x86 witnesses, never proprietary DLLs."""
import importlib.util
from pathlib import Path
import struct
import sys
import unittest

TOOLS = Path(__file__).resolve().parents[1] / "tools"
sys.path.insert(0, str(TOOLS))
spec = importlib.util.spec_from_file_location(
    "original_task_registry", TOOLS / "audit_sin_native_assault_task_ids.py")
a = importlib.util.module_from_spec(spec)
spec.loader.exec_module(a)


def direct_call(source, destination):
    return b"\xe8" + struct.pack("<i", destination - source - 5)


class FakePE:
    def __init__(self, data):
        self.data = data

    def at(self, address, length):
        for base, content in self.data.items():
            if base <= address and address + length <= base + len(content):
                off = address - base
                return content[off:off + length]
        raise a.VerificationError(f"missing synthetic x86 region 0x{address:08X}")


def fixture():
    fake = {}
    for task_id, symbol, name_va, name_site, id_site, call_site, name_slot, id_slot in a.IMMEDIATE_TASKS:
        fake[name_va] = symbol.encode("ascii") + b"\0"
        fake[name_site] = (b"\xc7\x44\x24" + bytes([name_slot]) +
                           struct.pack("<I", name_va))
        fake[id_site] = (b"\xc7\x44\x24" + bytes([id_slot]) +
                         struct.pack("<I", task_id))
        fake[call_site] = direct_call(call_site, a.TASK_REGISTER_FUNCTION_VA)
    for task_id, symbol, name_va, name_site, load_site, load_code, store_site, store_code, call_site in a.REGISTER_TASKS:
        fake[name_va] = symbol.encode("ascii") + b"\0"
        fake[name_site] = b"\xc7\x44\x24\x1c" + struct.pack("<I", name_va)
        fake[load_site] = load_code
        fake[store_site] = store_code
        fake[call_site] = direct_call(call_site, a.TASK_REGISTER_FUNCTION_VA)
    for _, offset, method, signature in a.TASK_DISPATCH_VIRTUALS:
        fake[a.ASSAULT_BEHAVIOR_VTABLE_VA + offset] = struct.pack("<I", method)
        fake[method] = signature
    return fake


class OriginalTaskIdsTests(unittest.TestCase):
    def test_all_original_specific_task_ids_and_dispatch_virtuals(self):
        pe = FakePE(fixture())
        items = a.verify_native_task_registrations(pe)
        self.assertEqual(items["verified_original_specific_task_count"], 11)
        expected = {
            "TASK_GET_PATH_TO_RALLY_POINT": "0x186A0",
            "TASK_FACE_RALLY_POINT": "0x186A1",
            "TASK_GET_PATH_TO_ASSAULT_POINT": "0x186A2",
            "TASK_FACE_ASSAULT_POINT": "0x186A3",
            "TASK_HIT_ASSAULT_POINT": "0x186A4",
            "TASK_HIT_RALLY_POINT": "0x186A5",
            "TASK_AWAIT_CUE": "0x186A6",
            "TASK_AWAIT_ASSAULT_TIMEOUT": "0x186A7",
            "TASK_ANNOUNCE_CLEAR": "0x186A8",
            "TASK_WAIT_ASSAULT_DELAY": "0x186A9",
            "TASK_ASSAULT_DEFER_SCHEDULE_SELECTION": "0x186AB",
        }
        self.assertEqual({r["symbol"]: r["original_task_id"]
                          for r in items["original_task_registry_entries"]}, expected)
        dispatch = a.verify_original_dispatch_virtuals(pe)
        self.assertEqual(len(dispatch), 2)
        self.assertEqual(dispatch[0]["original_method_va"], "0x220CF190")
        self.assertEqual(dispatch[1]["original_method_va"], "0x220CF770")

    def test_corrupted_immediate_id_fails(self):
        data = fixture()
        data[0x220D0520] = b"\xc7\x44\x24\x20" + struct.pack("<I", 0x186A3)
        with self.assertRaises(a.VerificationError):
            a.verify_native_task_registrations(FakePE(data))

    def test_corrupted_register_borne_id_fails(self):
        data = fixture()
        data[0x220D062D] = b"\xbe\xa4\x86\x01\x00"
        with self.assertRaises(a.VerificationError):
            a.verify_native_task_registrations(FakePE(data))

    def test_corrupted_registration_call_fails(self):
        data = fixture()
        data[0x220D04FD] = b"\xc7\x44\x24\x20" + struct.pack("<I", 0x186A0)
        data[0x220D0505] = direct_call(0x220D0505, a.TASK_REGISTER_FUNCTION_VA + 16)
        with self.assertRaises(a.VerificationError):
            a.verify_native_task_registrations(FakePE(data))

    def test_corrupted_vtable_dispatch_fails(self):
        data = fixture()
        data[a.ASSAULT_BEHAVIOR_VTABLE_VA + 0x60] = struct.pack("<I", 0x220CF770)
        with self.assertRaises(a.VerificationError):
            a.verify_original_dispatch_virtuals(FakePE(data))

    def test_corrupted_dispatch_instruction_fails(self):
        data = fixture()
        data[0x220CF770] = b"\x90" * len(data[0x220CF770])
        with self.assertRaises(a.VerificationError):
            a.verify_original_dispatch_virtuals(FakePE(data))

    def test_reject_wrong_dll_hash_before_parsing(self):
        with self.assertRaises(a.VerificationError):
            a.audit(b"synthetic bytes, not original DLL")


if __name__ == "__main__":
    unittest.main()
