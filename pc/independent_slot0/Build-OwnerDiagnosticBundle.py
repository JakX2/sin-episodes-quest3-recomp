"""Stage only independent test binaries and matching owner scripts."""
import argparse
import hashlib
import json
import pathlib
import re
import shutil
import subprocess
import tempfile
import unittest

BINARIES = ("client.dll", "server.dll")
SCRIPTS = (
    "Invoke-OwnerIndependentEngineProbe.ps1",
    "Get-OwnerEngineExitDiagnosis.ps1",
    "Get-OwnerIndependentStartupVerdict.ps1",
    "Run-ClientCvarDiagnostic.ps1",
    "OWNER_CLIENT_CVAR_BUNDLE.md",
)
# Separate explicit allowlist: do NOT expand the historical cvar bundle's
# fixed seven-entry manifest, which its own owner wrapper authenticates.
FS_SCRIPTS = (
    "Invoke-OwnerIndependentEngineProbe.ps1",
    "Get-OwnerEngineExitDiagnosis.ps1",
    "Get-OwnerIndependentStartupVerdict.ps1",
    "Invoke-OwnerFileSystemRetailGate.ps1",
    "OWNER_CLIENT_ENGINE_FILESYSTEM_V012.md",
)
# Human-facing ONE ZIP. All required independent binaries/scripts already
# inside, so the owner never has to move DLLs into a game folder manually.
CLIENT_STARTUP_SCRIPTS = (
    "Invoke-OwnerIndependentEngineProbe.ps1",
    "Get-OwnerEngineExitDiagnosis.ps1",
    "Get-OwnerIndependentStartupVerdict.ps1",
    "Run-OwnerClientStartupBoundary.ps1",
    "OWNER_CLIENT_STARTUP_BOUNDARY.md",
)
ONE_CLICK_SCRIPTS = FS_SCRIPTS + (
    "RUN_SIN_TEST.bat",
    "RUN_STAGED_STATS_TEST.bat",
    "RUN_STATS_HOOK_PROOF.bat",
    "RUN_SOUNDSCAPE_CATALOG_TEST.bat",
    "RUN_PHYSICS_READONLY_TEST.bat",
    "RUN_PHYSICS_MATERIALS_READONLY.bat",
    "RUN_SERVER_PHYSICS_READONLY.bat",
    "OWNER_SERVER_PHYSICS_ONECLICK_PHASE524.md",
    "OWNER_SURFACEPHYSICS_DATA_READONLY_PHASE522.md",
    "RUN_PHYSICS_READBACK_TEST.bat",
    "OWNER_PHYSICS_ONCE_CLICK_OWNER_RESULT_20261121.md",
    "OWNER_PHYSICS_READBACK_ACTUAL_WINDOWS_20261121.md",
    "Start-SiNFileSystemTest.ps1",
    "Recover-SiNRetainedTestCopy.ps1",
    "RECOVER_PREVIOUS_TEST.bat",
    "README_ONE_CLICK_FIRST.txt",
    "OWNER_ORIGINAL_STAGED_STATS_RETAIL_GATE_20261120.md",
    "OWNER_STATS_HOOK_PROOF_20261120.md",
    "OWNER_SOUNDSCAPE_RETAIL_GATE_20261120.md",
)
ORIGINAL_HASHES = {
    "client.dll": "6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d",
    "server.dll": "568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650",
}

def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def stage(binaries, scripts, output, commit, script_names=SCRIPTS):
    if not re.fullmatch(r"[0-9a-f]{40}", commit):
        raise ValueError("Expected a complete checked-out commit SHA")
    if output.exists():
        raise FileExistsError(output)
    plan = [(binaries / name, pathlib.Path("bin") / name) for name in BINARIES]
    plan += [(scripts / name, pathlib.Path(name)) for name in script_names]
    # Verify every input before creating the output directory.
    for source, relative in plan:
        if not source.is_file() or source.is_symlink():
            raise ValueError(f"Missing or linked input: {source.name}")
        if relative.parts[0] == "bin" and digest(source) in ORIGINAL_HASHES.values():
            raise ValueError("Refusing original retail DLL")
    output.mkdir(parents=True)
    try:
        files = {}
        for source, relative in plan:
            target = output / relative
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(source, target)
            files[relative.as_posix()] = digest(target)
        (output / "manifest.json").write_text(
            json.dumps({"schema": 1, "commit": commit, "files": files},
                       indent=2, sort_keys=True) + "\n", encoding="utf-8")
    except BaseException:
        shutil.rmtree(output)
        raise

