#!/usr/bin/env python3
"""Read-only SHA-pinned SiN Episodes 2006 CNPC_Jessica/CNPC_JC audit.

Usage: python sin_original_jessica_jc_audit.py ORIGINAL_SERVER.DLL [OUTPUT.json] [ORIGINAL_CLIENT.DLL]
Checks exact x86/PE32 addresses; does not extract or emit proprietary executable code.
"""
from __future__ import annotations
import hashlib
import json
import struct
import sys
from pathlib import Path

SHA256 = '568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650'

class PE:
    def __init__(self, filename):
        self.data = Path(filename).read_bytes()
        self.sha = hashlib.sha256(self.data).hexdigest()
        assert self.sha == SHA256, f'Wrong server.dll SHA-256: {self.sha}'
        mz = struct.unpack_from('<I', self.data, 0x3c)[0]
        assert self.data[mz:mz+4] == b'PE\0\0'
        count = struct.unpack_from('<H', self.data, mz+6)[0]
        optlen = struct.unpack_from('<H', self.data, mz+20)[0]
        self.base = struct.unpack_from('<I', self.data, mz+24+28)[0]
        assert self.base == 0x22000000
        self.sections = []
        for i in range(count):
            p = mz+24+optlen+40*i
            name = self.data[p:p+8].split(b'\0')[0].decode('ascii')
            size, rva, rawsize, rawoff = struct.unpack_from('<4I', self.data, p+8)
            self.sections.append((name, self.base+rva, size, rawsize, rawoff))
    def off(self, va, n=1):
        for name, start, virtualsize, rawsize, pos in self.sections:
            if start <= va and va+n <= start+rawsize:
                o = pos+va-start
                assert 0 <= o and o+n <= len(self.data)
                return o
        raise ValueError(f'Unmapped VA 0x{va:08X}')
    def read(self, va, n):return self.data[self.off(va,n):self.off(va,n)+n]
    def u32(self,va):return struct.unpack('<I',self.read(va,4))[0]
    def text(self,va):
        pos=self.off(va); stop=self.data.index(b'\0',pos,min(pos+2048,len(self.data)))
        return self.data[pos:stop].decode('ascii')
    def assert_bytes(self,va,hexbytes):
        expected=bytes.fromhex(hexbytes)
        actual=self.read(va,len(expected))
        assert actual==expected,(hex(va),actual.hex(),expected.hex())
    def relcall(self,va,target):
        op=self.read(va,5)
        assert op[0] in (0xE8,0xE9),hex(va)
        assert va+5+struct.unpack_from('<i',op,1)[0]==target,(hex(va),hex(target))

VTABLES = {
    'CNPC_Jessica':(0x224d224c,0x22497a14,514),
    'CNPC_PlayerCompanion':(0x224c0b18,0x224871bc,514),
    'CNPC_JC':(0x224d212c,0x22496fbc,497),
    'CAI_BaseActor':(0x224ba8c8,0x224168f4,497),
}
# COL pointers above are verified through each vtable[-1] and TypeDescriptor below;
# do not rely on the convenience COL guesses for non-character base classes.

DATAMAP=[
    ('m_throwName',0x144c,None,None,2),
    ('InputThrowItemAtPlayer',0,'ThrowItemAtPlayer',0x223622d0,2),
    ('InputSetPrimaryAmmoCountToThrow',0,'SetPrimaryAmmoCountToThrow',0x22362320,5),
    ('InputSetSecondaryAmmoCountToThrow',0,'SetSecondaryAmmoCountToThrow',0x22362350,5),
    ('InputHolsterWeapon',0,'HolsterWeapon',0x22362300,5),
]

