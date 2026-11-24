#!/usr/bin/env python3
"""SHA-gated read-only original client class registration inventory."""
import argparse
import hashlib
import json
import re
import runpy
import struct
from pathlib import Path

PE32 = runpy.run_path(str(Path(__file__).with_name(
    "Inspect-OwnerOriginalServerPE.py")))["PE32"]
PIN = "6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d"
INVENTORY_PIN = "79a42f076c80900a6b755fd531b03b26d94328c9b06c8c837e94cf8d5adaad73"
HEAD = 0x33fd90
CRT_BEGIN, CRT_END = 0x2f3008, 0x2f47a4


def u32(raw, offset=0):
    return struct.unpack_from("<I", raw, offset)[0]


def scan(code, start, base, head, read_word, string, mapped_data,
         executable, initializers):
    # Either four inlined immediate member stores or a preinitialized
    # static record. In both forms, the head is prepended by this routine.
    head_va = re.escape(struct.pack("<I", base + head))
    pattern = (rb"\xa1" + head_va + rb"((?:\xc7\x05.{8}){4})?"
               rb"\xa3(.{4})\xc7\x05" + head_va + rb"(.{4})\xc3")
    rows = []
    for match in re.finditer(pattern, code, re.DOTALL):
        init = start + match.start()
        next_va, obj_va = u32(match[2]), u32(match[3])
        obj = obj_va - base
        if next_va != obj_va + 16 or not mapped_data(obj, 24):
            raise ValueError("Invalid client class record storage")
        if match[1]:
            stores = {u32(match[1], i + 2): u32(match[1], i + 6)
                      for i in range(0, 40, 10)}
            if set(stores) != {obj_va+i for i in (0, 4, 8, 12)}:
                raise ValueError("Invalid inlined class field stores")
            values = [stores[obj_va+i] for i in (0, 4, 8, 12)]
            form = "inline"
        else:
            values = [read_word(obj+i) for i in (0, 4, 8, 12)]
            if read_word(obj+16) or read_word(obj+20):
                raise ValueError("Nonzero static next/class ID")
            form = "static"
        create, event, name, table = values
        if bool(create) == bool(event):
            raise ValueError("Expected exactly one creation callback")
        creator = (create or event) - base
        table -= base
        if not executable(creator) or not mapped_data(table, 20):
            raise ValueError("Invalid creator or table storage")
        name = string(name-base)
        if not re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", name):
            raise ValueError("Invalid network class name")
        refs = initializers.get(init, ())
        if len(refs) != 1:
            raise ValueError("Registration is not a unique CRT initializer")
        rows.append((name, table, obj, init, create-base if create else 0,
                     event-base if event else 0, refs[0], form))
    rows.sort(key=lambda row: row[6])
    for key in (lambda row: row[0], lambda row: row[1],
                lambda row: row[2], lambda row: row[6]):
        if len({key(row) for row in rows}) != len(rows):
            raise ValueError("Duplicate class identity, storage or CRT slot")
    return rows


def canonical(rows):
    return "".join(
        f"{name}\t{table:08x}\t{obj:08x}\t{init:08x}\t"
        f"{create:08x}\t{event:08x}\t{crt:08x}\t{form}\n"
        for name, table, obj, init, create, event, crt, form in rows)


