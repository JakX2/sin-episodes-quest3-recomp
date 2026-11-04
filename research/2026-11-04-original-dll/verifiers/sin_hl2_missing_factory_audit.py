#!/usr/bin/env python3
"""Read-only match of an HL2 runtime console log to the original SiN Episodes server.dll.

Usage: python sin_hl2_missing_factory_audit.py LOG.txt ORIGINAL_SERVER.dll [output.json]

Does NOT execute, modify, or copy the original DLL. Requires Python 3 stdlib.
Source binary SHA is pinned to the copy investigated on 2026-11-04.
"""
from __future__ import annotations
import collections
import hashlib
import json
import re
import struct
import sys
from pathlib import Path

SERVER_SHA256 = '568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650'
MISS_RE = re.compile(r'^Attempted to create unknown entity type ([A-Za-z0-9_]+)!$')

class PE32:
    def __init__(self, path: Path):
        self.data = path.read_bytes()
        sha = hashlib.sha256(self.data).hexdigest()
        if sha != SERVER_SHA256:
            raise ValueError(f'Unexpected server.dll SHA256: {sha}')
        pe_off = struct.unpack_from('<I', self.data, 0x3C)[0]
        if self.data[pe_off:pe_off+4] != b'PE\0\0':
            raise ValueError('Not a PE image')
        n_sections = struct.unpack_from('<H', self.data, pe_off+6)[0]
        opt_len = struct.unpack_from('<H', self.data, pe_off+20)[0]
        opt_off = pe_off+24
        magic = struct.unpack_from('<H', self.data, opt_off)[0]
        if magic != 0x10B:
            raise ValueError('Expected 32-bit PE image')
        self.base = struct.unpack_from('<I', self.data, opt_off+28)[0]
        self.sections = []
        for index in range(n_sections):
            pos = opt_off+opt_len+40*index
            name = self.data[pos:pos+8].split(b'\0')[0].decode('ascii')
            vsize, rva, rawsize, rawpos = struct.unpack_from('<IIII', self.data, pos+8)
            self.sections.append((name,vsize,rva,rawsize,rawpos))
    def off(self,va:int,size:int=1)->int:
        rva=va-self.base
        for name,vsize,s_rva,rawsize,rawpos in self.sections:
            if s_rva<=rva and rva+size<=s_rva+rawsize and rawpos+(rva-s_rva)+size<=len(self.data):
                return rawpos+(rva-s_rva)
        raise ValueError(f'Unbacked VA {va:08X}')
    def va(self,off:int)->int:
        for name,vsize,rva,rawsize,rawpos in self.sections:
            if rawpos<=off<rawpos+rawsize:
                return self.base+rva+off-rawpos
        raise ValueError(f'Unmapped file offset {off:#x}')
    def u32(self,va:int)->int:
        return struct.unpack_from('<I', self.data, self.off(va,4))[0]
    def text(self,va:int,limit:int=256)->str:
        pos=self.off(va)
        end=self.data.find(b'\0',pos,pos+limit)
        if end<0:raise ValueError('String not terminated')
        return self.data[pos:end].decode('ascii')
    def factory_class(self,factory_va:int)->str:
        vt=self.u32(factory_va)
        col=self.u32(vt-4)
        td=self.u32(col+12)
        rtti=self.text(td+8)
        match=re.fullmatch(r'\.\?AV\?\$CEntityFactory@V(.+?)@@@@',rtti)
        if not match:raise ValueError(f'Not CEntityFactory RTTI: {rtti}')
        return match.group(1)


def find_factory(pe:PE32,classname:str)->dict:
    text_section=next((s for s in pe.sections if s[0]=='.text'),None)
    if text_section is None:raise ValueError('No .text section')
    _,_,_,rawsize,rawpos=text_section
    code=pe.data[rawpos:rawpos+rawsize]
    results=[]
    sought=(classname+'\0').encode('ascii')
    pos=0
    while True:
        pos=pe.data.find(sought,pos)
        if pos<0:break
        try: string_va=pe.va(pos)
        except ValueError:pos+=1;continue
        # x86 "push classname; push CEntityFactory object; mov ecx,eax; call [edx]".
        # The second push and the vtable RTTI jointly distinguish registration
        # from coincidental string/constant appearances.
        pattern=b'\x68'+struct.pack('<I',string_va)+b'\x68'
        idx=0
        while True:
            idx=code.find(pattern,idx)
            if idx<0:break
            candidate_off=rawpos+idx
            factory_va=struct.unpack_from('<I',code,idx+6)[0]
            continuation=code[idx+10:idx+14]
            try:
                cppclass=pe.factory_class(factory_va)
                if continuation!=b'\x8b\xc8\xff\x12':
                    raise ValueError('registration call suffix differs')
                results.append({'register_va':f'0x{pe.va(candidate_off):08X}',
                                'classname_va':f'0x{string_va:08X}',
                                'factory_va':f'0x{factory_va:08X}',
                                'factory_cpp_class':cppclass})
            except ValueError:
                pass
            idx+=1
        pos+=1
    if not results: raise ValueError(f'No RTTI-backed factory registration for {classname}')
    # Some map classname strings may be aliased; refuse ambiguity rather than guess.
    unique={(r['register_va'],r['factory_va'],r['factory_cpp_class']):r for r in results}
    results=list(unique.values())
    if len(results)!=1: raise ValueError(f'Multiple factory registrations for {classname}: {results}')
    return results[0]


