#!/usr/bin/env python3
"""Verify SourceVR AArch64 filesystem module-search instruction signatures.

Provenance-locked, read-only static audit. Does NOT load/patch a native module,
execute APKs, prove linker namespace access, or certify a custom SiN plugin.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys

# The Phase 79 verifier provides independently tested ELF and instruction
# primitives; duplicate no proprietary module implementation.
from verify_sourcevr_loader import sections, xrefs, signed, VerificationError

PROFILES = {
    'hl2vr-0989-filesystem': {
        'sha256': '9641cca0307b152ee2b0d770962c69917a2ff70ff3e107e5281cb5c6fbf4389a',
        'env_refs': {'APP_LIB_PATH': 0x9a04c, 'APP_MOD_LIB': 0x9a05c},
        'formats': {'%s/lib%s': 0x99ec0, '%s/%s': 0x99eec},
        'loader_to_search': (0x5a9a8, 0x99f90),
        'search_to_dlopen_wrapper': (0x9a094, 0x99da4),
        'wrapper_to_dlopen_plt': (0x99e10, 0xbfe80),
    },
    'portalvr-112-filesystem': {
        'sha256': 'f3602bed1f9b80e9145e12a62cb0cc197d26511fe14bdbf0f8f103f2e6ecc7f8',
        'env_refs': {'APP_LIB_PATH': 0x8ab30, 'APP_MOD_LIB': 0x8ab40},
        'formats': {'%s/lib%s': 0x8a9a4, '%s/%s': 0x8a9d0},
        'loader_to_search': (0x4ce30, 0x8aa74),
        'search_to_dlopen_wrapper': (0x8ab78, 0x8a888),
        'wrapper_to_dlopen_plt': (0x8a8f4, 0xb0090),
    },
}


def decode_bl(opcode, address):
    if opcode & 0xfc000000 != 0x94000000:
        return None
    return address + (signed(opcode & 0x03ffffff, 26) << 2)


def verify_bytes(blob, profile):
    info = PROFILES[profile]
    if hashlib.sha256(blob).hexdigest() != info['sha256']:
        raise VerificationError('SHA-256 mismatch for ' + profile)
    mapping = sections(blob)
    addr, start, size = mapping['.text']
    raddr, rstart, rsize = mapping['.rodata']
    code, rodata = blob[start:start + size], blob[rstart:rstart + rsize]
    direct_refs = {}
    for name, expected in {**info['env_refs'], **info['formats']}.items():
        matches = xrefs(code, addr, rodata, raddr, name)
        if not any(pc == expected for pc, _ in matches):
            raise VerificationError('instruction reference mismatch for ' + name)
        direct_refs[name] = hex(expected)

    branches = {}
    for label in ('loader_to_search', 'search_to_dlopen_wrapper', 'wrapper_to_dlopen_plt'):
        callsite, target = info[label]
        relative = callsite - addr
        if not 0 <= relative <= len(code) - 4:
            raise VerificationError('callsite outside .text: ' + label)
        instruction = struct.unpack_from('<I', code, relative)[0]
        if decode_bl(instruction, callsite) != target:
            raise VerificationError('BL target mismatch: ' + label)
        branches[label] = {'callsite': hex(callsite), 'target': hex(target)}

    if b'dlopen\x00' not in blob or b'libdl.so\x00' not in blob:
        raise VerificationError('ELF missing expected libdl dependency/import strings')
    return {
        'profile': profile,
        'sha256_verified': True,
        'architecture': 'AArch64',
        'environment_references': {k: direct_refs[k] for k in info['env_refs']},
        'path_template_references': {k: direct_refs[k] for k in info['formats']},
        'verified_bl_edges': branches,
        'native_dlopen_import_present': True,
        'verified_scope': 'static filesystem module search path and dlopen callsite',
        'gamebin_used_as_native_directory_verified': False,
        'android_linker_acceptance_verified': False,
        'external_sin_game_module_loaded': False,
        'quest_runtime_verified': False,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--profile', required=True, choices=PROFILES)
    parser.add_argument('--filesystem', required=True, type=Path)
    args = parser.parse_args()
    try:
        result = verify_bytes(args.filesystem.read_bytes(), args.profile)
    except (OSError, VerificationError) as exc:
        print(json.dumps({'verified': False, 'error': str(exc)}))
        return 1
    print(json.dumps(result, indent=2, sort_keys=True))
    return 0


if __name__ == '__main__':
    sys.exit(main())
