#!/usr/bin/env python3
"""SiN Episodes original 2006 elite grunt and take-cover goal cross-audit.

Read-only; original server/client hashes pinned. Reads the existing DLLs,
original scripts.zip, and user console logs. Emits metadata-only JSON.
Run: python sin_elite_cover_audit_12.py [root_folder]
"""
from __future__ import annotations
import collections,hashlib,json,re,struct,sys,zipfile
from pathlib import Path
from sin_missing_entity_atlas_audit import PE, EXPECTED_CLIENT, EXPECTED_SERVER, LOG_FILES

ROOT=Path(sys.argv[1]) if len(sys.argv)>1 else Path(__file__).resolve().parent
SERVER=ROOT/'sin_private/server.dll'
CLIENT=ROOT/'sin_private/client.dll'
if not SERVER.exists(): SERVER=ROOT/'server.dll'
if not CLIENT.exists(): CLIENT=ROOT/'client.dll'


def audit():
 p=PE(SERVER); checks=[]
 def check(x,note):
  if not x: raise AssertionError(note)
  checks.append(note)
 def eq(got,want,note):check(got==want, f'{note} ({got!r} vs {want!r})')
 def ins(addr,op):p.assert_instruction(addr,op);checks.append(f'bytes@{addr:08X}')
 def call(addr,target):p.assert_call(addr,target);checks.append(f'call@{addr:08X}->{target:08X}')
 def field(d,name):return next(x for x in d['fields'] if x['internal_name']==name)
 check(p.sha==EXPECTED_SERVER,'original server SHA256')
 check(hashlib.sha256(CLIENT.read_bytes()).hexdigest()==EXPECTED_CLIENT,'original client SHA256')
 factories={n:p.factory(n) for n in ('npc_druglab_grunt_elite','npc_druglab_grunt_pistol','npc_druglab_grunt_assault_rifle','npc_druglab_grunt_scattergun','ai_goal_takecover','weapon_chaingun')}
 for name,cxx in [('npc_druglab_grunt_elite','CNPC_EliteGrunt'),('npc_druglab_grunt_pistol','CNPC_MobGrunt'),('npc_druglab_grunt_assault_rifle','CNPC_MobGrunt'),('npc_druglab_grunt_scattergun','CNPC_MobGrunt'),('ai_goal_takecover','CAI_TakeCoverGoal'),('weapon_chaingun','CWeaponChaingun')]:eq(factories[name]['class'],cxx,'factory '+name)
 elite=p.datamap('CNPC_EliteGrunt');goal=p.datamap('CAI_TakeCoverGoal')
 eq(elite['declared_records'],2,'elite own datamap count')
 eq([(r['internal_name'],r['offset'],r['field_type_code']) for r in elite['fields']], [('m_canFire','0xFD9',6),('m_pChaingun','0xFDC',13)],'elite field layouts')
 eq(goal['declared_records'],8,'goal fields count')
 goal_expected=[('Aggressiveness','m_aggressiveness','0x3C8'),('AbandonIfEnemyHides','m_bAbandonIfEnemyHides','0x3CC'),('CustomCoverOnReload','m_customParams.fCoverOnReload','0x3D0'),('CustomMinTimeShots','m_customParams.minTimeShots','0x3D4'),('CustomMaxTimeShots','m_customParams.maxTimeShots','0x3D8'),('CustomMinShots','m_customParams.minShots','0x3DC'),('CustomMaxShots','m_customParams.maxShots','0x3E0'),('CustomOddsCover','m_customParams.oddsCover','0x3E4')]
 for ext,internal,offset in goal_expected:
  r=field(goal,internal)
  eq((r['external_name'],r['offset']),(ext,offset),'map key '+ext)
 # Elite spawn calls shared base; it obtains an active weapon, RTTI-casts to CWeaponChaingun,
 # stores the enthandle to +0xfdc, or invalid handle -1 on mismatch.
 eq(p.u32(p.primary_vtable('CNPC_EliteGrunt')+0x64),0x22355820,'elite spawn vtable slot')
 call(0x223558cf,0x2235f2a0) # elite calls GruntBase spawn
 ins(0x223558da,'c6 86 d9 0f 00 00 00')
 ins(0x2235590c,'39 be 9c 00 00 00')
 ins(0x2235594c,'39 be 98 00 00 00')
 call(0x223559e4,0x222aa640) # access equipped weapon
 call(0x223559ea,0x223a5bab) # RTTI dynamic cast
 eq(p.text(0x2252da40),'.?AVCWeaponChaingun@@','RTTI cast destination')
 eq(p.text(0x224de308),'.?AVCBaseCombatWeapon@@','RTTI cast source')
 ins(0x223559ff,'89 86 dc 0f 00 00')
 ins(0x22355a0a,'c7 86 dc 0f 00 00 ff ff ff ff')
 ins(0x22355d1c,'c7 86 dc 0f 00 00 ff ff ff ff') # ctor initializes invalid handle
 # Elite animation/event transitions and weapon subroutines.
 for addr,op in [(0x22355489,'83 f8 03'),(0x22355494,'83 f8 43'),(0x22355499,'83 f8 47'),(0x223556a9,'83 f8 43'),(0x223556b2,'83 f8 47'),(0x2235550a,'88 86 d9 0f 00 00'),(0x22355737,'80 be d9 0f 00 00 00')]:ins(addr,op)
 call(0x223554bA,0x22315440)
 call(0x22355503,0x22315320)
 call(0x22355645,0x22315390)
 call(0x22355772,0x22315320)
 eq(p.read(0x2235565c,1),b'\xe9','elite behavior tail-jumps parent update')
 check(p.u32(p.primary_vtable('CNPC_EliteGrunt')+0x424)==0x223555f0,'elite vtable delegates weapon tick')
 # weapon finite-state machine, elapsed time in fields +0x58c/+0x590; state at +0x594
 for addr,op in [(0x22315323,'8b 86 94 05 00 00'),(0x22315329,'83 f8 03'),(0x22315343,'c7 86 94 05 00 00 01 00 00 00'),(0x22315393,'8b 86 94 05 00 00'),(0x223153c4,'c7 86 94 05 00 00 00 00 00 00'),(0x223153eb,'c7 86 94 05 00 00 03 00 00 00'),(0x22315413,'c7 86 94 05 00 00 02 00 00 00'),(0x22315440,'c7 81 94 05 00 00 03 00 00 00')]:ins(addr,op)
 jump=[p.u32(0x22315374+4*n) for n in range(4)]
 eq(jump,[0x22315335,0x2231536a,0x2231536e,0x2231536a],'chaingun four state jump table')
 for addr in (0x223153d8,0x22315400): ins(addr,('d8 a6 8c 05 00 00' if addr==0x223153d8 else 'd8 a6 90 05 00 00'))
 eq(struct.unpack('<f',p.read(0x22477c5c,4))[0],1.0,'state-machine interval constant 1.0 seconds')
 # Exact compiled elite schedule descriptions.
 elite_schedules={}
 for name in ('SCHED_ELITE_MELEE_ATTACK1','SCHED_ELITE_RANGE_ATTACK1'):
  pattern=b'\t\t'+name.encode()+b'\tTasks'; start=p.raw.find(pattern)
  check(start>=0,'elite schedule exists '+name)
  content=p.raw[start:p.raw.find(b'\0',start)].decode('ascii','replace')
  elite_schedules[name]=content.replace('\t',' ').strip()
  check('TASK_ANNOUNCE_ATTACK' in content,'elite announce attack '+name)
 check(elite_schedules['SCHED_ELITE_RANGE_ATTACK1'].count('TASK_RANGE_ATTACK1')==4,'elite repeated range attack four tasks')
 # Cover goal constructor and static preset table. Indices 0..4 have 7 4-byte fields.
 ins(0x22399a1e,'c7 85 c8 03 00 00 00 00 00 00')
 ins(0x22399a28,'c6 85 cc 03 00 00 00')
 ins(0x22399a35,'b9 07 00 00 00')
 ins(0x22399a3a,'be a8 b5 53 22')
 eq(p.text(0x2253bdec+8),'.?AVCAI_TakecoverBehavior@@','behavior cast destination RTTI')
 eq(p.text(0x224e4bc4+8),'.?AVCAI_BehaviorBase@@','behavior cast source RTTI')
 call(0x22399aac,0x22399b00)
 call(0x22399c2c,0x22399b00)
 call(0x22399b42,0x223a5bab) # dynamic_cast to takecover behavior
 ins(0x22399b85,'8b 88 c8 03 00 00')
 ins(0x22399b96,'80 b8 cc 03 00 00 00')
 ins(0x22399b9e,'8d 34 b5 00 b6 53 22')
 ins(0x22399ba5,'b9 07 00 00 00')
 ins(0x22399bae,'f3 a5')
 ins(0x22399bb2,'c7 44 24 24 00 00 00 00')
 ins(0x22399bc6,'b9 07 00 00 00')
 ins(0x22399bcf,'f3 a5')
 call(0x22399be5,0x220b1a80)
 call(0x22399bfb,0x220b1aa0)
 ins(0x22399acb,'68 e8 03 00 00')
 ins(0x22399c45,'68 e9 03 00 00')
 # Preset row has 7 members: cover-on-reload, min-time, max-time,
 # min-shots, max-shots, cover odds, seventh unknown-time/weight field.
 presets=[]
 expected=[(1,4.0,8.0,2,4,50,30.0),(1,2.0,5.0,3,5,25,20.0),(1,0.6000000238418579,2.5,3,6,25,10.0),(1,0.20000000298023224,1.5,5,8,10,10.0),(0,0.0,0.0,100,100,0,5.0)]
 for ix,expect in enumerate(expected):
  data=p.read(0x2253b600+28*ix,28)
  ints=struct.unpack('<7I',data); floats=struct.unpack('<7f',data)
  row=(ints[0],floats[1],floats[2],ints[3],ints[4],ints[5],floats[6])
  eq(row,expect,'cover aggressiveness preset '+str(ix))
  presets.append({'index':ix,'cover_on_reload':row[0],'min_time_between_shots':row[1], 'max_time_between_shots':row[2],'min_shots':row[3],'max_shots':row[4],'cover_odds_value':row[5],'unidentified_seventh_float':row[6]})
 initial=p.read(0x2253b5a8,28)
 initial_words=struct.unpack('<7I',initial)
 eq((initial_words[0],struct.unpack_from('<f',initial,4)[0],struct.unpack_from('<f',initial,8)[0],initial_words[3],initial_words[4],initial_words[5],initial_words[6]),(1,1.5,2.5,1,3,25,0),'constructor custom parameter defaults')
 # Asset/console crosswalk. Don't claim class unique instances: these are log attempts.
 counts=collections.Counter()
 for name in LOG_FILES:
  raw=(ROOT/name).read_text(encoding='utf8',errors='replace')
  counts.update(re.findall(r'Attempted to create unknown entity type ([a-zA-Z0-9_]+)!',raw))
 eq(counts['npc_druglab_grunt_elite'],69,'elite failures in five logs')
 eq(counts['ai_goal_takecover'],26,'takecover goal failures in five logs')
 eq(sum(counts.values()),1531,'full five-log count')
 with zipfile.ZipFile(ROOT/'scripts.zip') as z:
  data=z.read('scripts/weapon_chaingun.txt').decode('utf8','replace')
  for literal in ['"clip_size"\t\t\t"30"','"default_clip"\t\t\t"30"','"primary_ammo"\t\t\t"CHAIN"','"ITEM_FLAG_NOPLAYERPICKUP"\t"1"','Weapon_Chaingun.NPC_SpinUp','Weapon_Chaingun.NPC_SpinDown']:
   check(literal in data,'chaingun script '+literal)
  check('scripts/npc_sounds_elite.txt' in z.namelist(),'elite sound config exists')
 result={'chapter':'Elite grunt and take-cover behavior, decompilation 12',
  'disclaimer':'read-only static evidence and partial pseudocode, not a complete decompilation or tested quest runtime',
  'hashes':{'server.dll':p.sha,'client.dll':hashlib.sha256(CLIENT.read_bytes()).hexdigest(),'scripts.zip':hashlib.sha256((ROOT/'scripts.zip').read_bytes()).hexdigest()},
  'checks_passed':len(checks),'assertions':checks,'factories':factories,'elite':{'datamap':elite,'primary_vtable_va':f'0x{p.primary_vtable("CNPC_EliteGrunt"):08X}','spawn_va':'0x22355820','weapon_runtime_cast_va':'0x223559EA','weapon_handle_offset':'0xFDC','can_fire_offset':'0xFD9','animation_related_handler_va':'0x22355480','event_numbers_observed':[3,0x43,0x47], 'weapon_ticking_va':'0x223555F0','chaingun_fsm':{'state_offset':'0x594','clock_offsets':['0x58C','0x590'],'state_ids':[0,1,2,3],'check_true_in_state':2,'timing_constant_seconds':1.0,'start_check_va':'0x22315320','update_va':'0x22315390','force_end_va':'0x22315440','check_jump_table':[f'0x{x:08X}' for x in jump]},'schedule_descriptions':elite_schedules},
  'take_cover':{'datamap':goal,'constructor_va':'0x223999F0','preset_table_va':'0x2253B600','preset_stride_bytes':28,'presets':presets,'default_custom_words':[f'0x{x:08X}' for x in initial_words],'behavior_type':p.text(0x2253bdec+8),'actor_behavior_finder_va':'0x22399B00','parameter_apply_va':'0x22399B80','configuration_helper_calls':['0x220B1A80','0x220B1AA0'],'behavior_events_observed':['0x3E8','0x3E9'],'caveats':['The seventh float was not named in the datamap; do not assign semantic label from inference.','The code indexes presets using m_aggressiveness; valid user input range/validation not established by this trace.','Custom* key fields exist but direct use by 0x22399B80 is not demonstrated.','Not every compiled schedule is proven to execute in campaign combat.']},
  'hl2_log_creation_attempts':{'npc_druglab_grunt_elite':counts['npc_druglab_grunt_elite'],'ai_goal_takecover':counts['ai_goal_takecover']}}
 return result

if __name__=='__main__':
 out=audit()
 path=ROOT/'sin_elite_cover_evidence_12.json'
 path.write_text(json.dumps(out,indent=2,ensure_ascii=False)+'\n',encoding='utf8')
 print('PASS',out['checks_passed'],'binary+asset+log assertions')
 print('Evidence',path)