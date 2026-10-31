#!/usr/bin/env python3
"""Phase 60: bounded weapon ctor +0x508 writes and metadata-pointer xrefs.

Accept original user-provided, hash-matched SiN 2006 server/client DLLs.
No DLL bytes, disassembly excerpts, or confidential sources are shipped.
This is not a complete x86 dataflow/alias analysis and makes no dynamic claims.
"""
from __future__ import annotations

import argparse
import json
import struct
from pathlib import Path

from verify_weapon_field_metadata import PE32, VerificationError
from verify_weapon_dynamic_descriptors import verify as verify_phase59

# Zero-initialized byte stores in the known base weapon and three firearm
# constructor families. Start VAs are evidence anchors, NOT proofs of
# runtime class identity or the sole possible writes to these fields.
# (store_va, encoded store instruction, zero-producer_va, zero-producer instruction)
CTOR_ZERO_STORES = (
    (0x222AE408, bytes.fromhex('88 9e 08 05 00 00'), 0x222AE3AE, bytes.fromhex('33 db')),
    (0x223168C0, bytes.fromhex('88 86 08 05 00 00'), 0x223168B8, bytes.fromhex('33 c0')),
    (0x22316AEF, bytes.fromhex('c6 86 08 05 00 00 00'), 0x22316AD6, bytes.fromhex('e8 b5 78 f9 ff')),
    (0x22317BA0, bytes.fromhex('88 86 08 05 00 00'), 0x22317B98, bytes.fromhex('33 c0')),
    (0x22317D7E, bytes.fromhex('88 9e 08 05 00 00'), 0x22317D5E, bytes.fromhex('33 db')),
)
# Recorded shared reload reads, not writers.
SHARED_RELOAD_READS = (
    (0x222AF457, bytes.fromhex('38 9e 08 05 00 00')),
    (0x222AF7F5, bytes.fromhex('80 be 08 05 00 00 00')),
)
# Known direct embedded maps to extended datamap, not proof of the savegame
# path. Each is a mov dword ptr ds:[destination], 0x2251F810.
EXTENDED_MAP_STORES = (
    (0x22173C8F, 0x224F9E98, 0x2251F810),
    (0x222CD01F, 0x22522E7C, 0x2251F810),
)
# One pointer in a different static registration, close in memory to
# extended map data. Its text label is 'game_weapon_manager'. Treat it
# as a potential *false positive*, not a proven datamap consumer.
ADJACENT_REGISTRATION = 0x223C2B10
ADJACENT_DATA = 0x2251F828
ADJACENT_NAME = 0x2245B128


def at(pe: PE32, va: int, length: int) -> bytes:
    o = pe.va_to_offset(va)
    end = pe.va_to_offset(va + length - 1) + 1
    if end - o != length:
        raise VerificationError(f'Noncontiguous read at {va:#x}')
    return pe.content[o:end]


def require(pe: PE32, va: int, raw: bytes, label: str) -> None:
    if at(pe, va, len(raw)) != raw:
        raise VerificationError(f'{label}: unexpected machine code at {va:#x}')


def check_constructor_sites(pe: PE32) -> list[dict]:
    rows = []
    for store_va, instruction, origin_va, origin_instruction in CTOR_ZERO_STORES:
        require(pe, origin_va, origin_instruction, 'zero-source/constructor anchor')
        require(pe, store_va, instruction, 'weapon constructor offset +0x508 byte store')
        # Verify observed order only. This is not whole-program symbolic execution.
        if not origin_va < store_va:
            raise VerificationError('invalid zero-source ordering')
        rows.append({'store_va': hex(store_va), 'source_anchor_va': hex(origin_va),
                     'stored_value': 0, 'access': 'direct byte store [esi+0x508]'})
    for va, instruction in SHARED_RELOAD_READS:
        require(pe, va, instruction, 'shared reload +0x508 read')
    return rows


def check_static_map_links(pe: PE32) -> list[dict]:
    rows = []
    for va, destination, map_va in EXTENDED_MAP_STORES:
        raw = b'\xc7\x05' + struct.pack('<II', destination, map_va)
        require(pe, va, raw, 'static extended-datamap pointer binding')
        rows.append({'instruction_va': hex(va), 'destination_va': hex(destination),
                     'embedded_map_va': hex(map_va)})
    return rows


def check_adjacent_registration(pe: PE32) -> dict:
    # call 0x2228B110; mov edx,[eax]; push "game_weapon_manager";
    # push immediately-adjacent 0x2251F828; mov ecx,eax; call [edx].
    va = ADJACENT_REGISTRATION
    code = at(pe, va, 22)
    expected = (bytes.fromhex('e8 fb 85 ec ff 8b 10 68')
                + struct.pack('<I', ADJACENT_NAME)
                + b'\x68' + struct.pack('<I', ADJACENT_DATA)
                + bytes.fromhex('8b c8 ff 12 c3'))
    if code != expected:
        raise VerificationError('adjacent registration thunk instruction mismatch')
    raw_name = at(pe, ADJACENT_NAME, len('game_weapon_manager')+1)
    if raw_name != b'game_weapon_manager\0':
        raise VerificationError('adjacent registration target name mismatch')
    pointer = struct.unpack('<I', at(pe, ADJACENT_DATA, 4))[0]
    if pointer != 0x2245ABC0:
        raise VerificationError('adjacent registration data pointer mismatch')
    return {'entry_va': hex(va), 'name': 'game_weapon_manager',
            'name_va': hex(ADJACENT_NAME), 'passed_storage_va': hex(ADJACENT_DATA),
            'value_at_passed_storage': hex(pointer),
            'interpretation': 'Not evidence of CBaseCombatWeapon datamap save/restore consumption'}


def inspect_static(pe: PE32) -> dict:
    return {'zero_stores': check_constructor_sites(pe),
            'shared_reload_read_sites': [hex(v) for v, _ in SHARED_RELOAD_READS],
            'extended_map_static_links': check_static_map_links(pe),
            'adjacent_game_weapon_manager_registration': check_adjacent_registration(pe)}


def verify(server_file: Path, client_file: Path) -> dict:
    prior = verify_phase59(server_file, client_file)
    server = PE32(server_file.read_bytes())
    proof = inspect_static(server)
    return {'result': 'pass', 'phase': 60, 'sha256': prior['source_sha256'],
            'evidence': proof,
            'limitations': ('Bounded machine-code stores/read sites and static references only. '
                            'No complete control-flow/alias analysis, runtime setters, '
                            'prediction/save-game execution or Android build proved.')}


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('server', type=Path)
    ap.add_argument('client', type=Path)
    args = ap.parse_args()
    try:
        print(json.dumps(verify(args.server, args.client), indent=2))
    except (OSError, VerificationError, ValueError, struct.error) as exc:
        ap.exit(1, f'VERIFICATION FAILED: {exc}\n')
    return 0

if __name__ == '__main__':
    raise SystemExit(main())
