#!/usr/bin/env python3
"""Read-only SiN->HL2 scene actor triage and before/after console comparison.

No original game data is included. This does not implement game entities or
prove playback. Report avoids full paths and preserves only map key names,
classnames, targetnames, VCD actor tokens and diagnostic counts.

Commands:
  python sin_scene_actor_diagnostics.py preflight --bsp map.bsp \
      --scenes scenes.zip --log console.txt --output preflight.json
  python sin_scene_actor_diagnostics.py compare --before old_log.txt \
      --after new_log.txt --output comparison.json
"""
from __future__ import annotations

import argparse
import collections
import json
import pathlib
import re
import sys
import zipfile

# Import local tools, not proprietary game code.
from sin_bsp_scene_actor_trial import read_bsp
from sin_scene_archive_preflight import canon, ACTOR_RE, MISSING_RE

KV_TOKEN = re.compile(rb'"(?:\\.|[^"\\])*"|[{}]')
UNKNOWN = re.compile(r'Attempted to create unknown entity type ([A-Za-z_][A-Za-z_0-9]*)!', re.I)
MISSING_TARGET = re.compile(r"Couldn't find any entities named ([^,]+), which (.+?) is specifying\.", re.I)
SCENE_KEYS = ('scenefile', 'scene_file')
ACTOR_TYPES = ('npc_jc', 'npc_jessica', 'npc_generic_actor')


class DiagnosticsError(ValueError):
    pass


def entities_from_bsp(blob: bytes) -> list[list[tuple[str, str]]]:
    """Parse quoted Source BSP entity key/value pairs and reject malformed text.

    Keep duplicate keys: Source outputs share names and should not be flattened.
    Never expand game I/O or rewrite the user map.
    """
    _, _, _, _, raw = read_bsp(blob)
    entities = []
    current = None
    key = None
    for match in KV_TOKEN.finditer(raw):
        token = match.group(0)
        if token == b'{':
            if current is not None or key is not None:
                raise DiagnosticsError('Nested/malformed entity group')
            current = []
        elif token == b'}':
            if current is None or key is not None:
                raise DiagnosticsError('Unbalanced entity group')
            entities.append(current)
            current = None
        elif current is not None:
            # Match Source entity quotes without attempting to interpret I/O.
            value = token[1:-1].decode('utf-8', 'replace')
            if key is None:
                key = value
            else:
                current.append((key, value))
                key = None
    if current is not None or key is not None or not entities:
        raise DiagnosticsError('Incomplete or missing entity group')
    return entities


def pairs_to_dict(pairs):
    # Classname/model/targetname keys usually appear once; preserve raw pairs
    # separately for repeated outputs and avoid publishing all full map data.
    return {k.casefold(): v for k, v in pairs}


def scene_path(value):
    path = canon(value.strip()).lstrip('/')
    if path and not path.startswith('scenes/'):
        path = 'scenes/' + path
    return path if path.endswith('.vcd') else path + '.vcd'


def capture_log(text: str) -> dict:
    counts = collections.Counter(UNKNOWN.findall(text))
    missing = collections.Counter(canon(s) for s in MISSING_RE.findall(text))
    targets = collections.Counter(m.group(1).strip() for m in MISSING_TARGET.finditer(text))
    return {
        'unknown_entities': dict(sorted(counts.items())),
        'unknown_entity_attempts': sum(counts.values()),
        'missing_scenes': dict(sorted(missing.items())),
        'missing_scene_attempts': sum(missing.values()),
        'missing_template_targets': dict(sorted(targets.items())),
        'missing_template_target_attempts': sum(targets.values()),
    }


