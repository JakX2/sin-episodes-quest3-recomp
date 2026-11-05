#!/usr/bin/env python3
"""Read-only listing of *local* Source 1 IDST studio model sequence labels.

The game engine can merge local sequences from included models at runtime.
Pass both mobgrunt.mdl and mobgrunt_anims.mdl to inspect their local sets.
Does NOT play, recompile, modify, extract or redistribute any model assets.
"""
from __future__ import annotations

import argparse
from pathlib import Path
import struct
import sys

MAGIC = b'IDST'
HEADER_MIN = 196
NUM_SEQ_OFF = 188
SEQ_INDEX_OFF = 192
SEQ_DESC_BYTES = 212
MAX_SEQUENCE_COUNT = 10000


def u32(data: bytes, offset: int) -> int:
    if offset < 0 or offset + 4 > len(data):
        raise ValueError('invalid offset in model')
    return struct.unpack_from('<I', data, offset)[0]


def list_sequences(data: bytes) -> tuple[int, list[tuple[int, str]]]:
    if len(data) < HEADER_MIN or data[:4] != MAGIC:
        raise ValueError('not a Source IDST studio model')
    version = u32(data, 4)
    if version not in range(36, 54):
        raise ValueError(f'unsupported/unknown studio model version {version}; use HLMV')
    count = u32(data, NUM_SEQ_OFF)
    offset = u32(data, SEQ_INDEX_OFF)
    if count > MAX_SEQUENCE_COUNT:
        raise ValueError(f'implausible local sequence count {count}')
    if count == 0:
        return version, []
    if offset < HEADER_MIN or offset + count * SEQ_DESC_BYTES > len(data):
        raise ValueError(f'local sequence table outside file (count {count}, offset {offset})')
    found = []
    for index in range(count):
        base = offset + index * SEQ_DESC_BYTES
        rel_label = u32(data, base + 4)
        at = base + rel_label
        if rel_label == 0 or at >= len(data):
            raise ValueError(f'sequence #{index}: missing/bad label offset')
        end = data.find(b'\0', at, min(at + 128, len(data)))
        if end < at:
            raise ValueError(f'sequence #{index}: unterminated label')
        label = data[at:end]
        if not label or any(x < 32 or x > 126 for x in label):
            raise ValueError(f'sequence #{index}: invalid name bytes')
        found.append((index, label.decode('ascii')))
    return version, found


def main(argv: list[str] | None = None) -> int:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('models', nargs='+', type=Path,
                   help='original .mdl file(s), including mobgrunt_anims.mdl')
    args = p.parse_args(argv)
    errors = 0
    for filename in args.models:
        print(f'\nMODEL: {filename}')
        try:
            version, sequences = list_sequences(filename.read_bytes())
        except (OSError, ValueError) as exc:
            print(f'  ERROR: {exc}', file=sys.stderr)
            errors += 1
            continue
        print(f'  MDL version: {version}; local sequences: {len(sequences)}')
        if not sequences:
            print('  No local sequences; included model may provide them.')
        else:
            for index, label in sequences:
                print(f'  [{index:3d}] {label}')
    return 2 if errors else 0


if __name__ == '__main__':
    raise SystemExit(main())
