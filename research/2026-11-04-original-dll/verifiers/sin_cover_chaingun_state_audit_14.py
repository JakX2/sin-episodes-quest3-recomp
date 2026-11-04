#!/usr/bin/env python3
"""SiN Episodes original x86 DLL read-only Round 14 static audit.

Verifies chaingun spin state machine, elite animation-event gates, and
CAI_TakecoverBehavior retry/backoff paths. Does not modify or execute game.
Usage: python sin_cover_chaingun_state_audit_14.py [/mnt/data]
"""
from __future__ import annotations
import sys, hashlib, struct, json, zipfile, re, collections
from pathlib import Path
from sin_missing_entity_atlas_audit import PE, EXPECTED_SERVER, EXPECTED_CLIENT, LOG_FILES

ROOT=Path(sys.argv[1]) if len(sys.argv)>1 else Path(__file__).resolve().parent
SERVER=ROOT/'sin_private/server.dll'
CLIENT=ROOT/'sin_private/client.dll'
SCRIPTS=ROOT/'scripts.zip'

def run():
    p=PE(SERVER); checks=[]
    def ok(test,what):
        if not test: raise AssertionError(what)
        checks.append(what)
    def eq(actual,expected,what):
        ok(actual==expected,f'{what}; expected {expected!r}, got {actual!r}')
    def ins(va,code,label):
        p.assert_instruction(va,code); checks.append(f'{label} [0x{va:08X}]')
    def call(va,target,label):
        p.assert_call(va,target); checks.append(f'{label} [0x{va:08X} -> 0x{target:08X}]')
    def f32(va): return struct.unpack('<f',p.read(va,4))[0]
    def f64(va): return struct.unpack('<d',p.read(va,8))[0]
    eq(p.sha,EXPECTED_SERVER,'original 2006 server.dll SHA256')
    eq(hashlib.sha256(CLIENT.read_bytes()).hexdigest(),EXPECTED_CLIENT,'original 2006 client.dll SHA256')
    for classname,cxx in [('weapon_chaingun','CWeaponChaingun'),('npc_druglab_grunt_elite','CNPC_EliteGrunt'),('ai_goal_takecover','CAI_TakeCoverGoal')]:
        eq(p.factory(classname)['class'],cxx,'factory '+classname)

    # CWeaponChaingun readiness, 4-way switch and spin-state controller.
    eq(p.read(0x22315323,6).hex(),'8b8694050000','weapon readiness reads state +0x594')
    ins(0x22315329,'83 f8 03','readiness bounds state to 0..3')
    ins(0x2231532e,'ff 24 85 74 53 31 22','readiness uses four-state jump table')
    ready_table=[p.u32(0x22315374+4*i) for i in range(4)]
    eq(ready_table,[0x22315335,0x2231536A,0x2231536E,0x2231536A], 'readiness dispatch states 0/1/2/3')
    ins(0x2231536a,'32 c0','not-ready returns 0')
    ins(0x2231536e,'b0 01','ready returns 1 in state 2')
    ins(0x22315343,'c7 86 94 05 00 00 01 00 00 00','state0 startup sets state1')
    ins(0x22315356,'89 96 90 05 00 00','state0 stores current time +0x590')
    ins(0x22315364,'89 8e 8c 05 00 00','state0 stores current time +0x58C')
    ins(0x22315399,'83 e8 01','update dispatch first state step')
    ins(0x223153a1,'74 2d','update dispatch state2')
    ins(0x223153a6,'0f 85 8f 00 00 00','update dispatch state3')
    ins(0x22315400,'d8 a6 90 05 00 00','state1 compares time since +0x590')
    ins(0x22315413,'c7 86 94 05 00 00 02 00 00 00','state1 after timer enters ready state2')
    ins(0x223153d8,'d8 a6 8c 05 00 00','state2 compares time since +0x58C')
    ins(0x223153eb,'c7 86 94 05 00 00 03 00 00 00','state2 after timer enters state3')
    ins(0x223153c4,'c7 86 94 05 00 00 00 00 00 00','state3 stop enters state0')
    ins(0x22315440,'c7 81 94 05 00 00 03 00 00 00','force-stop sets state3')
    eq(f32(0x22477c5c),1.0,'weapon state timer threshold float equals 1s')
    ins(0x22315337,'6a 00 6a 0d','startup queues sound/event index 13')
    ins(0x223153ae,'6a 0d','shutdown stops sound/event index 13')
    ins(0x223153b8,'6a 00 6a 0e','shutdown starts sound/event index 14')
    ins(0x2231533d,'ff 90 c4 03 00 00','startup calls weapon sound/event vtable +0x3C4')
    ins(0x223153b0,'ff 90 c8 03 00 00','shutdown calls weapon stop/event vtable +0x3C8')
    ins(0x223153be,'ff 92 c4 03 00 00','shutdown calls weapon sound/event vtable +0x3C4')

    # Elite animation handlers, including special event 3's throttled reaction.
    eq(p.u32(p.primary_vtable('CNPC_EliteGrunt')+0x424),0x223555f0,'elite has custom weapon update vtable slot')
    ins(0x22355489,'83 f8 03','elite primary handler recognizes event 3')
    ins(0x22355494,'83 f8 43','elite primary handler recognizes event 0x43')
    ins(0x22355499,'83 f8 47','elite primary handler recognizes event 0x47')
    call(0x223554ba,0x22315440,'event 0x47 forces weapon state3')
    call(0x22355503,0x22315320,'event 0x43 queries weapon readiness')
    ins(0x2235550a,'88 86 d9 0f 00 00','event 0x43 stores elite m_canFire')
    ins(0x22355524,'d9 47 04','event 3 reads float event parameter')
    ins(0x2235553f,'83 7c 24 08 01','event 3 requires integer-converted parameter ==1')
    ins(0x22355581,'d8 a6 9c 09 00 00','event 3 compares time against elite+0x99C')
    ins(0x22355587,'dc 1d e8 9e 3d 22','event 3 compares time difference to double')
    eq(f64(0x223d9ee8),3.0,'event 3 cooldown constant 3 seconds')
    ins(0x2235559b,'89 86 9c 09 00 00','event 3 writes timestamp on accepted branch')
    call(0x223555b2,0x220a35c0,'event3 updates custom NPC state at +0xCC0')
    call(0x223555c0,0x220325c0,'event3 invokes float-2.0 NPC action helper')
    ins(0x223555cd,'ff 90 dc 04 00 00','event3 invokes NPC vtable+0x4DC')
    ins(0x223556a9,'83 f8 43','elite secondary handler also tests 0x43')
    ins(0x223556b2,'83 f8 47','elite secondary handler also tests 0x47')
    ins(0x22355737,'80 be d9 0f 00 00 00','secondary 0x43 tests m_canFire')
    call(0x22355772,0x22315320,'secondary 0x43 checks readiness if m_canFire is false')
    for va,n in [(0x22355713,3),(0x2235571c,4),(0x22355725,5)]:
        call(va,0x220398a0,f'secondary 0x47 clears condition {n}')
    call(0x22355645,0x22315390,'elite vtable update ticks chaingun controller')
    ins(0x223555f3,'8b 86 dc 0f 00 00','elite tick obtains chaingun handle +0xFDC')

    # CAI_TakecoverBehavior's retry budget and navigation fallback.
    eq(p.primary_vtable('CAI_TakecoverBehavior'),0x224ae624,'original cover behavior vtable')
    ins(0x22399d99,'89 5d 7c','behavior constructor resets retry counter +0x7C')
    ins(0x2239840f,'b9 03 00 00 00','cover selection threshold equals three')
    ins(0x22398414,'39 4e 7c','cover selection checks retry count +0x7C')
    ins(0x22398419,'c6 46 11 00','cover selection disables behavior at retry limit')
    for va,label in [(0x22398653,'schedule fallback'),(0x22399772,'cover path failure'),(0x2239983d,'target search failure')]:
        ins(va,'83 46 7c 01' if va!=0x22399772 else '01 7e 7c',label+' increments retry counter')
    ins(0x22399742,'c7 46 68 00 00 00 00','cover search starts with cleared retry timestamp')
    call(0x22399749,0x22398b80,'cover path calls custom nearest cover finding helper')
    for va,label in [(0x22399761,'first'),(0x2239979f,'second'),(0x22399804,'third')]:
        ins(va,'ff 15 88 21 3d 22',label+' fallback picks randomized timer')
    ins(0x22399757,'68 00 00 80 40','retry float upper bound 4 seconds')
    ins(0x2239975c,'68 00 00 00 40','retry float lower bound 2 seconds')
    ins(0x22399818,'6a 0b','cover task invokes condition/event 11 on fallback')
    ins(0x22399843,'68 5c eb 3e 22','missing target path uses No hint node diagnostic')
    ins(0x22399921,'68 50 e3 4a 22','lost cover path uses diagnostic string')
    eq(p.text(0x223eeb5c),'No hint node','cover diagnostic string No hint node')
    eq(p.text(0x224ae350),'No longer behind cover','cover diagnostic string No longer behind cover')
    eq(f64(0x223d9ee8),3.0,'elite cooldown double recheck')
    # Conservative: verify copied preset field but not an undocumented direct runtime reader.
    ins(0x22399bcf,'f3 a5','goal copies all seven preset words including cover odds')
    ins(0x22398216,'f3 a5','behavior copies seven preset words when updated')
    ins(0x22399bb2,'c7 44 24 24 00 00 00 00','AbandonIfEnemyHides false zeroes seventh word')
    eq(p.u32(0x2253b600+20),50,'preset zero sixth word = 50 (cover odds)')
    eq(p.u32(0x2253b600+4*28+20),0,'preset four sixth word = 0 (cover odds)')

    with zipfile.ZipFile(SCRIPTS) as z:
        weapon=z.read('scripts/weapon_chaingun.txt').decode('utf8',errors='replace')
        sounds=z.read('scripts/game_sounds_weapons.txt').decode('utf8',errors='replace')
    for tag,event in [('special1','Weapon_Chaingun.NPC_SpinUp'),('special2','Weapon_Chaingun.NPC_SpinDown')]:
        ok(bool(re.search(r'"'+tag+r'"\s+"'+re.escape(event)+r'"',weapon,re.I)),f'weapon script {tag} maps {event}')
        ok('"'+event+'"' in sounds,f'sound script defines {event}')
    for wave in ['weapons/chaingun/chaingun_spinup.wav','weapons/chaingun/chaingun_spindown.wav']:
        ok(wave in sounds,'game sounds has audio path '+wave)
    counts=collections.Counter()
    for name in LOG_FILES:
        counts.update(re.findall(r'Attempted to create unknown entity type ([a-zA-Z0-9_]+)!',(ROOT/name).read_text(encoding='utf8',errors='replace')))
    eq(counts['npc_druglab_grunt_elite'],69,'elite missing attempts in all logs')
    eq(counts['ai_goal_takecover'],26,'take-cover goal missing attempts in all logs')
    eq(sum(counts.values()),1531,'all five log missing creation attempts')
    result={
      'chapter':14,
      'scope':'static SHA-verified disassembly, original 2006 DLL, scripts.zip and user HL2 console logs; no runtime game test',
      'sha256':{'server.dll':p.sha,'client.dll':hashlib.sha256(CLIENT.read_bytes()).hexdigest(),'scripts.zip':hashlib.sha256(SCRIPTS.read_bytes()).hexdigest()},
      'checks_passed':len(checks),'checks':checks,
      'chaingun':{
       'readiness_function_va':'0x22315320','state_word_offset':'weapon+0x594',
       'readiness_jump_table_va':'0x22315374',
       'states':[{'state':n,'return_ready':n==2,'target_va':f'0x{addr:08X}'} for n,addr in enumerate(ready_table)],
       'controller_function_va':'0x22315390','force_state3_va':'0x22315440',
       'transitions':'0->1 on readiness/start, 1->2 if elapsed>=1.0, 2->3 if elapsed>=1.0, 3->0 on next controller tick; state3 can be forced by elite event 0x47',
       'timer_offset_seconds':'weapon+0x590 and weapon+0x58C; per phase elapsed vs literal 1.0f',
       'sound_event_indices':[13,14],
       'sound_script_special1':'Weapon_Chaingun.NPC_SpinUp','sound_script_special2':'Weapon_Chaingun.NPC_SpinDown',
       'note':'Exact mapping from sound event indices 13/14 to script special1/special2 is not independently verified by a direct lookup trace.'},
      'elite':{'event_primary_va':'0x22355480','event_secondary_va':'0x223556A0',
       'event3':{'requires_event_parameter_integer':1,'min_elapsed_since_elite_plus_0x99c':3.0,'timestamp_update_va':'0x2235559B','state_action_va':'0x223555B2','note':'event 3 symbolic name and exact gameplay semantic not recovered'},
       'event_0x43':'readiness gate via 0x22315320 and m_canFire +0xFD9',
       'event_0x47':'force weapon state 3 in primary handler; secondary event clears NPC conditions 3,4,5 if guards pass'},
      'cover':{'retry_counter_offset':'behavior+0x7C','retry_limit':3,'constructor_reset_va':'0x22399D99','schedule_gate_va':'0x22398400',
       'retry_count_increment_vas':['0x22398653','0x22399772','0x2239983D'],
       'cover_path_helper_va':'0x22398B80','retry_timer_random_float_range':[2.0,4.0],
       'diagnostics':['No hint node','No longer behind cover'],
       'cover_odds_preset_index':5,'cover_odds_examples':{'aggressiveness_0':50,'aggressiveness_4':0},
       'cover_odds_consumer':'not positively identified; copied to behavior, not proven applied to selection or outcome'},
      'logs':{'failed_elite_attempts':69,'failed_takecover_attempts':26,'failed_total_attempts':1531},
      'caveats':['This is partial static reverse engineering, not recovered full C++.','Symbolic animation-event names are not yet established.','Sound enum indices are not conclusively mapped to special1/special2.','Cover odds and seventh preset word consumer remain unresolved.']}
    return result

if __name__=='__main__':
    out=run();dest=ROOT/'sin_cover_chaingun_state_evidence_14.json'
    dest.write_text(json.dumps(out,indent=2,ensure_ascii=False)+'\n',encoding='utf8')
    print(f'PASS: {out["checks_passed"]} SHA-pinned static checks; wrote {dest}')