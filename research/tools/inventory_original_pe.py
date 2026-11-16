#!/usr/bin/env python3
"""Read-only batch inventory for the original SiN Episodes retail PE32 DLLs.

Outputs addresses, counts, metadata and evidence classes only: NEVER
original DLL bytes, disassembly listings or proprietary native bodies.
Uses GNU objdump linear sweep; most discovered entries are UNVALIDATED.
"""
from __future__ import annotations

import argparse
import collections
import csv
import hashlib
import json
import re
import struct
import subprocess
import tempfile
import zipfile
from pathlib import Path

INSTRUCTION = re.compile(r'^\s*([0-9a-fA-F]{8,16}):\s+([a-z][a-z0-9.]*)\s*(.*?)\s*$')
DIRECT = re.compile(r'^0x([0-9a-fA-F]+)(?:\s|$)')


class PEError(ValueError):
    pass


class PE32:
    """Minimal bounds-checked PE32 metadata parser; no third-party deps."""

    def __init__(self, data: bytes):
        self.data = data
        if len(data) < 0x100 or data[:2] != b'MZ':
            raise PEError('missing MZ header')
        header = struct.unpack_from('<I', data, 0x3c)[0]
        if header + 0xf8 > len(data) or data[header:header + 4] != b'PE\x00\x00':
            raise PEError('missing PE signature')
        _, count, _, _, _, optional_size, _ = struct.unpack_from(
            '<HHIIIHH', data, header + 4)
        optional = header + 24
        if struct.unpack_from('<H', data, optional)[0] != 0x10b:
            raise PEError('requires 32-bit Windows PE')
        self.base = struct.unpack_from('<I', data, optional + 28)[0]
        self.entry_rva = struct.unpack_from('<I', data, optional + 16)[0]
        directory_count = struct.unpack_from('<I', data, optional + 92)[0]
        self.directories = {
            i: struct.unpack_from('<II', data, optional + 96 + i * 8)
            for i in range(min(directory_count, 16))
            if optional + 96 + (i + 1) * 8 <= optional + optional_size
        }
        self.sections = []
        offset = optional + optional_size
        if count > 96:
            raise PEError('implausible PE section count')
        for i in range(count):
            at = offset + 40 * i
            if at + 40 > len(data):
                raise PEError('section header outside file')
            name = data[at:at + 8].split(b'\x00', 1)[0].decode(
                'ascii', 'replace')
            length, rva, raw_length, disk = struct.unpack_from(
                '<IIII', data, at + 8)
            flags = struct.unpack_from('<I', data, at + 36)[0]
            self.sections.append({
                'name': name, 'rva': rva, 'size': length,
                'raw_size': raw_length, 'offset': disk,
                'executable': bool(flags & 0x20000000),
            })

    def section(self, va: int):
        rva = va - self.base
        return next((s for s in self.sections
                     if s['rva'] <= rva < s['rva'] +
                     max(s['size'], s['raw_size'])), None)

    def executable(self, va: int) -> bool:
        section = self.section(va)
        return bool(section and section['executable'])

    def at(self, va: int, size: int) -> bytes:
        section = self.section(va)
        if section is None:
            raise PEError(f'unmapped original VA {va:#x}')
        delta = va - self.base - section['rva']
        if delta < 0 or delta + size > section['raw_size']:
            raise PEError('original VA extends beyond raw section')
        file_offset = section['offset'] + delta
        if file_offset + size > len(self.data):
            raise PEError('original PE has truncated section')
        return self.data[file_offset:file_offset + size]

    def u16(self, va: int) -> int:
        return struct.unpack('<H', self.at(va, 2))[0]

    def u32(self, va: int) -> int:
        return struct.unpack('<I', self.at(va, 4))[0]

    def string(self, va: int, limit: int = 512) -> str:
        for length in range(limit):
            if self.at(va + length, 1) == b'\x00':
                return self.at(va, length).decode('ascii', 'replace')
        raise PEError('unterminated PE metadata string')

    def relocations(self) -> list[int]:
        rva, size = self.directories.get(5, (0, 0))
        if not rva:
            return []
        if size > len(self.data):
            raise PEError('relocation directory size invalid')
        result = []
        pos = 0
        while pos + 8 <= size:
            page, block = struct.unpack(
                '<II', self.at(self.base + rva + pos, 8))
            if not page and not block:
                break
            if block < 8 or block % 2 or pos + block > size:
                raise PEError('malformed PE32 relocation block')
            for inside in range(8, block, 2):
                value = self.u16(self.base + rva + pos + inside)
                if value >> 12 == 3:  # IMAGE_REL_BASED_HIGHLOW
                    result.append(self.base + page + (value & 0xfff))
            pos += block
        return sorted(set(result))

    def exports(self) -> list[dict]:
        rva, size = self.directories.get(0, (0, 0))
        if not rva:
            return []
        parts = struct.unpack('<IIHHIIIIIII',
                              self.at(self.base + rva, 40))
        ordinal_base, num_funcs, num_names, fn_rva, names_rva, ord_rva = (
            parts[5:11])
        if num_funcs > 50000 or num_names > 50000:
            raise PEError('oversized PE export table')
        names = {}
        for i in range(num_names):
            name_rva = self.u32(self.base + names_rva + i * 4)
            index = self.u16(self.base + ord_rva + i * 2)
            names[index] = self.string(self.base + name_rva)
        exports = []
        for i in range(num_funcs):
            address_rva = self.u32(self.base + fn_rva + i * 4)
            if address_rva:
                exports.append({
                    'address': f'0x{self.base + address_rva:08X}',
                    'name': names.get(i, ''),
                    'ordinal': ordinal_base + i,
                    'forwarded': rva <= address_rva < rva + size,
                })
        return exports

    def imports(self) -> list[dict]:
        rva, _ = self.directories.get(1, (0, 0))
        if not rva:
            return []
        imports = []
        for i in range(4096):
            original, _, _, name_rva, iat = struct.unpack(
                '<IIIII', self.at(self.base + rva + i * 20, 20))
            if not any((original, name_rva, iat)):
                break
            dll = self.string(self.base + name_rva)
            for j in range(50000):
                val = self.u32(self.base + (original or iat) + j * 4)
                if val == 0:
                    break
                symbol = ('ordinal_' + str(val & 0xffff)
                          if val & 0x80000000 else
                          self.string(self.base + val + 2))
                imports.append({
                    'dll': dll, 'symbol': symbol,
                    'iat_address': f'0x{self.base + iat + j * 4:08X}',
                })
            else:
                raise PEError('unterminated original import descriptor')
        return imports


