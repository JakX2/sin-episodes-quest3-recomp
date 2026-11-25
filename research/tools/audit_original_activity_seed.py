#!/usr/bin/env python3
"""Owner-only activity initializer audit; emit a C++ seed include or TSV."""
import argparse
import hashlib
import pathlib
import struct

BASE = 0x22000000
START, END = 0x220E7040, 0x220EA614
TARGET = 0x220E6F60
SHA = "568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650"
BODY = "be2d64d38bfffb9d6285fbb36268bc308a5d3d03100caf75dd6081dcb4b82206"
ROWS = "1331c3f2bea2e829a22799fc91d3981eeca38507ed58a33229befd37284b985f"

def extract(data):
    if hashlib.sha256(data).hexdigest() != SHA:
        raise ValueError("unexpected owner server.dll SHA256")
    # This exact PE has identity RVA-to-file mapping in .text and .rdata.
    body = data[START-BASE:END-BASE]
    if hashlib.sha256(body).hexdigest() != BODY:
        raise ValueError("initializer hash mismatch")
    rows, pos = [], 0
    while pos < len(body):
        op = body[pos]
        if op == 0xC3:
            if pos + 1 != len(body):
                raise ValueError("unexpected early return")
            break
        if body[pos:pos+2] == bytes.fromhex("83c4"):
            pos += 3
            continue
        if op == 0x6A:
            ident = struct.unpack_from("<b", body, pos+1)[0]
            pos += 2
        elif op == 0x68:
            ident = struct.unpack_from("<i", body, pos+1)[0]
            pos += 5
        else:
            raise ValueError(f"unexpected instruction at {START+pos:#x}")
        if body[pos] != 0x68 or body[pos+5] != 0xE8:
            raise ValueError("expected name immediate and direct call")
        address = struct.unpack_from("<I", body, pos+1)[0]
        dest = START + pos + 10 + struct.unpack_from("<i", body, pos+6)[0]
        if dest != TARGET or not 0x223D2000 <= address < 0x224D5626:
            raise ValueError("unexpected call target or name RVA")
        off = address - BASE
        end = data.index(0, off, off+128)
        rows.append((ident, data[off:end].decode("ascii")))
        pos += 10
    if [i for i,_ in rows] != list(range(921)) or len({n.lower() for _,n in rows}) != 921:
        raise ValueError("unexpected ID coverage or duplicate names")
    tsv = "".join(f"{i}\t{n}\n" for i,n in rows)
    if hashlib.sha256(tsv.encode()).hexdigest() != ROWS:
        raise ValueError("canonical row digest mismatch")
    return rows, tsv

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("server", type=pathlib.Path)
    p.add_argument("--tsv", type=pathlib.Path)
    p.add_argument("--include", type=pathlib.Path)
    a = p.parse_args()
    rows, tsv = extract(a.server.read_bytes())
    if a.tsv:
        a.tsv.write_text(tsv, encoding="utf-8", newline="\n")
    if a.include:
        a.include.write_text("".join(f'{{{i}, "{n}"}},\n' for i,n in rows), encoding="utf-8", newline="\n")
    print(f"PASS {len(rows)} authenticated activity seeds, IDs 0..{rows[-1][0]}, SHA256 {ROWS}")
if __name__ == "__main__":
    main()