def preflight(bsp: pathlib.Path, scenes: pathlib.Path | None, logs: list[pathlib.Path]):
    ent = entities_from_bsp(bsp.read_bytes())
    summary = [pairs_to_dict(kv) for kv in ent]
    classcounts = collections.Counter(x.get('classname', '').casefold() for x in summary)
    named = collections.defaultdict(list)
    for x in summary:
        if x.get('targetname'):
            named[x['targetname'].casefold()].append(x.get('classname', ''))
    ctrls = []
    for x in summary:
        if x.get('classname','').casefold() != 'logic_choreographed_scene':
            continue
        key = next((x.get(k, '') for k in SCENE_KEYS if x.get(k)), '')
        aliases = {k:v for k,v in sorted(x.items()) if re.fullmatch(r'target[1-8]', k)}
        ctrls.append({'targetname': x.get('targetname', ''),
                      'scene': scene_path(key) if key else '', 'aliases': aliases})
    # Alias names belong to the scene CONTROLLER, not to the NPC entity.
    # Checking for a targetname in this map lump is only a candidate: some
    # map scripts can spawn actors dynamically after initial BSP parsing.
    alias_targetnames = {v.casefold() for c in ctrls for v in c['aliases'].values()}
    actors = [
        {'classname': x.get('classname', ''), 'targetname': x.get('targetname', ''),
         'model': x.get('model', ''),
         'has_scene_alias': bool(x.get('targetname')) and x['targetname'].casefold() in alias_targetnames}
        for x in summary if x.get('classname','').casefold() in ACTOR_TYPES
    ]
    archive = {}
    if scenes:
        with zipfile.ZipFile(scenes, 'r') as zipf:
            for name in zipf.namelist():
                if name.lower().endswith('.vcd'):
                    archive[canon(name)] = list(dict.fromkeys(
                        ACTOR_RE.findall(zipf.read(name).decode('utf-8','replace'))))
    missinglog = collections.Counter()
    for p in logs:
        missinglog.update(capture_log(p.read_text(encoding='utf-8', errors='replace'))['missing_scenes'])
    referenced = sorted(set(x['scene'] for x in ctrls if x['scene']) | set(missinglog))
    bindings = []
    for name in referenced:
        tokens = archive.get(name, [])
        controllers = [x for x in ctrls if x['scene'] == name]
        found = (name in archive) if scenes else None
        token_rows = []
        for token in tokens:
            if token.lower().startswith('!target') and token[7:].isdigit():
                # A VCD !Target1 is a *scene alias*. It is NOT an entity name.
                suffix = token[1:].casefold()
                slots = [c['aliases'][suffix] for c in controllers if suffix in c['aliases']]
                candidates = {name: named.get(name.casefold(), []) for name in slots}
                token_rows.append({'token':token,'kind':'scene_controller_alias',
                                   'aliased_targetnames':slots,'alias_set_in_controller': bool(slots),
                                   'alias_targetname_found_in_entity_lump': bool(slots) and
                                       all(name.casefold() in named for name in slots),
                                   'alias_candidate_classnames':candidates})
            else:
                token_rows.append({'token':token,'kind':'direct_actor_token',
                                   'targetname_candidate_present':token.casefold() in named,
                                   'direct_candidate_classnames':named.get(token.casefold(),[])})
        bindings.append({'scene':name,'present_in_supplied_archive':found,
                         'scene_controllers':len(controllers), 'actors':token_rows,
                         'log_missing_occurrences':missinglog[name]})
    return {
        'source_bsp_name': bsp.name,
        'source_entity_count':len(ent),
        'source_actor_counts': {x:classcounts[x] for x in ACTOR_TYPES},
        'source_actor_entities':actors,
        'source_scene_controller_count':len(ctrls),
        'scene_bindings':bindings,
        'archive_vcd_count':len(archive) if scenes else None,
        'warnings':[
            'A targetname match is a candidate only, not proof the VCD resolves that actor.',
            'An alias target absent from the initial entity lump may be created later by map I/O; confirm runtime.' ,
            '!Target1 refers to logic_choreographed_scene Target1, not a map entity named !Target1.',
            'VCD file presence does not prove scene.cache, mount or animation compatibility.',
            'This is a read-only preflight. No game map, DLL or APK has been modified.',
        ],
    }


