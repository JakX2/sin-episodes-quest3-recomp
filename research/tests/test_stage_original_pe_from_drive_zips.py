"""Synthetic ZIP inputs only; NEVER put retail game binaries in CI."""
from __future__ import annotations
import hashlib
import sys
import tempfile
import unittest
import zipfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import stage_original_pe_from_drive_zips as secure


class OriginalDriveStagingTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.dlls = {
            "client.dll": b"MZsynthetic-client-only-for-test",
            "server.dll": b"MZsynthetic-server-only-for-test",
        }
        self.expected = {
            name: (len(payload), hashlib.sha256(payload).hexdigest())
            for name, payload in self.dlls.items()
        }

    def archive(self, name: str, members: dict[str, bytes]) -> Path:
        output = self.root / name
        with zipfile.ZipFile(output, "w") as writer:
            for member, content in members.items():
                writer.writestr(member, content)
        return output

    def test_split_zip_original_dlls_at_arbitrary_nested_paths(self):
        first = self.archive("bin.zip", {
            "bin/subfolder/CLIENT.DLL": self.dlls["client.dll"],
            "misc/readme.txt": b"other data",
        })
        second = self.archive("bin 1.zip", {
            "folder/bin/server.dll": self.dlls["server.dll"],
        })
        out = self.root / "private"
        summary = secure.stage([first, second], out, self.expected)
        self.assertEqual(set(summary), {"client.dll", "server.dll"})
        for name, original in self.dlls.items():
            self.assertEqual((out / name).read_bytes(), original)
            self.assertEqual(summary[name]["sha256"],
                             hashlib.sha256(original).hexdigest())
        self.assertEqual(out.stat().st_mode & 0o777, 0o700)
        with self.assertRaises(secure.OriginalBinaryError):
            secure.stage([first, second], out, self.expected)

    def test_wrong_original_SHA_never_creates_output(self):
        first = self.archive("wrong.zip", {
            "client.dll": b"MZdifferent-client-original",
        })
        second = self.archive("server.zip", {
            "server.dll": self.dlls["server.dll"],
        })
        out = self.root / "private"
        with self.assertRaisesRegex(secure.OriginalBinaryError,
                                    "SHA256 or size mismatch"):
            secure.stage([first, second], out, self.expected)
        self.assertFalse(out.exists())

    def test_duplicate_zip_members_refused(self):
        first = self.archive("one.zip", {"client.dll": self.dlls["client.dll"]})
        second = self.archive("two.zip", {
            "client.dll": self.dlls["client.dll"],
            "server.dll": self.dlls["server.dll"],
        })
        with self.assertRaisesRegex(secure.OriginalBinaryError, "Duplicate"):
            secure.find_originals([first, second], self.expected)

    def test_missing_original_refused(self):
        first = self.archive("one.zip", {"client.dll": self.dlls["client.dll"]})
        with self.assertRaisesRegex(secure.OriginalBinaryError, "Missing"):
            secure.find_originals([first], self.expected)

    def test_html_drive_login_page_refused(self):
        html = self.root / "instead-of-zip.zip"
        html.write_text("<html>Google sign-in required</html>")
        with self.assertRaisesRegex(secure.OriginalBinaryError, "Drive download"):
            secure.find_originals([html], self.expected)

    def test_archive_path_does_not_escape_staging_dir(self):
        a = self.archive("odd.zip", {
            "../../../../client.dll": self.dlls["client.dll"],
            "server.dll": self.dlls["server.dll"],
        })
        out = self.root / "private"
        secure.stage([a], out, self.expected)
        self.assertFalse((self.root / "client.dll").exists())
        self.assertTrue((out / "client.dll").exists())

    def test_default_exact_original_SHA_fingerprints_are_pinned(self):
        self.assertEqual(secure.EXPECTED["client.dll"], (
            3592192,
            "6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d",
        ))
        self.assertEqual(secure.EXPECTED["server.dll"], (
            6049792,
            "568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650",
        ))
        self.assertLess(secure.EXPECTED["server.dll"][0],
                        secure.MAX_MEMBER_BYTES)


if __name__ == "__main__":
    unittest.main()
