#!/usr/bin/env python3
"""Reconstruct the 10 late-initialized CBaseCombatWeapon server datamap slots.

Pinned to an original SHA-256-checked 2006 SiN PE32 build. The verifier
inspects literal registration instructions, *not* executed engine behavior.
No DLL bytes are bundled or emitted.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path
import struct

from verify_weapon_field_metadata import PE32, VerificationError, EXPECTED_SHA256
from verify_weapon_descriptor_semantics import verify as verify_phase58
from verify_weapon_registration_groups import read_c_string

BASE = 0x2251FFEC
STRIDE = 52
INIT_START = 0x222B0940
INIT_END = 0x222B0E22
NAME_HELPER = 0x22044E50

# Six serialized callback registrations: index, original source-string VA,
# original callable VA. The name pointer *itself* is populated by a runtime
# helper, and its exact heap address cannot be inferred from the DLL.
CALLBACKS = (
    (32, 'DefaultTouch', 0x2245B118, 0x221725F0),
    (33, 'FallThink', 0x223F91F0, 0x222AAE90),
    (34, 'Materialize', 0x2241DCEC, 0x222AC5C0),
    (35, 'AttemptToMaterialize', 0x2245B100, 0x222AAC00),
    (36, 'DestroyItem', 0x2245B0F4, 0x222AB830),
    (37, 'SetPickupTouch', 0x2245B0E4, 0x222AC5B0),
)
INPUT = (38, 'InputHideWeapon', 0x2245B0D4, 'HideWeapon', 0x2245AF68, 0x222AD800)
OUTPUTS = (
    (39, 'm_OnPlayerUse', 0x2242BD78, 'OnPlayerUse', 0x2242BD6C, 0x524),
    (40, 'm_OnPlayerPickup', 0x2245B0C0, 'OnPlayerPickup', 0x2245B0B0, 0x53C),
    (41, 'm_OnNPCPickup', 0x2245B0A0, 'OnNPCPickup', 0x2245B094, 0x554),
)


def absolute_store(dest: int, value: int) -> bytes:
    """x86 C7 05: mov dword ptr [absolute], immediate."""
    return b'\xC7\x05' + struct.pack('<II', dest, value)


def byte_index(code: bytes, pattern: bytes, description: str) -> int:
    found = code.find(pattern)
    if found == -1:
        raise VerificationError(f'initializer missing {description}')
    if code.find(pattern, found + 1) != -1:
        raise VerificationError(f'initializer ambiguous {description}')
    return found


def helper_calls(code: bytes, start_va: int) -> list[int]:
    """Find exact E8 rel32 encodings that target the known helper."""
    locations = []
    for i in range(len(code) - 4):
        if code[i] == 0xE8:
            displacement = struct.unpack_from('<i', code, i + 1)[0]
            if start_va + i + 5 + displacement == NAME_HELPER:
                locations.append(i)
    return locations


def check_callback(code: bytes, slot: int, source_va: int, handler_va: int,
                   helper_starts: list[int], ordinal: int) -> dict:
    base = BASE + slot * STRIDE
    push = byte_index(code, b'\x68' + struct.pack('<I', source_va), f'callback name push #{slot}')
    call = helper_starts[ordinal]
    if not (push < call and (ordinal == 0 or push > helper_starts[ordinal - 1])):
        raise VerificationError(f'callback {slot} not immediately passed to registration helper')
    # The helper's return in eax is subsequently written into descriptor +4.
    store = byte_index(code, b'\xA3' + struct.pack('<I', base + 4), f'callback name result store #{slot}')
    next_call = helper_starts[ordinal + 1] if ordinal + 1 < len(helper_starts) else len(code)
    if not call < store < next_call:
        raise VerificationError(f'callback #{slot} missing result store after helper call')
    byte_index(code, absolute_store(base + 0x1C, handler_va), f'callback function pointer #{slot}')
    # Record has 52 bytes; callback field type/offset are explicitly written as zero.
    # For these six, initializer stores 1 to fieldSize and 0x20 to flags.
    byte_index(code, b'\x66\x89\x35' + struct.pack('<I', base + 16), f'callback fieldSize #{slot}')
    byte_index(code, b'\x66\x89\x3D' + struct.pack('<I', base + 18), f'callback flags #{slot}')
    return {'index': slot, 'name': CALLBACKS[ordinal][1], 'source_name_va': hex(source_va),
            'kind': 'function', 'field_type': 0, 'flags': '0x20',
            'callback_va': hex(handler_va), 'name_pointer': 'runtime-generated'}


def check_data_descriptor(code: bytes, record: tuple, kind: str) -> dict:
    slot, name, name_va, public_name, public_va, target = record
    base = BASE + slot * STRIDE
    byte_index(code, absolute_store(base + 4, name_va), f'{name} field-name store')
    byte_index(code, absolute_store(base + 20, public_va), f'{name} external-name store')
    if kind == 'input':
        byte_index(code, absolute_store(base + 28, target), 'InputHideWeapon input handler')
        # 66 C7 05 [abs] imm16: flags=FTYPEDESC_INPUT
        byte_index(code, b'\x66\xC7\x05' + struct.pack('<I', base + 18) + struct.pack('<H', 8),
                   'InputHideWeapon input flags')
        expected_flag = 8
        type_id = 0
        offset = 0
    else:
        byte_index(code, absolute_store(base + 8, target), f'{name} object offset')
        byte_index(code, b'\x66\x89\x0D' + struct.pack('<I', base + 18), f'{name} output flags')
        byte_index(code, b'\x89\x15' + struct.pack('<I', base), f'{name} output type')
        expected_flag = 0x16
        type_id = 11
        offset = target
    byte_index(code, b'\x66\x89\x35' + struct.pack('<I', base + 16), f'{name} fieldSize')
    return {'index': slot, 'name': name, 'kind': kind, 'type_id': type_id,
            'flags': hex(expected_flag), 'object_offset': hex(offset),
            'external_name': public_name, 'handler_va': hex(target) if kind == 'input' else None}


def verify_source_strings(pe: PE32) -> None:
    for _i, name, va, _handler in CALLBACKS:
        if read_c_string(pe, va) != name:
            raise VerificationError(f'{name} string source mismatch')
    for record in (INPUT,) + OUTPUTS:
        _idx, name, nva, public_name, pva, _target = record
        if read_c_string(pe, nva) != name or read_c_string(pe, pva) != public_name:
            raise VerificationError(f'{name} static descriptor string mismatch')


def inspect_initializer(code: bytes) -> list[dict]:
    # Runtime is compiled with esi=1 (fieldSize), edi=0x20 (function flags)
    # for callback initializers, then ecx=0x16 (output flags) and edx=11 (type).
    for pattern, label in ((b'\xBE\x01\x00\x00\x00', 'fieldSize=1'),
                           (b'\xBF\x20\x00\x00\x00', 'functiontable flag=0x20'),
                           (b'\xB9\x16\x00\x00\x00', 'output flags=0x16'),
                           (b'\xBA\x0B\x00\x00\x00', 'output field type=11')):
        byte_index(code, pattern, label)
    calls = helper_calls(code, INIT_START)
    if len(calls) != 6:
        raise VerificationError(f'expected 6 name-helper calls, got {len(calls)}')
    entries = [check_callback(code, slot, src, handler, calls, i)
               for i, (slot, _name, src, handler) in enumerate(CALLBACKS)]
    entries.append(check_data_descriptor(code, INPUT, 'input'))
    entries.extend(check_data_descriptor(code, record, 'output') for record in OUTPUTS)
    if [x['index'] for x in entries] != list(range(32, 42)):
        raise VerificationError('runtime descriptor slots are not contiguous')
    return entries


def verify(server_path: Path, client_path: Path) -> dict:
    # All prior hash and source-structure checks must pass first.
    baseline = verify_phase58(server_path, client_path)
    pe = PE32(server_path.read_bytes())
    verify_source_strings(pe)
    code = pe.content[pe.va_to_offset(INIT_START):pe.va_to_offset(INIT_END)]
    entries = inspect_initializer(code)
    return {'result': 'pass', 'phase': 59, 'source_sha256': baseline['sha256'],
            'scope': 'static instructions for 10 late-initialized CBaseCombatWeapon descriptors',
            'initializer_va': hex(INIT_START), 'helper_target_va': hex(NAME_HELPER),
            'entries': entries, 'limitations': 'No runtime execution, no proof of consumer behavior or reload-flag writes.'}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('server', type=Path)
    parser.add_argument('client', type=Path)
    args = parser.parse_args()
    try:
        print(json.dumps(verify(args.server, args.client), indent=2))
    except (OSError, VerificationError, ValueError, struct.error) as e:
        parser.exit(1, f'VERIFY FAILED: {e}\n')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