def disassemble(path: Path, pe: PE32):
    """Extract only E8/E9-like linear-sweep metadata, not native code."""
    sections = [s['name'] for s in pe.sections if s['executable']]
    if not sections:
        return [], set(), set()
    cmd = ['objdump', '-D', '-M', 'intel', '--no-show-raw-insn']
    cmd += [f'--section={section}' for section in sections]
    cmd.append(str(path))
    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                            text=True, errors='replace')
    assert proc.stdout is not None
    edges = []
    padding_starts = set()
    decoded = set()
    previous_int3 = False
    for line in proc.stdout:
        match = INSTRUCTION.match(line)
        if not match:
            continue
        site = int(match.group(1), 16)
        mnemonic, operand = match.group(2), match.group(3)
        if not pe.executable(site):
            continue
        decoded.add(site)
        if previous_int3 and mnemonic != 'int3':
            padding_starts.add(site)
        previous_int3 = mnemonic == 'int3'
        if mnemonic not in ('call', 'jmp'):
            continue
        target = DIRECT.match(operand)
        if target:
            dst = int(target.group(1), 16)
            if pe.executable(dst):
                edges.append((site, dst, mnemonic))
    error = proc.stderr.read() if proc.stderr else ''
    if proc.wait() != 0:
        raise RuntimeError('objdump failed: ' + error[-600:])
    return edges, padding_starts, decoded


def discover(data: bytes, path: Path, reviewed: dict[int, str]):
    pe = PE32(data)
    exports = pe.exports()
    imports = pe.imports()
    relocs = pe.relocations()
    edges, padding, decoded = disassemble(path, pe)
    evidence: dict[int, set[str]] = collections.defaultdict(set)
    calls = collections.Counter()
    jumps = collections.Counter()
    pointers = collections.Counter()
    for entry in exports:
        va = int(entry['address'], 16)
        if pe.executable(va) and not entry['forwarded']:
            evidence[va].add('export')
    if pe.executable(pe.base + pe.entry_rva):
        evidence[pe.base + pe.entry_rva].add('pe_entry')
    for site, target, kind in edges:
        if kind == 'call':
            evidence[target].add('direct_call_target')
            calls[target] += 1
        else:
            jumps[target] += 1
    for site in relocs:
        section = pe.section(site)
        if section is None or section['executable'] or \
                section['name'] == '.reloc':
            continue
        try:
            target = pe.u32(site)
        except PEError:
            continue
        if pe.executable(target):
            evidence[target].add(
                'address_taken_' + section['name'].lstrip('.'))
            pointers[target] += 1
    for va in reviewed:
        evidence[va].add('sha_reviewed_catalog')
    for va in padding:
        evidence[va].add('after_int3')
    candidates = []
    for va, clues in sorted(evidence.items()):
        if not pe.executable(va):
            continue
        if 'sha_reviewed_catalog' in clues:
            level = 'reviewed_catalog'
        elif 'export' in clues or 'pe_entry' in clues:
            level = 'strong_anchor'
        elif 'direct_call_target' in clues and \
                any(c.startswith('address_taken_') for c in clues):
            level = 'corroborated_anchor'
        elif 'direct_call_target' in clues:
            level = 'call_target'
        elif any(c.startswith('address_taken_') for c in clues):
            level = 'address_taken'
        else:
            level = 'heuristic_int3_boundary'
        candidates.append({
            'address': f'0x{va:08X}', 'tier': level,
            'evidence': ';'.join(sorted(clues)),
            'call_sites': calls[va], 'tail_jumps': jumps[va],
            'data_pointer_references': pointers[va],
            'reviewed_label': reviewed.get(va, ''),
            'decoded_instruction_start': va in decoded,
        })
    levels = collections.Counter(c['tier'] for c in candidates)
    summary = {
        'sha256': hashlib.sha256(data).hexdigest(),
        'imagebase': f'0x{pe.base:08X}',
        'entrypoint': f'0x{pe.base + pe.entry_rva:08X}',
        'exports': len(exports), 'imports': len(imports),
        'relocations': len(relocs),
        'direct_call_instructions': sum(k == 'call' for _, _, k in edges),
        'direct_jump_instructions': sum(k == 'jmp' for _, _, k in edges),
        'function_entry_candidates': len(candidates),
        'candidate_tiers': dict(levels),
        'reviewed_entries_from_catalog': sum(
            pe.executable(va) for va in reviewed),
        'decoded_instruction_starts': len(decoded),
    }
    return summary, candidates, edges, exports, imports


