"""Self-contained synthetic fixtures: verify probe safety only, not an engine."""

import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

SCRIPT = Path(__file__).resolve().parents[2] / "scripts" / "probe_engine_source.py"
SPEC = importlib.util.spec_from_file_location("probe_engine_source", SCRIPT)
probe = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(probe)


class SourceProbeTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name) / "engine"
        self.root.mkdir()
        self.git("init", "-q")
        self.git("config", "user.email", "test@example.invalid")
        self.git("config", "user.name", "Fixture")
        self.git("remote", "add", "origin", "https://github.com/example/source-fixture.git")
        (self.root / "LICENSE").write_text("Fixture license\n", encoding="utf-8")
        (self.root / "src").mkdir()
        (self.root / "src" / "engine.h").write_text("// Fixture, NOT Source\n", encoding="utf-8")
        (self.root / "src" / "server.cpp").write_text("// Fixture only\n", encoding="utf-8")
        self.git("add", ".")
        self.git("commit", "-qm", "fixture")
        files = ("LICENSE", "src/engine.h", "src/server.cpp")
        self.manifest = {
            "schema_version": 1,
            "upstream_url": "https://github.com/example/source-fixture",
            "commit": self.git("rev-parse", "HEAD"),
            "role": "reference-only",
            "license_path": "LICENSE",
            "required_files": {f: self.git("hash-object", f) for f in files},
        }
        self.path = Path(self.tmp.name) / "manifest.json"
        self.save()

    def git(self, *args):
        return subprocess.check_output(["git", "-C", str(self.root), *args], text=True).strip()

    def save(self):
        self.path.write_text(json.dumps(self.manifest), encoding="utf-8")

    def test_matches_pinned_git_checkout_but_never_claims_r1(self):
        result = probe.verify(self.path, self.root)
        self.assertTrue(result["source_snapshot_verified"])
        self.assertEqual(result["files_verified"], 3)
        for field in ("engine_adapter_compiled", "engine_module_loader_verified",
                      "sourcevr_compatibility_verified", "quest_runtime_verified", "r1_ready"):
            self.assertFalse(result[field])

    def test_cli_json_and_exit_code(self):
        p = subprocess.run([sys.executable, str(SCRIPT), "--manifest", str(self.path),
                            "--checkout", str(self.root)], capture_output=True, text=True)
        self.assertEqual(p.returncode, 0, p.stdout + p.stderr)
        self.assertFalse(json.loads(p.stdout)["r1_ready"])

    def test_wrong_commit_rejected(self):
        self.manifest["commit"] = "0" * 40
        self.save()
        with self.assertRaisesRegex(probe.ProbeError, "commit mismatch"):
            probe.verify(self.path, self.root)

    def test_wrong_repo_rejected(self):
        self.git("remote", "set-url", "origin", "https://github.com/stranger/repo")
        with self.assertRaisesRegex(probe.ProbeError, "origin repository mismatch"):
            probe.verify(self.path, self.root)

    def test_tampered_tracked_header_rejected(self):
        (self.root / "src" / "engine.h").write_text("uncommitted change\n")
        with self.assertRaisesRegex(probe.ProbeError, "modified/wrong file"):
            probe.verify(self.path, self.root)

    def test_missing_file_rejected(self):
        (self.root / "src" / "server.cpp").unlink()
        with self.assertRaisesRegex(probe.ProbeError, "required file missing"):
            probe.verify(self.path, self.root)

    def test_path_traversal_rejected(self):
        self.manifest["required_files"]["../bad"] = "0" * 40
        self.save()
        with self.assertRaisesRegex(probe.ProbeError, "unsafe manifest path"):
            probe.verify(self.path, self.root)

    def test_symlink_out_of_tree_rejected(self):
        outside = Path(self.tmp.name) / "outside"
        outside.write_text("external")
        (self.root / "src" / "outside").symlink_to(outside)
        self.manifest["required_files"]["src/outside"] = "0" * 40
        self.save()
        with self.assertRaisesRegex(probe.ProbeError, "path escapes checkout"):
            probe.verify(self.path, self.root)

    def test_license_required_in_hashed_set(self):
        del self.manifest["required_files"]["LICENSE"]
        self.save()
        with self.assertRaisesRegex(probe.ProbeError, "license_path"):
            probe.verify(self.path, self.root)

    def test_role_cannot_declare_runtime_ready(self):
        self.manifest["role"] = "quest-ready"
        self.save()
        with self.assertRaisesRegex(probe.ProbeError, "reference-only"):
            probe.verify(self.path, self.root)

    def test_non_github_upstream_rejected(self):
        self.manifest["upstream_url"] = "https://example.com/anything"
        self.save()
        with self.assertRaises(probe.ProbeError):
            probe.verify(self.path, self.root)

    def test_cli_failure_is_json_and_nonzero(self):
        self.manifest["commit"] = "0" * 40
        self.save()
        p = subprocess.run([sys.executable, str(SCRIPT), "--manifest", str(self.path),
                            "--checkout", str(self.root)], capture_output=True, text=True)
        self.assertEqual(p.returncode, 1)
        self.assertFalse(json.loads(p.stdout)["source_snapshot_verified"])


if __name__ == "__main__":
    unittest.main()
