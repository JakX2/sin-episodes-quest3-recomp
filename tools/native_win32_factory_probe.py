#!/usr/bin/env python3
"""Opt-in Windows x86 native CreateInterface smoke test for independent game DLLs.

This executes reconstructed DLL code in an isolated child process. It does not
invoke virtual methods or supply fabricated engine callbacks. Never use original
commercial binaries with this diagnostic; --allow-original is intentionally absent.
"""
import argparse
import ctypes
import json
import os
from pathlib import Path
import platform
import struct
import subprocess
import sys

INTERFACES = {
    "client": ("VClient011", "VClientPrediction001", "VClientEntityList003"),
    "server": ("ServerGameDLL003", "ServerGameClients003", "ServerGameEnts001"),
}


def inspect_pe(path):
    data = Path(path).read_bytes()
    if len(data) < 0x40 or data[:2] != b"MZ":
        raise ValueError("not an MZ executable")
    pe = struct.unpack_from("<I", data, 0x3c)[0]
    if pe + 26 > len(data) or data[pe:pe + 4] != b"PE\\0\\0":
        raise ValueError("invalid PE signature")
    machine, sections, _, _, _, optional_size, _ = struct.unpack_from("<HHIIIHH", data, pe + 4)
    if machine != 0x14c or sections == 0 or optional_size < 2:
        raise ValueError("expected x86 PE32 with sections")
    if struct.unpack_from("<H", data, pe + 24)[0] != 0x10b:
        raise ValueError("expected PE32, not PE32+")
    return {"machine": "i386", "pe": "PE32", "bytes": len(data)}


def child_run(kind, path):
    result = {"kind": kind, "dll": str(Path(path).resolve()), "interfaces": {}, "stage": "preflight"}
    try:
        result["pe"] = inspect_pe(path)
        if os.name != "nt" or struct.calcsize("P") != 4:
            raise RuntimeError("requires 32-bit Python on Windows")
        result["stage"] = "load_library"
        # WinDLL uses the Windows loader; CreateInterface itself is cdecl on Source x86.
        library = ctypes.WinDLL(str(Path(path).resolve()))
        result["stage"] = "resolve_export"
        factory = ctypes.CFUNCTYPE(ctypes.c_void_p, ctypes.c_char_p,
                                   ctypes.POINTER(ctypes.c_int))(
                                       ctypes.cast(getattr(library, "CreateInterface"),
                                                   ctypes.c_void_p).value)
        result["stage"] = "query_interfaces"
        for name in INTERFACES[kind]:
            status = ctypes.c_int(-1)
            pointer = factory(name.encode("ascii"), ctypes.byref(status))
            result["interfaces"][name] = {"non_null": bool(pointer), "return_code": status.value}
        result["stage"] = "complete"
        result["ok"] = all(v["non_null"] for v in result["interfaces"].values())
    except Exception as exc:
        result["ok"] = False
        result["error"] = "%s: %s" % (type(exc).__name__, exc)
    print(json.dumps(result, sort_keys=True))
    return 0 if result["ok"] else 1


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--client", type=Path)
    parser.add_argument("--server", type=Path)
    parser.add_argument("--timeout", type=float, default=15)
    parser.add_argument("--child-kind", choices=tuple(INTERFACES), help=argparse.SUPPRESS)
    parser.add_argument("--child-path", help=argparse.SUPPRESS)
    args = parser.parse_args()
    if args.child_kind:
        if not args.child_path:
            parser.error("--child-path required")
        return child_run(args.child_kind, args.child_path)
    if not args.client or not args.server or args.timeout <= 0:
        parser.error("--client, --server and positive --timeout required")
    reports = []
    for kind, path in (("server", args.server), ("client", args.client)):
        command = [sys.executable, str(Path(__file__).resolve()),
                   "--child-kind", kind, "--child-path", str(path)]
        try:
            run = subprocess.run(command, capture_output=True, text=True,
                                 timeout=args.timeout, check=False)
            try:
                report = json.loads(run.stdout.strip())
            except ValueError:
                report = {"kind": kind, "ok": False, "stage": "child_exit",
                          "exit_code": run.returncode,
                          "error": "no valid JSON; child may have crashed"}
            report["exit_code"] = run.returncode
        except subprocess.TimeoutExpired:
            report = {"kind": kind, "ok": False, "stage": "timeout",
                      "error": "child exceeded %.1f seconds" % args.timeout}
        reports.append(report)
    print(json.dumps({"schema": 1, "results": reports,
                      "all_factories_respond": all(r.get("ok") for r in reports),
                      "init_tested": False, "retail_boot_tested": False},
                     sort_keys=True, indent=2))
    return 0 if all(r.get("ok") for r in reports) else 1


if __name__ == "__main__":
    sys.exit(main())
