#!/usr/bin/env python3
"""Read-only SiN Episodes: Emergence 2006 missing-entity/datamap atlas.

Usage:
 python sin_missing_entity_atlas_audit.py DIRECTORY_WITH_DLLS_AND_LOGS [OUTPUT.json]

Required: original SHA-pinned server.dll, client.dll and five user console logs.
Never executes game code, never modifies binaries, never emits proprietary bytes.
Reconstructs factory RTTI, primary vtables, datamap fields, and selected call edges.
The original DLL uses static constructors: zero-valued preinitialization records
are intentionally retained as unresolved, never guessed.
"""
from __future__ import annotations
import collections, hashlib, json, pathlib, re, struct, sys

EXPECTED_SERVER = '568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650'
EXPECTED_CLIENT = '6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d'
LOG_FILES = ['sin stuff intro-u4-1.txt', 'sin stuff u4-2 to pit 2.txt',
             'sin stuff highrise.txt', 'sin stuff finale.txt', 'sin stuff arena.txt']

class PE:
    def __init__(self, path):
        self.raw=path.read_bytes(); self.sha=hashlib.sha256(self.raw).hexdigest()
        assert self.sha==EXPECTED_SERVER, f'Unexpected original server.dll hash: {self.sha}'
        pe=struct.unpack_from('<I',self.raw,0x3c)[0]
        assert self.raw[pe:pe+4]==b'PE\0\0'
        nsec=struct.unpack_from('<H',self.raw,pe+6)[0]
        optsz=struct.unpack_from('<H',self.raw,pe+20)[0]
        opt=pe+24; self.base=struct.unpack_from('<I',self.raw,opt+28)[0]
        assert self.base==0x22000000
        self.sections=[]
        for i in range(nsec):
            off=opt+optsz+i*40
            name=self.raw[off:off+8].split(b'\0')[0].decode('ascii')
            vsize,rva,rawsz,raw=struct.unpack_from('<IIII',self.raw,off+8)
            self.sections.append((name,rva,vsize,rawsz,raw))
    def off(self,va,n=1):
        rva=va-self.base
        for name,start,vsize,sz,raw in self.sections:
            if start<=rva and rva+n<=start+sz and raw+rva-start+n<=len(self.raw):return raw+rva-start
        raise ValueError(f'Unmapped original VA: 0x{va:08X}')
    def va(self,pos):
        for name,rva,vsize,sz,raw in self.sections:
            if raw<=pos<raw+sz:return self.base+rva+pos-raw
        return None
    def read(self,va,n):return self.raw[self.off(va,n):self.off(va,n)+n]
    def u32(self,va):return struct.unpack_from('<I',self.raw,self.off(va,4))[0]
    def text(self,va):
        off=self.off(va)
        end=self.raw.find(b'\0',off,min(len(self.raw),off+500))
        if end<0:raise ValueError('Unterminated text at '+hex(va))
        return self.raw[off:end].decode('ascii')
    def refs(self,addr):
        needle=struct.pack('<I',addr);i=0;vals=[]
        while True:
            i=self.raw.find(needle,i)
            if i<0:break
            v=self.va(i)
            if v: vals.append(v)
            i+=1
        return vals
    def all_text_function_pointers(self,vt):
        vals=[]
        for i in range(700):
            try:v=self.u32(vt+i*4)
            except ValueError:break
            if not(0x22001000<=v<0x223d2000):break
            vals.append(v)
        return vals
    def primary_vtable(self,cxx):
        st=f'.?AV{cxx}@@'.encode()+b'\0';at=self.raw.find(st)
        if at<0:return None
        nameva=self.va(at)
        if nameva is None:return None
        result=[]
        for tdref in self.refs(nameva-8):
            col=tdref-12
            try:
                if self.u32(col)!=0 or self.u32(col+4)>4096:continue
            except ValueError:continue
            for ref in self.refs(col):
                if not (0x22400000<=ref<0x224e0000):continue
                vt=ref+4;fns=self.all_text_function_pointers(vt)
                if fns:result.append((len(fns),vt))
        return max(result)[1] if result else None
    def init_u32(self,destination):
        pattern=b'\xc7\x05'+struct.pack('<I',destination)
        at=self.raw.find(pattern)
        return struct.unpack_from('<I',self.raw,at+6)[0] if at>=0 else None
    def descriptor(self,va):
        try:
            typ=self.u32(va);np=self.u32(va+4);extp=self.u32(va+0x14)
        except ValueError:
            return {'record_va':f'0x{va:08X}', 'statically_resolved':False,
                    'note':'Uninitialized .bss descriptor, populated by static constructor at runtime'}
        out={'record_va':f'0x{va:08X}','field_type_code':typ}
        try:out['internal_name']=self.text(np)
        except (ValueError,UnicodeDecodeError):out['internal_name']=None
        out['offset']=f'0x{self.u32(va+8):X}' if out['internal_name'] else None
        try:out['external_name']=self.text(extp) if extp else None
        except (ValueError,UnicodeDecodeError):out['external_name']=None
        fn=self.u32(va+0x1c)
        out['input_handler_va']=f'0x{fn:08X}' if 0x22001000<=fn<0x223d2000 else None
        out['statically_resolved']=bool(out['internal_name'])
        return out
    def datamap(self,cxx):
        vt=self.primary_vtable(cxx)
        if not vt:return {'error':'Original primary vtable not resolved'}
        method=self.u32(vt+0x34);code=self.read(method,6)
        if code[:1]!=b'\xb8' or code[5:]!=b'\xc3':
            return {'primary_vtable_va':f'0x{vt:08X}', 'error':'GetDataDescMap is not a direct literal-return accessor'}
        dm=struct.unpack_from('<I',code,1)[0]
        arr=self.init_u32(dm);count=self.init_u32(dm+4);base=self.init_u32(dm+12)
        if count is None or count<0 or count>150 or arr is None:
            return {'primary_vtable_va':f'0x{vt:08X}','datamap_va':f'0x{dm:08X}',
                    'error':'Static constructor metadata incomplete'}
        fields=[self.descriptor(arr+i*0x34) for i in range(count)]
        resolved=sum(r['statically_resolved'] for r in fields)
        return {'primary_vtable_va':f'0x{vt:08X}','datamap_accessor_va':f'0x{method:08X}',
                'datamap_va':f'0x{dm:08X}','field_array_va':f'0x{arr:08X}',
                'parent_datamap_va':f'0x{base:08X}' if base is not None else None,
                'compiled_datamap_name':self.text(self.u32(dm+8)),
                'declared_records':count,'statically_resolved_records':resolved,
                'runtime_initialized_or_empty_records':count-resolved,'fields':fields}
    def factory(self,name):
        hits=[];at=0
        while 1:
            i=self.raw.find(name.encode()+b'\0',at)
            if i<0:break
            at=i+1;nv=self.va(i)
            if nv is None:continue
            for ref in self.refs(nv):
                # push classname; push factory instance; original game registration helper
                start=self.off(ref)
                if start==0 or self.raw[start-1:start]!=b'\x68':continue
                if self.raw[start+4:start+5]!=b'\x68':continue
                obj=struct.unpack_from('<I',self.raw,start+5)[0]
                try:
                    vt=self.u32(obj);col=self.u32(vt-4);td=self.u32(col+12)
                    rtti=self.text(td+8)
                    m=re.fullmatch(r'\.\?AV\?\$CEntityFactory@V(.+)@@@@',rtti)
                    if m:hits.append({'classname':name,'class':m.group(1),
                                      'registration_va':f'0x{ref-1:08X}',
                                      'factory_instance_va':f'0x{obj:08X}'})
                except (ValueError,UnicodeDecodeError):continue
        if len(hits)!=1:raise AssertionError(f'{name}: ambiguous factory: {hits}')
        return hits[0]
    def assert_instruction(self,va,hexstr):
        expected=bytes.fromhex(hexstr)
        assert self.read(va,len(expected))==expected,hex(va)
    def assert_call(self,va,target):
        op=self.read(va,5)
        assert op[:1]==b'\xe8' and va+5+struct.unpack_from('<i',op,1)[0]==target,(hex(va),hex(target))

