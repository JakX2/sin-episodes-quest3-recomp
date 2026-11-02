import hashlib
import pathlib
import sys
import tempfile
import unittest
import zipfile

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from sourcevr_extracted_apk import inventory, reconstruct_unsigned
from sourcevr_isolated_apk_builder import build


class SourceVRExtractedFolderTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = pathlib.Path(self.tmp.name) / "original_sourcevr_extracted"
        self.root.mkdir()
        fixtures = {
            "AndroidManifest.xml": b"\x03\x00binary-xml-fixture",
            "resources.arsc": b"\x02\x00binary-resource-fixture",
            "classes.dex": b"dex\n039\x00fixture",
            "classes2.dex": b"dex\n039\x00fixture2",
            "lib/arm64-v8a/libsourcevrbootstrap.so": b"arm64-bootstrap-fixture",
            "lib/arm64-v8a/libhl2vrbridge.so": b"arm64-vrbridge-fixture",
            "META-INF/CERT.RSA": b"old-invalid-signature",
            "META-INF/CERT.SF": b"old-invalid-sf",
            "META-INF/NOTICE": b"non-signature-jar-metadata",
        }
        for i in range(24):
            fixtures[f"assets/sourcevr/v1/blobs/sha256/{i:064x}"] = f"blob_{i}".encode()
        for name, data in fixtures.items():
            f = self.root / name
            f.parent.mkdir(parents=True, exist_ok=True)
            f.write_bytes(data)

    def test_inventory_and_preserve_all_non_signatures(self):
        result = inventory(self.root, pin_dex=False)
        self.assertEqual(result.dex_files, ("classes.dex", "classes2.dex"))
        self.assertEqual(len(result.files), 31)  # 33 input files, 2 signatures
        self.assertIn(("META-INF/NOTICE", self.root / "META-INF/NOTICE"), result.files)
        self.assertEqual(result.dex_sha256,
                         hashlib.sha256((self.root / "classes.dex").read_bytes()).hexdigest())
        self.assertGreater(result.bytes_total, 100)

    def test_ignore_developer_helpers_outside_original_apk_layout(self):
        helper = self.root / "tools/sourcevr_extracted_apk.py"
        helper.parent.mkdir(parents=True)
        helper.write_text("# helper from GitHub\\n", encoding="utf-8")
        data = inventory(self.root, pin_dex=False)
        self.assertIn("tools/sourcevr_extracted_apk.py", data.ignored_files)
        self.assertNotIn(("tools/sourcevr_extracted_apk.py", helper), data.files)
        self.assertEqual(len(data.files), 31)

    def test_reconstruct_integrity_and_immutability(self):
        source = inventory(self.root, pin_dex=False)
        target = pathlib.Path(self.tmp.name) / "unsigned.apk"
        meta = reconstruct_unsigned(source, target)
        self.assertEqual(meta["entries"], len(source.files))
        with zipfile.ZipFile(target) as z:
            self.assertEqual(set(z.namelist()), {n for n, _ in source.files})
            self.assertNotIn("META-INF/CERT.RSA", z.namelist())
            self.assertEqual(z.read("classes.dex"), (self.root / "classes.dex").read_bytes())
        with self.assertRaisesRegex(ValueError, "refusing to overwrite"):
            reconstruct_unsigned(source, target)
        self.assertTrue((self.root / "META-INF/CERT.SF").is_file())

    def test_detect_missing_or_misnumbered_dex(self):
        (self.root / "classes2.dex").rename(self.root / "classes3.dex")
        with self.assertRaisesRegex(ValueError, "multidex"):
            inventory(self.root, pin_dex=False)

    def test_reject_bad_manifest_and_missing_library(self):
        (self.root / "AndroidManifest.xml").write_text("<manifest/>")
        with self.assertRaisesRegex(ValueError, "compiled binary XML"):
            inventory(self.root, pin_dex=False)
        (self.root / "AndroidManifest.xml").write_bytes(b"\x03\x00binary")
        (self.root / "lib/arm64-v8a/libsourcevrbootstrap.so").unlink()
        with self.assertRaisesRegex(ValueError, "missing"):
            inventory(self.root, pin_dex=False)

    def test_refuse_symlink(self):
        self.root.joinpath("indirect-link").symlink_to(self.root / "classes.dex")
        with self.assertRaisesRegex(ValueError, "symlink"):
            inventory(self.root, pin_dex=False)

    def test_original_dex_pinning_refuses_fixture(self):
        with self.assertRaisesRegex(ValueError, "unrecognized SourceVR"):
            inventory(self.root)

    def test_builder_refuses_unpinned_source_without_output(self):
        out = pathlib.Path(self.tmp.name) / "private-debug-build"
        probe = pathlib.Path(self.tmp.name) / "probe.so"
        probe.write_bytes(b"fake-not-an-ELF")
        with self.assertRaisesRegex(ValueError, "unrecognized SourceVR"):
            build(None, probe, out, "com.sourcevrport.hl2vr.sinprobe",
                  "apktool", "zipalign", "apksigner", "keytool",
                  extracted_dir=self.root)
        self.assertFalse(out.exists())


if __name__ == "__main__":
    unittest.main()
