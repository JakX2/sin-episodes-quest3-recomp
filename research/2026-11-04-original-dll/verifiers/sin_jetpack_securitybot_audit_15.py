#!/usr/bin/env python3
"""Read-only original SiN Episodes 2006 jetpack/security-bot audit (Round 15).

Usage: python sin_jetpack_securitybot_audit_15.py DIR [output.json]
Provide independently obtained original server/client DLLs and five console logs.
No game binary or proprietary asset is bundled or modified.
"""
from __future__ import annotations
import collections, hashlib, json, pathlib, re, sys
from sin_missing_entity_atlas_audit import PE, LOG_FILES, EXPECTED_CLIENT

def run(root: pathlib.Path) -> dict:
    server = root / 'sin_private' / 'server.dll'
    client = root / 'sin_private' / 'client.dll'
    if not server.exists(): server = root / 'server.dll'
    if not client.exists(): client = root / 'client.dll'
    pe = PE(server)
    assert hashlib.sha256(client.read_bytes()).hexdigest() == EXPECTED_CLIENT
    facts = {
        'server_sha256': pe.sha,
        'client_sha256': EXPECTED_CLIENT,
        'mode': 'read-only original x86 static verification; no game runtime behavior validated',
        'factory_registrations': {},
        'class_layout': {},
        'string_refs': {},
        'console_attempts': {},
    }
    tests = 2
    for classname, cxx in (
        ('npc_druglab_jetpack', 'CNPC_DruglabJetpack'),
        ('npc_securitybot', 'CNPC_SecurityBot'),
    ):
        f = pe.factory(classname)
        assert f['class'] == cxx
        facts['factory_registrations'][classname] = f
        tests += 1
    for name, parent, expected in (
        ('CNPC_DruglabJetpack', 'CNPC_JetpackGruntBase', 5),
        ('CNPC_JetpackGruntBase', 'CNPC_GruntBase', 31),
        ('CNPC_SecurityBot', 'CNPC_CBot', 16),
    ):
        childvt, parentvt = pe.primary_vtable(name), pe.primary_vtable(parent)
        assert childvt and parentvt
        child = pe.all_text_function_pointers(childvt)
        base = pe.all_text_function_pointers(parentvt)
        changes = [(i, hex(a), hex(b)) for i,(a,b) in enumerate(zip(child,base)) if a != b]
        assert len(changes) == expected, (name,len(changes),expected)
        facts['class_layout'][name] = {
            'primary_vtable': hex(childvt), 'parent': parent,
            'modified_slots': changes
        }
        tests += 1
    jetdm = pe.datamap('CNPC_JetpackGruntBase')
    securitydm = pe.datamap('CNPC_SecurityBot')
    assert jetdm['declared_records'] == 26
    assert jetdm['fields'][0]['internal_name'] == 'm_pEngineSound'
    assert jetdm['fields'][0]['offset'] == '0x1008'
    assert [(f['internal_name'],f['offset']) for f in securitydm['fields']] == [
        ('m_fAlertTime','0xD14'),('m_fWarningTime','0xD24')]
    facts['datamaps'] = {
        'jetpack_base': {'records':26,'readable':jetdm['statically_resolved_records'],'engine_sound_offset':'0x1008'},
        'securitybot': {'records':2,'alert_offset':'0xD14','warning_offset':'0xD24'}
    }
    tests += 4
    for va, value in {
        0x224982cc:'models/combine_soldier.mdl',
        0x2249825c:'models/weapons/jetpack.mdl',
        0x2242bb44:'prop_dynamic',
        0x224a3268:'models/characters/SecurityDrone_02/securitydrone_02.mdl',
        0x224a3238:'models/characters/SecurityDrone_02/gibs_01.mdl',
        0x224a3208:'models/characters/SecurityDrone_02/gibs_02.mdl',
        0x224a32ac:'gun_right',
        0x224a32a0:'gun_left',
        0x224a31f0:'NPC_CScanner.PreShoot',
    }.items():
        assert pe.text(va) == value, (hex(va),pe.text(va))
        facts['string_refs'][hex(va)] = value
        tests += 1
    for at,target in {
        0x22353d8a:0x22367f20,
        0x22367ff9:0x22144da0,
        0x2237e948:0x22149b70,
        0x2237e952:0x22149b70,
        0x2237e95c:0x22149b70,
        0x2237ea4a:0x22170f60,
        0x2237ea66:0x22170f60,
        0x2237f321:0x220ed470,
    }.items():
        pe.assert_call(at,target)
        tests += 1
    for va,pattern in {
        0x22368011:'89 85 28 10 00 00',
        0x22368019:'89 9d 28 10 00 00',
        0x2237e9ac:'89 8e 14 0d 00 00',
        0x2237e9c2:'89 86 24 0d 00 00',
        0x2237f28c:'83 e8 43',
        0x2237f291:'83 e8 52',
        0x2237f2b2:'68 00 00 c0 40',
        0x2237f2b7:'68 00 00 a0 40',
        0x2237f2d6:'d9 9e 24 0d 00 00',
        0x2237f2ea:'83 86 20 0d 00 00 01',
        0x2237f2fc:'c7 86 20 0d 00 00 00 00 00 00',
        0x2237f312:'8b 8c 86 18 0d 00 00',
    }.items():
        pe.assert_instruction(va,pattern)
        tests += 1
    counts=collections.Counter()
    for filename in LOG_FILES:
        text=(root/filename).read_text(encoding='utf-8',errors='replace')
        counts.update(re.findall(r'Attempted to create unknown entity type ([A-Za-z0-9_]+)!',text))
    assert counts['npc_druglab_jetpack']==29 and counts['npc_securitybot']==16
    assert sum(counts.values())==1531 and len(counts)==49
    facts['console_attempts']={
        'npc_druglab_jetpack':29,'npc_securitybot':16,
        'overall':1531,'class_count':49
    }
    tests+=4
    facts['verified_assertions']=tests
    return facts

if __name__=='__main__':
    root=pathlib.Path(sys.argv[1]) if len(sys.argv)>1 else pathlib.Path('/mnt/data')
    output=pathlib.Path(sys.argv[2]) if len(sys.argv)>2 else root/'sin_jetpack_securitybot_evidence_15.json'
    findings=run(root)
    output.write_text(json.dumps(findings,indent=2,sort_keys=True)+'\n',encoding='utf-8')
    print('PASS:',findings['verified_assertions'],'read-only original DLL/log assertions; wrote',output)