def category(name):
    if name.startswith('npc_'):return 'NPC / AI'
    if name.startswith('weapon_'):return 'Weapon'
    if name.startswith('item_ammo_'):return 'Ammo pickup'
    if name.startswith('item_'):return 'Health / item'
    if name.startswith('prop_'):return 'Interactive prop / vehicle'
    if name.startswith('trigger_'):return 'Map trigger / difficulty'
    if name.startswith('env_'):return 'Map effect / radio'
    if name.startswith('ai_'):return 'AI goal'
    return 'Other'

def audit(folder):
    p=PE(folder/'sin_private'/'server.dll' if (folder/'sin_private'/'server.dll').exists() else folder/'server.dll')
    clientpath=folder/'sin_private'/'client.dll' if (folder/'sin_private'/'client.dll').exists() else folder/'client.dll'
    clientsha=hashlib.sha256(clientpath.read_bytes()).hexdigest()
    assert clientsha==EXPECTED_CLIENT,clientsha
    counts=collections.Counter();loginfo={}
    for fn in LOG_FILES:
        raw=(folder/fn).read_bytes();content=raw.decode('utf-8','replace')
        names=re.findall(r'Attempted to create unknown entity type ([a-zA-Z0-9_]+)!',content)
        counts.update(names)
        loginfo[fn]={'failed_creation_lines':len(names),'distinct_classnames':len(set(names)),
                     'log_sha256':hashlib.sha256(raw).hexdigest()}
    classes={};factories=[];bycategory=collections.Counter()
    for name in sorted(counts):
        f=p.factory(name);cxx=f['class']
        if cxx not in classes:classes[cxx]=p.datamap(cxx)
        f.update({'attempts':counts[name],'category':category(name),
                  'datamap_static_fields':classes[cxx].get('statically_resolved_records',0),
                  'datamap_total_records':classes[cxx].get('declared_records',0)})
        factories.append(f);bycategory[f['category']]+=counts[name]
    assert len(factories)==49 and len(classes)==43
    assert sum(counts.values())==1531
    assert all('error' not in x for x in classes.values())
    # Pin exact new findings in the original 2006 binary, not an SDK clone.
    dd=classes['CTriggerDynamicDifficulty'];assert dd['declared_records']==3
    assert [(r['internal_name'],r['external_name'],r['offset']) for r in dd['fields']]==[
      ('m_estimatedTimeToGetHere','EstimatedTime','0x470'),
      ('m_estimatedDifficulty','EstimatedDifficulty','0x474'),
      ('m_finishesPlaythrough','EndOfGame','0x478')]
    assert dd['primary_vtable_va']=='0x2243B96C'
    assert p.u32(0x2243b96c+0x164)==0x221ccb70
    assert p.u32(0x2243b96c+0x64)==0x221d0740
    p.assert_instruction(0x221d54c1,'68 7c 04 00 00')
    p.assert_instruction(0x221d54df,'c7 06 6c b9 43 22')
    p.assert_instruction(0x221ccb79,'ff 90 20 01 00 00')
    p.assert_call(0x221ccbce,0x222b64d0)
    p.assert_call(0x221ccbe0,0x222bc4a0)
    p.assert_call(0x221ccbe7,0x222b93e0)
    p.assert_instruction(0x222b93e0,'c6 41 11 01')
    p.assert_instruction(0x222b64db,'68 2c c8 45 22')
    assert p.text(0x2245c82c)=='dynamic_difficulty'
    assert p.u32(0x2243bc0c+0x164)==0x221ccc10
    p.assert_call(0x221ccc41,0x222b63b0)
    assert classes['CNPC_MobGrunt']['declared_records']==39
    assert classes['CNPC_MobGrunt']['statically_resolved_records']==39
    assert classes['CPropSinVehicle']['declared_records']==54
    assert classes['CPropSinVehicle']['statically_resolved_records']==49
    assert sum(bool(r.get('input_handler_va')) for r in classes['CPropSinVehicle']['fields'])==25
    per_cat=dict(bycategory.most_common())
    return {'scope':'Original 2006 game server/client DLL; read-only static metadata and user HL2 console logs',
      'hashes':{'server':p.sha,'client':clientsha},
      'summary':{'missing_classnames':len(factories),'unique_original_cxx_classes':len(classes),
        'failed_creation_lines':sum(counts.values()),'original_class_datamaps_located':len(classes),
        'datamap_records_declared':sum(v['declared_records'] for v in classes.values()),
        'datamap_records_statically_resolved':sum(v['statically_resolved_records'] for v in classes.values()),
        'records_pending_runtime_constructor_decoding':sum(v['runtime_initialized_or_empty_records'] for v in classes.values()),
        'group_counts':per_cat},
      'logs':loginfo,'classnames':factories,'classes':classes,
      'new_traces':{'CTriggerDynamicDifficulty':{'compiled_object_size_bytes':0x47c,
         'constructor_va':'0x221D54C0','spawn_vtable_slot':'0x64','spawn_va':'0x221D0740',
         'touch_vtable_slot':'0x164','touch_va':'0x221CCB70',
         'touch_target_filter_virtual_offset':'0x120',
         'game_event_emitter_va':'0x222B64D0','event_root':'dynamic_difficulty',
         'end_of_game_bool_offset':'0x478','difficulty_manager_acquire_va':'0x222BC4A0',
         'manager_flag_setter_va':'0x222B93E0','manager_flag_write':'object +0x11 := 1',
         'evidence_grade':'Static exact instruction anchors. No end-to-end runtime test.'},
        'CTriggerSecret':{'touch_vtable_slot':'0x164','touch_va':'0x221CCC10',
           'event_emitter_va':'0x222B63B0', 'event_code_constant':12,
           'evidence_grade':'Static call and immediate; meaning of constant unverified.'}},
      'limitations':['All 49 classnames have original factory metadata; this is not source reconstruction.',
         'All 43 primary datamaps found; runtime-initialized descriptor records remain undecoded.',
         'A datamap input handler name does not establish complete gameplay behavior.',
         'Console failure counts include repeat map loads; no inference of map population.',
         'The remaining subsystem functionality needs method-by-method and client prediction analysis.',
         'No Quest 3 port/build changes.']}

if __name__=='__main__':
    base=pathlib.Path(sys.argv[1] if len(sys.argv)>1 else '/mnt/data')
    result=audit(base)
    output=pathlib.Path(sys.argv[2]) if len(sys.argv)>2 else base/'sin_missing_entity_atlas_evidence_10.json'
    output.write_text(json.dumps(result,indent=2,ensure_ascii=False)+'\n')
    print(json.dumps(result['summary'],indent=2))
    print('PASS: exact binary checks; 49 factory links; 43 primary datamaps; output',output)