import hashlib
import importlib.util
import struct
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

HERE = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location("auth", HERE / "verify_original_batch.py")
auth = importlib.util.module_from_spec(spec)
spec.loader.exec_module(auth)


def fake_pe(machine=0x14c, magic=0x10b, flags=0x2000, offset=0x80):
    data = bytearray(512)
    data[:2] = b"MZ"
    struct.pack_into("<I", data, 0x3c, offset)
    data[offset:offset+4] = bytes((80, 69, 0, 0))
    struct.pack_into("<HH", data, offset+4, machine, 3)
    struct.pack_into("<HH", data, offset+20, 224, flags)
    struct.pack_into("<H", data, offset+24, magic)
    return bytes(data)


class AuthenticationTests(unittest.TestCase):
    def check(self, content, expected_error=None):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "client.dll"
            path.write_bytes(content)
            digest = hashlib.sha256(content).hexdigest()
            if expected_error:
                with self.assertRaisesRegex(ValueError, expected_error):
                    auth.verify(path, digest)
            else:
                self.assertEqual(auth.verify(path, digest)["format"], "PE32")

    def test_campaign_trigger_evidence_mutations(self):
        trigger_spec = importlib.util.spec_from_file_location(
            "trigger_audit", HERE / "audit_sin_original_gameplay_triggers.py")
        trigger_audit = importlib.util.module_from_spec(trigger_spec)
        trigger_spec.loader.exec_module(trigger_audit)
        trigger_audit.self_test()

    def test_valid(self):
        self.check(fake_pe())

    def test_reject_bad_digest(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "client.dll"
            path.write_bytes(fake_pe())
            with self.assertRaisesRegex(ValueError, "SHA256 mismatch"):
                auth.verify(path, "0" * 64)

    def test_wrong_machine(self):
        self.check(fake_pe(machine=0x8664), "PE32 i386")

    def test_pe32_plus(self):
        self.check(fake_pe(magic=0x20b), "PE32 i386")

    def test_executable_not_dll(self):
        self.check(fake_pe(flags=2), "PE32 i386")

    def test_missing_mz(self):
        self.check(b"broken", "DOS header")

    def test_missing_pe(self):
        self.check(fake_pe(offset=0x80)[:128] + b"NOPE" + fake_pe()[132:], "PE signature")

    def test_both_required(self):
        with tempfile.TemporaryDirectory() as tmp:
            with self.assertRaises(FileNotFoundError):
                auth.authenticate(Path(tmp))

    def test_full_file_hash(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "client.dll"
            original = fake_pe() + b"tail"
            path.write_bytes(original)
            with self.assertRaisesRegex(ValueError, "SHA256 mismatch"):
                auth.verify(path, hashlib.sha256(fake_pe()).hexdigest())

    def test_all_or_nothing(self):
        with tempfile.TemporaryDirectory() as tmp:
            p = Path(tmp)
            client = fake_pe()
            (p / "client.dll").write_bytes(client)
            with patch.dict(auth.EXPECTED, {"client.dll": hashlib.sha256(client).hexdigest()}, clear=False):
                with self.assertRaises(FileNotFoundError):
                    auth.authenticate(p)


if __name__ == "__main__":
    unittest.main()
