#!/usr/bin/env python3
"""Read-only SiN Episodes 2006 Round 16 jetpack/securitybot/pulse audit.

Usage: python sin_jetpack_securitybot_pulse_audit_16.py /path/to/private-inputs [output.json]

Requires original SHA-pinned server.dll/client.dll and five captured console logs,
plus sin_missing_entity_atlas_audit.py from the research archive. Does not
execute original game code or distribute any original game content.
"""
from __future__ import annotations
import collections, hashlib, json, pathlib, re, struct, sys
from sin_missing_entity_atlas_audit import PE, LOG_FILES, EXPECTED_CLIENT

FACTORIES = {
 'npc_druglab_jetpack': ('CNPC_DruglabJetpack', 0x223C68B7),
 'npc_druglab_elite_jetpack': ('CNPC_EliteJetpack', 0x223C6A97),
 'npc_sintek_jetpack': ('CNPC_SintekJetpack', 0x223C77C7),
 'npc_securitybot': ('CNPC_SecurityBot', 0x223C7707),
 'pulse': ('CPulseShot', 0x223C7777),
}
PARENT_DIFFS = {
 ('CNPC_DruglabJetpack','CNPC_JetpackGruntBase'): 5,
 ('CNPC_EliteJetpack','CNPC_JetpackGruntBase'): 4,
 ('CNPC_SintekJetpack','CNPC_JetpackGruntBase'): 4,
 ('CNPC_JetpackGruntBase','CNPC_GruntBase'): 31,
 ('CNPC_SecurityBot','CNPC_CBot'): 16,
 ('CPulseShot','CBaseCombatCharacter'): 7,
}
CALL_EDGES = {
 0x2237F3B7: 0x2237ED60, # security-bot animation event to pulse launcher
 0x2237F408: 0x22141A00, # velocity application to created pulse
 0x2237ED69: 0x22266B00, # entity factory acquisition
 0x2237ED78: 0x22285AB0, # create entity from factory
 0x2237F321: 0x220ED470, # resolve gun attachment
 0x22367FF9: 0x22144DA0, # jetpack attached prop_dynamic creation
 0x22353D8A: 0x22367F20, # druglab jetpack -> common spawn
}
BYTE_SITES = {
 0x2237F28C: '83 e8 43', # event 0x43
 0x2237F291: '83 e8 52', # next event 0x95
 0x2237F2B2: '68 00 00 c0 40', # 6.0
 0x2237F2B7: '68 00 00 a0 40', # 5.0
 0x2237F2EA: '83 86 20 0d 00 00 01', # left/right selector increment
 0x2237F2FC: 'c7 86 20 0d 00 00 00 00 00 00', # wrap to 0
 0x2237F312: '8b 8c 86 18 0d 00 00', # indexed attachment ID read
 0x2237F3C0: 'd8 0d 98 31 4a 22', # 1500.0 speed constant
 0x22380481: '68 00 02 38 22', # CPulseShot think thunk
 0x22380488: 'c7 46 7c c0 f4 37 22', # CPulseShot touch callback
 0x2238049E: 'd8 05 a8 31 4a 22', # first think delay .25
 0x22368011: '89 85 28 10 00 00', # jetpack attached model handle
 0x22368019: '89 9d 28 10 00 00', # failed create -1
}
SCHEDULES = [
 'SCHED_JETPACK_PATROL',
 'SCHED_JETPACK_CHASE_ENEMY',
 'SCHED_JETPACK_FOLLOW_HOVER',
 'SCHED_JETPACK_RANGE_ATTACK1',
 'SCHED_JETPACK_MOVE_TO_INSPECT',
 'SCHED_JETPACK_SHOOT_COVER',
]

def hierarchy(pe: PE, classname: str):
    vt=pe.primary_vtable(classname)
    col=pe.u32(vt-4); chd=pe.u32(col+16); n=pe.u32(chd+8)
    arr=pe.u32(chd+12)
    return [pe.text(pe.u32(pe.u32(arr+4*i))+8).replace('.?AV','').removesuffix('@@')
            for i in range(n)]

