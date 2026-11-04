#!/usr/bin/env python3
"""Read-only original 2006 SiN Episodes grunt family audit.

Run: python sin_grunt_combat_audit_11.py [directory containing sin_private/server.dll]
Requires sin_missing_entity_atlas_audit.py beside this file. Never modifies DLLs.
"""
from __future__ import annotations
import collections
import hashlib
import json
import re
import struct
import sys
import zipfile
from pathlib import Path

from sin_missing_entity_atlas_audit import PE

ROOT = Path(sys.argv[1]) if len(sys.argv) > 1 else Path(__file__).resolve().parent
SERVER = ROOT/'sin_private'/'server.dll'
if not SERVER.exists(): SERVER=ROOT/'server.dll'
SCRIPTS = ROOT/'scripts.zip'
LOGS = [
 'sin stuff intro-u4-1.txt', 'sin stuff u4-2 to pit 2.txt',
 'sin stuff highrise.txt', 'sin stuff finale.txt', 'sin stuff arena.txt'
]
EXPECTED_SERVER='568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650'
EXPECTED_CLIENT='6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d'
CLIENT=ROOT/'sin_private'/'client.dll'
if not CLIENT.exists():CLIENT=ROOT/'client.dll'


def audit():
 p=PE(SERVER)
 clientsha=hashlib.sha256(CLIENT.read_bytes()).hexdigest()
 assert p.sha==EXPECTED_SERVER and clientsha==EXPECTED_CLIENT,(p.sha,clientsha)
 assert SCRIPTS.is_file()
 checks=2
 def ins(va,hexbytes):
  nonlocal checks
  p.assert_instruction(va,hexbytes);checks+=1
 def call(va,target):
  nonlocal checks
  p.assert_call(va,target);checks+=1
 def eq(got,expected,note):
  nonlocal checks
  assert got==expected,(note,got,expected)
  checks+=1
 def fld(c,name):
  for f in p.datamap(c)['fields']:
   if f['internal_name']==name:return f
  raise AssertionError('Missing original datamap member: '+c+' '+name)
 factories={n:p.factory(n) for n in [
  'npc_druglab_grunt_pistol','npc_druglab_grunt_scattergun',
  'npc_druglab_grunt_assault_rifle','npc_druglab_grunt_elite',
  'npc_sintek_grunt','weapon_chaingun']}
 for n in ['npc_druglab_grunt_pistol','npc_druglab_grunt_scattergun','npc_druglab_grunt_assault_rifle']:
  eq(factories[n]['class'],'CNPC_MobGrunt',n)
 eq(factories['npc_druglab_grunt_elite']['class'],'CNPC_EliteGrunt','elite factory')
 eq(factories['npc_sintek_grunt']['class'],'CNPC_SintekGrunt','sintek factory')
 eq(factories['weapon_chaingun']['class'],'CWeaponChaingun','chaingun factory')
 names={
  'npc_druglab_grunt_pistol':0x2249AA20,
  'npc_druglab_grunt_scattergun':0x2249AA00,
  'npc_druglab_grunt_assault_rifle':0x22490EA0,
  'weapon_npc_pistol':0x2249A974,
  'weapon_scattergun':0x2249A988,
  'weapon_assault_rifle':0x2249A99C,
  'npc_druglab_grunt_elite':0x2249A9B4,
  'models/characters/MobGrunt/MobGrunt.mdl':0x2249A9CC,
 }
 for name,va in names.items():eq(p.text(va),name,'string '+name)
 ins(0x22369e40,'83 ec 64') # MobGrunt variant-dependent spawn implementation
 ins(0x22369e4d,'be 20 aa 49 22') # pistol classname
 ins(0x22369e63,'be 00 aa 49 22') # scattergun classname
 ins(0x22369e74,'be a0 0e 49 22') # assault classname
 ins(0x22369ed0,'be 74 a9 49 22') # weapon_npc_pistol
 ins(0x22369fb0,'be 88 a9 49 22') # weapon_scattergun
 ins(0x2236a090,'68 9c a9 49 22') # weapon_assault_rifle
 call(0x22369e08,0x2235de00) # MobGrunt precache -> parent
 call(0x2236a0ff,0x2235f2a0) # MobGrunt spawn -> GruntBase spawn
 # Per-variant initial-health path: read three config float pointers, truncate, then compare to health
 for va,ref in [(0x2236a141,0x2259CC50),(0x2236a1ed,0x2259CC98),(0x2236a297,0x2259CCE0)]:
  eq(p.read(va,1),b'\xa1','health config global load')
  eq(p.u32(va+1),ref,'health config global VA')
 for va in [0x2236a165,0x2236a211,0x2236a2bb]:
  ins(va,'39 b5 98 00 00 00') # +0x98 max health comparison
 for va in [0x2236a2ff]:ins(va,'39 b5 9c 00 00 00') # +0x9c current health
 # Actually another max/health path occurs in the common GruntBase spawn.
 # Map-facing input functions and their side effects
 base=p.datamap('CNPC_GruntBase')
 eq(base['declared_records'],39,'grunt fields')
 eq(p.datamap('CNPC_MobGrunt')['datamap_va'],base['datamap_va'],'mob inherits datamap')
 eq(p.datamap('CNPC_SintekGrunt')['datamap_va'],base['datamap_va'],'sintek inherits datamap')
 input_expected={
   'StartPatrolling':('InputStartPatrolling','0x22357E30'),
   'StopPatrolling':('InputStopPatrolling','0x22357E40'),
   'Assault':('InputAssault','0x22357E50'),
   'ThrowGrenades':('InputThrowGrenades','0x22357EA0'),
   'LookOff':('InputLookOff','0x2235F260'),
   'LookOn':('InputLookOn','0x2235DDC0'),
 }
 inputs=[]
 for external,(internal,address) in input_expected.items():
  item=fld('CNPC_MobGrunt',internal)
  eq((item['external_name'],item['input_handler_va']),(external,address),'input '+external)
  inputs.append({'map_input':external,'internal_handler':internal,'entrypoint':address})
 ins(0x22357e30,'c6 81 65 0c 00 00 01')
 ins(0x22357e40,'c6 81 65 0c 00 00 00')
 ins(0x2235e7e6,'80 be 65 0c 00 00 00')
 ins(0x2235e7f2,'b8 64 00 00 00')
 ins(0x2235ddc8,'25 ff ff fe ff') # LookOn clears bit 0x10000
 ins(0x2235f268,'0d 00 00 01 00') # LookOff sets bit 0x10000
 ins(0x22357ea4,'83 78 18 06') # field type 6 (boolean)
 ins(0x22357ead,'c7 81 70 0c 00 00 00 00 00 00')
 ins(0x22357eb7,'88 81 6c 0c 00 00')
 ins(0x22357ec2,'89 81 70 0c 00 00')
 ins(0x22357ec8,'88 81 6c 0c 00 00')
 ins(0x22357e82,'6a 03') # Assault sends integer constant to a helper
 call(0x22357e8b,0x220cfc00)
 # Grenade-mode timer selection and confirmed usages.
 fl={}
 for name,va in [('fast_check',0x223d99d0),('normal_check',0x223d22f8),('fast_followup',0x22407cac),('normal_followup',0x223de180)]:
  fl[name]=struct.unpack('<f',p.read(va,4))[0]
 eq(fl,{'fast_check':0.25,'normal_check':1.0,'fast_followup':2.5,'normal_followup':6.0},'float timers')
 ins(0x22357fa0,'80 b9 6c 0c 00 00 00')
 ins(0x22357fc0,'80 b9 6c 0c 00 00 00')
 for site,target in [(0x2235b159,0x22357fa0),(0x2235b34b,0x22357fa0),(0x2235b36d,0x22357fa0),(0x2235c3dd,0x22357fc0),(0x2235eb25,0x22357fc0)]: call(site,target)
 ins(0x2235b177,'d9 9e 80 0c 00 00') # next grenade check float
 ins(0x2235eb32,'d9 9e 80 0c 00 00')
 # Verify vtable relationship; compare known slot counts + exact offsets.
 vt={};a={}
 for cls in ('CNPC_GruntBase','CNPC_MobGrunt','CNPC_SintekGrunt','CNPC_EliteGrunt'):
  v=p.primary_vtable(cls)
  assert v is not None
  vt[cls]=f'0x{v:08X}'
  a[cls]=p.all_text_function_pointers(v)
  eq(len(a[cls]),532 if cls!='CNPC_EliteGrunt' else 533,'vtable count '+cls)
 def diffs(c):return [{'slot_offset':f'0x{i*4:X}','entrypoint':f'0x{x:08X}','parent_entrypoint':f'0x{y:08X}'} for i,(x,y) in enumerate(zip(a[c],a['CNPC_GruntBase'])) if x!=y]
 mobdiff=diffs('CNPC_MobGrunt');sindiff=diffs('CNPC_SintekGrunt');elitediff=diffs('CNPC_EliteGrunt')
 eq([(x['slot_offset'],x['entrypoint']) for x in mobdiff],[('0x0','0x2236A3B0'),('0x64','0x22369E40'),('0x68','0x22369D90'),('0xD0','0x2236A3A0')],'MobGrunt four overrides')
 eq(len(sindiff),1,'Sintek Grunt override count')
 eq(len(elitediff),23,'EliteGrunt changed slots incl dtor')
 elite=p.datamap('CNPC_EliteGrunt')
 eq([r['internal_name'] for r in elite['fields']],['m_canFire','m_pChaingun'],'elite own datamap fields')
 # Count original names in binary, NOT evidence of runtime exercise/registration for every name.
 symbol_groups={}
 for kind,pattern in [('grunt_schedules',rb'SCHED_GRUNT_[A-Z0-9_]+'),('elite_schedules',rb'SCHED_ELITE_[A-Z0-9_]+'),('grunt_tasks',rb'TASK_GRUNT_[A-Z0-9_]+'),('grunt_conditions',rb'COND_GRUNT_[A-Z0-9_]+'),('jetpack_schedules',rb'SCHED_JETPACK_[A-Z0-9_]+')]:
  symbol_groups[kind]=sorted(set(re.findall(pattern,p.raw.decode('latin1').encode('latin1'))))
  symbol_groups[kind]=[x.decode('ascii') for x in symbol_groups[kind]]
 eq({k:len(v) for k,v in symbol_groups.items()},
    {'grunt_schedules':38,'elite_schedules':2,'grunt_tasks':19,'grunt_conditions':12,'jetpack_schedules':6},'compiled AI labels count')
 assert b'SCHED_GRUNT_KICK_GRENADE' in p.raw and b'SCHED_GRUNT_HELP_STUNNED' in p.raw and b'SCHED_GRUNT_ATTACK_BARREL' in p.raw
 checks+=3
 # Verify selected whole compiled AI schedule descriptions exist as task flows, not merely labels.
 schedule_task_anchors={
   'SCHED_GRUNT_ATTACK_BARREL': ['TASK_GRUNT_TARGET_BARREL','TASK_FACE_ENEMY','TASK_RANGE_ATTACK1'],
   'SCHED_GRUNT_KICK_GRENADE': ['TASK_GRUNT_DEFER_SQUAD_GRENADE_KICK','TASK_FACE_ENEMY','TASK_GRUNT_KICK_ATTACK'],
   'SCHED_GRUNT_HELP_STUNNED': ['TASK_GRUNT_GOTO_TARGET','TASK_GRUNT_FACE_SQUADMATE','TASK_GRUNT_HELP_UP'],
   'SCHED_GRUNT_PATROL': ['TASK_WANDER','TASK_WALK_PATH','TASK_WAIT_FOR_MOVEMENT'],
 }
 for schedule,tasks in schedule_task_anchors.items():
  anchor=b'\t\t'+schedule.encode()+b'\tTasks'
  at=p.raw.find(anchor)
  assert at>=0, schedule
  end=p.raw.find(b'\x00',at)
  body=p.raw[at:end].decode('ascii')
  assert all(t in body for t in tasks),(schedule,tasks,body)
  checks+=len(tasks)+1
 # Original game script crosswalk: separate source from compiled inference.
 weapon_data={};scriptsha=hashlib.sha256(SCRIPTS.read_bytes()).hexdigest()
 with zipfile.ZipFile(SCRIPTS) as z:
  for weapon,expected in {'weapon_npc_pistol':('20','MAG','DUAG'), 'weapon_scattergun':('6','BUCK','FLAK'), 'weapon_assault_rifle':('45','AR','ARNADE'), 'weapon_chaingun':('30','CHAIN',None)}.items():
   fn=f'scripts/{weapon}.txt';content=z.read(fn).decode('utf-8','replace')
   data={}
   for key in ('printname','clip_size','default_clip','primary_ammo','secondary_ammo'):
    m=re.search(r'(?m)^\s*"?'+re.escape(key)+r'"?\s+"([^"\r\n]+)"',content)
    data[key]=m.group(1) if m else None
   eq((data['default_clip'],data['primary_ammo'],data['secondary_ammo']),expected,'KV '+weapon)
   weapon_data[weapon]=data
 # Console ground truth (not unique placements)
 counts=collections.Counter()
 for n in LOGS:
  for c in re.findall(r'Attempted to create unknown entity type ([a-zA-Z0-9_]+)!',(ROOT/n).read_text(encoding='utf8',errors='replace')):counts[c]+=1
 eq(sum(counts.values()),1531,'five logs total')
 eq(sum(counts[n] for n in factories if n.startswith('npc_druglab_grunt') and n!='npc_druglab_grunt_elite'),364,'three MobGrunt combined')
 eq(counts['npc_druglab_grunt_elite'],69,'elite log count')
 result={
  'scope':'Read-only evidence from original PE32 x86 server.dll and client.dll, scripts.zip, five HL2 console logs',
  'source_sha256':{'server.dll':p.sha,'client.dll':clientsha,'scripts.zip':scriptsha},
  'checks_passed':checks,
  'original_entity_factories':factories,
  'vtable_primary_va':vt,
  'vtable_differences_vs_CNPC_GruntBase':{'CNPC_MobGrunt':mobdiff,'CNPC_SintekGrunt':sindiff,'CNPC_EliteGrunt':elitediff},
  'grunt_base_datamap':base,
  'elite_datamap':elite,
  'map_inputs':inputs,
  'confirmed_grenade_mode_timing_seconds':fl,
  'original_ai_symbol_spellings':symbol_groups,
  'verified_schedule_task_anchors':schedule_task_anchors,
  'original_weapon_script_data':weapon_data,
  'console_attempt_counts':{n:counts[n] for n in sorted(factories) if n.startswith('npc_')},
  'limitations':['A datamap method can be inherited; same return pointer does not prove an independent class-specific datamap.','Vtable differing slots include destructors and do not count unique gameplay systems.','String labels prove compiled names, not that every AI schedule executes during released gameplay.','Original CVAR runtime values and map override behavior not established.','WeaponData describes archive configuration, not proof of instantiated weapon runtime state.','Damage/accuracy/covers system not completely decompiled.']
 }
 return result

if __name__=='__main__':
 output=audit()
 path=ROOT/'sin_grunt_combat_evidence_11.json'
 path.write_text(json.dumps(output,indent=2,ensure_ascii=False)+'\n')
 print('PASS: {} original code/asset/log assertions'.format(output['checks_passed']))
 print('Report:',path)
 print('MobGrunt changed vtable slots:',len(output['vtable_differences_vs_CNPC_GruntBase']['CNPC_MobGrunt']))
 print('EliteGrunt changed vtable slots:',len(output['vtable_differences_vs_CNPC_GruntBase']['CNPC_EliteGrunt']))