def category(name:str)->str:
    if name.startswith('npc_'):return 'NPCs and AI'
    if name.startswith('ai_'):return 'NPCs and AI'
    if name.startswith('weapon_'):return 'Weapons and ammunition'
    if name.startswith('item_ammo_'):return 'Weapons and ammunition'
    if name.startswith('prop_'):return 'Props, vehicles and phones'
    if name.startswith('env_'):return 'Scripted events and environment'
    if name.startswith('trigger_'):return 'Map triggers and progression'
    if name.startswith('item_'):return 'Items, health and pickups'
    return 'Other'


def audit(log_path:Path,server_path:Path)->dict:
    pe=PE32(server_path)
    log=log_path.read_text(encoding='utf-8',errors='replace').splitlines()
    occurrences=collections.defaultdict(list)
    for line_no,line in enumerate(log,1):
        match=MISS_RE.fullmatch(line.strip())
        if match:occurrences[match.group(1)].append(line_no)
    rows=[]
    for name,lines in occurrences.items():
        factory=find_factory(pe,name)
        rows.append({'classname':name,'failed_creation_attempts':len(lines),
                     'first_log_line':lines[0], 'all_log_lines':lines,
                     'category':category(name), **factory})
    rows.sort(key=lambda row:(-row['failed_creation_attempts'],row['classname']))
    kinds=collections.Counter()
    for row in rows:kinds[row['category']]+=row['failed_creation_attempts']
    def count(substr:str)->int:return sum(substr in line for line in log)
    return {'original_server_sha256':SERVER_SHA256,
            'log_line_count':len(log),
            'missing_entity_attempts':sum(len(lines) for lines in occurrences.values()),
            'distinct_missing_classnames':len(occurrences),
            'original_rtti_factory_registrations_verified':len(rows),
            'categories_by_attempt_count':dict(kinds),
            'other_console_diagnostics':{
                'point_template_missing_named_target':count("Couldn't find any entities named"),
                'point_template_no_templates':count('no templates:'),
                'npc_template_maker_no_template':count('has no template NPC'),
                'door_model_missing_options':count('no door_options'),
                'mp3_invalid_sample_rate':count('Invalid sample rate (32000)'),
                'save_load_requests':count('Loading game from'),
                'save_open_failures':count("ERROR: couldn't open."),
                'material_abovewater_warning':count('need to set $abovewater'),
                'static_prop_no_physics_model':count('no vphysics model'),
                'empty_ambient_generic':count('Empty ambient_generic'),
                'null_entity_precache':count('NULL Ent in UTIL_PrecacheOther'),
            },
            'entity_factory_evidence':rows,
            'scope_notes':[
                'Each missing-entity attempt is one console message; repeated reloads and instances are not distinguished.',
                'Registration verifies presence in original Ritual DLL, not working spawn in an HL2-only runtime.',
                'Failures in templates may be caused by absent entities, but specific causal links require targetname inspection in the BSP.',
                'Without timestamps or per-map start markers, occurrences must not be asserted to come from specific maps.',
                'The screenshot localization tokens and turret appearance are separate visual observations, not parsed by this script.',
                'Original RTTI factory class can be shared by multiple entity classnames.',
            ]}

if __name__=='__main__':
    if len(sys.argv) not in (3,4):raise SystemExit(__doc__)
    r=audit(Path(sys.argv[1]),Path(sys.argv[2]))
    print(f"PASS: {r['missing_entity_attempts']} failures, {r['distinct_missing_classnames']} distinct names, {r['original_rtti_factory_registrations_verified']} verified original entity-factory registrations")
    for row in r['entity_factory_evidence'][:10]:
        print(f"{row['failed_creation_attempts']:3} {row['classname']:34} => {row['factory_cpp_class']}")
    if len(sys.argv)==4:
        Path(sys.argv[3]).write_text(json.dumps(r,indent=2)+'\n',encoding='utf-8')