def verify(root: pathlib.Path):
    server=(root/'sin_private'/'server.dll')
    client=(root/'sin_private'/'client.dll')
    if not server.exists():server=root/'server.dll'
    if not client.exists():client=root/'client.dll'
    pe=PE(server); assert hashlib.sha256(client.read_bytes()).hexdigest()==EXPECTED_CLIENT
    checks=2
    factories={}
    for name,(cxx,site) in FACTORIES.items():
        f=pe.factory(name);assert f['class']==cxx and int(f['registration_va'],16)==site
        factories[name]=f;checks+=1
    diffs={}
    for (child,parent),want in PARENT_DIFFS.items():
        a=pe.all_text_function_pointers(pe.primary_vtable(child))
        b=pe.all_text_function_pointers(pe.primary_vtable(parent))
        changed=[i*4 for i,(x,y) in enumerate(zip(a,b)) if x!=y]
        assert len(changed)==want, (child,parent,changed)
        diffs[f'{child} vs {parent}']={'changed_primary_vtable_slots':len(changed),
                                     'slot_offsets':[f'0x{x:X}' for x in changed]}
        checks+=1
    assert hierarchy(pe,'CPulseShot')[:2]==['CPulseShot','CBaseCombatCharacter'];checks+=1
    assert hierarchy(pe,'CNPC_SecurityBot')[:3]==['CNPC_SecurityBot','CNPC_CBot','CAI_BasePhysicsFlyingBot'];checks+=1
    assert hierarchy(pe,'CNPC_JetpackGruntBase')[:3]==['CNPC_JetpackGruntBase','CNPC_GruntBase','CAI_BaseHumanoid'];checks+=1
    assert pe.datamap('CPulseShot')['declared_records']==3;checks+=1
    assert pe.u32(pe.primary_vtable('CPulseShot')+0x64)==0x223803F0;checks+=1
    assert pe.u32(pe.primary_vtable('CPulseShot')+0x68)==0x2237EB80;checks+=1
    assert pe.text(0x224A31E8)=='pulse';checks+=1
    assert pe.text(0x224A31F0)=='NPC_CScanner.PreShoot';checks+=1
    assert pe.text(0x2248304C)=='NPC_CScanner.Shoot';checks+=1
    assert pe.text(0x224A32AC)=='gun_right' and pe.text(0x224A32A0)=='gun_left';checks+=1
    assert pe.text(0x2249825C)=='models/weapons/jetpack.mdl';checks+=1
    assert pe.text(0x2242BB44)=='prop_dynamic';checks+=1
    assert struct.unpack('<f',pe.read(0x224A3198,4))[0]==1500.0;checks+=1
    assert struct.unpack('<f',pe.read(0x224A31A8,4))[0]==.25;checks+=1
    for va,target in CALL_EDGES.items():pe.assert_call(va,target);checks+=1
    for va,bytestr in BYTE_SITES.items():pe.assert_instruction(va,bytestr);checks+=1
    for s in SCHEDULES:
        assert s.encode('ascii')+b'\0' in pe.raw, s
        checks+=1
    counter=collections.Counter()
    for name in LOG_FILES:
        txt=(root/name).read_text(encoding='utf-8',errors='replace')
        counter.update(re.findall(r'Attempted to create unknown entity type ([\w]+)!',txt))
    assert len(counter)==49 and sum(counter.values())==1531;checks+=1
    assert counter['npc_securitybot']==16 and counter['npc_druglab_jetpack']==29;checks+=1
    assert not any(counter[k] for k in ['pulse','npc_druglab_elite_jetpack','npc_sintek_jetpack']);checks+=1
    return {'research':'Round 16','source':'read-only static original 2006 PE + five HL2 console logs',
      'verified_checks':checks, 'original_server_sha256':pe.sha,'original_client_sha256':EXPECTED_CLIENT,
      'factories':factories,'primary_vtable_differences':diffs,
      'security_bot_attack':{'animation_dispatch_va':'0x2237F280','gun_event_id':'0x43',
        'warning_event_id':'0x95','projectile_factory_classname':'pulse','projectile_cxx':'CPulseShot',
        'gun_attachment_names':['gun_right','gun_left'],'constant_speed':1500.0,
        'pulse_first_think_delay_sec':.25,'pulse_spawn_va':'0x223803F0',
        'pulse_think_thunk_va':'0x22380200','pulse_think_target_va':'0x22380010',
        'pulse_touch_callback_va':'0x2237F4C0'},
      'jetpack':{'variants':['npc_druglab_jetpack','npc_druglab_elite_jetpack','npc_sintek_jetpack'],
        'schedules':SCHEDULES,'attached_model_entity_classname':'prop_dynamic',
        'attached_entity_handle_offset':'0x1028'},
      'console':{'entity_failure_attempts':1531,'unique_missing_classnames':49,
        'jetpack_attempts':29,'securitybot_attempts':16,'unobserved_dependencies':['pulse','npc_druglab_elite_jetpack','npc_sintek_jetpack']},
      'limitations':['No original-game runtime tests','No complete jetpack flight/physics reconstruction',
        'Projectile collision/damage values unresolved','Symbolic names of numeric animation events unresolved']}

if __name__=='__main__':
    root=pathlib.Path(sys.argv[1]) if len(sys.argv)>1 else pathlib.Path('/mnt/data')
    result=verify(root)
    out=pathlib.Path(sys.argv[2]) if len(sys.argv)>2 else root/'sin_jetpack_securitybot_pulse_evidence_16.json'
    out.write_text(json.dumps(result,indent=2,sort_keys=True)+'\n',encoding='utf-8')
    print(f"PASS: {result['verified_checks']} static assertions; wrote {out}")
