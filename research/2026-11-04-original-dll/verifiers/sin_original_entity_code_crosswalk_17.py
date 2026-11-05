#!/usr/bin/env python3
"""Read-only original SiN x86 DLL vs HL2-log code crosswalk.

Usage:
    python sin_original_entity_code_crosswalk_17.py /path/to/private-logs-and-original-DLLs OUTPUT_STEM

Requires sin_missing_entity_atlas_audit.py alongside this script. Reads only.
Verifies SHA-256 and all 49 compiler factory registration sequences, then
locates direct client RTTI/vtables when the original classname is known.
Contains no original game source, binary bytes or protected assets.
"""
import collections
import csv
import hashlib
import json
import pathlib
import struct
import sys

from sin_missing_entity_atlas_audit import audit, EXPECTED_CLIENT

# These are strictly one-to-one named client RTTI counterparts in client.dll,
# not a complete mapping of all render/network/HUD dependencies.
CLIENT_CLASS_CORRESPONDENCES = {
    'CNPC_EliteGrunt': 'C_NPC_EliteGrunt',
    'CNPC_Hologram': 'C_NPC_Hologram',
    'CPropPhone': 'C_PropPhone',
    'CPropSinVehicle': 'C_PropSinVehicle',
    'CWeaponAssaultRifle': 'C_WeaponAssaultRifle',
    'CWeaponGrenade': 'C_WeaponGrenade',
    'CWeaponMagnum': 'C_WeaponMagnum',
    'CWeaponNPCPistol': 'C_WeaponNPCPistol',
    'CWeaponScatterGun': 'C_WeaponScatterGun',
}
CLIENT_ADDITIONAL_CLASSES = (
    'C_NPC_GruntBase', 'C_DynamicDifficulty', 'CHudPoliceRadio',
    'CHudGrenades', 'CHudPoisonTimer', 'CHudArena',
)

class ClientPE:
    def __init__(self, path):
        self.data = pathlib.Path(path).read_bytes()
        self.sha = hashlib.sha256(self.data).hexdigest()
        assert self.sha == EXPECTED_CLIENT, f'client.dll SHA-256 mismatch: {self.sha}'
        off = struct.unpack_from('<I', self.data, 0x3c)[0]
        assert self.data[off:off+4] == b'PE\0\0'
        nsec = struct.unpack_from('<H', self.data, off+6)[0]
        opts = struct.unpack_from('<H', self.data, off+20)[0]
        self.base = struct.unpack_from('<I', self.data, off+24+28)[0]
        assert self.base == 0x24000000
        self.sections = []
        for i in range(nsec):
            q = off+24+opts+i*40
            name=self.data[q:q+8].split(b'\0')[0].decode('ascii')
            vsize,rva,rawsize,rawpos=struct.unpack_from('<IIII',self.data,q+8)
            self.sections.append((name,rva,rawsize,rawpos))
        self.ranges = {name:(self.base+rva,self.base+rva+size) for name,rva,size,pos in self.sections}

    def va(self,offset):
        for name,rva,sz,raw in self.sections:
            if raw <= offset < raw+sz:
                return self.base+rva+offset-raw
        return None

    def offset(self,address,n=4):
        rva = address-self.base
        for name,start,size,raw in self.sections:
            if start<=rva and rva+n<=start+size:
                assert raw+rva-start+n<=len(self.data)
                return raw+rva-start
        raise ValueError(f'VA not mapped: 0x{address:08X}')

    def u32(self,address):
        return struct.unpack_from('<I',self.data,self.offset(address))[0]

    def refs(self,address):
        pat=struct.pack('<I',address); at=0
        while True:
            q=self.data.find(pat,at)
            if q < 0: return
            val=self.va(q)
            if val is not None: yield val
            at=q+1

    def class_primary_vtable(self,cxx):
        # MSVC x86 RTTI CompleteObjectLocator -> decorated TypeDescriptor ->
        # candidate primary vtable. Chooses longest contiguous executable table.
        key=(f'.?AV{cxx}@@').encode('ascii')+b'\0'
        at=self.data.find(key)
        if at<0: return None
        td=self.va(at-8)
        best=None
        for tdptr in self.refs(td):
            col=tdptr-12
            try:
                if self.u32(col)!=0 or self.u32(col+4)>4096:continue
            except ValueError:continue
            for vtref in self.refs(col):
                if not self.ranges['.rdata'][0]<=vtref<self.ranges['.data'][1]:continue
                candidate=vtref+4
                functions=[]
                for i in range(600):
                    try: pointer=self.u32(candidate+i*4)
                    except ValueError:break
                    if not self.ranges['.text'][0]<=pointer<self.ranges['.text'][1]:break
                    functions.append(pointer)
                if functions and (best is None or len(functions)>best['vtable_entry_count']):
                    best={'rtti_type_descriptor_va':f'0x{td:08X}',
                          'primary_vtable_va':f'0x{candidate:08X}',
                          'vtable_entry_count':len(functions),
                          'first_method_va':f'0x{functions[0]:08X}'}
        return best

