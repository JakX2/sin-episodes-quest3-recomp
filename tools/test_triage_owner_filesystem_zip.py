"""Original-free tests for bounded owner filesystem report ZIP triage."""
import importlib.util
import pathlib
import tempfile
import unittest
import zipfile

SCRIPT = pathlib.Path(__file__).with_name("triage_owner_filesystem_zip.py")
spec = importlib.util.spec_from_file_location("owner_zip_triage", SCRIPT)
triage = importlib.util.module_from_spec(spec)
spec.loader.exec_module(triage)


class OwnerZipTriageTests(unittest.TestCase):
    def make_zip(self, entries):
        temp = tempfile.TemporaryDirectory()
        self.addCleanup(temp.cleanup)
        path = pathlib.Path(temp.name) / "result.zip"
        with zipfile.ZipFile(path, "w") as archive:
            for name, value in entries.items():
                archive.writestr(name, value)
        return path

    def test_empty_evidence_does_not_claim_boot(self):
        result = triage.inspect(self.make_zip({"SUMMARY.txt": "test started",
                                               "presence.txt": "not yet run"}))
        self.assertEqual(result["classification"], "insufficient_evidence")
        self.assertFalse(result["normal_game_boot_verified"])

    def test_presence_report_is_not_normal_boot(self):
        result = triage.inspect(self.make_zip({
            "SUMMARY.txt": "presence only",
            "presence.txt": "0x53494E68 both-present\nOWNER_INDEPENDENT_RESTORATION: VERIFIED_COMPLETE"}))
        self.assertEqual(result["classification"], "reported_presence_gate")
        self.assertFalse(result["original_method_engine_tested"])

    def test_read_report_requires_presence_and_restoration(self):
        result = triage.inspect(self.make_zip({
            "SUMMARY.txt": "read",
            "presence.txt": "0x53494E68 both-present\nOWNER_INDEPENDENT_RESTORATION: VERIFIED_COMPLETE",
            "read.txt": "0x53494E6B 62:8:156:66\nOWNER_INDEPENDENT_RESTORATION: VERIFIED_COMPLETE"}))
        self.assertEqual(result["classification"], "reported_read_gate")
        self.assertFalse(result["normal_game_boot_verified"])

    def test_read_without_presence_is_insufficient(self):
        result = triage.inspect(self.make_zip({
            "SUMMARY.txt": "read",
            "presence.txt": "missing",
            "read.txt": "0x53494E6B 62:8:156:66\nOWNER_INDEPENDENT_RESTORATION: VERIFIED_COMPLETE"}))
        self.assertEqual(result["classification"], "insufficient_evidence")

    def test_rejects_archive_traversal(self):
        with self.assertRaises(ValueError):
            triage.inspect(self.make_zip({"../presence.txt": "bad"}))

    def test_rejects_case_colliding_archive_names(self):
        with self.assertRaises(ValueError):
            triage.inspect(self.make_zip({"presence.txt": "first", "PRESENCE.TXT": "second"}))

    def test_rejects_symlink_member(self):
        temp = tempfile.TemporaryDirectory()
        self.addCleanup(temp.cleanup)
        path = pathlib.Path(temp.name) / "symlink.zip"
        info = zipfile.ZipInfo("presence.txt")
        info.create_system = 3
        info.external_attr = (0o120777 << 16)
        with zipfile.ZipFile(path, "w") as archive:
            archive.writestr(info, "target")
        with self.assertRaises(ValueError):
            triage.inspect(path)

    def test_rejects_binary_member(self):
        with self.assertRaises(ValueError):
            triage.inspect(self.make_zip({"client.dll": "not allowed"}))

    def test_rejects_oversized_member(self):
        with self.assertRaises(ValueError):
            triage.inspect(self.make_zip({"SUMMARY.txt": "x" * (triage.MAX_MEMBER + 1)}))


if __name__ == "__main__":
    unittest.main()
