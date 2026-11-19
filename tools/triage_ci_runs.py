#!/usr/bin/env python3
"""Deterministic, offline GitHub Actions triage from saved REST workflow-run JSON.

Usage: python tools/triage_ci_runs.py runs.json [more-runs.json ...]
Inputs are GitHub Actions workflow-runs API objects, arrays, or {"workflow_runs": [...]}.
No authentication, network calls, secrets, or proprietary game data are needed.
"""
import json
import pathlib
import sys

PRIORITY = {"failure": 0, "timed_out": 1, "cancelled": 2,
            "action_required": 3, "startup_failure": 4,
            "neutral": 5, "skipped": 6, "success": 7}
ACTIVE = {"queued", "in_progress", "requested", "waiting", "pending"}


def unpack(obj):
    if isinstance(obj, list):
        return obj
    if isinstance(obj, dict):
        if isinstance(obj.get("workflow_runs"), list):
            return obj["workflow_runs"]
        if "id" in obj and "status" in obj:
            return [obj]
    raise ValueError("expected workflow run, array, or workflow_runs envelope")


def summarize(paths):
    runs = {}
    for path in paths:
        for run in unpack(json.loads(pathlib.Path(path).read_text(encoding="utf-8"))):
            if not isinstance(run, dict) or not isinstance(run.get("id"), int):
                raise ValueError("run must contain an integer id")
            # Latest attempt wins; ties resolve deterministically by input order.
            key = run["id"]
            attempt = int(run.get("run_attempt") or 1)
            if key not in runs or attempt >= int(runs[key].get("run_attempt") or 1):
                runs[key] = run
    def rank(run):
        status = str(run.get("status") or "")
        conclusion = str(run.get("conclusion") or "")
        category = 0 if status == "completed" and conclusion in PRIORITY and PRIORITY[conclusion] < 5 else (1 if status in ACTIVE else 2)
        return (category, PRIORITY.get(conclusion, 99), str(run.get("name") or ""), run["id"])
    for run in sorted(runs.values(), key=rank):
        status = str(run.get("status") or "unknown")
        conclusion = str(run.get("conclusion") or "pending")
        url = run.get("html_url") or ("https://github.com/" + str(run.get("repository", {}).get("full_name", "")) + "/actions/runs/" + str(run["id"]))
        print(f"{status:12} {conclusion:16} {run['id']:>12}  {run.get('name', 'unnamed')}  {url}")
    failed = [r for r in runs.values() if r.get("status") == "completed" and r.get("conclusion") in {"failure", "timed_out", "startup_failure", "action_required"}]
    pending = [r for r in runs.values() if r.get("status") in ACTIVE]
    print(f"SUMMARY total={len(runs)} failed={len(failed)} pending={len(pending)}")
    return 1 if failed else 2 if pending else 0


if __name__ == "__main__":
    if len(sys.argv) < 2:
        sys.exit("usage: triage_ci_runs.py runs.json [more-runs.json ...]")
    try:
        sys.exit(summarize(sys.argv[1:]))
    except (OSError, ValueError, TypeError, json.JSONDecodeError) as exc:
        sys.exit(f"invalid CI input: {exc}")