class TestStage(unittest.TestCase):
    def test_filesystem_bundle_is_separate_and_original_free(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = pathlib.Path(temporary)
            binaries, scripts, output = (
                root / "binaries", root / "scripts", root / "filesystem")
            binaries.mkdir()
            scripts.mkdir()
            for name in BINARIES:
                (binaries / name).write_bytes(b"synthetic independent PE test")
            for name in FS_SCRIPTS:
                (scripts / name).write_text(
                    "synthetic script "+name, encoding="utf-8")
            (scripts / "Run-ClientCvarDiagnostic.ps1").write_text(
                "not part of filesystem bundle", encoding="utf-8")
            stage(binaries, scripts, output, "b"*40,FS_SCRIPTS)
            manifest = json.loads(
                (output/"manifest.json").read_text(encoding="utf-8"))
            required = {("bin/"+name) for name in BINARIES} | set(FS_SCRIPTS)
            self.assertEqual(set(manifest["files"]),required)
            self.assertEqual(manifest["commit"],"b"*40)
            self.assertNotIn("Run-ClientCvarDiagnostic.ps1",manifest["files"])
            self.assertNotIn("OWNER_CLIENT_CVAR_BUNDLE.md",manifest["files"])
            for name,expected in manifest["files"].items():
                self.assertEqual(digest(output/name),expected)

    def test_client_startup_bundle_is_exact_and_original_free(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = pathlib.Path(temporary)
            binaries, scripts, output = (
                root/"binaries", root/"scripts", root/"client-startup")
            binaries.mkdir()
            scripts.mkdir()
            for name in BINARIES:
                (binaries/name).write_bytes(b"synthetic independent PE32 DLL")
            for name in CLIENT_STARTUP_SCRIPTS:
                (scripts/name).write_text("synthetic "+name, encoding="utf-8")
            (scripts/"private-retail-dll.bin").write_bytes(b"never ship")
            stage(binaries, scripts, output, "d"*40, CLIENT_STARTUP_SCRIPTS)
            manifest=json.loads((output/"manifest.json").read_text(encoding="utf-8"))
            expected={"bin/"+name for name in BINARIES} | set(CLIENT_STARTUP_SCRIPTS)
            self.assertEqual(set(manifest["files"]), expected)
            self.assertEqual(manifest["commit"], "d"*40)
            self.assertNotIn("private-retail-dll.bin", manifest["files"])
            for name,expected_hash in manifest["files"].items():
                self.assertEqual(digest(output/name), expected_hash)

    def test_one_click_bundle_complete_and_distinct(self):
        with tempfile.TemporaryDirectory() as temporary:
            root=pathlib.Path(temporary)
            binaries=root/"binaries"
            scripts=root/"scripts"
            destination=root/"one-click"
            binaries.mkdir()
            scripts.mkdir()
            for name in BINARIES:
                (binaries/name).write_bytes(b"synthetic independent PE test")
            for name in ONE_CLICK_SCRIPTS:
                (scripts/name).write_text("test only: "+name, encoding="utf-8")
            (scripts/"UNRELATED_PRIVATE_RETAIL.dll").write_bytes(b"forbidden")
            stage(binaries,scripts,destination,"c"*40,ONE_CLICK_SCRIPTS)
            manifest=json.loads(
                (destination/"manifest.json").read_text(encoding="utf-8"))
            desired={"bin/"+name for name in BINARIES} | set(ONE_CLICK_SCRIPTS)
            self.assertEqual(set(manifest["files"]),desired)
            self.assertEqual(len(manifest["files"]),len(desired))
            self.assertEqual(manifest["commit"],"c"*40)
            self.assertTrue((destination/"RUN_SIN_TEST.bat").is_file())
            self.assertTrue((destination/"RUN_STAGED_STATS_TEST.bat").is_file())
            self.assertTrue((destination/"RUN_STATS_HOOK_PROOF.bat").is_file())
            self.assertTrue((destination/"RUN_SOUNDSCAPE_CATALOG_TEST.bat").is_file())
            self.assertTrue((destination/"RUN_PHYSICS_READONLY_TEST.bat").is_file())
            self.assertTrue((destination/"RUN_PHYSICS_MATERIALS_READONLY.bat").is_file())
            self.assertTrue((destination/"RUN_SERVER_PHYSICS_READONLY.bat").is_file())
            self.assertTrue((destination/"RUN_PHYSICS_READBACK_TEST.bat").is_file())
            self.assertTrue((destination/"OWNER_PHYSICS_READBACK_ACTUAL_WINDOWS_20261121.md").is_file())
            self.assertTrue((destination/"Start-SiNFileSystemTest.ps1").is_file())
            self.assertTrue((destination/"RECOVER_PREVIOUS_TEST.bat").is_file())
            self.assertTrue((destination/"Recover-SiNRetainedTestCopy.ps1").is_file())
            self.assertNotIn("UNRELATED_PRIVATE_RETAIL.dll",manifest["files"])
            self.assertNotIn("Run-ClientCvarDiagnostic.ps1",manifest["files"])
            for name,digest_string in manifest["files"].items():
                self.assertEqual(digest(destination/name),digest_string)

    def test_allowlist_and_manifest(self):
        with tempfile.TemporaryDirectory() as root:
            root = pathlib.Path(root)
            bins, scripts, output = (root / x for x in ("binaries", "scripts", "out"))
            bins.mkdir()
            scripts.mkdir()
            for name in BINARIES:
                (bins / name).write_bytes(name.encode())
            for name in SCRIPTS:
                (scripts / name).write_text(name, encoding="utf-8")
            (bins / "unrelated.dll").write_bytes(b"excluded")
            (scripts / "private-log.txt").write_bytes(b"excluded")
            (scripts / SCRIPTS[-1]).unlink()
            with self.assertRaises(ValueError):
                stage(bins, scripts, output, "a" * 40)
            self.assertFalse(output.exists())
            (scripts / SCRIPTS[-1]).write_bytes(b"doc")
            with self.assertRaises(ValueError):
                stage(bins, scripts, output, "bad")
            stage(bins, scripts, output, "a" * 40)
            manifest = json.loads((output / "manifest.json").read_text(encoding="utf-8"))
            self.assertEqual(manifest["commit"], "a" * 40)
            self.assertEqual(len(manifest["files"]), 7)
            self.assertEqual({p.relative_to(output).as_posix()
                              for p in output.rglob("*") if p.is_file()},
                             set(manifest["files"]) | {"manifest.json"})
            for name, expected in manifest["files"].items():
                self.assertEqual(digest(output / name), expected)
            with self.assertRaises(FileExistsError):
                stage(bins, scripts, output, "a" * 40)

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--bin", type=pathlib.Path)
    parser.add_argument("--output", type=pathlib.Path)
    parser.add_argument("--self-test", action="store_true")
    parser.add_argument("--filesystem", action="store_true",
                        help="build dedicated filesystem-presence/read bundle")
    parser.add_argument("--client-startup", action="store_true",
                        help="build client-only original-engine startup diagnostic bundle")
    parser.add_argument("--one-click", action="store_true",
                        help="build user-ready standalone Windows one-click ZIP contents")
    args = parser.parse_args()
    if args.self_test:
        unittest.main(argv=["self-test"])
    else:
        if args.bin is None or args.output is None:
            parser.error("--bin and --output required")
        if sum((args.one_click, args.filesystem, args.client_startup)) > 1:
            parser.error("--one-click, --filesystem and --client-startup are mutually exclusive")
        commit = subprocess.check_output(
            ["git", "rev-parse", "HEAD"], text=True).strip()
        stage(args.bin, pathlib.Path(__file__).resolve().parent,
              args.output, commit,
              CLIENT_STARTUP_SCRIPTS if args.client_startup else (
                  ONE_CLICK_SCRIPTS if args.one_click else (
                      FS_SCRIPTS if args.filesystem else SCRIPTS)))

if __name__ == "__main__":
    main()
