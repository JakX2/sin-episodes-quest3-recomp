#!/usr/bin/env python3
"""Aggregate Hammer output patterns from the user's original SiN SDK VMFs.

Static inspection only. Never emits raw targetnames, output values or map bytes.
Uses the Phase76 VPK v2 and CRC reader; not a Source entity I/O runtime.
"""
from collections import Counter
import argparse
import json
import math
from pathlib import Path
import re
import zipfile

from audit_sin_sdk import VPK_DIR, VPK_DATA, MAP_PATHS, MAP_MAX, SDKFormatError
from audit_sin_sdk import read_vpk_directory, vpk_file

ENTITY_PATTERN = re.compile(r'(?m)^entity\s*\r?\n\{\r?\n(.*?)^\}\r?$', re.S)
CLASS_PATTERN = re.compile(r'(?m)^\t"classname"\s+"([^"]+)"')
NAME_PATTERN = re.compile(r'(?m)^\t"targetname"\s+"([^"]+)"')
CONNECTION_PATTERN = re.compile(r'(?m)^\tconnections\r?\n\t\{\r?\n(.*?)^\t\}\r?$', re.S)
OUTPUT_PATTERN = re.compile(r'(?m)^\t\t"([^"]+)"\s+"([^"]*)"\s*$')


def audit_io_patterns(vmf: bytes) -> dict:
    if len(vmf) > MAP_MAX:
        raise SDKFormatError('VMF exceeds maximum size')
    text = vmf.decode('latin-1')
    blocks = ENTITY_PATTERN.findall(text)
    if len(blocks) != len(re.findall(r'(?m)^entity\s*$', text)):
        raise SDKFormatError('could not safely identify all top-level entities')
    names = Counter()
    entries = []
    for block in blocks:
        klass = CLASS_PATTERN.search(block)
        if not klass:
            raise SDKFormatError('entity without classname')
        name = NAME_PATTERN.search(block)
        if name:
            names[name.group(1).casefold()] += 1
        connection = CONNECTION_PATTERN.search(block)
        if connection:
            entries.extend(OUTPUT_PATTERN.findall(connection.group(1)))
    counts = Counter()
    output_names = Counter()
    for output, raw in entries:
        counts['connections'] += 1
        output_names[output] += 1
        parts = raw.split('\x1b' if '\x1b' in raw else ',')
        if len(parts) != 5:
            counts['wrong_field_count'] += 1
            continue
        target, input_name, _, delay, max_fires = (part.strip() for part in parts)
        if not target or not input_name:
            counts['empty_target_or_input'] += 1
            continue
        try:
            seconds = float(delay)
            fires = int(max_fires)
        except ValueError:
            counts['invalid_delay_or_fire_count'] += 1
            continue
        if not math.isfinite(seconds) or seconds < 0 or fires < -1:
            counts['invalid_delay_or_fire_count'] += 1
            continue
        if seconds > 0:
            counts['delayed'] += 1
        else:
            counts['immediate'] += 1
        if fires == -1:
            counts['unlimited'] += 1
        elif fires == 1:
            counts['one_shot'] += 1
        elif fires == 0:
            counts['disabled'] += 1
        else:
            counts['other_finite_fire_limit'] += 1
        if target.startswith('!'):
            counts['special_targets'] += 1
        elif '*' in target or target.startswith('@'):
            counts['wildcard_or_dynamic_targets'] += 1
        else:
            matched = names[target.casefold()]
            if matched == 0:
                counts['unresolved_literal_targets'] += 1
            elif matched == 1:
                counts['unique_literal_targets'] += 1
            else:
                counts['duplicate_name_targets'] += 1
    return {
        'scope': 'static_sdk_vmf_output_patterns_only_not_executed',
        'entity_count': len(blocks),
        'output_counts': {key: counts[key] for key in (
            'connections','immediate','delayed','unlimited','one_shot','disabled',
            'other_finite_fire_limit','special_targets','wildcard_or_dynamic_targets',
            'unique_literal_targets','duplicate_name_targets','unresolved_literal_targets',
            'wrong_field_count','empty_target_or_input','invalid_delay_or_fire_count')},
        'non_on_prefixed_output_connections': sum(value for key,value in output_names.items()
                                                   if not key.lower().startswith('on')),
        'notes': [
            'Literal name links are only static candidates: targets can be runtime-created or renamed.',
            'Multiple entities with a name are not necessarily errors.',
            'Wildcard and special target resolution requires a live Source engine host.',
            'No entity, targetname, parameter, or original map source is emitted.',
        ],
    }


def audit_zip(sdk_zip: Path) -> dict:
    with zipfile.ZipFile(sdk_zip) as archive:
        entries = read_vpk_directory(archive.read(VPK_DIR))
        packed = archive.read(VPK_DATA)
        return {name.split('/')[-1]: audit_io_patterns(vpk_file(entries, packed, name))
                for name in MAP_PATHS}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('sdk_zip', type=Path)
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    try:
        report = {'maps': audit_zip(args.sdk_zip),
                  'provenance': 'owner_supplied_original_SiN_Episodes_SDK_zip'}
    except (OSError, ValueError, KeyError, zipfile.BadZipFile) as e:
        parser.exit(1, 'SDK I/O AUDIT FAILED: ' + str(e) + '\n')
    formatted = json.dumps(report, indent=2, sort_keys=True) + '\n'
    if args.output:
        args.output.write_text(formatted, encoding='utf-8')
    else:
        print(formatted, end='')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
