#!/usr/bin/env python3
"""Safely produce an experimental *copy* of a SiN Source BSP with actor proxies.

This is a temporary stock-HL2 scene test, not a restoration of Ritual classes.
Never rewrites the input, never redistributes the game map. Usage:
  python sin_bsp_scene_actor_trial.py map.bsp --report
  python sin_bsp_scene_actor_trial.py map.bsp --write map_actor_trial.bsp
  python sin_bsp_scene_actor_trial.py map.bsp --verify map_actor_trial.bsp

Use only on lawfully obtained local BSPs; test the output in a separate mod
folder, with the correct original assets and scene cache mounted.
"""
import argparse
import pathlib
import re
import struct
import sys

HEADER = 8 + 64 * 16 + 4
ALIASES = {
    "npc_jc": "npc_citizen",        # tentative Source actor substitute
    "npc_jessica": "npc_alyx",       # tentative companion substitute
}
GENERIC = {"npc_generic_actor": "npc_citizen"}
CLASSNAME_RE = re.compile(rb'(?m)(^\s*"classname"\s*")([^"\r\n]+)(")')


class BspError(ValueError):
    pass


def read_bsp(blob: bytes):
    if len(blob) < HEADER or blob[:4] != b'VBSP':
        raise BspError("Not a valid little-endian VBSP header")
    version = struct.unpack_from('<i', blob, 4)[0]
    if not 17 <= version <= 22:
        raise BspError(f"Unsupported BSP version {version} (expected 17..22)")
    ofs, size, lumpversion, fourcc = struct.unpack_from('<iii4s', blob, 8)
    if ofs < HEADER or size <= 0 or ofs + size > len(blob):
        raise BspError("Invalid/overlapping entity-lump offset or length")
    if fourcc != b'\0\0\0\0':
        raise BspError("Compressed entity lumps are unsupported: refusing to guess")
    data = blob[ofs:ofs+size]
    if not data.endswith(b'\0'):
        raise BspError("Original entity lump lacks terminator")
    if b'{' not in data or b'"classname"' not in data:
        raise BspError("No standard entity text in lump 0")
    return version, ofs, size, lumpversion, data


def patch_entities(data: bytes, include_generic=False):
    replacements = dict(ALIASES)
    if include_generic:
        replacements.update(GENERIC)
    count = {k: 0 for k in replacements}
    def sub(m):
        name = m.group(2).decode('ascii', errors='ignore')
        if name not in replacements:
            return m.group(0)
        count[name] += 1
        return m.group(1) + replacements[name].encode() + m.group(3)
    patched = CLASSNAME_RE.sub(sub, data)
    return patched, count


def make_trial(blob: bytes, include_generic=False):
    version, offset, size, lumpversion, data = read_bsp(blob)
    changed, counts = patch_entities(data, include_generic)
    if not any(counts.values()):
        return None, counts
    # Appending a replacement lump avoids relocating any existing maps/lighting
    # lumps or recompressing unrecognized BSP payloads. Old lump is orphaned.
    padding = (-len(blob)) % 4
    position = len(blob) + padding
    out = bytearray(blob)
    out.extend(b'\0' * padding)
    out.extend(changed)
    struct.pack_into('<iii4s', out, 8, position, len(changed), lumpversion, b'\0'*4)
    # Structural safety checks only. Real Source-engine acceptance is untested.
    _, off2, size2, _, readback = read_bsp(bytes(out))
    assert off2 == position and size2 == len(changed) and readback == changed
    assert blob[8+16:HEADER] == out[8+16:HEADER]  # all non-entity lump headers and revision
    assert out[offset:offset+size] == data       # original entity payload untouched
    return bytes(out), counts


def main(argv=None):
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('bsp',type=pathlib.Path)
    action=ap.add_mutually_exclusive_group(required=True)
    action.add_argument('--report', action='store_true')
    action.add_argument('--write',type=pathlib.Path)
    action.add_argument('--verify',type=pathlib.Path,
                        help='check an existing trial copy byte-for-byte against the expected rewrite')
    ap.add_argument('--include-generic',action='store_true',help='also try npc_generic_actor -> npc_citizen')
    args=ap.parse_args(argv)
    source=args.bsp.resolve()
    try:
        data=source.read_bytes()
        result, counts=make_trial(data,args.include_generic)
        print(f"Source {source.name}: class rewrites {counts}")
        if result is None:
            if args.verify:
                raise BspError('No matching original actor classes; cannot verify a trial rewrite')
            print('No matching actor classnames; input unchanged')
            return 0
        if args.verify:
            target = args.verify.resolve()
            if target == source:
                raise BspError('Verification requires a separate experimental BSP copy')
            # Reconstruct the complete expected BSP from the untouched original.
            # This verifies the entity rewrite AND that all other bytes match.
            if target.read_bytes() != result:
                raise BspError('Trial BSP does not match the expected copy-only actor rewrite')
            print(f'VERIFIED experimental BSP copy: {target.name} (all {len(result)} bytes)')
            print('Structural verification only; HL2/SourceVR loading and scenes remain untested.')
            return 0
        if args.write:
            target=args.write.resolve()
            if target == source:
                raise BspError('Will not overwrite input BSP')
            # The --write request explicitly authorizes this destination. The
            # original input is never touched; parent directories may be new.
            target.parent.mkdir(parents=True, exist_ok=True)
            try:
                # Atomic existence check: no race between exists() and write().
                # 'xb' also refuses to follow an existing destination symlink.
                with target.open('xb') as output:
                    output.write(result)
            except FileExistsError:
                raise BspError('Will not overwrite existing output BSP')
            print(f"Created experimental map COPY: {target} ({len(result)} bytes)")
            print('Not validated against Source runtime; original SiN AI and scenes remain unresolved.')
        else:
            print('Dry-run only. Use --write OUTPUT.bsp to create an isolated copy.')
        return 0
    except (OSError, BspError) as e:
        print('Error:',e,file=sys.stderr)
        return 2


if __name__=='__main__':
    sys.exit(main())