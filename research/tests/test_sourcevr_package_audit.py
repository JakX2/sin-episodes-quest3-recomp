"""Original-asset-free tests for the read-only SourceVR package auditor."""
import json
from pathlib import Path
import sys
import tempfile
import unittest
from zipfile import ZipFile

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "scripts"))
import sourcevr_package_audit as audit


def elf64_arm():
    header = bytearray(64)
    header[:4] = b"\x7fELF"
    header[4] = 2
    header[5] = 1
    header[18:20] = (183).to_bytes(2, "little")
    return bytes(header)


def test_paths():
    hash_a = "a" * 64
    return {
        "assets/sourcevr/v1/native/hl2.manifest": b"hl2 native config\n",
        "assets/sourcevr/v1/native/hl2.load": b"load engine\n",
        "assets/sourcevr/v1/native/portal.manifest": b"portal native config\n",
        "assets/sourcevr/v1/vr/hl2vr.manifest": b"vr data\n",
        "assets/sourcevr/v1/overlay/hl2.manifest": b"overlay data\n",
        "assets/sourcevr/v1/blobs/sha256/" + hash_a: b"packed data",
        "assets/sourcevr/v1/native/blobs/" + hash_a + "/libserver.so": elf64_arm(),
        "lib/arm64-v8a/libsourcevrbootstrap.so": elf64_arm(),
        "lib/arm64-v8a/libhl2vrbridge.so": elf64_arm(),
    }


def write_tree(root, items):
    for name, content in items.items():
        target = root / name
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(content)


class SourceVrPackageAuditTests(unittest.TestCase):
    def test_read_extracted_tree_and_count_components(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            write_tree(root, test_paths())
            index = audit.PackageIndex(root)
            try:
                result = audit.analyze(index)
            finally:
                index.close()
            self.assertEqual(result["input_kind"], "extracted_directory")
            self.assertEqual(result["counts"]["files"], 9)
            self.assertEqual(result["counts"]["content_sha256_blobs"], 1)
            self.assertEqual(result["counts"]["native_library_variants"], 1)
            self.assertEqual(result["counts"]["distinct_native_library_names"], 1)
            self.assertEqual(result["native_libraries"][0]["name"], "libserver.so")
            self.assertEqual(result["native_libraries"][0]["architectures"], {"ELF64_AArch64": 1})
            self.assertEqual(len(result["android_arm64_libraries"]), 2)

    def test_game_entries_are_manifest_and_load_pairs_not_assumed_compatible(self):
        with tempfile.TemporaryDirectory() as tmp:
            write_tree(Path(tmp), test_paths())
            index = audit.PackageIndex(tmp)
            try:
                result = audit.analyze(index)
            finally:
                index.close()
            games = {entry["id"]: entry for entry in result["native_game_entries"]}
            self.assertEqual(games["hl2"]["native_manifest"], True)
            self.assertEqual(games["hl2"]["native_load"], True)
            self.assertEqual(games["portal"]["native_manifest"], True)
            self.assertEqual(games["portal"]["native_load"], False)
            self.assertNotIn("sin", games)
            self.assertTrue(any("No SiN-specific" in warning for warning in result["warnings"]))

    def test_zip_archive_is_inspected_in_place(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "sample.apk"
            with ZipFile(path, "w") as archive:
                for name, data in test_paths().items():
                    archive.writestr(name, data)
                archive.writestr("../cannot_extract", "bad path")
            index = audit.PackageIndex(path)
            try:
                result = audit.analyze(index)
            finally:
                index.close()
            self.assertEqual(result["input_kind"], "zip_or_apk")
            self.assertEqual(result["counts"]["files"], 9)
            self.assertEqual(result["android_arm64_libraries"][0]["architecture"], "ELF64_AArch64")
            self.assertFalse((Path(tmp).parent / "cannot_extract").exists())

    def test_txt_listing_is_useful_without_any_binary_access(self):
        with tempfile.TemporaryDirectory() as tmp:
            listing = Path(tmp) / "files.txt"
            listing.write_text(
                "\n".join(name.replace("/", "\\") for name in test_paths()) + "\n",
                encoding="utf-8",
            )
            index = audit.PackageIndex(listing)
            try:
                report = audit.analyze(index)
            finally:
                index.close()
            self.assertEqual(report["counts"]["files"], 9)
            self.assertEqual(report["input_kind"], "path_listing_only")
            self.assertEqual(
                report["native_libraries"][0]["architectures"],
                {"not_checked_listing_only": 1},
            )
            self.assertEqual(
                report["android_arm64_libraries"][0]["architecture"],
                "not_checked_listing_only",
            )
            self.assertEqual(
                report["metadata"][0]["kind"], "not_checked_listing_only"
            )

    def test_txt_listing_with_lengths_and_bom(self):
        with tempfile.TemporaryDirectory() as tmp:
            listing = Path(tmp) / "files.txt"
            listing.write_text(
                "100 bytes\tlib\\arm64-v8a\\libhl2vrbridge.so\n"
                "5 bytes\tassets\\sourcevr\\v1\\native\\hl2.load\n",
                encoding="utf-8-sig",
            )
            index = audit.PackageIndex(listing)
            self.assertEqual(len(index.entries), 2)
            self.assertIn("lib/arm64-v8a/libhl2vrbridge.so", index.entries)

    def test_text_vs_binary_metadata(self):
        self.assertEqual(audit.manifest_preview(b"hello\n")["kind"], "text")
        self.assertEqual(audit.manifest_preview(b"\x00\x01\xfa")["kind"], "binary")
        self.assertEqual(audit.manifest_preview(b"")["kind"], "empty")
        self.assertEqual(audit.manifest_preview(None)["kind"], "not_checked_listing_only")

    def test_elf_probe_cannot_derive_abi_from_names(self):
        self.assertEqual(audit.elf_identity(elf64_arm()), "ELF64_AArch64")
        self.assertEqual(audit.elf_identity(b"not an ELF"), "not_elf_or_unreadable")
        self.assertEqual(audit.elf_identity(None), "not_checked_listing_only")
        self.assertEqual(audit.elf_identity(b"\x7fELF"), "not_elf_or_unreadable")

    def test_prefix_read_is_bounded_even_for_large_files(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            metadata = root / "assets/sourcevr/v1/native/hl2.load"
            metadata.parent.mkdir(parents=True)
            metadata.write_bytes(b"A" * 20000)
            index = audit.PackageIndex(root)
            try:
                self.assertEqual(len(index.read_prefix("assets/sourcevr/v1/native/hl2.load", 999999)), 512)
                result = audit.analyze(index)
            finally:
                index.close()
            self.assertEqual(len(result["metadata"][0]["preview"]), 512)
            self.assertEqual(result["metadata"][0]["size_bytes"], 20000)

    def test_main_writes_json_report_without_copying_payloads(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp) / "sourcevr"
            root.mkdir()
            write_tree(root, test_paths())
            output = Path(tmp) / "report.json"
            result = audit.main([str(root), "--output", str(output)])
            self.assertEqual(result, 0)
            report = json.loads(output.read_text(encoding="utf-8"))
            self.assertEqual(report["format_version"], 1)
            self.assertNotIn("packed data", output.read_text(encoding="utf-8"))
            self.assertIn("unsupported", " ".join(report["warnings"]))


if __name__ == "__main__":
    unittest.main()
