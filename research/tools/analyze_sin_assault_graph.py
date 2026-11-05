#!/usr/bin/env python3
"""Read-only original SiN BSP assault graph diagnostic (not a BSP patcher).

Maps the decompiled CAI_AssaultBehavior rally-point contract to source map I/O.
No proprietary BSP contents or game assets are committed.
"""
from __future__ import annotations

import argparse
from collections import Counter, defaultdict
import json
from pathlib import Path

RALLY = 'assault_rallypoint'
ASSAULT = 'assault_assaultpoint'
GOAL = 'ai_goal_assault'
NPC_CLASSES = {
    'npc_druglab_grunt_assault_rifle', 'npc_druglab_grunt_elite',
    'npc_sintek_grunt', 'npc_sintek_jetpack',
}


def normalize(name):
    return name.strip().casefold()


def parse_output(value):
    # Source Hammer outputs may use comma or ESC (0x1b) as the separator.
    sep = '\x1b' if '\x1b' in value else ','
    pieces = value.split(sep, 4)
    if len(pieces) != 5:
        return None
    return dict(target=pieces[0].strip(), input=pieces[1].strip(),
                parameter=pieces[2].strip(), delay=pieces[3].strip(),
                times=pieces[4].strip())


def analyze_entities(entities):
    indexed = []
    by_name = defaultdict(list)
    for idx, pairs in enumerate(entities):
        item = {key.casefold(): value for key, value in pairs}
        cls = normalize(item.get('classname', ''))
        name = item.get('targetname', '')
        indexed.append(dict(index=idx, classname=cls, targetname=name, keys=item, pairs=pairs))
        if name:
            by_name[normalize(name)].append(idx)

    def resolve(name):
        return [{'index': idx, 'classname': indexed[idx]['classname']}
                for idx in by_name.get(normalize(name), [])]

    def link(name, expected_class):
        matched = resolve(name) if name else []
        good = [row for row in matched if row['classname'] == expected_class]
        return dict(name=name, all_matching_entities=matched,
                    matching_expected_class=good,
                    status=('unspecified' if not name else
                            'missing' if not matched else
                            'wrong_class' if not good else
                            'ambiguous' if len(good) != 1 or len(matched) != 1 else
                            'ok'))

    rally, assault, goals, inputs = [], [], [], []
    for ent in indexed:
        cls = ent['classname']
        kv = ent['keys']
        if cls == RALLY:
            rally.append({
                'index': ent['index'], 'name': ent['targetname'],
                'priority': kv.get('priority', '1'),
                'assault_delay': kv.get('assaultdelay', '0'),
                'assault_point': link(kv.get('assaultpoint', ''), ASSAULT),
            })
        elif cls == ASSAULT:
            next_name = kv.get('nextassaultpoint', '')
            assault.append({
                'index': ent['index'], 'name': ent['targetname'],
                'assault_group': kv.get('assaultgroup', ''),
                'timeout': kv.get('assaulttimeout', '3.0'),
                'clear_on_contact': kv.get('clearoncontact', '0'),
                'allow_diversion': kv.get('allowdiversion', '0'),
                'never_timeout': kv.get('nevertimeout', '0'),
                'next_assault_point': link(next_name, ASSAULT),
            })
        elif cls == GOAL:
            seed = kv.get('rallypoint', '')
            if seed.endswith('*'):
                matches = [dict(index=r['index'], classname=r['classname'])
                           for r in indexed if r['classname'] == RALLY and
                           normalize(r['targetname']).startswith(normalize(seed[:-1]))]
            else:
                matches = link(seed, RALLY)['matching_expected_class']
            goals.append({
                'index': ent['index'], 'name': ent['targetname'],
                'actor': kv.get('actor', ''),
                'rallypoint': seed, 'matching_rallypoints': matches,
                'assault_cue': kv.get('assaultcue', '1'),
            })
        for key, value in ent['pairs']:
            if not key.casefold().startswith('on'):
                continue
            parsed = parse_output(value)
            if not parsed or normalize(parsed['input']) != 'assault':
                continue
            recipients = resolve(parsed['target'])
            cue = link(parsed['parameter'], RALLY)
            inputs.append({
                'entity_index': ent['index'], 'controller_classname': cls,
                'controller_name': ent['targetname'], 'output': key,
                'recipient': parsed['target'], 'recipients': recipients,
                'rallypoint_parameter': cue,
                'delay': parsed['delay'], 'times': parsed['times'],
            })
    states = Counter()
    for point in rally:
        states[point['assault_point']['status']] += 1
    for point in assault:
        if point['next_assault_point']['name']:
            states[point['next_assault_point']['status']] += 1
    for signal in inputs:
        states[signal['rallypoint_parameter']['status']] += 1
    return dict(
        summary=dict(rally_points=len(rally), assault_points=len(assault),
                     goals=len(goals), directed_assault_inputs=len(inputs),
                     point_or_input_link_status=dict(sorted(states.items()))),
        rally_points=rally, assault_points=assault, assault_goals=goals,
        original_assault_input_signals=inputs,
        limitations=[
            'This reads static BSP entity keys, not the live runtime entity or AI schedule.',
            'Targetname ambiguity and wrong-class links are diagnostics, not automatic fixes.',
            'Runtime code may spawn, rename, disable or remove entities after map load.',
            'The original input Assault(string) expects a rally-point name (SDK base.fgd).',
        ],
    )


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--bsp', type=Path, required=True)
    ap.add_argument('--output', type=Path, help='write a new report, never overwrite')
    args = ap.parse_args(argv)
    try:
        from sin_scene_actor_diagnostics import entities_from_bsp
        result = analyze_entities(entities_from_bsp(args.bsp.read_bytes()))
        content = json.dumps(result, indent=2) + '\n'
        if args.output:
            with args.output.open('x', encoding='utf8') as report:
                report.write(content)
            print('Wrote read-only assault graph report')
        else:
            print(content, end='')
        return 0
    except (OSError, ValueError) as error:
        ap.exit(2, 'SiN assault BSP graph scan failed: ' + str(error) + '\n')


if __name__ == '__main__':
    raise SystemExit(main())
