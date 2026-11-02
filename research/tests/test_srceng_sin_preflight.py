"""Original-free tests for srceng/SiN asset-header preflight."""
import json
from pathlib import Path
import struct
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "scripts"))
import srceng_sin_preflight as audit


def bsp(version=20, bad_lump=False):
    data = bytearray(audit.BSP_HEADER_SIZE + 24)
    data[:4] = b"VBSP"
    struct.pack_into("<I", data, 4, version)
    struct.pack_into("<IIII", data, 8, 9000 if bad_lump else
                     audit.BSP_HEADER_SIZE, 20, 0, 0)
    struct.pack_into("<I", data, 8 + 64 * 16, 3)
    return bytes(data)


def mdl(version=48):
    return b"IDST" + struct.pack("<I", version) + b"fake"


def vtf(major=7, minor=5):
    return b"VTF\x00" + struct.pack("<III", major, minor, 80) + bytes(64)


def vpk():
    return struct.pack("<III", 0x55AA1234, 1, 100)


def write(root, name, data):
    path = root / name
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(data)
    return path


class SrcengPreflightTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name) / "owned"
        self.root.mkdir()

    def scan(self, **kwargs):
        return audit.scan_owned_game(self.root, **kwargs)

    def test_bsp_valid_range_and_lump_bounds(self):
        p = write(self.root, "maps/scene.bsp", bsp())
        report = audit.bsp_header(p, p.stat().st_size)
        self.assertEqual(report["version"], 20)
        self.assertEqual(report["populated_lumps"], 1)
        self.assertEqual(report["out_of_bounds_lumps"], 0)
        self.assertEqual(report["map_revision"], 3)
        self.assertIn("not_proven_compatible", report["srceng_advertised_bsp_range"])

    def test_bsp_outside_advertised_range_is_not_a_definitive_failure(self):
        p = write(self.root, "maps/legacy.bsp", bsp(17))
        report = audit.bsp_header(p, p.stat().st_size)
        self.assertIn("not_proven_incompatible", report["srceng_advertised_bsp_range"])

    def test_bsp_invalid_lump_recorded_not_executed(self):
        p = write(self.root, "maps/bad_lump.bsp", bsp(bad_lump=True))
        self.assertEqual(audit.bsp_header(p, p.stat().st_size)["out_of_bounds_lumps"], 1)

    def test_short_or_invalid_bsp_header(self):
        p = write(self.root, "maps/broken.bsp", b"VBSP\x00")
        self.assertEqual(audit.bsp_header(p, 5)["status"], "truncated_header")
        p.write_bytes(b"ZZZZ" + bsp()[4:])
        self.assertEqual(audit.bsp_header(p, p.stat().st_size)["status"],
                         "unexpected_signature")

    def test_mdl_range_and_non_source_models(self):
        p = write(self.root, "models/test.mdl", mdl(47))
        self.assertEqual(audit.mdl_header(p, 12)["version"], 47)
        self.assertIn("not_proven_compatible", audit.mdl_header(p, 12)
                      ["srceng_advertised_mdl_range"])
        p.write_bytes(mdl(36))
        self.assertIn("not_proven_incompatible", audit.mdl_header(p, 12)
                      ["srceng_advertised_mdl_range"])
        p.write_bytes(b"fakeFILE")
        self.assertEqual(audit.mdl_header(p, 8)["status"], "unexpected_signature")

    def test_vtf_version_is_observed_without_compat_claim(self):
        p = write(self.root, "materials/brick.vtf", vtf())
        result = audit.vtf_header(p, p.stat().st_size)
        self.assertEqual(result["version"], "7.5")
        self.assertEqual(result["srceng_compatibility"], "unknown_requires_runtime_check")

    def test_vpk_directory_header(self):
        p = write(self.root, "assets_dir.vpk", vpk())
        result = audit.vpk_header(p, p.stat().st_size)
        self.assertEqual(result["version"], 1)
        self.assertEqual(result["declared_tree_size"], 100)

    def test_scan_reports_only_file_counts_and_header_samples(self):
        write(self.root, "gameinfo.txt", b'"GameInfo" { "SearchPaths" { "Game" "SINFOLDER" } }')
        write(self.root, "maps/scene.bsp", bsp())
        write(self.root, "models/test.mdl", mdl())
        write(self.root, "materials/brick.vtf", vtf())
        write(self.root, "materials/brick.vmt", b'"LightmappedGeneric" {}')
        write(self.root, "sound/hit.wav", b"RIFF")
        result = self.scan()
        self.assertEqual(result["counts"]["files_scanned"], 6)
        self.assertEqual(result["counts"]["resources_by_extension"][".bsp"], 1)
        self.assertEqual(result["counts"]["resources_by_extension"][".mdl"], 1)
        self.assertTrue(result["gameinfo"]["contains_searchpaths_token"])
        self.assertEqual(result["bsp_samples"][0]["relative_path"], "maps/scene.bsp")
        self.assertNotIn("SINFOLDER", json.dumps(result))

    def test_detect_case_collision_relevant_to_android_fs(self):
        write(self.root, "materials/Metal.vmt", b"one")
        write(self.root, "materials/metal.vmt", b"two")
        report = self.scan()
        self.assertEqual(report["counts"]["case_insensitive_path_collision_count_sampled"], 1)
        self.assertEqual(len(report["case_collision_samples"]), 1)

    def test_file_limit_truncation_is_explicit(self):
        for i in range(5):
            write(self.root, "maps/%02d.bsp" % i, bsp())
        result = self.scan(max_files=2)
        self.assertEqual(result["counts"]["files_scanned"], 2)
        self.assertTrue(result["counts"]["scan_truncated_at_limit"])

    def test_optional_srceng_layout_and_missing_gameinfo(self):
        srceng = Path(self.tmp.name) / "srceng"
        (srceng / "platform").mkdir(parents=True)
        (srceng / "hl2").mkdir()
        report = self.scan(srceng_root=srceng)
        self.assertTrue(report["srceng_layout"]["platform_directory_exists"])
        self.assertTrue(report["srceng_layout"]["hl2_directory_exists"])
        self.assertFalse(report["gameinfo"]["present"])

    def test_invalid_directory_and_file_limit_fail(self):
        with self.assertRaisesRegex(ValueError, "does not exist"):
            audit.scan_owned_game(self.root / "missing")
        with self.assertRaisesRegex(ValueError, "max_files"):
            self.scan(max_files=0)

    def test_json_cli_does_not_publish_file_bytes_or_private_abs_path(self):
        write(self.root, "maps/sinlevel.bsp", bsp())
        write(self.root, "gameinfo.txt", b'"GameInfo" { "Secret" "private-asset-content" }')
        output = Path(self.tmp.name) / "audit.json"
        self.assertEqual(audit.main([str(self.root), "-o", str(output)]), 0)
        text = output.read_text(encoding="utf-8")
        self.assertNotIn("private-asset-content", text)
        self.assertNotIn(str(self.root), text)
        report = json.loads(text)
        self.assertEqual(report["format_version"], 1)
        self.assertEqual(report["scope"],
                         "read_only_header_and_layout_preflight_no_engine_test")

    def test_error_on_symlinked_root_if_supported(self):
        link = Path(self.tmp.name) / "alias"
        try:
            link.symlink_to(self.root, target_is_directory=True)
        except (OSError, NotImplementedError):
            self.skipTest("symlinks unsupported")
        with self.assertRaisesRegex(ValueError, "symlink"):
            audit.scan_owned_game(link)


if __name__ == "__main__":
    unittest.main()
