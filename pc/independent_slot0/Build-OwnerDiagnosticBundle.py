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
ORIGINAL_HASHES = {
    "client.dll": "6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d",
    "server.dll": "568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650",
}

def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def stage(binaries, scripts, output, commit):
    if not re.fullmatch(r"[0-9a-f]{40}", commit):
        raise ValueError("Expected a complete checked-out commit SHA")
    if output.exists():
        raise FileExistsError(output)
    plan = [(binaries / name, pathlib.Path("bin") / name) for name in BINARIES]
    plan += [(scripts / name, pathlib.Path(name)) for name in SCRIPTS]
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
    args = parser.parse_args()
    if args.self_test:
        unittest.main(argv=["self-test"])
    else:
        if args.bin is None or args.output is None:
            parser.error("--bin and --output required")
        commit = subprocess.check_output(
            ["git", "rev-parse", "HEAD"], text=True).strip()
        stage(args.bin, pathlib.Path(__file__).resolve().parent,
              args.output, commit)

if __name__ == "__main__":
    main()
