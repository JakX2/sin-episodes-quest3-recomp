#!/usr/bin/env python3
"""Check source choreography archive availability and original actor tokens.

Scans scene archive metadata only, without redistributing voice, VCDs or sounds.
Usage: python sin_scene_archive_preflight.py scenes.zip --log "sin stuff finale.txt" --log "sin stuff highrise.txt" --json report.json
"""
import argparse
import collections
import json
import pathlib
import re
import sys
import zipfile

ACTOR_RE = re.compile(r'^\s*actor\s+"([^"\r\n]+)"', re.MULTILINE)
MISSING_RE = re.compile(r"Scene\s+'(scenes[\\/][^']+?\.vcd)'\s+missing!", re.IGNORECASE)


def canon(path):
    return path.replace('\\','/').lower()


def audit(scenes_zip, log_paths=()):
    scene_records={}
    with zipfile.ZipFile(scenes_zip,'r') as archive:
        for member in archive.namelist():
            if not member.lower().endswith('.vcd'):
                continue
            # VCDs are original user-owned TEXT choreography files in this archive.
            # We publish tokens/counts but never include original VCD event bodies.
            source=archive.read(member).decode('utf-8',errors='replace')
            scene_records[canon(member)]={
                'path':member,
                'actor_tokens':list(dict.fromkeys(ACTOR_RE.findall(source))),
                'byte_length':archive.getinfo(member).file_size,
            }
    missing=collections.Counter()
    for path in log_paths:
        source=pathlib.Path(path).read_text(encoding='utf-8',errors='replace')
        missing.update(canon(x) for x in MISSING_RE.findall(source))
    crosswalk=[]
    for key,n in sorted(missing.items()):
        found=scene_records.get(key)
        crosswalk.append({'scene':key,'log_occurrences':n,
                          'archive_present':found is not None,
                          'actor_tokens':found['actor_tokens'] if found else []})
    present=sum(1 for row in crosswalk if row['archive_present'])
    return {'vcd_count':len(scene_records),
            'log_missing_scene_distinct':len(missing),
            'log_missing_scene_occurrences':sum(missing.values()),
            'log_scenes_found_in_archive':present,
            'log_scenes_absent_from_archive':len(missing)-present,
            'log_crosswalk':crosswalk,
            'example_actor_tokens':{
                key:scene_records[key]['actor_tokens']
                for key in sorted(scene_records)
                if key in (
                    'scenes/se1_finale/teaser_jc_hospital.vcd',
                    'scenes/se1_finale/teaser_radek_and_jessica.vcd',
                    'scenes/se1_highrise/jess_wakeupblade.vcd',
                )},
            'limits':[
                'Archive presence does not establish VCD runtime mounting, scene.cache, engine compatibility or working animation.',
                'Actor names inside VCDs are map bindings, not necessarily NPC classnames.',
                'No VCD, voice data or proprietary scene payload is redistributed.'
            ]}


def main(argv=None):
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('scenes_zip',type=pathlib.Path)
    p.add_argument('--log',action='append',type=pathlib.Path,default=[])
    p.add_argument('--json',type=pathlib.Path)
    a=p.parse_args(argv)
    try:
        result=audit(a.scenes_zip,a.log)
        print(f"Found {result['vcd_count']} VCDs; logs reported {result['log_missing_scene_distinct']} distinct missing scene paths; {result['log_scenes_found_in_archive']} present in the archive")
        for row in result['log_crosswalk']:
            print(('PRESENT' if row['archive_present'] else 'ABSENT'),row['scene'],row['actor_tokens'])
        if a.json:a.json.write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
        return 0 if result['log_scenes_absent_from_archive']==0 else 1
    except (OSError,ValueError,zipfile.BadZipFile) as exc:
        print('Error:',exc,file=sys.stderr)
        return 2
if __name__=='__main__':sys.exit(main())