def scan(root):
    old=audit(root)
    client_path=root/'sin_private'/'client.dll'
    if not client_path.is_file():client_path=root/'client.dll'
    cp=ClientPE(client_path)
    counterparts={}
    for srv,cl in CLIENT_CLASS_CORRESPONDENCES.items():
        found=cp.class_primary_vtable(cl)
        assert found is not None, f'Missing expected client vtable {cl}'
        counterparts[srv]={'client_class':cl,**found}
    helpers={}
    for cl in CLIENT_ADDITIONAL_CLASSES:
        found=cp.class_primary_vtable(cl)
        assert found is not None, f'Missing expected client helper vtable: {cl}'
        helpers[cl]=found
    rows=[]
    for f in old['classnames']:
        cl=counterparts.get(f['class'])
        datamap=old['classes'][f['class']]
        named=[x for x in datamap['fields'] if x.get('statically_resolved')]
        io=[x for x in named if x.get('input_handler_va')]
        externals=[x['external_name'] for x in named if x.get('external_name')]
        rows.append({
            'classname':f['classname'], 'category':f['category'],
            'failed_attempts':f['attempts'], 'server_cxx_class':f['class'],
            'server_registration_va':f['registration_va'],
            'server_factory_instance_va':f['factory_instance_va'],
            'server_primary_vtable_va':datamap['primary_vtable_va'],
            'server_datamap_va':datamap['datamap_va'],
            'server_datamap_records':datamap['declared_records'],
            'server_datamap_names_resolved':datamap['statically_resolved_records'],
            'server_named_input_handlers':len(io),
            'server_named_inputs':'; '.join(x['external_name'] for x in io if x.get('external_name')),
            'server_named_map_keys_and_outputs':'; '.join(externals),
            'client_named_counterpart':cl['client_class'] if cl else '',
            'client_primary_vtable_va':cl['primary_vtable_va'] if cl else '',
            'client_first_virtual_method_va':cl['first_method_va'] if cl else '',
            'client_vtable_entry_count':cl['vtable_entry_count'] if cl else '',
        })
    assert len(rows)==49
    assert len({f['server_cxx_class'] for f in rows})==43
    assert len([f for f in rows if f['client_named_counterpart']])==9
    assert sum(x['failed_attempts'] for x in rows)==1531
    assert sum(old['classes'][s]['declared_records'] for s in old['classes'])==561
    assert sum(old['classes'][s]['statically_resolved_records'] for s in old['classes'])==305
    assert len(helpers)==6
    return {'source':'Original SiN Episodes 2006 PE32 DLLs, user HL2 console logs',
            'method':'factory registration proof + server datamap and client RTTI primary vtable crosswalk',
            'sha256':{'server':old['hashes']['server'],'client':cp.sha},
            'summary':{'observed_missing_classnames':49, 'unique_server_cxx_classes':43,
                'observed_unknown_entity_creation_attempts':1531,
                'server_datamap_records_declared':561,
                'server_datamap_names_statically_resolved':305,
                'datamap_records_runtime_initialized_or_empty':256,
                'direct_named_client_counterparts':9,
                'additional_client_subsystems_located':len(helpers)},
            'log_counts':old['logs'], 'entities':rows,
            'client_counterparts':counterparts,
            'client_additional_systems':helpers,
            'limitations':[
                'Original executable RTTI/factories and method pointers are NOT original source code.',
                'Missing client class-name RTTI is NOT proof that no client code implements the entity.',
                'Static method pointers and input names do not prove behavior or engine ABI compatibility.',
                'Unknown-entity console counts include repeat map loads, not unique spawns.',
                'No original proprietary executable bytes or game assets are redistributed.',
                'No Quest/native gameplay code is implemented by this crosswalk.'
            ]}

