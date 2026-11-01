#!/usr/bin/env python3
"""Summarize only nonidentifying placement settings from owner-held VMF maps.

Reuses the private-source VPK reader and VMF block tokenizer from previous
research. No original target/template names or entity coordinates are emitted.
"""
import argparse
from collections import Counter
import json
import math
from pathlib import Path
import zipfile
from audit_sin_sdk import MAP_PATHS,VPK_DIR,VPK_DATA,read_vpk_directory,vpk_file,SDKFormatError
from audit_sin_sdk_encounters import ENTITY,FIELD


def summary(blob):
    content=blob.decode('latin1')
    entities=[dict(FIELD.findall(x)) for x in ENTITY.findall(content)]
    makers=[e for e in entities if e.get('classname')=='npc_template_maker']
    dest=[e for e in entities if e.get('classname')=='info_npc_spawn_destination']
    if not makers and not dest:
        return {'makers':0,'destinations':0,'templates_named':0,
                'makers_with_groups':0,'makers_force_radius':0,
                'makers_visible_block':0,'visibility_criterion':{},
                'distance_criterion':{},'radius':{},'destination_reuse_delay_seconds':{},
                'destinations_with_rename':0}
    v=Counter();d=Counter();r=Counter();reuse=Counter()
    force=0;visible_block=0;named=0;groups=0
    for maker in makers:
        named+=bool(maker.get('TemplateName',''))
        groups+=bool(maker.get('DestinationGroup',''))
        try:
            flags=int(maker.get('spawnflags','0'))
            radius=float(maker.get('Radius','256'))
            vc=int(maker.get('CriterionVisibility','2'))
            dc=int(maker.get('CriterionDistance','2'))
        except ValueError as error:
            raise SDKFormatError('invalid placement settings') from error
        if flags<0 or not math.isfinite(radius) or radius<0 or vc not in (0,1,2) or dc not in (0,1,2):
            raise SDKFormatError('placement settings out of range')
        force+=bool(flags & 256);visible_block+=bool(flags & 128)
        v[vc]+=1;d[dc]+=1;r[radius]+=1
    for e in dest:
        try: delay=float(e.get('ReuseDelay','1'))
        except ValueError as error:raise SDKFormatError('invalid reuse delay') from error
        if not math.isfinite(delay) or delay<0:raise SDKFormatError('out of range reuse delay')
        reuse[delay]+=1
    def ordered(values):
        return {str(k):values[k] for k in sorted(values)}
    return {'makers':len(makers),'destinations':len(dest),'templates_named':named,
            'makers_with_groups':groups,'makers_force_radius':force,
            'makers_visible_block':visible_block,'visibility_criterion':ordered(v),
            'distance_criterion':ordered(d),'radius':ordered(r),
            'destination_reuse_delay_seconds':ordered(reuse),
            'destinations_with_rename':sum(bool(e.get('RenameNPC','')) for e in dest)}


def audit(archive):
    with zipfile.ZipFile(archive) as z:
        idx=read_vpk_directory(z.read(VPK_DIR)); data=z.read(VPK_DATA)
        return {'scope':'aggregate original SDK Hammer VMF placement metadata ONLY',
                'maps':{p.split('/')[-1]:summary(vpk_file(idx,data,p)) for p in MAP_PATHS},
                'original_placement_algorithm_verified':False,
                'includes_original_names_or_coordinates':False}


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('sdk_zip',type=Path)
    args=parser.parse_args()
    try: print(json.dumps(audit(args.sdk_zip),indent=2,sort_keys=True))
    except (OSError,ValueError,KeyError,zipfile.BadZipFile) as error:
        parser.exit(1,'SDK placement audit failed: '+str(error)+'\n')
if __name__=='__main__':main()
