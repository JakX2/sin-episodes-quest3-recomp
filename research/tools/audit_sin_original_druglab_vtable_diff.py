#!/usr/bin/env python3
"""Compare original MobGrunt vs EliteGrunt primary vtables, original 2006 server.dll.

Read-only hash-pinned x86 analysis. Only addresses, virtual slot indices and
the number of shared/overridden entries are emitted, not copyrighted code.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import struct

from verify_gameplay_entity_registry import PEImage, SERVER_SHA256, VerificationError
from audit_sin_original_druglab_vtables import audit as audit_druglab_vtables

MOB_VTABLE = 0x2249AAA4
ELITE_VTABLE = 0x2248DF04
EXPECTED_COUNTS = (532, 533)
EXPECTED_DIFF_SLOTS = (
    0, 10, 11, 12, 13, 25, 26, 52, 58, 96, 265, 271, 273,
    275, 281, 290, 291, 366, 419, 456, 472, 489, 530,
)
MAX_SCAN_SLOTS = 640


def walk_virtual_methods(pe: PEImage, vtable_va: int, *, limit=MAX_SCAN_SLOTS) -> list[int]:
    """Read contiguous .text pointers until the original vtable's first non-code word."""
    result = []
    for i in range(limit):
        ptr, = struct.unpack('<I', pe.at(vtable_va + 4*i, 4))
        if pe.section_of(ptr) != '.text':
            return result
        result.append(ptr)
    raise VerificationError('vtable pointer scan exceeded maximum sanity bound')


def compare(pe: PEImage, *, require_original: bool = True) -> dict:
    mob = walk_virtual_methods(pe, MOB_VTABLE)
    elite = walk_virtual_methods(pe, ELITE_VTABLE)
    if require_original and (len(mob), len(elite)) != EXPECTED_COUNTS:
        raise VerificationError('original Druglab primary vtable boundaries differ')
    common = min(len(mob), len(elite))
    overrides = [
        {'slot_index': idx, 'byte_offset_hex': f'0x{4*idx:X}',
         'mobgrunt_method_va': f'0x{mob[idx]:08X}',
         'elite_method_va': f'0x{elite[idx]:08X}'}
        for idx in range(common) if mob[idx] != elite[idx]
    ]
    if require_original and tuple(item['slot_index'] for item in overrides) != EXPECTED_DIFF_SLOTS:
        raise VerificationError('MobGrunt/Elite virtual override positions differ')
    return dict(
        mobgrunt_primary_vtable_va=f'0x{MOB_VTABLE:08X}',
        elite_primary_vtable_va=f'0x{ELITE_VTABLE:08X}',
        mobgrunt_contiguous_executable_slots=len(mob),
        elite_contiguous_executable_slots=len(elite),
        compared_slots=common,
        shared_slot_count=common-len(overrides),
        differing_slot_count=len(overrides),
        differing_slots=overrides,
        interpretation_limit=[
            'The scanned contiguous .text pointers are a conservative original binary vtable boundary heuristic.',
            'Equal pointers prove shared virtual implementation addresses; different pointers are candidate overrides, not identified source method names.',
            'Disassembly/callers/RTTI are required before labeling a given slot as a specific AI tactic.',
            'Original 32-bit MSVC vtable slot positions are not compatible with AArch64 source module ABIs.',
        ],
    )


def audit(server: bytes) -> dict:
    digest = hashlib.sha256(server).hexdigest()
    if digest != SERVER_SHA256:
        raise VerificationError('expected original SiN 2006 server.dll SHA-256')
    # Do not compare arbitrary tables without first verifying both C++ RTTI
    # class names, their complete object locators and constructor vptr installs.
    audit_druglab_vtables(server)
    pe = PEImage(server)
    return dict(original_server_sha256=digest, **compare(pe))


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--server', type=Path, required=True)
    ap.add_argument('--output', type=Path)
    args = ap.parse_args()
    try:
        result = json.dumps(audit(args.server.read_bytes()), indent=2) + '\n'
        if args.output:
            with args.output.open('x', encoding='utf-8') as dest:
                dest.write(result)
        else:
            print(result, end='')
        return 0
    except (OSError, ValueError, struct.error) as exc:
        ap.exit(2, f'Original Druglab primary vtable diff failed: {exc}\n')


if __name__ == '__main__':
    raise SystemExit(main())
