#!/usr/bin/env python3
"""Phase177: pin the ORIGINAL 2006 x86 server/client PE exports and interface ABI.

Works with owner-local DLL files only. Checks complete original DLL SHA256s,
complete native registration constructor and CreateInterface function SHA256s,
actual PE export tables, and every native compiler-generated registration stub.
The original binaries and disassembly must not be checked into the repository.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
from pathlib import Path
class VerificationError(ValueError):
    pass


class PEImage:
    """Minimal read-only original PE32 section mapper, standard library only."""
    def __init__(self, data: bytes):
        self.data = data
        if len(data) < 0x100 or data[:2] != b'MZ':
            raise VerificationError('not a valid DOS/PE image')
        pe = struct.unpack_from('<I', data, 0x3c)[0]
        if pe + 24 > len(data) or data[pe:pe+4] != b'PE\x00\x00':
            raise VerificationError('invalid PE signature')
        machine, sections = struct.unpack_from('<HH', data, pe+4)
        optional_size = struct.unpack_from('<H', data, pe+20)[0]
        opt = pe + 24
        if machine != 0x14c or not (1 <= sections <= 96) or opt+optional_size+sections*40 > len(data):
            raise VerificationError('expected original 32-bit x86 PE image')
        if struct.unpack_from('<H', data, opt)[0] != 0x10b:
            raise VerificationError('expected PE32 optional header')
        self.base = struct.unpack_from('<I', data, opt+28)[0]
        self.sections = []
        for i in range(sections):
            start = opt+optional_size+i*40
            name = data[start:start+8].split(b'\x00')[0].decode('ascii', 'replace')
            virtual_size, rva, raw_size, raw_offset = struct.unpack_from('<IIII', data, start+8)
            self.sections.append((name, rva, virtual_size, raw_offset, raw_size))

    def section_of(self, va: int) -> str:
        rva = va-self.base
        for name, start, vsize, offset, raw_size in self.sections:
            if start <= rva < start + max(vsize,raw_size):
                return name
        return ''

    def at(self, va: int, size: int) -> bytes:
        if size < 0: raise VerificationError('negative section read')
        rva=va-self.base
        for name, start, vsize, offset, raw_size in self.sections:
            if start <= rva and rva-start+size <= raw_size:
                file_offset=offset+rva-start
                if file_offset+size <= len(self.data):
                    return self.data[file_offset:file_offset+size]
        raise VerificationError(f'VA outside original raw sections: {va:#x}')

ORIGINALS = {
    'server': {
        'sha256': '568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650',
        'base': 0x22000000, 'constructor': 0x221FB5D0,
        'export': 0x221FB5F0, 'head': 0x22573C5C,
        'registrations': 10,
        'registration_manifest_digest': '586dc57ff45e59d4b10e313245a120c02643418eea34403544bdd1edb480d461',
        'exports': {
            '?Detonate@CGrenadeHomer@@UAEXXZ': 0x2230F4E0,
            '?Detonate@CGrenadePathfollower@@UAEXXZ': 0x22310EA0,
            '?Detonate@CGrenadeSpitMutant@@UAEXXZ': 0x223120B0,
            '?Detonate@CGrenadeSpitPoison@@UAEXXZ': 0x22312B70,
            'CreateInterface': 0x221FB5F0,
        },
        'required': {
            'IEffects001', 'ServerGameDLL003', 'ServerGameEnts001',
            'ServerGameClients003', 'PluginHelpersCheck001',
            'PlayerInfoManager001', 'PlayerInfoManager002', 'BotManager001',
            'HLTVDirector001', 'GameMovement001',
        },
        'constructor_digest': '5e4a38bbe846cdfddece56859b823676d516af3f171181d7415ebbdfc1376654',
        'dispatch_digest': '39ed7fe9be839faa563964d7f5c2ad810dcd1992511131659da7fe823194b98a',
    },
    'client': {
        'sha256': '6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d',
        'base': 0x24000000, 'constructor': 0x24164160,
        'export': 0x24164180, 'head': 0x24369378,
        'registrations': 55,
        'registration_manifest_digest': '88c9b99ce45befa36461be556e49d0f4abdb47c9cd759c80db74ad82dd3034a9',
        'exports': {'CreateInterface': 0x24164180},
        'required': {
            'GameClientExports001', 'VClient011', 'VClientEntityList003',
            'VClientPrediction001', 'GameMovement001', 'ClientLeafSystem001',
            'VCENTERPRINT002', 'IEffects001',
        },
        'constructor_digest': '48699a1f29f3e63113789813739fcb588bfdc154be3047b2a5e712ea873c1a80',
        'dispatch_digest': 'f74837ce59ab917becd4c214e627ed91762085cb52883617278a206118267eec',
    },
}


def _u16(b: bytes, off: int = 0) -> int:
    return struct.unpack_from('<H', b, off)[0]


def _u32(b: bytes, off: int = 0) -> int:
    return struct.unpack_from('<I', b, off)[0]


def read_name(pe: PEImage, absolute_va: int, maximum: int = 140) -> str:
    if pe.section_of(absolute_va) != '.rdata':
        raise VerificationError(f'original interface name not in .rdata: {absolute_va:#x}')
    out = bytearray()
    for i in range(maximum):
        b = pe.at(absolute_va + i, 1)[0]
        if b == 0:
            try: return out.decode('ascii')
            except UnicodeDecodeError as exc:
                raise VerificationError('non-ASCII original interface name') from exc
        if b < 0x20 or b > 0x7e:
            raise VerificationError('invalid original interface name character')
        out.append(b)
    raise VerificationError('unterminated original interface name')


def parse_pe_exports(pe: PEImage) -> dict[str, int]:
    data = pe.data
    pe_offset = _u32(data, 0x3c)
    optional = pe_offset + 24
    export_rva, export_size = struct.unpack_from('<II', data, optional + 96)
    if not export_rva or export_size < 40:
        raise VerificationError('missing original PE export directory')
    va = pe.base + export_rva
    if pe.section_of(va) != '.rdata':
        raise VerificationError('original exports not in .rdata')
    (_, _, _, _, name_rva, ordinal_base, function_count, name_count,
     functions_rva, names_rva, ordinals_rva) = struct.unpack(
         '<IIHHIIIIIII', pe.at(va, 40))
    if not (0 < function_count <= 128 and 0 < name_count <= function_count):
        raise VerificationError('unexpected PE export table cardinality')
    dll_name = read_name(pe, pe.base + name_rva)
    if dll_name not in ('server.dll', 'client.dll'):
        raise VerificationError('wrong original DLL export name')
    exported = {}
    for i in range(name_count):
        name_ptr = _u32(pe.at(pe.base + names_rva + 4*i, 4))
        export_name = read_name(pe, pe.base + name_ptr)
        ordinal_index = _u16(pe.at(pe.base + ordinals_rva + 2*i, 2))
        if ordinal_index >= function_count:
            raise VerificationError('export ordinal beyond address table')
        function_rva = _u32(pe.at(pe.base + functions_rva + 4*ordinal_index, 4))
        if export_rva <= function_rva < export_rva + export_size:
            raise VerificationError('unexpected forwarded original export')
        method_va = pe.base + function_rva
        if pe.section_of(method_va) != '.text':
            raise VerificationError('original exported function outside code')
        if export_name in exported:
            raise VerificationError('duplicated original export name')
        exported[export_name] = method_va
    if len(exported) != name_count or ordinal_base != 1:
        raise VerificationError('unexpected PE export ordinals')
    return exported


def scan_static_registration_wrappers(pe: PEImage, constructor_va: int) -> list[dict]:
    """Read 21-byte MSVC static-registration wrappers, not inferred strings.

    Each native wrapper is: PUSH string, PUSH factory, MOV ECX node,
    CALL original thiscall constructor, RET (or POP EBP; RET). These are selected by the
    *actual resolved rel32 CALL target*, then all operands are validated.
    """
    section = next((s for s in pe.sections if s[0] == '.text'), None)
    if section is None: raise VerificationError('original DLL lacks code section')
    _, virtual_rva, virtual_size, file_offset, file_size = section
    size = min(virtual_size, file_size)
    raw = pe.data[file_offset:file_offset + size]
    records: list[dict] = []
    for i in range(15, len(raw) - 5):
        if raw[i] != 0xe8:
            continue
        call_va = pe.base + virtual_rva + i
        destination = call_va + 5 + struct.unpack_from('<i', raw, i+1)[0]
        if destination != constructor_va:
            continue
        start = i - 15
        if not (raw[start] == 0x68 and raw[start+5] == 0x68
                and raw[start+10] == 0xb9 and
                (raw[i+5:i+6] == b'\xc3' or raw[i+5:i+7] == b'\x5d\xc3')):
            raise VerificationError(f'noncanonical original registration wrapper: {call_va:#x}')
        name_va = _u32(raw, start+1)
        factory_va = _u32(raw, start+6)
        node_va = _u32(raw, start+11)
        if pe.section_of(factory_va) != '.text' or pe.section_of(node_va) != '.data':
            raise VerificationError(f'factory/node in unexpected original section at {call_va:#x}')
        # Native constructor writes three WORD-sized pointers (12 bytes).
        if not (pe.section_of(node_va+11) == '.data'):
            raise VerificationError('interface registration node spans uninitialized section')
        name = read_name(pe, name_va)
        records.append({
            'interface': name,
            'registration_va': pe.base + virtual_rva + start,
            'name_va': name_va,
            'factory_va': factory_va,
            'registration_node_va': node_va,
        })
    records.sort(key=lambda rec: rec['registration_va'])
    if len({r['registration_node_va'] for r in records}) != len(records):
        raise VerificationError('nonunique original interface nodes')
    return records


def verify_original_interface_abi(data: bytes, dll: str) -> dict:
    conf = ORIGINALS[dll]
    if hashlib.sha256(data).hexdigest() != conf['sha256']:
        raise VerificationError(f'wrong original {dll}.dll full SHA256')
    pe = PEImage(data)
    if pe.base != conf['base']:
        raise VerificationError(f'wrong original {dll}.dll PE image base')
    exports = parse_pe_exports(pe)
    if exports != conf['exports']:
        raise VerificationError(f'original {dll}.dll export contract mismatch')
    constructor, dispatcher = conf['constructor'], conf['export']
    for va, size, digest in (
        (constructor, 32, conf['constructor_digest']),
        (dispatcher, 114, conf['dispatch_digest']),
    ):
        if hashlib.sha256(pe.at(va, size)).hexdigest() != digest:
            raise VerificationError(f'original {dll}.dll complete native function digest mismatch: {va:#x}')
    # Constructor: copies factory/name into node and chains it at head;
    # dispatcher: loads the correct module-local linked-list head.
    if pe.at(constructor+15, 6) != b'\x8b\x0d' + struct.pack('<I', conf['head']):
        raise VerificationError('native interface constructor list-head read mismatch')
    if pe.at(constructor+24, 5) != b'\xa3' + struct.pack('<I', conf['head']):
        raise VerificationError('native interface constructor list-head store mismatch')
    if pe.at(dispatcher+3, 6) != b'\x8b\x3d' + struct.pack('<I', conf['head']):
        raise VerificationError('native CreateInterface list-head read mismatch')
    if pe.at(dispatcher+110, 4) != b'\x5f\x5e\x5d\xc3':
        raise VerificationError('native CreateInterface ending changed')
    wrappers = scan_static_registration_wrappers(pe, constructor)
    names = {row['interface'] for row in wrappers}
    if len(wrappers) != conf['registrations'] or not conf['required'].issubset(names):
        raise VerificationError(f'original {dll}.dll static interface registrations changed')
    # Complete ordered-registration manifest digest protects factory pointers,
    # static registration storage, and all supported interface versions.
    canonical = '\n'.join(
        f"{r['interface']}|{r['registration_va']:08x}|{r['name_va']:08x}|"
        f"{r['factory_va']:08x}|{r['registration_node_va']:08x}"
        for r in wrappers
    ).encode('ascii')
    manifest_digest = hashlib.sha256(canonical).hexdigest()
    if manifest_digest != conf['registration_manifest_digest']:
        raise VerificationError(f'original {dll}.dll registration site/factory/node manifest mismatch')
    return {
        'dll': f'{dll}.dll', 'image_base': f"{pe.base:#x}",
        'complete_original_dll_hash_verified': True,
        'complete_native_constructor_hash_verified': True,
        'complete_native_CreateInterface_hash_verified': True,
        'native_registration_node_stride_bytes': 12,
        'native_constructor_va': f'{constructor:#x}',
        'native_CreateInterface_va': f'{dispatcher:#x}',
        'native_registration_head_va': f"{conf['head']:#x}",
        'pe_exports': {k: hex(v) for k, v in exports.items()},
        'static_registration_count': len(wrappers),
        'ordered_registration_manifest_sha256': manifest_digest,
        'interface_registrations': wrappers,
    }


def audit(client_bytes: bytes, server_bytes: bytes) -> dict:
    return {
        'phase': 177,
        'status': 'original_win32_dll_entrypoints_verified',
        'server': verify_original_interface_abi(server_bytes, 'server'),
        'client': verify_original_interface_abi(client_bytes, 'client'),
        'runtime_limitations': [
            'Only native export/registration entrypoints and linked-list dispatch are reconstructed.',
            'Original MSVC32 interface object vtables and engine callback methods remain unresolved.',
            'Synthetic Win32 loader probe does not instantiate a real SiN engine interface.',
        ],
    }


def main() -> None:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--server', type=Path, required=True)
    p.add_argument('--client', type=Path, required=True)
    p.add_argument('--output', type=Path)
    args = p.parse_args()
    report = json.dumps(audit(args.client.read_bytes(), args.server.read_bytes()), indent=2) + '\n'
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(report, encoding='utf-8')
    else:
        print(report, end='')


if __name__ == '__main__':
    main()