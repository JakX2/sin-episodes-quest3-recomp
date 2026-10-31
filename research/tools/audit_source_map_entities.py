#!/usr/bin/env python3
"""Inventory a user-owned Source BSP's entity lump or a plain .ent file.

Static analysis only. Does not run entity I/O, NPC AI, level changes or map
scripts. Entity classname support is NOT inferred from simple string matches.
No commercial game map bytes are included or emitted in the report.
"""
from __future__ import annotations
import argparse
from collections import Counter, defaultdict
import json
from pathlib import Path
import struct

from verify_gameplay_entity_registry import FACTORIES

MAX_ENTITY_BYTES = 32 * 1024 * 1024
SELECTED = {name:cat for name,cat,*_ in FACTORIES}
OUTPUT_PREFIX='on'
DYNAMIC_TARGETS={'!activator','!caller','!self','!player','!picker'}

class MapFormatError(ValueError): pass


def extract_entity_lump(data: bytes, source: str) -> bytes:
    if source.lower().endswith('.ent'):
        if len(data) > MAX_ENTITY_BYTES: raise MapFormatError('entity text exceeds safety limit')
        return data.rstrip(b'\0')
    if not source.lower().endswith('.bsp'):
        raise MapFormatError('expected a user-owned .bsp or plain .ent file')
    if len(data) < 8+64*16+4 or data[:4] != b'VBSP':
        raise MapFormatError('missing Source VBSP header or truncated lump table')
    version = struct.unpack_from('<i', data, 4)[0]
    if not 17 <= version <= 31:
        raise MapFormatError(f'unsupported Source BSP version {version}; not guessing layout')
    offset,size,lump_version,fourcc=struct.unpack_from('<IIII',data,8)
    if size > MAX_ENTITY_BYTES or offset < 8+64*16+4 or offset > len(data) or size > len(data)-offset:
        raise MapFormatError('invalid/out-of-bounds entity lump')
    entity=data[offset:offset+size]
    if entity.startswith(b'LZMA') or fourcc != 0:
        raise MapFormatError('compressed BSP entity lump not supported; extract lawful plain .ent first')
    return entity.rstrip(b'\0')


def tokenize(text: str):
    i,n=0,len(text)
    while i<n:
        c=text[i]
        if c.isspace(): i+=1;continue
        if c=='/' and i+1<n and text[i+1]=='/':
            e=text.find('\n',i+2)
            i=n if e<0 else e+1
            continue
        if c in '{}':
            yield c;i+=1;continue
        if c!='"': raise MapFormatError(f'unexpected nonquoted key/value at character {i}')
        i+=1;chars=[]
        while i<n:
            c=text[i]
            if c=='"':i+=1;break
            if c=='\\' and i+1<n and text[i+1] in ('"','\\'):
                chars.append(text[i+1]);i+=2;continue
            chars.append(c);i+=1
        else: raise MapFormatError('unterminated quoted string')
        yield ''.join(chars)


def parse_entity_text(data: bytes) -> list[list[tuple[str,str]]]:
    if len(data)>MAX_ENTITY_BYTES: raise MapFormatError('entity text exceeds safety limit')
    try: text=data.decode('utf-8-sig')
    except UnicodeDecodeError:
        # Source map keyvalues commonly use single-byte legacy encodings.
        text=data.decode('latin-1')
    tok=iter(tokenize(text))
    objects=[]
    while True:
        marker=next(tok,None)
        if marker is None:break
        if marker!='{':raise MapFormatError('expected start of entity block')
        items=[]
        while True:
            key=next(tok,None)
            if key is None: raise MapFormatError('unterminated entity block')
            if key=='}':break
            if key in ('{','}'):raise MapFormatError('unexpected nested entity block')
            val=next(tok,None)
            if val is None or val in ('{','}'):
                raise MapFormatError('missing quoted value for key')
            items.append((key,val))
        objects.append(items)
        if len(objects)>100000:
            raise MapFormatError('entity count exceeds safety limit')
    return objects


def first_property(pairs: list[tuple[str,str]],name:str) -> str:
    return next((v for k,v in pairs if k.lower()==name.lower()),'')


