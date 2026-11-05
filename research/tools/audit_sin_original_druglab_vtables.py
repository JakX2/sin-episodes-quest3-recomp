#!/usr/bin/env python3
"""Original SiN 2006 Druglab NPC class vtable topology (read-only PE32).

Verifies RTTI TypeDescriptor, MSVC32 COL, vtable pointer, and constructor
assignments for the original MobGrunt and EliteGrunt implementation. The
results describe the original x86 object layout only. No proprietary
machine code is emitted or redistributed.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import struct
from verify_gameplay_entity_registry import PEImage, SERVER_SHA256, VerificationError

# (class RTTI, type descriptor VA, constructor site in original server.dll,
#  (subobject offset, COL VA, vtable VA, first virtual method VA))
CLASSES = (
    ('CNPC_MobGrunt', 0x2253681C, 0x2236A35F,
     ((0x000, 0x224D24A0, 0x2249AAA4, 0x2236A3B0),
      (0x790, 0x224D241C, 0x2249AA7C, 0x22032260),
      (0xC14, 0x224D2408, 0x2249AA40, 0x2216E350))),
    ('CNPC_EliteGrunt', 0x2253477C, 0x22355CD8,
     ((0x000, 0x224D17C0, 0x2248DF04, 0x22355DB0),
      (0x790, 0x224D1738, 0x2248DED8, 0x22032260),
      (0xC14, 0x224D1724, 0x2248DE9C, 0x2216E350))),
)


def verify_ctor_group(pe: PEImage, code_va: int, rows) -> None:
    """Three x86 MOV [ESI+offset], immediate-pointer writes (6+10+10 bytes)."""
    prefix = b'\xc7\x06' + struct.pack('<I', rows[0][2])
    if pe.at(code_va, len(prefix)) != prefix:
        raise VerificationError(f'{code_va:08X}: original primary vtable install differs')
    for n, offset in ((1, 0x790), (2, 0xC14)):
        start = code_va + 6 + (n-1)*10
        opcode = b'\xc7\x86' + struct.pack('<I', offset) + struct.pack('<I', rows[n][2])
        if pe.at(start, 10) != opcode:
            raise VerificationError(f'{start:08X}: original subobject vtable install differs')


def verify_class(pe: PEImage, name, descriptor_va, constructor_va, rows) -> dict:
    assert len(rows) == 3
    label = f'.?AV{name}@@'.encode('ascii') + b'\0'
    if pe.at(descriptor_va + 8, len(label)) != label:
        raise VerificationError(f'{name}: RTTI class string mismatch')
    hierarchy = None
    entries = []
    for offset, col_va, vtable_va, first_method in rows:
        # MSVC32 RTTI COL: signature, offset, cdOffset, TypeDescriptor*, CHD*.
        sig, col_offset, cd, typeptr, hier = struct.unpack('<5I', pe.at(col_va, 20))
        if (sig, col_offset, cd, typeptr) != (0, offset, 0, descriptor_va):
            raise VerificationError(f'{name}: COL mismatch at 0x{col_va:08X}')
        if hierarchy is None:
            hierarchy = hier
        if hier != hierarchy:
            raise VerificationError(f'{name}: COL hierarchy pointer mismatch')
        actual_col = struct.unpack('<I', pe.at(vtable_va-4, 4))[0]
        first_ptr = struct.unpack('<I', pe.at(vtable_va, 4))[0]
        if actual_col != col_va or first_ptr != first_method:
            raise VerificationError(f'{name}: vtable/RTTI witness mismatch at 0x{vtable_va:08X}')
        entries.append({
            'object_offset_hex': f'0x{offset:X}',
            'col_va': f'0x{col_va:08X}',
            'vtable_va': f'0x{vtable_va:08X}',
            'first_virtual_va': f'0x{first_method:08X}',
        })
    verify_ctor_group(pe, constructor_va, rows)
    return {
        'original_rtti_class': name,
        'original_type_descriptor_va': f'0x{descriptor_va:08X}',
        'constructor_vtable_assignments_begin_va': f'0x{constructor_va:08X}',
        'class_hierarchy_descriptor_va': f'0x{hierarchy:08X}',
        'vtable_subobjects': entries,
    }


# Original MSVC32 CHD contains 17 base classes. The array includes the
# concrete class at index 0, shared CNPC_GruntBase at index 1, and interfaces.
HIERARCHIES = (
    ('CNPC_MobGrunt', 0x224D2490, 0x224D2448),
    ('CNPC_EliteGrunt', 0x224D17B0, 0x224D1768),
)
BASE_WITNESSES = (
    (1, 'CNPC_GruntBase', 0x2252D5B0, 0),
    (2, 'CAI_BaseHumanoid', 0x224F9960, 0),
    (5, 'CAI_BaseNPC', 0x224DBF58, 0),
    (14, 'CAI_DefMovementSink', 0x224DBF74, 0x790),
    (15, 'IAI_MovementSink', 0x224DBE2C, 0x790),
    (16, 'IBehaviorBackBridge', 0x224F98C0, 0xC14),
)


def verify_hierarchy(pe: PEImage, classname: str, hierarchy_va: int,
                     array_va: int) -> dict:
    """Read original MSVC 32-bit class-hierarchy and selected base PMDs."""
    signature, attrs, count, base_array = struct.unpack('<4I', pe.at(hierarchy_va, 16))
    if (signature, attrs, count, base_array) != (0, 1, 17, array_va):
        raise VerificationError(f'{classname}: original 17-base RTTI hierarchy differs')
    bcds = [struct.unpack('<I', pe.at(array_va + 4*i, 4))[0]
            for i in range(count)]
    seen = []
    witnesses = [(0, classname, None, 0)] + list(BASE_WITNESSES)
    for index, expected, type_va, expected_mdisp in witnesses:
        bcd = bcds[index]
        td, _, mdisp = struct.unpack('<3I', pe.at(bcd, 12))
        if mdisp != expected_mdisp or (type_va is not None and td != type_va):
            raise VerificationError(f'{classname}: inherited {expected} PMD/RTTI changed')
        label = f'.?AV{expected}@@'.encode('ascii') + b'\0'
        if pe.at(td + 8, len(label)) != label:
            raise VerificationError(f'{classname}: base RTTI class differs for {expected}')
        seen.append({
            'base_index': index,
            'base_rtti_class': expected,
            'member_displacement_hex': f'0x{mdisp:X}',
            'base_descriptor_va': f'0x{bcd:08X}',
        })
    return {
        'class': classname,
        'original_class_hierarchy_va': f'0x{hierarchy_va:08X}',
        'original_base_class_count_including_self': count,
        'selected_inheritance_witnesses': seen,
        'meaning': 'MobGrunt and EliteGrunt share CNPC_GruntBase but are distinct sibling classes',
    }


def audit(server: bytes, hash_check: bool = True) -> dict:
    digest = hashlib.sha256(server).hexdigest()
    if hash_check and digest != SERVER_SHA256:
        raise VerificationError('original 2006 server.dll hash mismatch')
    pe = PEImage(server)
    if pe.base != 0x22000000:
        raise VerificationError('unexpected original server.dll image base')
    return {
        'original_server_sha256': digest,
        'class_layouts': [
            verify_class(pe, name, td, constructor, rows)
            for name, td, constructor, rows in CLASSES
        ],
        'original_rtti_inheritance': [
            verify_hierarchy(pe, name, hierarchy_va, array_va)
            for name, hierarchy_va, array_va in HIERARCHIES
        ],
        'limits': [
            'These are original 32-bit MSVC virtual table and RTTI witnesses, not C++ class definitions.',
            'Both grunts have subobject vptr assignments at +0, +0x790 and +0xC14 in the original binary.',
            'The additional vtables do not by themselves identify each inherited interface type.',
            'Original offsets and virtual method addresses cannot be ported as raw pointers to ARM64.',
        ],
    }


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--server', type=Path, required=True)
    ap.add_argument('--output', type=Path)
    args = ap.parse_args()
    try:
        report = json.dumps(audit(args.server.read_bytes()), indent=2) + '\n'
        if args.output:
            with args.output.open('x', encoding='utf-8') as dest:
                dest.write(report)
        else:
            print(report, end='')
        return 0
    except (OSError, ValueError, struct.error) as exc:
        ap.exit(2, f'Original Druglab NPC vtable audit failed: {exc}\n')


if __name__ == '__main__':
    raise SystemExit(main())
