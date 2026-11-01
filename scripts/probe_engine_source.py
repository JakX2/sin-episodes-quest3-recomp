#!/usr/bin/env python3
"""Fail-closed provenance check for an external game/engine SOURCE checkout.

This is not a compile, ABI test, mod-loader test, or proof of Quest support.
No SDK/game files are copied into the SiN compatibility repository.
"""

import argparse
import json
from pathlib import Path
import re
import subprocess
import sys
from urllib.parse import urlparse

SHA_RE = re.compile(r"[0-9a-f]{40}\Z")


class ProbeError(ValueError):
    pass


def _run_git(root, *args):
    p = subprocess.run(["git", "-C", str(root), *args], capture_output=True,
                       text=True, check=False)
    if p.returncode:
        raise ProbeError("git " + " ".join(args) + ": " +
                         (p.stderr.strip() or "command failed"))
    return p.stdout.strip()


def _repo_identity(url):
    if not isinstance(url, str) or not url:
        raise ProbeError("upstream_url must be a nonempty GitHub repository URL")
    if url.startswith("git@github.com:"):
        name = url.split(":", 1)[1]
    else:
        parsed = urlparse(url)
        if parsed.scheme != "https" or parsed.hostname != "github.com" or parsed.query or parsed.fragment:
            raise ProbeError("only https://github.com/OWNER/REPO or git@github.com:OWNER/REPO supported")
        name = parsed.path.lstrip("/")
    name = name.removesuffix(".git").rstrip("/")
    if not re.fullmatch(r"[a-zA-Z0-9_.-]+/[a-zA-Z0-9_.-]+", name):
        raise ProbeError("GitHub URL must contain exactly OWNER/REPO")
    return name.lower()


def _relative_path(root, raw):
    if not isinstance(raw, str) or not raw or "\\" in raw:
        raise ProbeError("file paths must be nonempty POSIX-relative strings")
    parts = raw.split("/")
    if any(x in ("", ".", "..") for x in parts) or raw.startswith("/") or ":" in raw:
        raise ProbeError("unsafe manifest path: " + raw)
    path = (root / raw).resolve()
    if not path.is_relative_to(root.resolve()):
        raise ProbeError("path escapes checkout: " + raw)
    if not path.is_file():
        raise ProbeError("required file missing: " + raw)
    return path


def verify(manifest_path, checkout_path):
    manifest = json.loads(Path(manifest_path).read_text(encoding="utf-8"))
    if not isinstance(manifest, dict) or manifest.get("schema_version") != 1:
        raise ProbeError("expected schema_version=1 object")
    url = manifest.get("upstream_url")
    expected_repo = _repo_identity(url)
    expected_commit = manifest.get("commit")
    if not isinstance(expected_commit, str) or not SHA_RE.fullmatch(expected_commit):
        raise ProbeError("commit must be an exact 40-digit lowercase SHA-1")
    if manifest.get("role") != "reference-only":
        raise ProbeError("role must be reference-only; this probe cannot certify R1")
    license_path = manifest.get("license_path")
    files = manifest.get("required_files")
    if not isinstance(files, dict) or len(files) < 2 or len(files) > 32:
        raise ProbeError("required_files must map 2..32 relative paths to Git blob SHA-1 hashes")
    if license_path not in files:
        raise ProbeError("license_path must be in the verified required_files set")
    root = Path(checkout_path).resolve()
    if not root.is_dir():
        raise ProbeError("checkout directory does not exist")
    git_top = Path(_run_git(root, "rev-parse", "--show-toplevel")).resolve()
    if git_top != root:
        raise ProbeError("checkout must point to the repository root")
    actual_repo = _repo_identity(_run_git(root, "remote", "get-url", "origin"))
    if actual_repo != expected_repo:
        raise ProbeError("origin repository mismatch: " + actual_repo)
    actual_commit = _run_git(root, "rev-parse", "HEAD").lower()
    if actual_commit != expected_commit:
        raise ProbeError("checkout commit mismatch: " + actual_commit)
    for name, expected_blob in files.items():
        if not isinstance(expected_blob, str) or not SHA_RE.fullmatch(expected_blob):
            raise ProbeError("invalid Git blob SHA-1 for " + str(name))
        _relative_path(root, name)
        # SHA-1 of Git's content-addressed blob, not a plain-file SHA-1.
        actual_blob = _run_git(root, "hash-object", "--", name)
        if actual_blob != expected_blob:
            raise ProbeError("modified/wrong file: " + name)
        tracked = _run_git(root, "ls-files", "--error-unmatch", "--", name)
        if tracked != name:
            raise ProbeError("file not tracked in pinned revision: " + name)
    # Deliberately report unverified capabilities even after a successful audit.
    return {
        "upstream_url": url,
        "commit": actual_commit,
        "files_verified": len(files),
        "source_snapshot_verified": True,
        "engine_adapter_compiled": False,
        "engine_module_loader_verified": False,
        "sourcevr_compatibility_verified": False,
        "quest_runtime_verified": False,
        "r1_ready": False,
    }


def main(argv=None):
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--manifest", required=True, type=Path)
    p.add_argument("--checkout", required=True, type=Path)
    args = p.parse_args(argv)
    try:
        result = verify(args.manifest, args.checkout)
    except (OSError, ValueError, json.JSONDecodeError) as e:
        print(json.dumps({"source_snapshot_verified": False, "r1_ready": False,
                          "error": str(e)}, sort_keys=True))
        return 1
    print(json.dumps(result, sort_keys=True, indent=2))
    return 0


if __name__ == "__main__":
    sys.exit(main())
