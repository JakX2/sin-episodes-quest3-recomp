"""Synthetic-only x86/RTTI corruption checks; real SiN game binaries are not bundled."""
import importlib.util
from pathlib import Path
import struct
import sys
import unittest

TOOLS = Path(__file__).resolve().parents[1] / "tools"
sys.path.insert(0, str(TOOLS))
spec = importlib.util.spec_from_file_location(
    "original_cue4_provenance", TOOLS / "audit_sin_assault_cue4_provenance.py")
a = importlib.util.module_from_spec(spec)
spec.loader.exec_module(a)


class FakePE:
    base = 0x22000000
    def __init__(self):
        self.memory = {
            a.STAT_RTTI+8: b".?AVCStatAdvisor@@\0",
            0x224E4C3C: b".?AVCAI_AssaultBehavior@@\0",
            a.STAT_VTABLE_LOCATOR: struct.pack("<I", a.STAT_COL),
            a.STAT_COL+12: struct.pack("<I", a.STAT_RTTI),
            a.CUE_SETTER: bytes.fromhex("8b 41 04 33 d2 3b c2 74 0c"),
            0x220CE535: bytes.fromhex("8b 44 24 04 89 41 20"),
            0x220CE547: b"\xe9"+struct.pack("<i", 0x22039760-0x220CE54C),
        }
        for slot, target in a.STAT_VTABLE_ENTRIES:
            self.memory[slot] = struct.pack("<I", target)
        for _, va, signature in a.WRITES:
            self.memory[va] = bytes.fromhex(signature)
        self.data = b"\x90"*80 + b"\0"*160
        self.sections = [
            (".text", 0x1000, 80, 0, 80),
            (".rdata", 0x3D2000, 80, 80, 80),
            (".data", 0x4D6000, 80, 160, 80),
        ]

    def at(self, va, length):
        for start, content in self.memory.items():
            if start <= va and va+length <= start+len(content):
                return content[va-start:va-start+length]
        raise a.VerificationError(f"unseeded synthetic VA {va:#x}")


class OriginalCueProvenanceTests(unittest.TestCase):
    def test_complete_2006_witness_fixture(self):
        report = a.audit_original(FakePE())
        self.assertEqual(report["statadvisor_write_four_va"], "0x222C4D2B")
        self.assertEqual(report["original_assault_cue_writes"]["goal_BeginAssault_received_1"], "0x220CFFA0")
        self.assertFalse(report["generic_cue_setter_direct_e8_e9_candidates"])
        self.assertEqual(report["generic_cue_setter_literal_addresses_in_data"],
                         {".rdata": [], ".data": []})
        self.assertIn("not an established", report["conclusion"])

    def test_wrong_statadvisor_rtti_rejected(self):
        pe = FakePE()
        pe.memory[a.STAT_RTTI+8] = b".?AVCAI_AssaultBehavior@@\0"
        with self.assertRaisesRegex(a.VerificationError, "CStatAdvisor RTTI"):
            a.audit_original(pe)

    def test_wrong_vtable_col_owner_rejected(self):
        pe = FakePE()
        pe.memory[a.STAT_COL+12] = struct.pack("<I", 0x224E4C34)
        with self.assertRaisesRegex(a.VerificationError, "COL owner"):
            a.audit_original(pe)

    def test_changed_vtable_target_rejected(self):
        pe = FakePE()
        pe.memory[a.STAT_VTABLE_ENTRIES[0][0]] = struct.pack("<I", a.CUE_SETTER)
        with self.assertRaisesRegex(a.VerificationError, "method pointer"):
            a.audit_original(pe)

    def test_statadvisor_4_mutated_to_3_rejected(self):
        pe = FakePE()
        pe.memory[0x222C4D2B] = bytes.fromhex("c7 41 20 03 00 00 00")
        with self.assertRaisesRegex(a.VerificationError, "CStatAdvisor writes 4"):
            a.audit_original(pe)

    def test_begin_assault_1_mutated_to_4_rejected(self):
        pe = FakePE()
        pe.memory[0x220CFFA0] = bytes.fromhex("c7 41 20 04 00 00 00")
        with self.assertRaisesRegex(a.VerificationError, "goal BeginAssault"):
            a.audit_original(pe)

    def test_bad_generic_setter_field_rejected(self):
        pe = FakePE()
        pe.memory[0x220CE535] = bytes.fromhex("8b 44 24 04 89 41 24")
        with self.assertRaisesRegex(a.VerificationError, "generic cue helper"):
            a.audit_original(pe)

    def test_wrong_tail_jump_rejected(self):
        pe = FakePE()
        pe.memory[0x220CE547] = b"\xe9"+struct.pack("<i", 0x220397C0-0x220CE54C)
        with self.assertRaisesRegex(a.VerificationError, "tail"):
            a.audit_original(pe)

    def test_direct_call_to_setter_is_not_silently_ignored(self):
        pe = FakePE()
        site = 0x22001000+10
        transfer = b"\xe8"+struct.pack("<i", a.CUE_SETTER-site-5)
        pe.data = pe.data[:10]+transfer+pe.data[15:]
        with self.assertRaisesRegex(a.VerificationError, "direct CALL/JMP"):
            a.audit_original(pe)

    def test_literal_data_ptr_to_setter_is_not_silently_ignored(self):
        pe = FakePE()
        pe.data = pe.data[:100]+struct.pack("<I", a.CUE_SETTER)+pe.data[104:]
        with self.assertRaisesRegex(a.VerificationError, "literal data pointer"):
            a.audit_original(pe)

    def test_hash_failure_precedes_pe_parsing(self):
        with self.assertRaisesRegex(a.VerificationError, "SHA-256 mismatch"):
            a.audit(b"synthetic bytes instead of owner DLL")


if __name__ == "__main__":
    unittest.main()
