#!/usr/bin/env python3
"""Additional original x86 DDDecisionMade wire-width and x87 branch proof.

Requires the prior PR #72 original-binary SHA/registry verifier. This module
verifies NEW call chains; no source-level feature behavior or payload framing
is inferred. Proprietary game DLL bytes never enter the generated JSON.
"""
import argparse
import json
from pathlib import Path
import struct

from audit_sin_difficulty_protocol import audit as audit_protocol
from verify_gameplay_entity_registry import PEImage, VerificationError


def check(pe, va, expected):
    if pe.at(va, len(expected)) != expected:
        raise VerificationError('bitwidth/branch opcode mismatch at %#x' % va)


def edge(pe, va, target):
    check(pe, va, b'\xe8' + struct.pack('<i', target - va - 5))


def verify_bits(client_pe, server_pe):
    # The original CPU member loads are 32-bit but the wire operation is s16!
    edge(server_pe, 0x221e87c2, 0x2225a160)
    check(server_pe, 0x2225a164, b'\x6a\x10')
    edge(server_pe, 0x2225a167, 0x22259b00)
    check(server_pe, 0x22259b12, b'\x6a\x00') # signed write -
    check(server_pe, 0x22259b51, b'\x6a\x01') # signed write +
    check(client_pe, 0x240a5600, b'\x6a\x10')
    edge(client_pe, 0x240a5602, 0x240a5080)
    check(client_pe, 0x240a5087, b'\x83\xc3\xff')
    edge(client_pe, 0x240a508d, 0x240a4760)
    edge(server_pe, 0x221e8852, 0x22258f00)
    check(server_pe, 0x22258f00, b'\x6a\x20')
    check(client_pe, 0x240a5631, b'\x6a\x20')
    edge(client_pe, 0x240a5638, 0x240a4da0)
    # x87 FCOMP and JP are threshold tests, not equality sentinels.
    for site, constant in ((0x241fab26, 0x242de2f8), (0x241fab37, 0x24276f84)):
        check(client_pe, site, b'\xd8\x1d' + struct.pack('<I', constant))
    for site in (0x241fab2e, 0x241fab3f):
        check(client_pe, site, b'\xf6\xc4\x05')
    check(client_pe, 0x241fab31, b'\x7a\x3a')
    check(client_pe, 0x241fab42, b'\x7a\x13')
    check(client_pe, 0x242de2f8, struct.pack('<f', 0.0))
    check(client_pe, 0x24276f84, struct.pack('<f', -1.0))
    check(client_pe, 0x241fab44, bytes.fromhex('8d8fa4000000'))
    edge(client_pe, 0x241fab4a, 0x241fa450)
    check(client_pe, 0x241fab59, bytes.fromhex('89879c000000'))
    check(client_pe, 0x241fab5f, bytes.fromhex('8987a0000000'))
    check(server_pe, 0x222ba885, bytes.fromhex('68000080bf'))
    edge(server_pe, 0x222ba88a, 0x221e8830)
    return {
        'DDDecisionMade_ordinary_wire_types': ['f32', 's16', 's16', 's16', 'f32', 'f32', 'f32', 'f32'],
        'payload_bits_without_message_framing': 208,
        'client_first_float_branches': ['first < -1: list function', '-1 <= first < 0: two state zeros', 'first >= 0 or unordered: ordinary 8 fields'],
        'special_server_first_float': -1.0,
        'warning': 'Original client branch effects verified; exact gameplay meaning and full Source message framing not recovered.',
    }


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--client', required=True, type=Path)
    ap.add_argument('--server', required=True, type=Path)
    ap.add_argument('--output', required=True, type=Path)
    args = ap.parse_args()
    cb, sb = args.client.read_bytes(), args.server.read_bytes()
    original = audit_protocol(cb, sb) # enforces BOTH exact original hashes and PR #72 evidence
    original['new_signed_wire_and_client_threshold_proof'] = verify_bits(PEImage(cb), PEImage(sb))
    args.output.write_text(json.dumps(original, indent=2) + '\n', encoding='utf-8')
    print(json.dumps(original['new_signed_wire_and_client_threshold_proof'], indent=2))


if __name__ == '__main__':
    main()
