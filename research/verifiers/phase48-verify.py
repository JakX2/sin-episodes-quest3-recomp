#!/usr/bin/env python3
"""Phase 48: static verification of original user-supplied SiN 32-bit DLL archives.
No proprietary binaries are copied into the output package.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
import zipfile
from pathlib import Path

SIN_DIGESTS = {
    'server.dll': '568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650',
    'client.dll': '6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d',
}
HL2_DIGESTS = {
    'server.dll': 'e1984ba7ea6c344be55a91c3b9914f43550dcac0670e09ad9346f2c2d8f0406d',
    'client.dll': '2095adcf963a3eff8dd575056bf1e1ca840ab67c2b847181e291d0ccac093aae',
}
VTABLES = {
    'CBaseHLCombatWeapon': 0x22459194,
    'CWeaponMagnum': 0x2247875C,
    'CWeaponAssaultRifle': 0x224776AC,
    'CWeaponScatterGun': 0x2247A1E4,
}
# These are fixed expected bytes observed in the ORIGINAL DLLs, not derived from the input.
SERVER_SIGNATURES = {
    'deadline_virtual_slot_200_function': (0x222AC8D0, 'a1 90 21 57 22 d9 40 0c d8 99 a4 04 00 00'),
    'strict_greater_than_deadline_return': (0x222AC8E0, 'f6 c4 41 75 03 b0 01 c3 32 c0 c3'),
    'zoom_state_activity_reads': (0x222CB963, '80 be 78 05 00 00 00'),
    'zoom_activity_for_completion': (0x222CB96C, '8b 86 bc 04 00 00'),
    'zoom_idle_id': (0x222CB972, '3d c0 00 00 00'),
    'zoom_primary_id': (0x222CB979, '3d c1 00 00 00'),
    'zoom_secondary_id': (0x222CB980, '3d c2 00 00 00'),
    'zoom_finished_virtual_call': (0x222CB98B, 'ff 92 20 03 00 00'),
    'zoom_wait_if_not_finished': (0x222CB993, '0f 84 cb 00 00 00'),
    'zoom_idle_request': (0x222CB99B, '68 c0 00 00 00'),
    'lookup_activity_call': (0x222AFC19, 'e8 b2 ef e3 ff'),
    'missing_sequence_fails': (0x222AFC1E, '83 f8 ff 75 07 5f 32 c0 5e c2 04 00'),
    'requested_activity_and_sequence_store': (0x222AFC37, '89 be c4 04 00 00 89 86 c0 04 00 00'),
    'actual_activity_store': (0x222AFC89, '89 86 bc 04 00 00'),
    'sequence_duration_calculation': (0x222AFCD6, 'e8 05 f1 e3 ff'),
    'add_current_time_to_duration': (0x222AFCDB, 'a1 90 21 57 22 d8 40 0c'),
    'set_animation_deadline_virtual_call': (0x222AFCEB, 'ff 92 24 03 00 00'),
    'deadline_setter_compare': (0x222AE720, 'd9 44 24 04 d9 81 a4 04 00 00'),
    'deadline_setter_write_1': (0x222AE747, '89 81 a4 04 00 00'),
    'deadline_setter_write_2': (0x222AE761, '89 81 a4 04 00 00'),
    'server_viewmodel_field_offset': (0x22240CAC, '68 b0 04 00 00'),
    'server_viewmodel_field_name': (0x22240CB1, '68 10 f2 44 22'),
}
CLIENT_SIGNATURES = {
    'recv_viewmodel_field_size': (0x2409CD3C, '6a 0c'),
    'recv_viewmodel_field_offset': (0x2409CD3E, '68 a8 07 00 00'),
    'recv_viewmodel_field_name': (0x2409CD43, '68 88 5d 28 24'),
    'client_viewmodel_pose_prologue': (0x2409C970, '83 ec 78'),
    'client_angle_basis_generator': (0x2409C9B9, 'e8 c2 cc 0c 00'),
    'origin_offset_component_z': (0x2409C9BE, 'd9 86 b0 07 00 00'),
    'origin_offset_component_x': (0x2409C9FF, 'd9 86 a8 07 00 00'),
    'origin_offset_component_y': (0x2409CA3D, 'd9 86 ac 07 00 00'),
    'origin_add_x': (0x2409CA86, 'd8 00'),
    'origin_add_y': (0x2409CA94, 'd8 40 04'),
    'origin_add_z': (0x2409CAA3, 'd8 40 08'),
}
class PE:
    def __init__(self, data: bytes):
        self.data = data
        off = struct.unpack_from('<I', data, 0x3c)[0]
        assert data[off:off+4] == b'PE\0\0', 'not a PE file'
        opt_sz = struct.unpack_from('<H', data, off+20)[0]
        self.base = struct.unpack_from('<I', data, off+52)[0]
        self.sections = []
        for idx in range(struct.unpack_from('<H', data, off+6)[0]):
            p = off+24+opt_sz+idx*40
            label = data[p:p+8].split(b'\0')[0].decode('ascii')
            virt_size, rva, raw_size, raw_offset = struct.unpack_from('<IIII', data, p+8)
            self.sections.append((label, self.base+rva, raw_size, raw_offset))

    def read(self, va: int, count: int) -> bytes:
        for label, base, size, off in self.sections:
            if base <= va and va + count <= base + size:
                return self.data[off+va-base:off+va-base+count]
        raise AssertionError(f'Unmapped virtual address {va:#x}')

    def u32(self, va):
        return struct.unpack('<I', self.read(va, 4))[0]

    def string_addresses(self, value):
        needle = value.encode('ascii') + b'\0'
        found = []
        for name, va, size, off in self.sections:
            if name not in ('.rdata','.data'): continue
            blob = self.data[off:off+size]
            i = 0
            while True:
                loc = blob.find(needle, i)
                if loc < 0: break
                found.append(va+loc)
                i=loc+1
        return found

    def rtti_name(self, table_va):
        # MSVC x86 Complete Object Locator pointer is stored at vtable - 4.
        col = self.u32(table_va-4)
        type_descriptor = self.u32(col+12)
        return self.read(type_descriptor+8,128).split(b'\0')[0].decode('ascii')

def read_originals(path, digests):
    with zipfile.ZipFile(path) as archive:
        assert archive.testzip() is None, 'bad ZIP CRC'
        assert set(digests) <= set(archive.namelist()), 'missing required DLL files'
        blobs = {name: archive.read(name) for name in digests}
    hashes = {name: hashlib.sha256(blob).hexdigest() for name,blob in blobs.items()}
    assert hashes == digests, f'hashes do not match expected originals: {hashes}'
    return {name: PE(data) for name,data in blobs.items()}, hashes

def audit(sin_path, hl2_path=None):
    bins, hashes = read_originals(sin_path, SIN_DIGESTS)
    s,c = bins['server.dll'], bins['client.dll']
    assert s.base == 0x22000000 and c.base == 0x24000000
    sigs={}
    for module,patterns in [('server.dll',SERVER_SIGNATURES),('client.dll',CLIENT_SIGNATURES)]:
        pe=bins[module]
        for name,(va,raw) in patterns.items():
            exp=bytes.fromhex(raw)
            actual=pe.read(va,len(exp))
            assert actual == exp, f'{module}:{name} unexpected bytes at {va:#x}: {actual.hex(" ")}'
            sigs[name]={'dll':module,'address':f'{va:#x}','expected_bytes':raw}
    weapons={}
    for nm, vt in VTABLES.items():
        assert s.rtti_name(vt)=='.?AV'+nm+'@@'
        obj={str(i):hex(s.u32(vt+4*i)) for i in [197,200,201,219,228,229]}
        assert s.u32(vt+4*200)==0x222AC8D0
        assert s.u32(vt+4*201)==0x222AE720
        assert s.u32(vt+4*219)==0x222CB960
        weapons[nm]={'vtable':hex(vt),'virtuals':obj}
    assert c.rtti_name(0x24285DF4)=='.?AVC_BaseViewModel@@'
    assert c.u32(0x24285DF4+4*200)==0x2409C970
    assert s.string_addresses('m_viewmodelOriginOffset') == [0x2244F210]
    assert c.string_addresses('m_viewmodelOriginOffset') == [0x24285D88]
    assert c.string_addresses('DT_BaseViewModel') == [0x24285D00]
    assert s.string_addresses('DT_BaseViewModel') == [0x2244F298]
    for ident,name in [(0xc0,'ACT_VM_ZOOMED_IDLE'),(0xc1,'ACT_VM_ZOOMED_PRIMARYATTACK'),(0xc2,'ACT_VM_ZOOMED_SECONDARYATTACK')]:
        assert s.string_addresses(name) and c.string_addresses(name)
    hl2=None
    if hl2_path:
        hb, hh = read_originals(hl2_path, HL2_DIGESTS)
        absent={name: {word:hb[name].string_addresses(word)==[] for word in ['m_viewmodelOriginOffset','ACT_VM_ZOOMED_IDLE','ACT_VM_ZOOMED_PRIMARYATTACK','ACT_VM_ZOOMED_SECONDARYATTACK']} for name in hb}
        assert all(all(v.values()) for v in absent.values()),absent
        hl2={'sha256':hh,'literal_absence_from_provided_hl2':absent}
    return {
        'phase':48,
        'scope':'SiN original x86 DLL static analysis, not functional PC or Quest runtime testing',
        'sha256':hashes,
        'signature_count':len(sigs),
        'signatures':sigs,
        'shared_firearm_virtuals':weapons,
        'client_viewmodel_vtable':{'address':'0x24285df4','rtti':'.?AVC_BaseViewModel@@','slot_200':'0x2409c970'},
        'deadline':{'weapon_offset':'0x4a4','completion_comparison':'current_time > deadline (strict)','guard_virtual_slot':200,'setter_virtual_slot':201},
        'activity_memory':{'requested_activity':'0x4c4','requested_sequence':'0x4c0','active_activity':'0x4bc'},
        'origin_offset':{'name':'m_viewmodelOriginOffset','server_data_field':'0x4b0','client_recv_field':'0x7a8','size':12,'client_pose_function':'0x2409c970','component_mapping':'right*x + forward*y + up*z'},
        'hl2_comparison':hl2,
        'boundaries':['No original weapon models or script assets: exact per-gun ADS alignment, actual model sequences and ZoomFOV numerical values unverified.','The existence of a general viewmodel-origin-offset mechanism does not establish that ADS changes it.','Server-side named field registration is not by itself proof of network-send semantics.','Gameplay control, animation transitions and physical VR alignment should be modeled separately.']
    }

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('sin_archive',type=Path)
    parser.add_argument('--hl2',type=Path)
    parser.add_argument('--json',type=Path)
    a=parser.parse_args()
    result=audit(a.sin_archive,a.hl2)
    if a.json:
        a.json.write_text(json.dumps(result,indent=2)+'\n')
    print(f'Phase 48 passed: {result["signature_count"]} fixed signatures, {len(result["shared_firearm_virtuals"])} weapon vtables, 1 RTTI-verified client viewmodel vtable.')
    print('SiN SHA-256 verified; HL2 comparison '+('verified' if a.hl2 else 'skipped'))

if __name__=='__main__':
    main()
