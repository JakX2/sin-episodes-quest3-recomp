"""Original-free mutation tests for signed-16 and threshold evidence.

The original SHA-pinned game binaries are NEVER required for test fixtures.
"""
import pathlib
import struct
import sys
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1] / 'tools'))
from audit_sin_difficulty_bitwidths import verify_bits, VerificationError
from test_sin_difficulty_protocol import fixtures, rel32


def fixtures_with_bitwidths():
    c, s = fixtures()
    for site, target in (
        (0x221e87c2, 0x2225a160),
        (0x2225a167, 0x22259b00),
        (0x221e8852, 0x22258f00),
        (0x222ba88a, 0x221e8830),
    ):
        s.put(site, rel32(site, target))
    for site, target in (
        (0x240a5602, 0x240a5080),
        (0x240a508d, 0x240a4760),
        (0x240a5638, 0x240a4da0),
        (0x241fab4a, 0x241fa450),
    ):
        c.put(site, rel32(site, target))
    for site, hex_bytes in {
        0x2225a164: '6a10',
        0x22259b12: '6a00',
        0x22259b51: '6a01',
        0x22258f00: '6a20',
        0x222ba885: '68000080bf',
    }.items():
        s.put(site, bytes.fromhex(hex_bytes))
    for site, hex_bytes in {
        0x240a5600: '6a10',
        0x240a5087: '83c3ff',
        0x240a5631: '6a20',
        0x241fab2e: 'f6c405',
        0x241fab31: '7a3a',
        0x241fab3f: 'f6c405',
        0x241fab42: '7a13',
        0x241fab44: '8d8fa4000000',
        0x241fab59: '89879c000000',
        0x241fab5f: '8987a0000000',
    }.items():
        c.put(site, bytes.fromhex(hex_bytes))
    # The FCOMP sites and float constants are already supplied by the
    # previous PR #72 fixture (and still independently checked here).
    return c, s


class BitWidthTests(unittest.TestCase):
    def setUp(self):
        self.c, self.s = fixtures_with_bitwidths()

    def test_signed_wire_fields_and_thresholds(self):
        result = verify_bits(self.c, self.s)
        self.assertEqual(result['payload_bits_without_message_framing'], 208)
        self.assertEqual(result['DDDecisionMade_ordinary_wire_types'][1:4], ['s16'] * 3)

    def test_width_mismatch_rejected(self):
        for side, site in (('server', 0x2225a164), ('client', 0x240a5600)):
            with self.subTest(side=side):
                c, s = fixtures_with_bitwidths()
                (s if side == 'server' else c).corrupt(site + 1)
                with self.assertRaises(VerificationError):
                    verify_bits(c, s)

    def test_wrong_signed_writer_target_rejected(self):
        self.s.corrupt(0x2225a167 + 1)
        with self.assertRaises(VerificationError):
            verify_bits(self.c, self.s)

    def test_threshold_must_be_conditional_jump(self):
        self.c.corrupt(0x241fab42)
        with self.assertRaises(VerificationError):
            verify_bits(self.c, self.s)

    def test_client_state_offsets_checked(self):
        self.c.corrupt(0x241fab5f + 2)
        with self.assertRaises(VerificationError):
            verify_bits(self.c, self.s)

    def test_server_short_negative_message_checked(self):
        self.s.corrupt(0x222ba885 + 4)
        with self.assertRaises(VerificationError):
            verify_bits(self.c, self.s)


if __name__ == '__main__':
    unittest.main()
