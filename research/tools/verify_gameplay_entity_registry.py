#!/usr/bin/env python3
"""Phase 73: verify selected original SiN server entity factory registrations.

Verifies short original x86 registration *instruction patterns*, not gameplay
implementation, static map placement, or Android engine support. Do not publish
original DLL bytes. This verifier is standard-library only.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
from pathlib import Path

SERVER_SHA256 = '568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650'
CLIENT_SHA256 = '6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d'
REGISTRY_GETTER = 0x2228B110

# Original 2006 server.dll x86 VAs: registration function, classname C string,
# class factory object. This is a deliberately selected subset, not an exhaustive
# list of all registered Source/SiN entities.
FACTORIES = (
  ('logic_auto', 'logic', 0x223BC2F0, 0x2241F360, 0x224FB288),
  ('logic_relay', 'logic', 0x223BC5C0, 0x22422BBC, 0x224FDA88),
  ('logic_timer', 'logic', 0x223BC350, 0x224227E0, 0x224FB604),
  ('logic_case', 'logic', 0x223BC4A0, 0x22422834, 0x224FB620),
  ('logic_branch', 'logic', 0x223BC500, 0x22422850, 0x224FB628),
  ('logic_compare', 'logic', 0x223BC4D0, 0x22422840, 0x224FB624),
  ('math_counter', 'logic', 0x223BC470, 0x22422824, 0x224FB61C),
  ('env_global', 'logic', 0x223BC420, 0x22422818, 0x224FB614),
  ('trigger_changelevel', 'transition', 0x223BE170, 0x22437EAC, 0x2250B3C0),
  ('trigger_transition', 'transition', 0x223BE150, 0x22438048, 0x2250B3BC),
  ('info_landmark', 'transition', 0x223BDBC0, 0x224017D4, 0x22508B38),
  ('trigger_multiple', 'trigger', 0x223BE0D0, 0x2243C9A0, 0x2250B3B0),
  ('trigger_once', 'trigger', 0x223BE100, 0x2243C9B4, 0x2250B3B4),
  ('trigger_teleport', 'trigger', 0x223BE1D0, 0x2240A67C, 0x2250B3C8),
  ('trigger_hurt', 'trigger', 0x223BE0B0, 0x2243C990, 0x2250B3AC),
  ('trigger_look', 'trigger', 0x223BE120, 0x2243C9C4, 0x2250B3B8),
  ('trigger_dynamic_difficulty', 'sin_specific', 0x223BE4A0, 0x2243CAF4, 0x2250B404),
  ('npc_sintek_grunt', 'sin_specific', 0x223C77A0, 0x224A49BC, 0x22537CEC),
  ('npc_sintek_jetpack', 'sin_specific', 0x223C77C0, 0x224A529C, 0x22537D50),
  ('npc_maker', 'npc_script', 0x223BC750, 0x2242489C, 0x224FE52C),
  ('npc_template_maker', 'npc_script', 0x223BC780, 0x224248A8, 0x224FE530),
  ('scripted_sequence', 'npc_script', 0x223BD490, 0x2242EE1C, 0x22505008),
  ('aiscripted_schedule', 'npc_script', 0x223BD4C0, 0x2242FE60, 0x2250500C),
  ('logic_choreographed_scene', 'npc_script', 0x223BD380, 0x2242ED3C, 0x22503AC4),
  ('point_template', 'spawn', 0x223BC960, 0x22427024, 0x224FFBDC),
  ('env_entity_maker', 'spawn', 0x223BBB20, 0x224133D4, 0x224F7C48),
  ('game_player_equip', 'spawn', 0x223BF990, 0x22445140, 0x22510B24),
  ('func_door', 'interaction', 0x223BB500, 0x22410428, 0x224F4260),
  ('func_button', 'interaction', 0x223BB2B0, 0x2240EC1C, 0x224F2A3C),
  ('func_movelinear', 'interaction', 0x223BEB90, 0x224417B8, 0x2250F8FC),
  ('path_corner', 'interaction', 0x223B90B0, 0x223F3368, 0x224E45F4),
  ('path_track', 'interaction', 0x223B9110, 0x223F3394, 0x224E47B8),
)

class VerificationError(ValueError): pass

class PEImage:
    def __init__(self, data: bytes):
        self.data = data
        if len(data) < 0x100 or data[:2] != b'MZ':
            raise VerificationError('not a DOS/PE DLL')
        pe = struct.unpack_from('<I', data, 0x3c)[0]
        if pe + 24 > len(data) or data[pe:pe+4] != b'PE\0\0':
            raise VerificationError('invalid PE header')
        machine, sections = struct.unpack_from('<HH', data, pe+4)
        if machine != 0x14c or not 1 <= sections <= 96:
            raise VerificationError('expected original PE32 x86 image')
        optional_size = struct.unpack_from('<H', data, pe+20)[0]
        opt = pe + 24
        if opt + optional_size + sections * 40 > len(data) or struct.unpack_from('<H', data, opt)[0] != 0x10b:
            raise VerificationError('bad optional header')
        self.base = struct.unpack_from('<I', data, opt+28)[0]
        self.sections = []
        for i in range(sections):
            s = opt + optional_size + i*40
            name = data[s:s+8].split(b'\0')[0].decode('ascii', 'replace')
            vsize, va, raw_size, raw_off = struct.unpack_from('<IIII', data, s+8)
            self.sections.append((name, va, vsize, raw_off, raw_size))
    def offset(self, va: int, count: int) -> int:
        for name, sv, virtual_size, raw, size in self.sections:
            rva = va - self.base
            if sv <= rva and rva - sv + count <= min(size, max(virtual_size,size)):
                start = raw + rva - sv
                if start + count <= len(self.data): return start
        raise VerificationError(f'VA {va:#x} not mapped into a raw section')
    def at(self, va: int, count: int) -> bytes:
        i = self.offset(va, count)
        return self.data[i:i+count]
    def section_of(self, va: int) -> str:
        for name, sv, virtual_size, raw, size in self.sections:
            if sv <= va-self.base < sv+max(virtual_size,size): return name
        return ''


def verify_registrations(server: bytes, *, check_hash: bool = True) -> list[dict]:
    if check_hash and hashlib.sha256(server).hexdigest() != SERVER_SHA256:
        raise VerificationError('original server.dll SHA-256 mismatch')
    pe = PEImage(server)
    if pe.base != 0x22000000:
        raise VerificationError('unexpected server PE image base')
    rows=[]
    for name, category, register_va, name_va, factory_va in FACTORIES:
        if pe.section_of(register_va) != '.text' or pe.section_of(name_va) != '.rdata' or pe.section_of(factory_va) != '.data':
            raise VerificationError(f'{name}: invalid section for registration/name/factory')
        if pe.at(name_va, len(name)+1) != name.encode('ascii')+b'\x00':
            raise VerificationError(f'{name}: original entity classname mismatch')
        code = pe.at(register_va, 22)
        # call registry getter; mov edx,[eax]; push literal name; push factory;
        # mov ecx,eax; call virtual register; ret
        if not (code[0] == 0xe8 and code[5:8] == b'\x8b\x10\x68' and
                code[12] == 0x68 and code[17:] == b'\x8b\xc8\xff\x12\xc3'):
            raise VerificationError(f'{name}: registration call shape changed')
        target = register_va + 5 + struct.unpack_from('<i', code, 1)[0]
        name_ptr = struct.unpack_from('<I', code, 8)[0]
        factory_ptr = struct.unpack_from('<I', code, 13)[0]
        if (target, name_ptr, factory_ptr) != (REGISTRY_GETTER, name_va, factory_va):
            raise VerificationError(f'{name}: registration call target/name/factory mismatch')
        rows.append({'classname':name,'category':category,'registration_va':f'{register_va:#x}',
                     'name_va':f'{name_va:#x}','factory_object_va':f'{factory_va:#x}'})
    return rows


def verify(server: bytes, client: bytes) -> dict:
    if hashlib.sha256(client).hexdigest() != CLIENT_SHA256:
        raise VerificationError('original client.dll SHA-256 mismatch')
    rows = verify_registrations(server)
    return {'phase':73,'result':'pass','evidence':'Original 2006 registered entity factory calls only; no original BSP maps supplied.',
            'server_sha256':SERVER_SHA256,'client_sha256':CLIENT_SHA256,
            'selected_entity_factories':len(rows),'factories':rows}


def main() -> int:
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('server',type=Path);parser.add_argument('client',type=Path)
    parser.add_argument('--output',type=Path,default=None)
    args=parser.parse_args()
    try:
        result=verify(args.server.read_bytes(),args.client.read_bytes())
    except (OSError,VerificationError,struct.error) as exc:
        parser.exit(1,f'PHASE 73 VERIFICATION FAILED: {exc}\n')
    output=json.dumps(result,indent=2)+'\n'
    if args.output: args.output.write_text(output)
    else: print(output,end='')
    return 0

if __name__=='__main__':raise SystemExit(main())
