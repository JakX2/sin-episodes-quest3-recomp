"""Synthetic runner contract tests: no original game binaries or Ghidra install."""
from __future__ import annotations

import json
import os
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

RUNNER = Path(__file__).resolve().parents[1] / "tools" / "run_ghidra_original.sh"
REPO = RUNNER.parents[2]


class RunnerContracts(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.ghidra = self.root / "fake-ghidra"
        (self.ghidra / "support").mkdir(parents=True)
        self.originals = self.root / "bin"
        self.originals.mkdir()
        self.metadata = self.root / "metadata"
        self.private = self.root / "private"
        self.seeds = self.root / "reviewed-seeds"
        self.seeds.mkdir()
        self.vtable_seeds = self.root / "reviewed-vtable-seeds"
        self.vtable_seeds.mkdir()
        self.argv_file = self.root / "invocation.json"
        for module in ("client.dll", "server.dll"):
            (self.originals / module).write_bytes(b"synthetic only")
            (self.seeds / (module + "_factory_seeds.csv")).write_text(
                "entry,interface,source\n1000,SyntheticEngine001,reviewed_original_static_registration\n",
                encoding="utf-8")
            (self.vtable_seeds / (module + "_vtable_method_seeds.csv")).write_text(
                "entry,interface,slot,source\n1000,SyntheticVtable001,0,sha256_authenticated_original_pe32_vtable_prefix\n",
                encoding="utf-8")
        fake = self.ghidra / "support" / "analyzeHeadless"
        fake.write_text(
            "#!/usr/bin/env python3\n"
            "import json, os, sys\n"
            "from pathlib import Path\n"
            "Path(os.environ['TEST_CAPTURE_ARGV']).write_text(json.dumps(sys.argv[1:]))\n"
            "for mod in ('client.dll', 'server.dll'):\n"
            "    Path(os.environ['TEST_METADATA_DIR'],mod+'_factory_seed_results.csv').write_text('entry,interface,outcome,result_function_entry\\n1000,SyntheticEngine001,already_inferred,1000\\n')\n"
            "    if os.getenv('SIN_GHIDRA_VTABLE_SEED_DIRECTORY'):\n"
            "        Path(os.environ['TEST_METADATA_DIR'],mod+'_vtable_seed_results.csv').write_text('entry,interface,slot,outcome,result_function_entry\\n1000,SyntheticVtable001,0,already_inferred,1000\\n')\n"
            "    if os.getenv('SIN_GHIDRA_TARGETED_PRIVATE_DIRECTORY'):\n"
            "        Path(os.environ['SIN_GHIDRA_TARGETED_PRIVATE_DIRECTORY'],mod+'_targeted_decomp_status.csv').write_text('interface,entry,outcome,inferred_function_bytes\\nSyntheticEngine001,0x00001000,decompiled_candidate,22\\n')\n"
            "        if os.getenv('SIN_GHIDRA_VTABLE_SEED_DIRECTORY'):\n"
            "            Path(os.environ['SIN_GHIDRA_TARGETED_PRIVATE_DIRECTORY'],mod+'_vtable_targeted_decomp_status.csv').write_text('interface,slot,entry,outcome,inferred_function_bytes\\nSyntheticVtable001,0,0x00001000,decompiled_candidate,22\\n')\n",
            "            Path(os.environ['SIN_GHIDRA_TARGETED_PRIVATE_DIRECTORY'],mod+'_startup_factory_census.csv').write_text('interface,slot,entry,sdk_method_hypothesis,outcome,inferred_body_bytes,instruction_count,direct_call_sites,indirect_call_sites,interface_literal_count,interface_version_literals\\nSyntheticVtable001,0,0x00001000,SyntheticTestOnly,bounded_inferred_function,22,3,1,0,0,\\n')\\n",
            encoding="utf-8",
        )
        fake.chmod(0o755)

    def execute(self, with_private=True, ack="1", private_dir=None,
                with_vtable=False):
        env = os.environ.copy()
        env.update({
            "TEST_CAPTURE_ARGV": str(self.argv_file),
            "TEST_METADATA_DIR": str(self.metadata),
            "SIN_GHIDRA_SEED_DIRECTORY": str(self.seeds),
            "SIN_GHIDRA_VTABLE_SEED_DIRECTORY": str(self.vtable_seeds) if with_vtable else "",
            "SIN_PRIVATE_DECOMP_ACK": ack,
            "SIN_GHIDRA_TARGETED_PRIVATE_DIRECTORY": (
                str(private_dir if private_dir is not None else self.private)
                if with_private else ""),
        })
        return subprocess.run([
            "bash", str(RUNNER), str(self.ghidra), str(self.originals),
            str(self.metadata)
        ], env=env, capture_output=True, text=True)

    def test_private_mode_forwards_two_postscripts_without_fourth_positional_arg(self):
        run = self.execute()
        self.assertEqual(run.returncode, 0, run.stderr)
        args = json.loads(self.argv_file.read_text(encoding="utf-8"))
        self.assertEqual(args.count("-postScript"), 3)
        self.assertIn("SeedReviewedInterfaceFactories.java", args)
        self.assertIn("ExportTargetedInterfacePseudocode.java", args)
        self.assertIn("ExportOriginalFunctionAtlas.java", args)
        self.assertNotIn(str(self.seeds), args[:6])
        self.assertTrue((self.private / "client.dll_targeted_decomp_status.csv").exists())

    def test_reviewed_vtable_adds_bounded_private_decomp_postscript(self):
        run = self.execute(with_vtable=True)
        self.assertEqual(run.returncode, 0, run.stderr)
        args = json.loads(self.argv_file.read_text(encoding="utf-8"))
        self.assertEqual(args.count("-postScript"), 6)
        self.assertIn("SeedAuthenticatedVtablePrefixMethods.java", args)
        self.assertIn("ExportTargetedVtableMethodPseudocode.java", args)
        self.assertIn("ExportStartupFactoryDependencyCensus.java", args)
        self.assertLess(args.index("SeedAuthenticatedVtablePrefixMethods.java"),
                        args.index("ExportTargetedVtableMethodPseudocode.java"))
        self.assertTrue((self.private / "client.dll_vtable_targeted_decomp_status.csv").exists())
        self.assertTrue((self.private / "client.dll_startup_factory_census.csv").exists())
        self.assertTrue((self.metadata / "client.dll_vtable_seed_results.csv").exists())
        self.assertNotIn(str(self.vtable_seeds), args[:6])

    def test_vtable_seed_without_private_output_writes_only_metadata(self):
        run = self.execute(with_private=False, with_vtable=True)
        self.assertEqual(run.returncode, 0, run.stderr)
        args = json.loads(self.argv_file.read_text(encoding="utf-8"))
        self.assertIn("SeedAuthenticatedVtablePrefixMethods.java", args)
        self.assertNotIn("ExportTargetedVtableMethodPseudocode.java", args)
        self.assertFalse(self.private.exists())

    def test_metadata_mode_does_not_request_any_c_decomp(self):
        run = self.execute(with_private=False)
        self.assertEqual(run.returncode, 0, run.stderr)
        args = json.loads(self.argv_file.read_text(encoding="utf-8"))
        self.assertIn("SeedReviewedInterfaceFactories.java", args)
        self.assertNotIn("ExportTargetedInterfacePseudocode.java", args)

    def test_missing_ack_fails_without_invoking_ghidra(self):
        run = self.execute(ack="")
        self.assertNotEqual(run.returncode, 0)
        self.assertIn("requires reviewed seeds", run.stderr)
        self.assertFalse(self.argv_file.exists())

    def test_private_output_inside_repo_is_rejected(self):
        run = self.execute(private_dir=REPO / "research" / "ghidra")
        self.assertNotEqual(run.returncode, 0)
        self.assertIn("Refusing original pseudocode output", run.stderr)
        self.assertFalse(self.argv_file.exists())

    def test_private_output_inside_metadata_is_rejected(self):
        run = self.execute(private_dir=self.metadata / "private")
        self.assertNotEqual(run.returncode, 0)
        self.assertIn("Refusing original pseudocode output", run.stderr)
        self.assertFalse(self.argv_file.exists())


if __name__ == "__main__":
    unittest.main()
