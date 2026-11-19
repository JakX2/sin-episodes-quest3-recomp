"""Offline regression tests: python -m unittest discover -s tools -p 'test_triage_ci_runs.py'."""
import contextlib
import io
import json
import tempfile
import unittest
from pathlib import Path
from triage_ci_runs import summarize


class TriageTests(unittest.TestCase):
    def check(self, objects):
        with tempfile.TemporaryDirectory() as root:
            files = []
            for i, obj in enumerate(objects):
                path = Path(root) / f"{i}.json"
                path.write_text(json.dumps(obj), encoding="utf-8")
                files.append(path)
            out = io.StringIO()
            with contextlib.redirect_stdout(out):
                result = summarize(files)
            return result, out.getvalue()

    def test_failure_before_success_and_deduplicate(self):
        code, output = self.check([
            {"workflow_runs": [
                {"id": 2, "name": "green", "status": "completed", "conclusion": "success"},
                {"id": 1, "name": "red", "status": "completed", "conclusion": "failure", "run_attempt": 1}]},
            [{"id": 1, "name": "red", "status": "completed", "conclusion": "failure", "run_attempt": 2}]])
        self.assertEqual(code, 1)
        self.assertLess(output.index("red"), output.index("green"))
        self.assertIn("total=2 failed=1 pending=0", output)

    def test_pending_distinct_from_pass(self):
        code, output = self.check([[{"id": 3, "status": "queued", "name": "waiting"}]])
        self.assertEqual(code, 2)
        self.assertIn("pending=1", output)

    def test_all_green(self):
        code, output = self.check([{"id": 4, "status": "completed", "conclusion": "success"}])
        self.assertEqual(code, 0)
        self.assertIn("failed=0 pending=0", output)

    def test_invalid_shape(self):
        with self.assertRaises(ValueError):
            self.check([{"workflow_runs": [{"name": "missing id"}]}])


if __name__ == "__main__":
    unittest.main()