def render_markdown(info):
    s=info['summary'];rows=info['entities']
    out=[
        '# Original SiN Episodes DLL Code Crosswalk: 49 HL2-Missing Map Entities (Round 17)', '',
        '**Read-only static analysis** of original 2006 Windows x86 `server.dll` and `client.dll`, matched to five user HL2 console logs. Factory registration is confirmed code evidence, not a decompiled implementation or working Quest 3 entity.','',
        f"- `server.dll` SHA256: `{info['sha256']['server']}`",
        f"- `client.dll` SHA256: `{info['sha256']['client']}`",'',
        f"**Result:** {s['observed_missing_classnames']} / 49 classnames have exact server factory registrations; {s['unique_server_cxx_classes']} underlying server C++ classes; {s['direct_named_client_counterparts']} named client RTTI/vtable counterparts. All {s['server_datamap_records_declared']} server datamap records located, with {s['server_datamap_names_statically_resolved']} names statically readable and {s['datamap_records_runtime_initialized_or_empty']} awaiting static-constructor reconstruction.", '',
        '## Complete log classname -> original server code', '',
        '| Missing classname | Failed attempts | Original server C++ class | Original registration VA | Client counterpart |','|---|---:|---|---|---|'
    ]
    for row in sorted(rows,key=lambda r:(-r['failed_attempts'],r['classname'])):
        out.append(f"| `{row['classname']}` | {row['failed_attempts']} | `{row['server_cxx_class']}` | `{row['server_registration_va']}` | `{row['client_named_counterpart']}` |" if row['client_named_counterpart'] else f"| `{row['classname']}` | {row['failed_attempts']} | `{row['server_cxx_class']}` | `{row['server_registration_va']}` | no direct matching RTTI |")
    out+=['', '## Verified client.dll virtual code entrypoints','',
        'These nine have **client-side MSVC RTTI + primary vtable + executable first-method pointers**. Many other server classes can reuse generic client rendering, so lack of a directly named client class is not proof of missing presentation code.','',
        '| Server C++ class | Client C++ class | Client primary vtable VA | First virtual-method VA | Vtable entries |','|---|---|---|---|---:|']
    for server,cl in sorted(info['client_counterparts'].items()):
        out.append(f"| `{server}` | `{cl['client_class']}` | `{cl['primary_vtable_va']}` | `{cl['first_method_va']}` | {cl['vtable_entry_count']} |")
    out+=['','## Additional client-side systems','',
          'Not one-to-one representations of the 49 logged map classes, but relevant for missing SiN mechanics and player presentation.','',
          '| Client class | Primary vtable VA | First virtual method VA |','|---|---|---|']
    for name,d in sorted(info['client_additional_systems'].items()):
        out.append(f"| `{name}` | `{d['primary_vtable_va']}` | `{d['first_method_va']}` |")
    out+=['','## Server datamap pointers and named inputs for high-value entities','',
          '| Map entity | Server datamap VA | Resolved / declared fields | Named inputs |','|---|---|---|---|']
    focus=['trigger_dynamic_difficulty','ai_goal_takecover','npc_jessica','npc_druglab_grunt_pistol','npc_druglab_grunt_elite','npc_securitybot','prop_phone','prop_vehicle_sin','env_radio_message','item_antigen_dispenser','prop_exploding_barrel','trigger_secret','weapon_grenade']
    byname={x['classname']:x for x in rows}
    for name in focus:
        row=byname[name]
        inputs=row['server_named_inputs'] or 'none resolved statically'
        if len(inputs)>160:inputs=inputs[:157]+'…'
        out.append(f"| `{name}` | `{row['server_datamap_va']}` | {row['server_datamap_names_resolved']}/{row['server_datamap_records']} | {inputs} |")
    out+=['','## How to use this for a working port','',
        '1. Implement the map entity factories on the **server** side first and honor the original keyvalues, inputs and outputs. Keep the original `C++` class family relationships where behavior depends on them.',
        '2. For player-visible entities, verify the original server-to-client network representation. The nine direct client classes are an initial target list, not an exhaustive nettable map.',
        '3. Supply the original entity scripts, models and scenes through lawful game mounts. The console errors for missing VCDs, material proxies and audio sample rates are **separate** from unknown entity factories.',
        '4. Start with a minimal no-NPC level and one usable object, then add player weapons, spawnable grunts, one AI encounter and checkpoint logic. Avoid inserting fabricated stubs that hide errors but have no functionality.',
        '5. Test the native Source/ARM64 ABI and record errors on the target build. None of the recovered x86 VAs can be called directly from Quest 3.', '',
        '## Evidence boundaries', '',
        '* All map classnames came from the supplied five console logs. The server classes and addresses came from exact original PE factory bytes and RTTI. Client classes and pointers were independently found from the original `client.dll` RTTI and vtable pointers.',
        '* 1,531 is a count of **error lines** and can include the same entity more than once.',
        '* A direct client counterpart was not identified for 40 classnames; this is **not** proof that 40 lack client functionality.',
        '* The audit does not decompile complete behavior for 49 entities or make them work in stock HL2.',
        '', '## Reproduction', '',
        'Run `python3 sin_original_entity_code_crosswalk_17.py /path/to/private/files /tmp/sin_original_entity_code_crosswalk_17` with `sin_missing_entity_atlas_audit.py` in the same directory. The private folder must contain the original SHA-matching DLLs in `sin_private/` and the five exact original HL2 logs. The output includes `.json`, `.csv`, `.md`.','']
    return '\n'.join(out)

def main(root,stem):
    info=scan(root)
    stem.parent.mkdir(parents=True,exist_ok=True)
    (pathlib.Path(str(stem)+'.json')).write_text(json.dumps(info,indent=2,ensure_ascii=False)+'\n',encoding='utf-8')
    with open(str(stem)+'.csv','w',newline='',encoding='utf-8') as f:
        w=csv.DictWriter(f,fieldnames=info['entities'][0].keys());w.writeheader();w.writerows(info['entities'])
    pathlib.Path(str(stem)+'.md').write_text(render_markdown(info),encoding='utf-8')
    print('PASS: 49 server factories; 43 server C++ classes; 9 client counterparts; 6 extra client types; 5 logs; original DLL hashes match.')
    for extension in ('.json','.csv','.md'):print('WROTE',str(stem)+extension)

if __name__=='__main__':
    base=pathlib.Path(sys.argv[1] if len(sys.argv)>1 else '/mnt/data')
    stem=pathlib.Path(sys.argv[2] if len(sys.argv)>2 else '/mnt/data/sin_original_entity_code_crosswalk_17')
    main(base,stem)