def audit(filename):
    p=PE(filename)
    checks=[]
    def check(va,code,note):p.assert_bytes(va,code);checks.append(note)
    def eq(va,expected,note):assert p.u32(va)==expected,(note,hex(va),hex(p.u32(va)));checks.append(note)
    def text(va,expected,note):assert p.text(va)==expected,(note,p.text(va));checks.append(note)
    def call(va,target,note):p.relcall(va,target);checks.append(note)
    # Original RTTI and vtables; every pointer must be inside original .text.
    vt_out={}
    for name,(_,addr,count) in VTABLES.items():
        col=p.u32(addr-4)
        td=p.u32(col+12)
        text(td+8,'.?AV'+name+'@@',name+' RTTI name')
        assert p.u32(col)==0
        values=[p.u32(addr+i*4) for i in range(count)]
        assert all(0x22001000<=v<0x223d111a for v in values),name
        checks.append(name+' primary vtable contains exactly the expected code pointers')
        vt_out[name]={'rtti_col':f'0x{col:08X}','vtable':f'0x{addr:08X}','entries':count}
    def changes(a,b):
        start=VTABLES[a][1]; parent=VTABLES[b][1]
        return [{'slot':i,'slot_byte_offset':f'0x{i*4:03X}','child_func':f'0x{p.u32(start+i*4):08X}', 'base_func':f'0x{p.u32(parent+i*4):08X}'} for i in range(min(VTABLES[a][2],VTABLES[b][2])) if p.u32(start+i*4)!=p.u32(parent+i*4)]
    jess_changes=changes('CNPC_Jessica','CNPC_PlayerCompanion')
    jc_changes=changes('CNPC_JC','CAI_BaseActor')
    assert len(jess_changes)==29 and len(jc_changes)==6
    checks += ['Jessica has 29 changed primary-vtable pointers','JC has six changed primary-vtable pointers']
    eq(0x22497a14+0x2c0,0x22362420,'Jessica animation-event override +0x2C0')
    eq(0x22497a14+0x64,0x223628e0,'Jessica Spawn override +0x64')
    eq(0x22496fbc+0x64,0x22361c10,'JC Spawn override +0x64')
    # Exact datamap records 0x34 bytes apart; one trailing uninitialized sentinel.
    fields=[]
    for i,(field,offset,alias,handler,fieldtype) in enumerate(DATAMAP):
        va=0x225359d4+i*0x34
        eq(va,fieldtype,f'{field} field kind')
        np=p.u32(va+4);text(np,field,f'{field} compiled member/handler name')
        eq(va+8,offset,f'{field} object field offset')
        if alias:
            text(p.u32(va+0x14),alias,f'{alias} external map input')
            eq(va+0x1c,handler,f'{alias} handler pointer')
        else:
            eq(va+0x14,0,'m_throwName has no map input alias')
        fields.append({'record':f'0x{va:08X}','member':field,'type_code':fieldtype,'object_offset':f'0x{offset:X}' if offset else None,'map_input':alias,'handler':f'0x{handler:08X}' if handler else None})
    eq(0x22535ad8,0,'Sixth datamap record is an empty sentinel, not another input')
    check(0x22362a8d,'c7 05 40 59 53 22 06 00 00 00','Jessica datamap registered with count six (five meaningful records and sentinel)')
    check(0x22362a97,'c7 05 3c 59 53 22 d4 59 53 22','Jessica datamap registered at 0x225359D4')
    text(0x22497780,'npc_jc','JC map classname string')
    text(0x2249821c,'npc_jessica','Jessica map classname string')
    text(0x22496f1c,'models/characters/JC/jc_torso.mdl','JC default model')
    text(0x224977a0,'models/characters/Jessica/Jessica.mdl','Jessica default model')
    # Spawn and health: direct instructions, no inference of actual difficulty tuning.
    check(0x22361c2c,'bf 1c 6f 49 22','JC fallback model chosen when no model specified')
    check(0x22361c3e,'89 86 40 02 00 00','JC fallback model stored at entity+0x240')
    check(0x22361cac,'bf 08 00 00 00','JC spawn hardcodes health 8')
    check(0x22361cc3,'89 be 9c 00 00 00','JC health written into entity+0x9C')
    check(0x22361ebf,'68 a0 77 49 22','Jessica fallback model reference')
    check(0x22361ecc,'89 86 40 02 00 00','Jessica model name stored at entity+0x240')
    check(0x2236294f,'83 be 9c 00 00 00 50','Jessica spawn compares health against 80')
    check(0x22362962,'c7 86 9c 00 00 00 50 00 00 00','Jessica spawn writes 80 health')
    check(0x22362970,'c7 86 54 14 00 00 ff ff ff ff','Jessica secondary ammo sentinel -1')
    check(0x2236297a,'c7 86 50 14 00 00 ff ff ff ff','Jessica primary ammo sentinel -1')
    text(0x223eb090,'TLK_HELLO','Jessica greeting concept')
    text(0x223eae34,'TLK_PLDEAD','Jessica player death dialogue concept')
    text(0x223eaf2c,'TLK_INPOISONCLOUD','Jessica poison cloud dialogue concept')
    check(0x22362990,'68 90 b0 3e 22','Jessica spawn passes TLK_HELLO')
    check(0x2236262d,'68 34 ae 3e 22','Jessica condition passes TLK_PLDEAD')
    check(0x2236265f,'68 2c af 3e 22','Jessica damage callback passes TLK_INPOISONCLOUD')
    # AI event and schedule registration with original names.
    for va,label in [(0x22497980,'AE_JESSICA_THROW_ITEM'),(0x22497970,'COND_THROW_ITEM'),(0x22497960,'ACT_THROW_ITEM'),(0x2249794c,'SCHED_THROW_ITEM')]:
        text(va,label,label+' compiled label')
    schedule=p.text(0x224978c0)
    for token in ('SCHED_THROW_ITEM','TASK_FACE_PLAYER','TASK_PLAY_SEQUENCE','ACTIVITY:ACT_THROW_ITEM'):
        assert token in schedule
        checks.append('Throw schedule includes '+token)
    check(0x22362c10,'68 80 79 49 22','Animation event AE_JESSICA_THROW_ITEM used in registration')
    eq(0x22362c15+1,0x2259c9a8,'Animation event runtime ID assigned at 0x2259C9A8')
    check(0x22362c34,'c7 44 24 1c 47 00 00 00','COND_THROW_ITEM ID 0x47')
    check(0x22362c70,'c7 44 24 1c 59 00 00 00','SCHED_THROW_ITEM ID 0x59')
    check(0x22362381,'6a 47','Jessica tests throw condition')
    check(0x2236238e,'b8 59 00 00 00','Jessica selects throw schedule when condition present')
    check(0x223623a1,'6a 47','Jessica handles throw condition in secondary override')
    # Inputs and actual spawned object; differentiate from a simple entity I/O stub.
    check(0x223622eb,'89 81 4c 14 00 00','ThrowItemAtPlayer stores string handle in m_throwName')
    check(0x223622f1,'c7 44 24 04 47 00 00 00','ThrowItemAtPlayer triggers condition 0x47')
    call(0x223622f9,0x22039760,'ThrowItemAtPlayer forwards into condition-setting helper')
    check(0x2236232d,'89 81 50 14 00 00','SetPrimaryAmmoCountToThrow stores at +0x1450')
    check(0x2236235d,'89 81 54 14 00 00','SetSecondaryAmmoCountToThrow stores at +0x1454')
    call(0x22362300,0x222aa640,'HolsterWeapon resolves player helper')
    check(0x22362315,'ff a2 48 03 00 00','HolsterWeapon forwards via player virtual slot +0x348')
    check(0x2236242c,'3b 0d a8 c9 59 22','Animation-event handler checks registered throw event ID')
    text(0x22490254,'anim_attachment_RH','Throw hand attachment')
    check(0x22362463,'68 54 02 49 22','Throw function uses right-hand attachment')
    check(0x2236246f,'8b 86 4c 14 00 00','Throw handler reads m_throwName')
    call(0x2236248b,0x22144da0,'Throw handler invokes custom entity/item creation helper')
    text(0x224de300+8,'.?AVCBaseCombatWeapon@@','Throw function casts spawned object to CBaseCombatWeapon')
    text(0x224d88d4+8,'.?AVCBaseEntity@@','Throw function uses CBaseEntity RTTI')
    call(0x223624ac,0x223a5bab,'Throw handler uses runtime-type conversion helper')
    check(0x223624c5,'89 88 d4 04 00 00','Primary ammo count copied to spawned weapon +0x4D4')
    check(0x223624db,'89 88 d8 04 00 00','Secondary ammo count copied to spawned weapon +0x4D8')
    check(0x223624cb,'89 96 50 14 00 00','Primary ammo count reset to -1 after copy')
    check(0x223624e1,'89 96 54 14 00 00','Secondary ammo count reset to -1 after copy')
    check(0x223624fd,'68 00 00 70 41','Throw function passes 15.0f as float parameter')
    call(0x2236251a,0x2204c850,'Throw motion helper computes direction/vector')
    call(0x22362529,0x2214ba20,'Throw finalizes object motion through helper')
    return {
        'source':{'filename':'server.dll','sha256':p.sha,'preferred_base':f'0x{p.base:08X}','format':'2006 PE32 x86'},
        'inheritance':{'CNPC_Jessica':['CNPC_PlayerCompanion','CAI_PlayerAlly','CAI_BaseActor'], 'CNPC_JC':['CAI_BaseActor']},
        'vtables':vt_out,'vtable_differences':{'jessica_vs_playercompanion':jess_changes,'jc_vs_baseactor':jc_changes},
        'jessica_datamap':{'record_size_bytes':52,'total_records':6,'meaningful_records':5,'entries':fields},
        'spawn_defaults':{'Jessica':{'model':'models/characters/Jessica/Jessica.mdl','health':80,'ammo_fields_initialized_to':-1},'JC':{'model':'models/characters/JC/jc_torso.mdl','health':8}},
        'jessica_custom_ai':{'animation_event':'AE_JESSICA_THROW_ITEM','event_id_storage':'0x2259C9A8 (runtime)','condition':{'name':'COND_THROW_ITEM','id':71},'schedule':{'name':'SCHED_THROW_ITEM','id':89,'tasks':['TASK_FACE_PLAYER','TASK_PLAY_SEQUENCE ACTIVITY:ACT_THROW_ITEM']},'throw_handler':'0x22362420'},
        'jessica_throw':{'class_input':'ThrowItemAtPlayer','stored_entity_name_offset':'0x144C','primary_ammo_temp_offset':'0x1450','secondary_ammo_temp_offset':'0x1454','hand_attachment':'anim_attachment_RH','spawn_helper':'0x22144DA0','weapon_cast':'CBaseCombatWeapon','ammo_target_offsets':['0x4D4','0x4D8'],'float_constant':15.0},
        'jessica_dialogue_concepts':['TLK_HELLO','TLK_PLDEAD','TLK_INPOISONCLOUD','TLK_PLYR_PHYSATK'],
        'checks_count':len(checks),'checks':checks,
        'limitations':['Static binary proof only: no live map/gameplay test.','Event ID is registered dynamically; the binary holds its variable address, not a universal numeric value.','Health 80 and 8 are values written in spawn routines, not proof that other scripts never modify health.','The throw helper is identified by calls, not yet independently named to a complete original C++ signature.','No dedicated client Jessica/JC class is established by this server-only audit.']
    }

if __name__=='__main__':
    if len(sys.argv) not in (2,3,4):raise SystemExit(__doc__)
    d=audit(sys.argv[1])
    if len(sys.argv)==4:
        client=Path(sys.argv[3]).read_bytes()
        sha=hashlib.sha256(client).hexdigest()
        assert sha=='6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d',sha
        strings=['C_NPC_Jessica','C_NPC_JC','DT_NPC_Jessica','DT_NPC_JC','npc_jessica','npc_jc']
        counts={s:client.count(s.encode('ascii')) for s in strings}
        assert all(x==0 for x in counts.values())
        d['client_name_scan']={'sha256':sha,'name_occurrence_counts':counts,'caution':'No matching literals, not proof of the absence of generic client representation or networking'}
        d['checks_count']+=len(counts)+1
    print(f'PASS: {d["checks_count"]} static verifications, 29 Jessica and six JC vtable differences, 5 meaningful Jessica datamap fields')
    if len(sys.argv)>=3:Path(sys.argv[2]).write_text(json.dumps(d,indent=2)+'\n',encoding='utf8')