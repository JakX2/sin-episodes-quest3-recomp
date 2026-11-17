"""Protect the original-SiN Actions job from invalid runner CLI and unsafe output.

CI pull_request runs only the synthetic Ghidra job, so a malformed owner-
original shell invocation could otherwise pass checks until after merge.
These tests are static workflow contract checks, not retail-game testing.
"""
from __future__ import annotations

import shlex
import unittest
from pathlib import Path

WORKFLOW = Path(__file__).resolve().parents[2] / ".github" / "workflows" / "sin-ghidra-headless.yml"

OWNER_JOB = "  analyze-owner-originals:"
RUN_STEP = "      - name: Run real Ghidra headless batch analysis on both owner-original DLLs"
NEXT_STEP = "      - name: Separate Ghidra inferred functions from SHA-reviewed methods"
CLEANUP_STEP = "      - name: Remove private originals and generated analysis from runner scratch"

EXPECTED_ARGUMENTS = [
    "$GHIDRA_HOME",
    "$RUNNER_TEMP/sin-original-pe-private",
    "$RUNNER_TEMP/sin-ghidra-metadata",
]
CLEANUP_PATHS = [
    "$RUNNER_TEMP/sin-original-drive-private",
    "$RUNNER_TEMP/sin-original-pe-private",
    "$RUNNER_TEMP/sin-ghidra-metadata",
    "$RUNNER_TEMP/sin-reviewed-factory-seeds",
    "$RUNNER_TEMP/sin-ghidra-private",
]


def between(text, start, end=None):
    if text.count(start) != 1:
        raise ValueError(f"missing or repeated workflow marker: {start}")
    fragment = text.split(start, 1)[1]
    if end is not None:
        if end not in fragment:
            raise ValueError(f"missing workflow marker: {end}")
        fragment = fragment.split(end, 1)[0]
    return fragment


def validate_owner_job(workflow):
    owner = between(workflow, OWNER_JOB)
    if not all(value in owner for value in (
        "github.event_name == 'schedule'",
        "github.event_name == 'workflow_dispatch'",
        "github.ref ==",
        "stage_original_pe_from_drive_zips.py",
    )):
        raise ValueError("original analysis must be main-only and SHA256 authenticated")

    run = between(owner, RUN_STEP, NEXT_STEP)
    lines = run.splitlines()
    start = next((i for i, line in enumerate(lines)
                  if "bash research/tools/run_ghidra_original.sh" in line), None)
    if start is None:
        raise ValueError("original Ghidra runner invocation missing")
    command_lines = []
    index = start
    while index < len(lines):
        line = lines[index].strip()
        continued = line.endswith("\\")
        command_lines.append(line[:-1] if continued else line)
        index += 1
        if not continued:
            break
    arguments = shlex.split(" ".join(command_lines))
    if arguments[:2] != ["bash", "research/tools/run_ghidra_original.sh"]:
        raise ValueError("unexpected Ghidra runner executable")
    if arguments[2:] != EXPECTED_ARGUMENTS:
        raise ValueError(f"original Ghidra runner positional argument mismatch: {arguments[2:]}")
    # The runner obtains seeds, private decomp and xref mode exclusively
    # from exported environment variables, never positional CLI arguments.
    for setting in (
        'echo "SIN_GHIDRA_SEED_DIRECTORY=$RUNNER_TEMP/sin-reviewed-factory-seeds" >> "$GITHUB_ENV"',
        'echo "SIN_GHIDRA_FACTORY_XREFS=1" >> "$GITHUB_ENV"',
        'echo "SIN_GHIDRA_TARGETED_PRIVATE_DIRECTORY=$RUNNER_TEMP/sin-ghidra-private" >> "$GITHUB_ENV"',
        'echo "SIN_PRIVATE_DECOMP_ACK=1" >> "$GITHUB_ENV"',
    ):
        if setting not in owner:
            raise ValueError(f"missing owner Ghidra environment setting: {setting}")

    upload = between(owner, "      - name: Upload metadata only (NEVER game binaries or pseudocode)", CLEANUP_STEP)
    if not all(item in upload for item in (
        "actions/upload-artifact@v4",
        "${{ runner.temp }}/sin-ghidra-metadata/*.csv",
        "${{ runner.temp }}/sin-ghidra-metadata/*.json",
    )):
        raise ValueError("metadata upload contract changed")
    if any(item in upload for item in (
        "sin-ghidra-private", "sin-original-pe-private",
        "sin-reviewed-factory-seeds", "PRIVATE_TARGETED_DECOMPILATION",
    )):
        raise ValueError("owner private files included in artifact upload")

    cleanup = between(owner, CLEANUP_STEP)
    if "if: always()" not in cleanup:
        raise ValueError("private data cleanup must always run")
    for path in CLEANUP_PATHS:
        if cleanup.count('"' + path + '"') != 1:
            raise ValueError(f"missing or duplicated cleanup path: {path}")
    return arguments


class OwnerGhidraWorkflowContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.source = WORKFLOW.read_text(encoding="utf-8")

    def test_owner_workflow_matches_runner_three_arg_mode(self):
        args = validate_owner_job(self.source)
        self.assertEqual(len(args), 5)
        self.assertEqual(args[2:], EXPECTED_ARGUMENTS)

    def test_regression_fourth_argument_as_seed_dir_rejected(self):
        baseline = '"$RUNNER_TEMP/sin-original-pe-private" "$RUNNER_TEMP/sin-ghidra-metadata"'
        self.assertIn(baseline, self.source)
        mutated = self.source.replace(
            baseline,
            baseline + ' \\\n            "$RUNNER_TEMP/sin-reviewed-factory-seeds"',
            1,
        )
        with self.assertRaisesRegex(ValueError, "positional argument mismatch"):
            validate_owner_job(mutated)

    def test_regression_fifth_argument_as_private_dir_rejected(self):
        baseline = '"$RUNNER_TEMP/sin-original-pe-private" "$RUNNER_TEMP/sin-ghidra-metadata"'
        mutated = self.source.replace(
            baseline,
            baseline + ' \\\n            "$RUNNER_TEMP/sin-reviewed-factory-seeds" \\\n'
                       '            "$RUNNER_TEMP/sin-ghidra-private"',
            1,
        )
        with self.assertRaisesRegex(ValueError, "positional argument mismatch"):
            validate_owner_job(mutated)

    def test_regression_missing_private_cleanup_rejected(self):
        tail = between(self.source, CLEANUP_STEP)
        mutated = self.source.replace(
            tail, tail.replace('"$RUNNER_TEMP/sin-ghidra-private"', '""'), 1)
        with self.assertRaisesRegex(ValueError, "cleanup path"):
            validate_owner_job(mutated)

    def test_regression_missing_seed_env_rejected(self):
        setting = 'echo "SIN_GHIDRA_SEED_DIRECTORY=$RUNNER_TEMP/sin-reviewed-factory-seeds" >> "$GITHUB_ENV"'
        with self.assertRaisesRegex(ValueError, "environment setting"):
            validate_owner_job(self.source.replace(setting, 'echo "disabled"', 1))

    def test_regression_unprotected_original_job_rejected(self):
        with self.assertRaisesRegex(ValueError, "main-only"):
            validate_owner_job(self.source.replace(
                "github.ref == format('refs/heads/{0}', github.event.repository.default_branch)",
                "true", 1))

    def test_regression_private_code_uploaded_rejected(self):
        marker = "            ${{ runner.temp }}/sin-ghidra-metadata/*.json"
        with self.assertRaisesRegex(ValueError, "private files"):
            validate_owner_job(self.source.replace(
                marker, marker + "\n            ${{ runner.temp }}/sin-ghidra-private/**", 1))


if __name__ == "__main__":
    unittest.main()