def write_csv(path: Path, rows: list[dict], fields: list[str]):
    with path.open('w', newline='', encoding='utf-8') as stream:
        writer = csv.DictWriter(stream, fieldnames=fields)
        writer.writeheader()
        writer.writerows(rows)


def run(archive: Path, out: Path, catalog: Path | None = None):
    out.mkdir(parents=True, exist_ok=True)
    methods = json.loads(catalog.read_text(encoding='utf-8'))['methods'] \
        if catalog else []
    report = {
        'warning': 'All noncatalog candidate entrypoints are heuristic, '
        'not verified function boundaries, source or coverage percentages.',
        'modules': {},
    }
    with zipfile.ZipFile(archive) as z:
        for filename in ('client.dll', 'server.dll'):
            if z.namelist().count(filename) != 1:
                raise PEError('expected exactly one root ' + filename)
            data = z.read(filename)
            reviewed = {
                int(m['entry_va'], 16): m.get('label', '')
                for m in methods
                if m.get('dll') == filename.split('.')[0]
            }
            # Original game DLLs live only in a discarded temporary dir.
            with tempfile.TemporaryDirectory(prefix='sin_original_pe_') as tmp:
                source = Path(tmp) / filename
                source.write_bytes(data)
                summary, candidates, edges, exports, imports = discover(
                    data, source, reviewed)
            stem = filename.split('.')[0]
            report['modules'][filename] = summary
            write_csv(out / f'{stem}_function_candidates.csv', candidates,
                      ['address', 'tier', 'evidence', 'call_sites',
                       'tail_jumps', 'data_pointer_references',
                       'reviewed_label', 'decoded_instruction_start'])
            write_csv(
                out / f'{stem}_direct_edges.csv',
                [{'caller': f'0x{site:08X}',
                  'target': f'0x{target:08X}', 'kind': kind}
                 for site, target, kind in edges],
                ['caller', 'target', 'kind'])
            write_csv(out / f'{stem}_exports.csv', exports,
                      ['address', 'name', 'ordinal', 'forwarded'])
            write_csv(out / f'{stem}_imports.csv', imports,
                      ['dll', 'symbol', 'iat_address'])
    (out / 'summary.json').write_text(
        json.dumps(report, indent=2) + '\n', encoding='utf-8')
    lines = [
        '# Original SiN Episodes DLL discovery inventory',
        '',
        '**Candidates are not verified functions or decompilation progress.**',
        'GNU objdump linear sweep can misinterpret embedded data. '
        'An after-INT3 boundary is a particularly weak heuristic.',
        '',
    ]
    for name, summary in report['modules'].items():
        lines += [
            f'## {name}',
            f'- Original whole-file SHA256: {summary["sha256"]}',
            f'- Exports: {summary["exports"]}; imports: {summary["imports"]}; '
            f'relocations: {summary["relocations"]}',
            f'- Direct call instructions: {summary["direct_call_instructions"]}; '
            f'direct jumps: {summary["direct_jump_instructions"]}',
            f'- Catalogued SHA-reviewed entries: '
            f'{summary["reviewed_entries_from_catalog"]}',
            f'- Total unverified/verified entry candidates: '
            f'{summary["function_entry_candidates"]}',
            '- Tiers: ' + ', '.join(
                f'{k}={v}' for k, v in sorted(
                    summary['candidate_tiers'].items())),
            '',
        ]
    (out / 'README.md').write_text('\n'.join(lines), encoding='utf-8')
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--archive', type=Path, required=True)
    parser.add_argument('--catalog', type=Path)
    parser.add_argument('--out', type=Path, required=True)
    args = parser.parse_args()
    print(json.dumps(run(args.archive, args.out, args.catalog)['modules'],
                     indent=2))


if __name__ == '__main__':
    main()