def inspect(data):
    if hashlib.sha256(data).hexdigest() != PIN:
        raise ValueError("Original client SHA256 mismatch")
    pe = PE32(data)
    if pe.base != 0x24000000:
        raise ValueError("Unexpected original client image base")
    if pe.u32(0x296c6c + 5*4) != pe.base+0xfd990:
        raise ValueError("VClient011 slot 5 mismatch")
    if pe.raw(0xfd990, 6) != b"\xa1"+struct.pack("<I", pe.base+HEAD)+b"\xc3":
        raise ValueError("Original class getter mismatch")
    def mapped_data(rva, length):
        return any(s == ".data" and start <= rva and
                   rva+length <= start+max(size, raw)
                   for s, start, size, raw, _ in pe.sections)
    def executable(rva):
        return any(s == ".text" and start <= rva < start+size
                   for s, start, size, _, _ in pe.sections)
    initializers = {}
    for crt in range(CRT_BEGIN, CRT_END, 4):
        value = pe.u32(crt)-pe.base
        if executable(value):
            initializers.setdefault(value, []).append(crt)
    _, start, size, raw, _ = next(s for s in pe.sections if s[0] == ".text")
    rows = scan(pe.raw(start, min(size, raw)), start, pe.base, HEAD,
                pe.u32, pe.string, mapped_data, executable, initializers)
    digest = hashlib.sha256(canonical(rows).encode("ascii")).hexdigest()
    if len(rows) != 191 or digest != INVENTORY_PIN:
        raise ValueError("Original client registration inventory mismatch")
    return {
        "status": "authenticated registration metadata; not engine publication",
        "head_global_rva": hex(HEAD),
        "getter_rva": "0xfd990",
        "class_count": len(rows),
        "entity_creators": sum(bool(row[4]) for row in rows),
        "event_creators": sum(bool(row[5]) for row in rows),
        "inlined_registrations": sum(row[7] == "inline" for row in rows),
        "static_registrations": sum(row[7] == "static" for row in rows),
        "inventory_sha256": digest,
        "classes_in_registration_order": [
            dict(name=n, recv_table_storage_rva=hex(t), class_storage_rva=hex(o),
                 initializer_rva=hex(i), entity_creator_rva=hex(c),
                 event_creator_rva=hex(e), crt_slot_rva=hex(p), form=f)
            for n, t, o, i, c, e, p, f in rows],
        "linked_list_order": [row[0] for row in reversed(rows)]
    }


def self_test():
    base, head, start = 0x24000000, 0x9000, 0x1000
    pack = lambda v: struct.pack("<I", v)
    def record(obj, fields=None):
        stores = b""
        if fields is not None:
            stores = b"".join(b"\xc7\x05"+pack(base+obj+i*4)+pack(v)
                              for i, v in enumerate(fields))
        return (b"\xa1"+pack(base+head)+stores+b"\xa3"+pack(base+obj+16)
                +b"\xc7\x05"+pack(base+head)+pack(base+obj)+b"\xc3")
    a = record(0x5000, [base+0x3000, 0, base+1, base+0x6000])
    b = record(0x5020)
    static = {0x5020: 0, 0x5024: base+0x3010, 0x5028: base+2,
              0x502c: base+0x6040, 0x5030: 0, 0x5034: 0}
    crt = {start: [0x7004], start+len(a): [0x7000]}
    def run(code, positions=crt):
        return scan(code, start, base, head, static.__getitem__,
                    {1: "CAlpha", 2: "CBeta"}.__getitem__,
                    lambda r, n: 0x5000 <= r and r+n <= 0x6800,
                    lambda r: 0x3000 <= r < 0x4000, positions)
    rows = run(a+b)
    assert [row[0] for row in rows] == ["CBeta", "CAlpha"]
    assert rows[0][5] == 0x3010 and rows[1][4] == 0x3000
    assert rows[0][7] == "static" and rows[1][7] == "inline"
    bad_cases = [
        (a+b, {start: [0x7004, 0x7008], start+len(a): [0x7000]}),
        (record(0x5000, [0, 0, base+1, base+0x6000])+b, crt),
        (record(0x5000, [base+0x3000, base+0x3010,
                         base+1, base+0x6000])+b, crt),
        (record(0x5000, [base+0x3000, 0, base+2, base+0x6000])+b, crt),
    ]
    for code, positions in bad_cases:
        try:
            run(code, positions)
        except ValueError:
            pass
        else:
            raise AssertionError("Malformed registration accepted")
    try:
        inspect(b"not an owner original")
    except ValueError as exc:
        assert "SHA256 mismatch" in str(exc)
    else:
        raise AssertionError("Invented original accepted")
    print("PASS synthetic original client class registration inventory")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("client", nargs="?", type=Path)
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    if args.self_test:
        self_test()
    elif args.client:
        print(json.dumps(inspect(args.client.read_bytes()), indent=2))
    else:
        parser.error("Provide owner-private client DLL or --self-test")
