#!/usr/bin/env python3
"""SiN Episodes (2006) Round 13: take-cover behavior and elite chaingun runtime.

Read-only, SHA-pinned original DLL inspection. Never launches game or edits binaries.
Requires local original server/client and the five user console logs.

Usage: python sin_cover_elite_runtime_audit_13.py [/mnt/data]
Outputs: /mnt/data/sin_cover_elite_runtime_evidence_13.json
"""
from __future__ import annotations
import collections, hashlib, json, re, struct, sys
from pathlib import Path
from sin_missing_entity_atlas_audit import PE, EXPECTED_CLIENT, EXPECTED_SERVER, LOG_FILES

ROOT=Path(sys.argv[1]) if len(sys.argv)>1 else Path(__file__).resolve().parent
SERVER=ROOT/'sin_private/server.dll'
CLIENT=ROOT/'sin_private/client.dll'
if not SERVER.exists(): SERVER=ROOT/'server.dll'
if not CLIENT.exists(): CLIENT=ROOT/'client.dll'


def run():
 p=PE(SERVER); checks=[]
 def ok(t,label):
  if not t: raise AssertionError(label)
  checks.append(label)
 def eq(a,b,label):ok(a==b, f'{label}: {a!r} != {b!r}')
 def ins(va,hexdata):
  p.assert_instruction(va,hexdata)
  checks.append(f'code bytes 0x{va:08X}')
 def call(va,target):
  p.assert_call(va,target)
  checks.append(f'call 0x{va:08X} to 0x{target:08X}')
 def f32(va):return struct.unpack('<f',p.read(va,4))[0]
 ok(p.sha==EXPECTED_SERVER,'server SHA matches 2006 original')
 eq(hashlib.sha256(CLIENT.read_bytes()).hexdigest(),EXPECTED_CLIENT,'client SHA matches 2006 original')
 for name,cxx in [('ai_goal_takecover','CAI_TakeCoverGoal'),('npc_druglab_grunt_elite','CNPC_EliteGrunt'),('weapon_chaingun','CWeaponChaingun')]:
  eq(p.factory(name)['class'],cxx,f'factory {name}')
 bv=p.primary_vtable('CAI_TakecoverBehavior')
 eq(bv,0x224AE624,'CAI_TakecoverBehavior primary vtable')
 behavior_funcs=p.all_text_function_pointers(bv)
 selected={
  'CanSelectSchedule':(0x18,0x22398250),
  'BeginBehavior':(0x1c,0x223982c0),
  'BehaviorTick':(0x20,0x22398990),
  'BehaviorEnterState':(0x24,0x223983d0),
  'UpdateShotParameters':(0x28,0x22398390),
  'SelectCombatSchedule':(0x5c,0x220cd2e0),
  'SelectScheduleSpecific':(0x64,0x22398470),
  'SelectTask':(0x7c,0x22398670),
  'StopOrReleaseBehavior':(0x78,0x22398450),
  'ScheduleLookup':(0xdc,0x223984f0),
  'ConditionScheduleLookup':(0xe0,0x22398520),
 }
 # Only slots, not virtual symbolic names, are exact. Keys above are analyst descriptions.
 for k,(offset,addr) in selected.items():
  eq(p.u32(bv+offset),addr,'behavior virtual '+k)
 # Cover goal applies parameters to take-cover behavior and writes a 4-field substructure to NPC.
 goal=p.datamap('CAI_TakeCoverGoal')
 eq(goal['declared_records'],8,'cover map key records')
 ins(0x22399b85,'8b 88 c8 03 00 00') # m_aggressiveness
 ins(0x22399b96,'80 b8 cc 03 00 00 00') # m_bAbandonIfEnemyHides
 ins(0x22399b9e,'8d 34 b5 00 b6 53 22') # preset base/stride
 ins(0x22399bae,'f3 a5') # copy 7 DWORDs to local
 ins(0x22399bb0,'75 08') # if AbandonIfEnemyHides != 0, keep 7th row word
 ins(0x22399bb2,'c7 44 24 24 00 00 00 00') # else zero 7th word
 ins(0x22399bcb,'8d 74 24 0c')
 ins(0x22399bcf,'f3 a5') # copy 7 words to behavior + 0x14
 ins(0x22399bd3,'8d 88 a8 09 00 00') # NPC substructure + 0x9A8
 call(0x22399be5,0x220b1a80) # NPC +0x9b0/0x9b2 two WORDs
 call(0x22399bfb,0x220b1aa0) # NPC +0x9b4/0x9b8 two DWORDs
 ins(0x220b1a8a,'66 89 41 08')
 ins(0x220b1a8e,'66 89 51 0a')
 ins(0x220b1aa8,'89 41 0c')
 ins(0x220b1aab,'89 51 10')
 call(0x22399aac,0x22399b00)
 call(0x22399ae9,0x22399b80)
 call(0x22399c2c,0x22399b00)
 call(0x22399c60,0x22399b80)
 ins(0x22399acb,'68 e8 03 00 00')
 ins(0x22399c45,'68 e9 03 00 00')
 # Behavior copies existing NPC values at +0x9A8, writes replacement values, preserves/restores +0xA20.
 call(0x223982d8,0x220b1ae0)
 call(0x223982ee,0x220b1b00)
 call(0x22398304,0x220b1a80)
 call(0x2239831a,0x220b1aa0)
 call(0x2239832a,0x220b1b40)
 ins(0x22398332,'8b 90 20 0a 00 00')
 ins(0x2239833b,'c7 80 20 0a 00 00 ff ff 7f 7f')
 eq(p.u32(0x22398342),0x7f7fff,'FLT_MAX immediate trailing bytes') if False else None
 call(0x22398993,0x223988b0)
 ins(0x2239899e,'89 88 20 0a 00 00')
 call(0x223989b5,0x220b1a80)
 call(0x223989cb,0x220b1aa0)
 # Eligibility: time guard, NPC condition 0x08000000, state=3 and existing weapon.
 ins(0x22398259,'d9 46 68')
 eq(f32(0x223d2590),0.0,'eligibility float zero')
 ins(0x22398282,'a9 00 00 00 08')
 ins(0x2239829a,'83 b9 50 08 00 00 03')
 call(0x223982a3,0x222aa640)
 # Behavior's explicit schedule and code paths.
 schedule=p.text(0x224ad79d)
 for tok in ['SCHED_COVER_TAKE_COVER_FROM_BEST_SOUND','TASK_STOP_MOVING','TASK_FIND_COVER_FROM_BEST_SOUND','TASK_RUN_PATH','TASK_WAIT_FOR_MOVEMENT','TASK_WAIT_RANDOM','TASK_FACE_REASONABLE']:
  ok(tok in schedule,'compiled behavior schedule has '+tok)
 ins(0x22398526,'6a 2b')
 call(0x22398528,0x220397c0)
 ins(0x22398534,'6a 0c')
 call(0x22398536,0x220397c0)
 call(0x22398544,0x220398a0)
 call(0x2239854e,0x220398a0)
 ins(0x22398553,'6a 0a 6a 00')
 ins(0x22398560,'83 f8 08')
 ins(0x22398565,'b8 a1 86 01 00')
 ins(0x2239865d,'b8 a0 86 01 00')
 group_ids=[0x186a0,0x186a1,0x186a2,0x186a3,0x186a4,0x186a5,0x186a6]
 for index,va in enumerate([0x2239865d,0x22398565,0x223985dc,0x223985f5,0x2239860e,0x22398627,0x22398114]):
  ins(va,'b8 '+struct.pack('<I',group_ids[index]).hex(' '))
 ins(0x223985d1,'66 83 b9 a4 03 00 00 64')
 ins(0x223985ea,'66 83 b8 a4 03 00 00 65')
 ins(0x22398603,'66 83 b8 a4 03 00 00 6b')
 ins(0x2239861c,'66 83 b8 a4 03 00 00 6c')
 ins(0x22398675,'83 fb 10')
 ins(0x223987a4,'83 fb 42')
 ins(0x223987e5,'83 fb 01')
 ins(0x223987ea,'83 fb 48')
 # Six-case task dispatcher jump table based on 0x186A0..0x186A5.
 ins(0x22399662,'05 60 79 fe ff')
 ins(0x22399667,'83 f8 05')
 ins(0x22399671,'ff 24 85 80 99 39 22')
 dispatch=[p.u32(0x22399980+4*i) for i in range(6)]
 eq(dispatch,[0x2239971E,0x2239982D,0x22399940,0x22399955,0x223996E5,0x22399678], 'six task dispatch entries')
 ins(0x22399757,'68 00 00 80 40')
 ins(0x2239975c,'68 00 00 00 40')
 ins(0x22399778,'6a 08')
 ins(0x22399818,'6a 0b')
 # Elite sound/weapon gate, animation codes 0x43 / 0x47 without guessed symbolic names.
 eq(p.u32(p.primary_vtable('CNPC_EliteGrunt')+0x424),0x223555f0,'elite calls tick from primary vtable')
 ins(0x22355494,'83 f8 43')
 ins(0x22355499,'83 f8 47')
 call(0x223554ba,0x22315440)
 call(0x22355503,0x22315320)
 ins(0x2235550a,'88 86 d9 0f 00 00')
 ins(0x223556a9,'83 f8 43')
 ins(0x223556b2,'83 f8 47')
 ins(0x22355737,'80 be d9 0f 00 00 00')
 call(0x22355772,0x22315320)
 call(0x22355645,0x22315390)
 ins(0x22315343,'c7 86 94 05 00 00 01 00 00 00')
 ins(0x22315413,'c7 86 94 05 00 00 02 00 00 00')
 ins(0x223153eb,'c7 86 94 05 00 00 03 00 00 00')
 ins(0x223153c4,'c7 86 94 05 00 00 00 00 00 00')
 eq(f32(0x22477c5c),1.0,'chaingun state seconds threshold')
 # Original logs used to justify highest-impact restoration priority.
 counts=collections.Counter()
 for fn in LOG_FILES:
  counts.update(re.findall(r'Attempted to create unknown entity type ([a-zA-Z0-9_]+)!',(ROOT/fn).read_text('utf8',errors='replace')))
 eq(sum(counts.values()),1531,'five-log total attempts')
 eq(counts['ai_goal_takecover'],26,'cover goal failures')
 eq(counts['npc_druglab_grunt_elite'],69,'elite failures')
 presets=[]
 for i in range(5):
  words=struct.unpack('<7I',p.read(0x2253B600+28*i,28))
  fs=struct.unpack('<7f',p.read(0x2253B600+28*i,28))
  presets.append({'index':i,'cover_on_reload':words[0],'min_time_between_shots':fs[1],'max_time_between_shots':fs[2], 'min_shots':words[3],'max_shots':words[4], 'odds_cover':words[5],'seventh_float_when_abandon_enabled':fs[6], 'seventh_float_when_abandon_disabled':0.0})
 result={
  'chapter':'13 - active takecover behavior, six-task dispatch, and elite weapon event gates',
  'scope':'verified original 2006 binary code and archived console logs; incomplete static decompilation, no runnable port',
  'sha256':{'server.dll':p.sha,'client.dll':hashlib.sha256(CLIENT.read_bytes()).hexdigest()},
  'checks_passed':len(checks),'checks':checks,
  'takecover':{
   'behavior_class':'CAI_TakecoverBehavior','vtable_va':f'0x{bv:08X}','vtable_entries':len(behavior_funcs),
   'selected_virtuals':{k:{'vtable_offset':f'0x{off:X}','function_va':f'0x{va:08X}','note':'descriptive analyst label; not recovered C++ virtual name'} for k,(off,va) in selected.items()},
   'map_goal_apply_va':'0x22399B80','map_goal_begin_va':'0x22399AA0','map_goal_stop_va':'0x22399C20',
   'copies_28_bytes_to_behavior_at':'behavior+0x14',
   'npc_cover_config_base_offset':'NPC+0x9A8',
   'npc_cover_config_written_offsets':['NPC+0x9B0 (16-bit)','NPC+0x9B2 (16-bit)','NPC+0x9B4 (32-bit)','NPC+0x9B8 (32-bit)'],
   'behavior_begin_va':'0x223982C0','behavior_end_va':'0x22398990',
   'npc_preserved_field':'NPC+0xA20','npc_temporary_value':'0x7F7FFFFF (FLT_MAX)',
   'abandon_if_enemy_hides_effect':'when false zero 7th float; when true preserve row 7th float',
   'presets':presets,'schedule_text':schedule,
   'schedule_selection_va':'0x22398520','schedule_ids':[f'0x{x:X}' for x in group_ids],
   'schedule_candidate_branch':'0x22398553 calls an RNG with arguments 0..10; >8 selects schedule 0x186A1, no probability asserted',
   'target_handle_field':'NPC+0xA18; likely enemy but not yet independently assigned an original C++ member name',
   'target_classification_codes_observed':[0x64,0x65,0x6B,0x6C],
   'six_task_dispatch_va':'0x22399650','six_task_jump_table_va':'0x22399980','six_task_targets':[f'0x{x:08X}' for x in dispatch],
   'active_gate_va':'0x22398250',
   'caveats':['C++ method names for vtable slots are analyst descriptions, not recovered symbols.','A full schedule/animation playback model remains unresolved.','The function that reads cover odds at runtime remains to be traced.','Custom map values versus table index bounds require further confirmation.']
  },
  'elite':{'class':'CNPC_EliteGrunt','animation_event_handler_va':'0x22355480','secondary_event_handler_va':'0x223556A0',
     'event_ids':[3,0x43,0x47], 'id_0x43':'queries CWeaponChaingun::0x22315320 and stores result to elite+0xFD9',
     'id_0x47':'calls CWeaponChaingun::0x22315440, setting state 3',
     'control_flag_offset':'elite+0xFD9','weapon_handle_offset':'elite+0xFDC',
     'weapon_tick_override_va':'0x223555F0','weapon_state_tick_va':'0x22315390',
     'weapon_state_word':'chaingun+0x594','elapsed_time_threshold_seconds':1.0,
     'caveats':['Event numeric IDs are verified, but original symbolic animation-event names remain unknown.','No fixed RPM or burst cadence is established from state-timer data alone.']},
  'log_context':{'ai_goal_takecover_failed_attempts':counts['ai_goal_takecover'],'npc_druglab_grunt_elite_failed_attempts':counts['npc_druglab_grunt_elite'],'all_failed_attempts':sum(counts.values())}}
 return result

if __name__=='__main__':
 out=run();path=ROOT/'sin_cover_elite_runtime_evidence_13.json'
 path.write_text(json.dumps(out,indent=2,ensure_ascii=False)+'\n',encoding='utf8')
 print('PASS',out['checks_passed'],'assertions; evidence:',path)