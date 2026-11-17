#!/usr/bin/env python3
"""Write private Ghidra seed CSVs from reviewed SiN interface registration metadata.

This tool does not read executable bytes or independently verify function bodies.
The original DLLs must be SHA256-authenticated separately before seeding.
"""
import argparse
import csv
import json
from pathlib import Path

MODULES = ("client", "server")


def validate_va(value):
    if isinstance(value, bool) or not isinstance(value, int):
        raise ValueError("reviewed seed VA must be an integer")
    if value < 0 or value > 0xffffffff:
        raise ValueError("reviewed seed VA outside PE32")
    return f"{value:08x}"


def seeds(manifest):
    dlls = manifest.get("dlls")
    if not isinstance(dlls, dict) or not all(m in dlls for m in MODULES):
        raise ValueError("reviewed manifest must include both DLLs")
    result = {}
    for module in MODULES:
        item = dlls[module]
        registrations = item["interface_registrations"]
        if len(registrations) != item["static_registration_count"]:
            raise ValueError(f"{module}: incomplete reviewed interface manifest")
        seen = set()
        records = []
        for registration in registrations:
            interface = registration["interface"]
            if (not isinstance(interface, str) or not interface or
                    any(c not in "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_" for c in interface)):
                raise ValueError(f"{module}: invalid interface name")
            if interface in seen:
                raise ValueError(f"{module}: duplicate interface name")
            seen.add(interface)
            records.append({
                "entry": validate_va(registration["factory_va"]),
                "interface": interface,
                "source": "reviewed_original_static_registration",
            })
        records.sort(key=lambda row: (row["entry"], row["interface"]))
        result[module + ".dll_factory_seeds.csv"] = records
    return result


def write_seeds(manifest_path, directory):
    data = json.loads(manifest_path.read_text(encoding="utf-8"))
    planned = seeds(data)  # all-or-nothing validation before output
    directory.mkdir(parents=True, exist_ok=True)
    for filename, rows in planned.items():
        with (directory / filename).open("w", newline="", encoding="utf-8") as stream:
            writer = csv.DictWriter(stream, fieldnames=["entry", "interface", "source"])
            writer.writeheader()
            writer.writerows(rows)
    return {name: len(rows) for name, rows in planned.items()}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=Path, required=True)
    parser.add_argument("--out-dir", type=Path, required=True)
    args = parser.parse_args()
    print(json.dumps(write_seeds(args.manifest, args.out_dir), indent=2))


if __name__ == "__main__":
    main()