def parse_output(value: str) -> dict:
    delimiter='\x1b' if '\x1b' in value else ','
    parts=value.split(delimiter)
    if len(parts)!=5:
        return {'raw_format':'unparsed','target':'','input':'','valid_shape':False}
    target,input_name,parameter,delay,fire_count=parts
    return {'raw_format':'escape' if delimiter=='\x1b' else 'comma',
            'target':target.strip(),'input':input_name.strip(),'valid_shape':True,
            'delay':delay.strip(),'fire_count':fire_count.strip()}


def inspect_entities(entities: list[list[tuple[str,str]]]) -> dict:
    byname=defaultdict(list)
    counts=Counter()
    registered=Counter()
    out=[]
    transitions=[]
    for index,pairs in enumerate(entities):
        classname=first_property(pairs,'classname') or '(missing classname)'
        targetname=first_property(pairs,'targetname')
        counts[classname]+=1
        if classname in SELECTED:registered[classname]+=1
        if targetname:byname[targetname.lower()].append(index)
        if classname=='trigger_changelevel':
            transitions.append({'entity_index':index,'map':first_property(pairs,'map'),
                                'landmark':first_property(pairs,'landmark')})
        for key,value in pairs:
            if key.lower().startswith(OUTPUT_PREFIX) and len(key)>2:
                p=parse_output(value)
                out.append({'entity_index':index,'source_classname':classname,
                            'output':key,'target':p['target'],'input':p['input'],
                            'format':p['raw_format'],'valid_shape':p['valid_shape']})
    unresolved=[];ambiguous=[];dynamic=[];invalid=[];resolved=0
    for link in out:
        if not link['valid_shape'] or not link['target'] or not link['input']:
            invalid.append(link);continue
        target=link['target'].lower()
        if target in DYNAMIC_TARGETS or '*' in target or target.startswith('@'):
            dynamic.append(link);continue
        matches=byname.get(target,[])
        if not matches:unresolved.append(link)
        elif len(matches)>1:ambiguous.append({**link,'candidate_count':len(matches)})
        else:resolved+=1
    landmark_issues=[]
    for t in transitions:
        if t['landmark'] and t['landmark'].lower() not in byname:
            landmark_issues.append(t)
    categories=Counter()
    for cl,count in registered.items():categories[SELECTED[cl]]+=count
    return {'scope':'static_entity_lump_inventory_only_not_executed',
            'entity_count':len(entities),'class_counts':dict(sorted(counts.items())),
            'selected_original_registered_subset':dict(sorted(registered.items())),
            'selected_category_counts':dict(sorted(categories.items())),
            'classnames_not_in_selected_32_factory_subset':sorted(set(counts)-set(SELECTED)),
            'targetname_count':len(byname),'output_count':len(out),
            'resolved_unique_output_links':resolved,
            'unresolved_literal_outputs':unresolved,
            'ambiguous_duplicate_target_outputs':ambiguous,
            'dynamic_or_wildcard_outputs':dynamic,
            'malformed_output_shapes':invalid,
            'changelevel_references':transitions,
            'unresolved_local_landmark_names':landmark_issues,
            'limits':['An unknown name here is NOT proof of unsupported engine entity.',
                      'Target references can be created dynamically or loaded from other levels.',
                      'No player AI, NPC schedules, entity inputs, map transitions or scripted scenes are run.',
                      'Entity models, animations, scripts, and assets are not verified.']}


def audit_bytes(data:bytes, filename:str)->dict:
    entity_lump=extract_entity_lump(data,filename)
    result=inspect_entities(parse_entity_text(entity_lump))
    result['source_kind']='BSP entity lump' if filename.lower().endswith('.bsp') else 'plain entity text'
    return result


def main() -> int:
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('map',type=Path,help='Lawfully held Source .bsp or extracted .ent')
    p.add_argument('--output',type=Path,help='Write JSON report, never embed entity contents')
    args=p.parse_args()
    try: result=audit_bytes(args.map.read_bytes(),args.map.name)
    except (OSError,MapFormatError) as exc:p.exit(1,f'SOURCE MAP AUDIT FAILED: {exc}\n')
    out=json.dumps(result,indent=2)+'\n'
    if args.output:args.output.write_text(out)
    else:print(out,end='')
    return 0

if __name__=='__main__':raise SystemExit(main())
