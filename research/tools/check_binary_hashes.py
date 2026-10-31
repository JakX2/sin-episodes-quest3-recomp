#!/usr/bin/env python3
"""Check local, lawfully held game DLL ZIPs against public SHA-256 provenance."""
import argparse
import hashlib
import json
from pathlib import Path
import zipfile

HERE = Path(__file__).resolve().parents[1]

def sha256(data):
    return hashlib.sha256(data).hexdigest()

def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('binsin', type=Path, help='Local binsin.zip containing server.dll and client.dll')
    ap.add_argument('binhl2', type=Path, help='Local binhl2.zip containing server.dll and client.dll')
    args = ap.parse_args()
    expected = json.loads((HERE/'binary-provenance.json').read_text())['binary_inventory']
    passed = 0
    for package, path in [('binsin.zip', args.binsin),('binhl2.zip', args.binhl2)]:
        with zipfile.ZipFile(path) as z:
            for row in (x for x in expected if x['source_archive'] == package):
                actual = z.read(row['dll_name'])
                match = len(actual) == row['size_bytes'] and sha256(actual) == row['sha256']
                print(f'{package}:{row["dll_name"]}: {"PASS" if match else "FAIL"}')
                if not match: raise SystemExit(1)
                passed += 1
    print(f'{passed} DLL identities verified')

if __name__ == '__main__': main()
