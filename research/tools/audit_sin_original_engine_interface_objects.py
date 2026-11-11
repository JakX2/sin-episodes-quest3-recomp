#!/usr/bin/env python3
"""Phase178: verify original 2006 MSVC32 interface factory singletons/vtables.

Read-only, owner-original PE32 SHA verification. Source-tree manifest records
addresses and RTTI names, never original executable bodies or commercial assets.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
from pathlib import Path
from audit_sin_original_dll_interface_abi import (
    PEImage, VerificationError, audit as phase177_audit,
)

MANIFEST_FILE = Path(__file__).resolve().parents[1] / 'catalog' / 'original_dll_engine_interface_objects.json'
MANIFEST_SHA256 = 'cbe177e1e7277179e18e16a6255c3c44d7f0be695ba3e98749b5b04686b73c13'
EXPECTED_COUNT = {'server': 4, 'client': 5}
EXPECTED_STATIC = {'server': 3, 'client': 2}
EXPECTED_STATIC_SLOTS = {'server': 53, 'client': 52}


def original_manifest() -> dict:
    data = MANIFEST_FILE.read_bytes()
    if hashlib.sha256(data).hexdigest() != MANIFEST_SHA256:
        raise VerificationError('Phase178 manifest SHA256 mismatch')
    return json.loads(data)


def _read_u32(pe: PEImage, va: int) -> int:
    return struct.unpack('<I', pe.at(va, 4))[0]


def verify_original_objects(pe: PEImage, dll: str, records: list[dict],
                            registered: dict[str, int], *, strict_digest: bool = True) -> dict:
    """Verify all native 6-byte singleton thunks and initialized RTTI/vtables.

    Original PE section contents cannot show C++ constructors' runtime writes
    to uninitialized static data. Those interfaces remain intentionally unknown.
    """
    if len(records) != EXPECTED_COUNT[dll]:
        raise VerificationError('wrong number of pinned original interface factories')
    seen, static, pointers = set(), 0, 0
    for rec in records:
        name = rec['interface']
        if name in seen: raise VerificationError('duplicate original interface name')
        seen.add(name)
        fva = int(rec['factory_va'], 16)
        obj = int(rec['singleton_va'], 16)
        if registered.get(name) != fva:
            raise VerificationError('original static registration/factory mismatch: ' + name)
        if pe.section_of(fva) != '.text' or pe.section_of(obj) != '.data':
            raise VerificationError('original factory/object PE section mismatch: ' + name)
        code = pe.at(fva, 6)
        if code[0] != 0xb8 or code[5] != 0xc3 or struct.unpack_from('<I', code, 1)[0] != obj:
            raise VerificationError('original singleton factory MOV EAX/RET mismatch: ' + name)
        if strict_digest and hashlib.sha256(code).hexdigest() != rec['factory_sha256']:
            raise VerificationError('original factory complete code hash mismatch: ' + name)
        if rec['factory_len'] != 6:
            raise VerificationError('wrong original factory function length')
        if rec['vtable_storage'] == 'runtime_uninitialized':
            if rec['static_vtable_va'] is not None or rec['static_slots']:
                raise VerificationError('fabricated native BSS vtable: ' + name)
            try:
                pe.at(obj, 4)
            except VerificationError:
                # The native .data virtual address is outside raw file-backed
                # content; its vptr must be filled by initialization at runtime.
                pass
            else:
                raise VerificationError('original supposedly-uninitialized singleton has raw vptr')
            continue
        if rec['vtable_storage'] != 'pe_initialized':
            raise VerificationError('unknown original vtable storage type')
        static += 1
        vt = int(rec['static_vtable_va'], 16)
        if pe.section_of(vt) != '.rdata' or _read_u32(pe, obj) != vt:
            raise VerificationError('original static object/vtable pointer mismatch: ' + name)
        ptrs = [int(x, 16) for x in rec['static_slots']]
        if len(ptrs) != rec['vtable_prefix_code_slots'] or not ptrs:
            raise VerificationError('invalid original vtable prefix length')
        if any(pe.section_of(x) != '.text' for x in ptrs):
            raise VerificationError('original vtable entry outside executable section')
        raw = pe.at(vt, 4*len(ptrs))
        if struct.unpack('<' + 'I'*len(ptrs), raw) != tuple(ptrs):
            raise VerificationError('original vtable slot pointer mismatch: ' + name)
        if strict_digest and hashlib.sha256(raw).hexdigest() != rec['vtable_prefix_sha256']:
            raise VerificationError('original vtable complete prefix SHA mismatch: ' + name)
        next_word = _read_u32(pe, vt + 4*len(ptrs))
        if next_word != int(rec['first_noncode_word_after_prefix'], 16) or pe.section_of(next_word) == '.text':
            raise VerificationError('native vtable prefix/noncode boundary mismatch: ' + name)
        col = int(rec['msvc_rtti_col_va'], 16)
        if _read_u32(pe, vt - 4) != col or pe.section_of(col) != '.rdata':
            raise VerificationError('original MSVC RTTI COL vtable[-1] mismatch: ' + name)
        colraw = pe.at(col, 20)
        signature, offset, cd_offset, typedesc, hierarchy = struct.unpack('<IIIII', colraw)
        if (signature, offset, cd_offset) != (0,0,0):
            raise VerificationError('original MSVC32 RTTI COL header mismatch: ' + name)
        if typedesc != int(rec['msvc_type_descriptor_va'], 16) or hierarchy != int(rec['msvc_class_hierarchy_va'], 16):
            raise VerificationError('original MSVC32 RTTI type/hierarchy pointer mismatch: ' + name)
        if strict_digest and hashlib.sha256(colraw).hexdigest() != rec['msvc_rtti_col_sha256']:
            raise VerificationError('original RTTI COL whole-record SHA mismatch: ' + name)
        type_name = (rec['msvc_type_name'] + '\x00').encode('ascii')
        if pe.at(typedesc+8, len(type_name)) != type_name:
            raise VerificationError('original MSVC RTTI type name mismatch: ' + name)
        pointers += len(ptrs)
    if static != EXPECTED_STATIC[dll] or pointers != EXPECTED_STATIC_SLOTS[dll]:
        raise VerificationError('unexpected original static vtable coverage')
    return {'original_factory_body_hashes_verified':len(records) if strict_digest else 0,
            'original_factory_bytes_checked':len(records)*6,
            'original_static_singletons':static,
            'original_runtime_initialized_singletons':len(records)-static,
            'original_static_vtable_method_pointers':pointers,
            'original_complete_rtti_col_records':static,
            'limitations':['Native vtable slot names, arg types and execution remain unproven',
                           'Runtime-only singleton vptrs cannot be recovered from raw file']}


def audit(client: bytes, server: bytes) -> dict:
    report = phase177_audit(client, server)
    manifest = original_manifest()
    for dll, blob in [('server',server),('client',client)]:
        pe = PEImage(blob)
        registration = {r['interface']:r['factory_va'] for r in report[dll]['interface_registrations']}
        report[dll]['engine_interface_singletons'] = verify_original_objects(
            pe, dll, manifest['dlls'][dll]['interfaces'], registration)
    report['phase'] = 178
    report['original_engine_interface_manifest_sha256'] = MANIFEST_SHA256
    return report


def main() -> None:
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--server',type=Path,required=True)
    p.add_argument('--client',type=Path,required=True)
    p.add_argument('--output',type=Path)
    args=p.parse_args()
    data=json.dumps(audit(args.client.read_bytes(),args.server.read_bytes()),indent=2)+'\n'
    if args.output:
        args.output.parent.mkdir(parents=True,exist_ok=True)
        args.output.write_text(data,encoding='utf-8')
    else:
        print(data,end='')
if __name__ == '__main__':main()