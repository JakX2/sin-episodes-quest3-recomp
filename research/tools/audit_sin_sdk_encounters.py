#!/usr/bin/env python3
"""Read-only aggregate encounter fields in privately held SiN SDK VMFs.

Original map bytes, targets, output payloads and positions are never emitted.
This is not compiled BSP gameplay, a spawn simulator or difficulty parity.
"""
import argparse
from collections import Counter
import json
import math
from pathlib import Path
import re
import zipfile

from audit_sin_sdk import MAP_PATHS, VPK_DIR, VPK_DATA, MAP_MAX
from audit_sin_sdk import read_vpk_directory, vpk_file, SDKFormatError

ENTITY = re.compile(r'(?m)^entity\s*\r?\n\{\r?\n(.*?)^\}\r?$', re.S)
FIELD = re.compile(r'(?m)^\t"([^"\r\n]+)"\s+"([^"]*)"\s*$')


def vmf_encounters(blob):
    if len(blob) > MAP_MAX:
        raise SDKFormatError('VMF larger than maximum')
    s = blob.decode('latin1')
    blocks = ENTITY.findall(s)
    if len(blocks) != len(re.findall(r'(?m)^entity\s*$', s)):
        raise SDKFormatError('malformed top-level VMF entity block')
    tiers = Counter()
    times = []
    maker_limits = Counter()
    maker_frequencies = Counter()
    maker_disabled = Counter()
    result = {'entity_count': len(blocks), 'checkpoint_count': 0,
              'checkpoint_repeat_flag_count': 0, 'checkpoint_endgame_count': 0,
              'maker_count': 0, 'maker_no_live_limit_count': 0}
    for block in blocks:
        values = dict(FIELD.findall(block))
        kind = values.get('classname')
        if kind == 'trigger_dynamic_difficulty':
            result['checkpoint_count'] += 1
            try:
                tier = int(values.get('EstimatedDifficulty', '2'))
                secs = float(values.get('EstimatedTime', '0'))
                flags = int(values.get('spawnflags', '0'))
                end = int(values.get('EndOfGame', '0'))
            except ValueError as error:
                raise SDKFormatError('malformed checkpoint parameter') from error
            if not 0 <= tier <= 4 or not math.isfinite(secs) or secs < 0 or flags < 0:
                raise SDKFormatError('invalid checkpoint parameter range')
            tiers[tier] += 1
            times.append(secs)
            result['checkpoint_repeat_flag_count'] += bool(flags & 1024)
            result['checkpoint_endgame_count'] += bool(end)
        elif kind == 'npc_template_maker':
            result['maker_count'] += 1
            if 'MaxLiveChildren' not in values:
                result['maker_no_live_limit_count'] += 1
            else:
                try:
                    lim = int(values['MaxLiveChildren'])
                except ValueError as error:
                    raise SDKFormatError('noninteger maker MaxLiveChildren') from error
                maker_limits[lim] += 1
            freq = values.get('SpawnFrequency')
            if freq is not None:
                try:
                    rate = float(freq)
                except ValueError as error:
                    raise SDKFormatError('invalid maker SpawnFrequency') from error
                if not math.isfinite(rate):
                    raise SDKFormatError('nonfinite maker SpawnFrequency')
                maker_frequencies[rate] += 1
            maker_disabled[values.get('StartDisabled', '(unset)')] += 1
    result['difficulty_tier_counts'] = dict(sorted(tiers.items()))
    result['estimated_time_seconds'] = {
        'sum': sum(times), 'min': min(times) if times else None,
        'max': max(times) if times else None,
    }
    result['maker_max_live_children_counts'] = dict(sorted(maker_limits.items()))
    result['maker_spawn_frequency_counts'] = dict(sorted(maker_frequencies.items()))
    result['maker_start_disabled_counts'] = dict(sorted(maker_disabled.items()))
    return result


def audit(path):
    with zipfile.ZipFile(path) as z:
        index = read_vpk_directory(z.read(VPK_DIR))
        archive = z.read(VPK_DATA)
        return {
            'scope': 'static_sdk_map_entity_settings_only_not_gameplay',
            'maps': {name.split('/')[-1]: vmf_encounters(vpk_file(index, archive, name))
                     for name in MAP_PATHS},
            'contains_raw_map_content': False,
            'original_engine_behavior_verified': False,
        }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('sdk_zip', type=Path)
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    try:
        result = json.dumps(audit(args.sdk_zip), indent=2, sort_keys=True) + '\n'
    except (OSError, ValueError, KeyError, zipfile.BadZipFile) as error:
        parser.exit(1, 'SDK encounter audit failed: ' + str(error) + '\n')
    if args.output:
        args.output.write_text(result, encoding='utf8')
    else:
        print(result, end='')


if __name__ == '__main__':
    main()
