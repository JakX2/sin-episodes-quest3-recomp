#!/usr/bin/env python3
"""Phase 46 SiN 2006 binary verifier. Proprietary DLLs are not included."""
import argparse
import hashlib
import json
import struct
import zipfile
from pathlib import Path

SHA = {
    'server.dll': '568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650',
    'client.dll': '6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d',
}
VT = {
    'CBaseHLCombatWeapon': 0x22459194,
    'CWeaponMagnum': 0x2247875c,
    'CWeaponAssaultRifle': 0x224776ac,
    'CWeaponScatterGun': 0x2247a1e4,
}
SIGNATURES = {
    'server.dll': [
        ('button_old_to_previous', 0x2223d240, '8b 81 38 09 00 00 89 81 44 09 00 00'),
        ('button_xor_changed', 0x2223d250, '8b 91 44 09 00 00 33 d0'),
        ('button_pressed_bitwise_and', 0x2223d259, '8b f2 23 f0'),
        ('button_store_current', 0x2223d25d, '89 81 38 09 00 00'),
        ('button_released_complement', 0x2223d263, 'f7 d0 23 c2'),
        ('button_store_pressed', 0x2223d267, '89 b1 3c 09 00 00'),
        ('button_store_released', 0x2223d26d, '89 81 40 09 00 00'),
        ('zoom_pressed_bit_test', 0x222ccdcb, 'f6 86 3e 09 00 00 08'),
        ('zoom_in_activity_argument', 0x222cbba0, '68 c0 00 00 00'),
        ('zoom_in_activity_vcall', 0x222cbba7, 'ff 90 14 03 00 00'),
        ('zoom_out_activity_guard', 0x222cbbf5, '81 be bc 04 00 00 c0 00 00 00'),
        ('zoom_out_activity_argument', 0x222cbc04, '68 9d 00 00 00'),
        ('zoom_out_activity_vcall', 0x222cbc0b, 'ff 92 14 03 00 00'),
        ('slot197_thunk', 0x222b07c0, 'e9 4b f4 ff ff'),
        ('activity_lookup_call', 0x222afc19, 'e8 b2 ef e3 ff'),
        ('store_activity', 0x222afc37, '89 be c4 04 00 00'),
        ('store_sequence', 0x222afc3d, '89 86 c0 04 00 00'),
        ('store_current_activity', 0x222afc89, '89 86 bc 04 00 00'),
    ],
    'client.dll': [
        ('zoom_bit_into_command', 0x2416269d, '81 ce 00 00 08 00'),
    ],
}

class PE:
    def __init__(self, data):
        self.data = data
        pe_off = struct.unpack_from('<I', data, 0x3c)[0]
        assert data[pe_off:pe_off + 4] == b'PE\0\0'
        self.base = struct.unpack_from('<I', data, pe_off + 52)[0]
        opt_size = struct.unpack_from('<H', data, pe_off + 20)[0]
        self.sections = []
        n = struct.unpack_from('<H', data, pe_off + 6)[0]
        for i in range(n):
            o = pe_off + 24 + opt_size + i * 40
            name = data[o:o + 8].split(b'\0')[0].decode('ascii')
            virtual_size, rva, raw_size, raw_offset = struct.unpack_from('<IIII', data, o + 8)
            self.sections.append((name, self.base + rva, raw_size, raw_offset))

    def read(self, addr, length):
        for _, va, size, raw in self.sections:
            if va <= addr and addr + length <= va + size:
                return self.data[raw + addr - va:raw + addr - va + length]
        raise ValueError('Unmapped VA: ' + hex(addr))

    def u32(self, addr):
        return struct.unpack('<I', self.read(addr, 4))[0]

    def string(self, addr):
        for _, va, size, raw in self.sections:
            if va <= addr < va + size:
                return self.data[raw + addr - va:raw + size].split(b'\0')[0].decode('ascii')
        raise ValueError('Unmapped string VA: ' + hex(addr))

    def rtti(self, vtable):
        locator = self.u32(vtable - 4)
        type_descr = self.u32(locator + 12)
        return self.string(type_descr + 8)


def rel32_target(pe, addr):
    op, delta = struct.unpack('<Bi', pe.read(addr, 5))
    assert op in (0xe8, 0xe9)
    return addr + 5 + delta


def audit(sin_zip):
    with zipfile.ZipFile(sin_zip) as f:
        assert f.testzip() is None, 'Bad ZIP CRC'
        blobs = {name: f.read(name) for name in SHA}
    digests = {k: hashlib.sha256(v).hexdigest() for k, v in blobs.items()}
    for name in SHA:
        assert digests[name] == SHA[name], f'Unexpected {name} hash'
    pe = {name: PE(blob) for name, blob in blobs.items()}
    assert pe['server.dll'].base == 0x22000000
    assert pe['client.dll'].base == 0x24000000
    checks = []
    for name, group in SIGNATURES.items():
        for label, addr, hex_bytes in group:
            expected = bytes.fromhex(hex_bytes)
            assert pe[name].read(addr, len(expected)) == expected, f'{name} {label} at {addr:#x}'
            checks.append(dict(dll=name, label=label, address=f'{addr:#x}', hex=hex_bytes))
    s = pe['server.dll']
    assert rel32_target(s, 0x222b07c0) == 0x222afc10
    assert rel32_target(s, 0x222afc19) == 0x220eebd0
    assert s.read(0x222afc10, 1) == b'\x56'
    slots = {}
    for name, vt in VT.items():
        assert s.rtti(vt) == '.?AV' + name + '@@', name
        assert s.u32(vt + 4 * 197) == 0x222b07c0, name
        assert s.u32(vt + 4 * 272) == 0x222ac030, name
        slots[name] = {'vtable': hex(vt), 'slot197': '0x222b07c0', 'slot272': '0x222ac030'}
    return {
        'original_dll_sha256': digests,
        'fixed_address_signature_count': len(checks),
        'instruction_signatures': checks,
        'input_state': {'current_offset': '0x938', 'pressed_offset': '0x93c', 'released_offset': '0x940', 'previous_offset': '0x944', 'zoom_mask': '0x00080000'},
        'weapon_virtuals': slots,
        'activity_dispatch': {'slot197_thunk': '0x222b07c0', 'implementation': '0x222afc10', 'activity_to_sequence_call': '0x220eebd0', 'zoom_in_argument': '0xc0', 'conditional_zoom_out_argument': '0x9d'},
        'caveats': ['Numeric activities are not yet identified by authoritative enum names.', 'Actual per-weapon sequences and precise VR sight coordinates are not established.', 'No gameplay integration or on-device Quest test has been conducted.'],
    }

if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('binsin_zip', type=Path)
    p.add_argument('--json', type=Path)
    args = p.parse_args()
    result = audit(args.binsin_zip)
    if args.json:
        args.json.parent.mkdir(exist_ok=True, parents=True)
        args.json.write_text(json.dumps(result, indent=2) + '\n')
    print(f"PASS: both original DLL hashes, {result['fixed_address_signature_count']} binary instruction signatures, 4 weapon RTTIs/vtables and 2 relative branch targets")