def compare(before_text: str, after_text: str) -> dict:
    old = capture_log(before_text)
    new = capture_log(after_text)
    names = sorted(set(old['unknown_entities']) | set(new['unknown_entities']))
    delta = [
        {'classname':n,'before':old['unknown_entities'].get(n,0),
         'after':new['unknown_entities'].get(n,0),
         'count_difference':new['unknown_entities'].get(n,0)-old['unknown_entities'].get(n,0)}
        for n in names
    ]
    actors = {n:{'before':old['unknown_entities'].get(n,0),
                 'after':new['unknown_entities'].get(n,0)} for n in ACTOR_TYPES}
    scene_names=sorted(set(old['missing_scenes'])|set(new['missing_scenes']))
    return {'actor_unknown_classnames':actors,
            'all_entity_deltas':delta,
            'missing_scene_deltas':[{'scene':n, 'before':old['missing_scenes'].get(n,0),
                                     'after':new['missing_scenes'].get(n,0)} for n in scene_names],
            'before_totals':{k:v for k,v in old.items() if k.endswith('_attempts')},
            'after_totals':{k:v for k,v in new.items() if k.endswith('_attempts')},
            'interpretation':[
                'After=0 indicates no unknown-class attempts in this capture, not proof an NPC spawned/animated.',
                'Repeated map loads or different map coverage make raw failure counts non-comparable.',
                'Look for model, animation, scene resolution and scripted outputs in live engine logs.',
                'The scene cache and VCD mounting must be validated separately on the actual HL2 game build.',
            ]}


def main(argv=None):
    ap=argparse.ArgumentParser(description=__doc__)
    sub=ap.add_subparsers(dest='mode',required=True)
    p=sub.add_parser('preflight',help='inspect original BSP entity bindings without changing it')
    p.add_argument('--bsp',type=pathlib.Path,required=True)
    p.add_argument('--scenes',type=pathlib.Path)
    p.add_argument('--log',action='append',type=pathlib.Path,default=[])
    p.add_argument('--output',type=pathlib.Path)
    b=sub.add_parser('baseline',help='summarize one or more existing HL2 logs without a BSP')
    b.add_argument('--log',action='append',type=pathlib.Path,required=True)
    b.add_argument('--output',type=pathlib.Path)
    c=sub.add_parser('compare',help='compare original and patched HL2 console captures')
    c.add_argument('--before',type=pathlib.Path,required=True)
    c.add_argument('--after',type=pathlib.Path,required=True)
    c.add_argument('--output',type=pathlib.Path)
    args=ap.parse_args(argv)
    try:
        if args.mode=='preflight':
            result=preflight(args.bsp,args.scenes,args.log)
            print('BSP:',result['source_bsp_name'], 'entities:',result['source_entity_count'],
                  'actors:',result['source_actor_counts'],'scene controllers:',result['source_scene_controller_count'])
            for row in result['scene_bindings']:
                print('Scene',row['scene'], 'archive present',row['present_in_supplied_archive'],
                      'actors',row['actors'])
        elif args.mode=='baseline':
            records=[{'file':p.name,**capture_log(p.read_text(encoding='utf-8',errors='replace'))}
                     for p in args.log]
            merged=collections.Counter()
            scenes=collections.Counter()
            for row in records:
                merged.update(row['unknown_entities'])
                scenes.update(row['missing_scenes'])
            result={'logs':records,'aggregate':{'unknown_entities':dict(sorted(merged.items())),
                'unknown_entity_attempts':sum(merged.values()),
                'unknown_classnames':len(merged),
                'actor_attempts':{k:merged[k] for k in ACTOR_TYPES},
                'missing_scene_paths':dict(sorted(scenes.items()))},
                'limits':['Counts are log occurrences and may include repeated map loads.',
                          'The report is an HL2 compatibility baseline, not gameplay behavior.']}
            print('Logs:',len(records),'unknown entity attempts:',sum(merged.values()),
                  'distinct classnames:',len(merged),'actor attempts:',result['aggregate']['actor_attempts'])
        else:
            result=compare(args.before.read_text(encoding='utf-8',errors='replace'),
                           args.after.read_text(encoding='utf-8',errors='replace'))
            print('Actor unknown-class attempts, before -> after:')
            for name,counts in result['actor_unknown_classnames'].items():
                print(' ',name,counts['before'],'->',counts['after'])
            print('Missing scenes:',len(result['missing_scene_deltas']))
        if args.output:
            if args.output.exists():
                raise DiagnosticsError('Refusing to overwrite existing report')
            args.output.write_text(json.dumps(result,indent=2,sort_keys=True)+'\n',encoding='utf-8')
            print('Wrote diagnostic report:',args.output.name)
        return 0
    except (OSError, ValueError, zipfile.BadZipFile) as exc:
        print('Error:',exc,file=sys.stderr)
        return 2

if __name__=='__main__':
    sys.exit(main())
