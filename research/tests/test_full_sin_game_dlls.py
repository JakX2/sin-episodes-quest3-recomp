"""Synthetic validation of exhaustive Source x86 registration-callshape finder."""
import pathlib
import struct
import sys
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1] / "tools"))
from audit_full_sin_game_dlls import (
    VerificationError, inspect_weapon_scripts, rtti_names,
    scan_registrations, report,
)
from test_gameplay_entity_registry import fake_registered_server, FACTORIES, PEImage


class FullRegistryTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.fake = fake_registered_server()

    def test_original_32_selected_entries_appear_from_auto_discovery(self):
        rows = scan_registrations(self.fake, check_hash=False)
        expected = {x[0] for x in FACTORIES}
        self.assertEqual(len(rows), 32)
        self.assertEqual({x["classname"] for x in rows}, expected)
        self.assertEqual(len({x["factory_object_va"] for x in rows}), 32)

    def test_wrong_registry_getter_rejected_by_scanner(self):
        b = bytearray(self.fake)
        pe = PEImage(b)
        first = FACTORIES[0][2]
        b[pe.offset(first + 1, 1)] ^= 0x7f
        rows = scan_registrations(bytes(b), check_hash=False, verify_subset=False)
        self.assertEqual(len(rows), 31)
        with self.assertRaisesRegex(VerificationError, "cross-check"):
            scan_registrations(bytes(b), check_hash=False)

    def test_bad_factory_section_and_malformed_name_rejected(self):
        b = bytearray(self.fake)
        pe = PEImage(b)
        first, second = FACTORIES[0], FACTORIES[1]
        struct.pack_into("<I", b, pe.offset(first[2] + 13, 4), 0x22001000)
        b[pe.offset(second[3], 1)] = ord("!")
        rows = scan_registrations(bytes(b), check_hash=False, verify_subset=False)
        self.assertEqual(len(rows), 30)

    def test_wrong_original_hash_rejected(self):
        with self.assertRaisesRegex(VerificationError, "SHA-256"):
            scan_registrations(self.fake)

    def test_rtti_requires_valid_msvc_terminator(self):
        blob = (b"junk.?AVCNPC_SintekGrunt@@\x00"
                b".?AUCWeaponMagnum@@\x00"
                b".?AVCorrupt@@notterminated")
        self.assertEqual(rtti_names(blob), ["CNPC_SintekGrunt", "CWeaponMagnum"])

    def test_weapon_script_crosswalk(self):
        import io
        import zipfile
        dst = io.BytesIO()
        with zipfile.ZipFile(dst, "w") as z:
            z.writestr("scripts/weapon_magnum.txt", "x")
            z.writestr("scripts/weapon_rocket_launcher.txt", "x")
            z.writestr("scripts/other.txt", "x")
        rows = inspect_weapon_scripts(dst.getvalue(), {"weapon_magnum", "weapon_smg"})
        self.assertEqual(rows["script_only_not_in_this_factory_callshape"],
                         ["weapon_rocket_launcher"])
        self.assertEqual(rows["registered_without_named_script"], ["weapon_smg"])

    def test_metadata_only_report(self):
        x = report(self.fake, b".?AVC_WeaponMagnum@@\x00",
                   check_hash=False)
        self.assertEqual(x["registry"]["matching_callshape_entries"], 32)
        self.assertEqual(x["rtti"]["client_count"], 1)
        self.assertEqual(x["rtti"]["server_count"], 0)


if __name__ == "__main__":
    unittest.main()
