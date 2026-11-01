#!/usr/bin/env python3
"""Read-only SHA-pinned metadata audit of owner-held SiN stats text files.

No original stat definitions, UI layout rows, or assets are printed.
This checks declared cross-references, not actual difficulty calculations.
"""
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import re
import zipfile

PINS = {
 'scripts/stats/stats_definitions.txt': 'a18afd2e8fba54653a9bd173c3eacf440fe016b3659b74578f2ecd1f37a156a8',
 'scripts/stats/stats_display_layout.txt': '850dd06942b459843236dd8c6770b40a5b28e6b6a0fcab7bf4e60dec55322dbe',
 'scripts/stats/stats_graphs_layout.txt': '8e5bdc958138eccef57554f48d6c74dfcf5d7956dee324bc95e722fa17bf2032',
}
class LayoutError(ValueError):
    pass

def uncomment(source):
    return re.sub(r'(?m)//[^\r\n]*','',source)

def parse_defs(source):
    source=uncomment(source)
    if not re.match(r'^\s*"StatDefinitions"\s*\{',source):
        raise LayoutError('missing definitions root')
    blocks=re.findall(r'"stat"\s*\{([^{}]*)\}',source,flags=re.S)
    names={};links={}
    if not 0<len(blocks)<=1024:
        raise LayoutError('invalid definition count')
    for block in blocks:
        fields=re.findall(r'"([^"\r\n]+)"\s*"([^"\r\n]*)"',block)
        item={}
        for key,value in fields:
            if key in item or key not in ('name','short_text','long_text','percentage_of'):
                raise LayoutError('bad or duplicate field')
            item[key]=value
        if not all(item.get(k) for k in ('name','short_text','long_text')):
            raise LayoutError('incomplete definition')
        name=item['name']
        if name in names:
            raise LayoutError('duplicate stat definition')
        names[name]=item
        if item.get('percentage_of'):
            links[name]=item['percentage_of']
    if any(target not in names for target in links.values()):
        raise LayoutError('unresolved percentage reference')
    return names,links

def parse_display(source,names,links):
    refs=[];formats=Counter()
    # Unlike standard nested KeyValues, this original file is line-oriented.
    for line in uncomment(source).splitlines():
        values=re.findall(r'"([^"\r\n]+)"',line)
        if not values or values==['==========================']:
            continue
        if len(values)!=2:
            raise LayoutError('invalid display layout row')
        key,fmt=values
        if key.startswith('STAT_'):
            if key not in names:
                raise LayoutError('unknown display stat')
            if fmt=='percent_of' and key not in links:
                raise LayoutError('display percent_of missing denominator')
            if fmt not in {'value','time','percent','percent_of','highest_name',
                           'lowest_name','highest_value','lowest_value'}:
                raise LayoutError('unknown display stat format')
            refs.append(key)
        elif fmt not in {'title','tab','dynamic_block'}:
            raise LayoutError('unknown display heading type')
        formats[fmt]+=1
    return {'stat_rows':len(refs),'distinct_stats':len(set(refs)),
            'percentage_rows':formats['percent_of'],
            'dynamic_blocks':formats['dynamic_block']}

def graph_stats(source,names):
    # Repeated name keys inside graph stats blocks are intentional.
    src=uncomment(source)
    if not re.search(r'^\s*"GraphPages"\s*\{',src):
        raise LayoutError('missing GraphPages')
    graphs=re.findall(r'"graph"\s*\{\s*"name"\s*"([^"\n]+)"\s*'
                      r'"maxvalue"\s*"([^"\n]+)"\s*'
                      r'"format"\s*"([^"\n]+)"\s*'
                      r'"stats"\s*\{([^{}]*)\}\s*\}',src,re.S)
    if not graphs:
        raise LayoutError('no graph definitions')
    refs=[]
    for title,maximum,fmt,stats in graphs:
        if fmt not in {'percent','value','time'}:
            raise LayoutError('invalid graph format')
        if maximum.startswith('STAT_'):
            if maximum not in names:
                raise LayoutError('unknown graph maxvalue stat')
        else:
            try: n=float(maximum)
            except ValueError as ex: raise LayoutError('invalid graph maxvalue') from ex
            if not 0<=n<float('inf'): raise LayoutError('invalid graph maxvalue')
        values=re.findall(r'"name"\s*"([^"\r\n]+)"',stats)
        if not values or any(not v.startswith('STAT_') or v not in names for v in values):
            raise LayoutError('unknown graph stat')
        refs.extend(values)
    return {'graph_pages':len(graphs),'graph_stat_rows':len(refs),
            'distinct_graph_stats':len(set(refs))}

def inspect(definitions,display,graphs):
    names,links=parse_defs(definitions)
    return {'definitions':len(names),'percentage_relationships':len(links),
            'display':parse_display(display,names,links),
            'graphs':graph_stats(graphs,names),
            'defines_challenge_rating':'STAT_CHALLENGE_RATING' in names,
            'defines_instant_challenge_rating':'STAT_INSTANT_CHALLENGE_RATING' in names,
            'defines_gameplay_var_averages':'STAT_GAME_PLAY_VAR_AVERAGES' in names,
            'actual_difficulty_formula_verified':False,
            'original_game_runtime_executed':False}

def audit_zip(owner_zip):
    with zipfile.ZipFile(owner_zip) as z:
        texts={}
        for name,digest in PINS.items():
            raw=z.read(name)
            if hashlib.sha256(raw).hexdigest()!=digest:
                raise LayoutError('stats file provenance SHA256 mismatch')
            texts[name]=raw.decode('utf8')
    result=inspect(texts['scripts/stats/stats_definitions.txt'],
                   texts['scripts/stats/stats_display_layout.txt'],
                   texts['scripts/stats/stats_graphs_layout.txt'])
    result['three_original_files_sha256_verified']=True
    return result

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('owner_zip',type=Path)
    args=parser.parse_args()
    try: print(json.dumps(audit_zip(args.owner_zip),indent=2,sort_keys=True))
    except (OSError,ValueError,KeyError,zipfile.BadZipFile,UnicodeError) as ex:
        parser.exit(1,'private stat layout audit failed: '+str(ex)+'\n')
if __name__=='__main__':main()
