#!/usr/bin/env python3
"""Read-only, reproducible SiN Episodes 2006 asset/DLL/HL2 log crosswalk.

Usage:
 python sin_drive_crosswalk_audit.py /mnt/data /mnt/data/sin_private/server.dll

Requires Python 3 stdlib, the user's own server.dll, and Drive archives/logs placed
inside the given directory. Emits ONLY reconstructed metadata (not game code).
"""
from __future__ import annotations
import collections, hashlib, json, pathlib, re, struct, sys, zipfile

EXPECTED_SERVER = '568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650'
EXPECTED_CLIENT = '6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d'
LOG_FILES = ['sin stuff intro-u4-1.txt','sin stuff u4-2 to pit 2.txt','sin stuff highrise.txt','sin stuff finale.txt','sin stuff arena.txt']
BUNDLES = ['scripts.zip','scenes.zip','resource.zip','expressions.zip','cfg.zip','bin.zip','bin 1.zip']
SCENE_ERRORS = [
'scenes/se1_highrise/jess_wakeupblade.vcd',
'scenes/se1_finale/teaser_radek_and_jessica.vcd',
'scenes/se1_finale/teaser_jc_hospital.vcd',
'scenes/se1_finale/teaser_elexis_laughing.vcd']
LOCALIZED_KEYS = ['SE1_VGUI_DOCKS_1A_SEWERHEADER','SE1_VGUI_DOCKS_1A_DOORSCLOSED','SE1_VGUI_DOCKS_1A_OVERRIDEOFFLINE','SE1_VGUI_DOCKS_1A_SEALDOORS','SE1_VGUI_DOCKS_1A_RAISEWATER','SE1_VGUI_DOCKS_1A_LOWERWATER']

class PE:
    def __init__(self,path):
        self.b=path.read_bytes(); self.sha=hashlib.sha256(self.b).hexdigest()
        if self.sha!=EXPECTED_SERVER:raise AssertionError('Unsupported original server.dll sha256: '+self.sha)
        peoff=struct.unpack_from('<I',self.b,0x3c)[0]
        assert self.b[peoff:peoff+4]==b'PE\0\0'
        nsec=struct.unpack_from('<H',self.b,peoff+6)[0]
        optsz=struct.unpack_from('<H',self.b,peoff+20)[0]; opt=peoff+24
        self.base=struct.unpack_from('<I',self.b,opt+28)[0]
        self.sec=[]
        for i in range(nsec):
            pos=opt+optsz+40*i
            vs,rva,rawsz,raw=struct.unpack_from('<IIII',self.b,pos+8)
            self.sec.append((rva,rawsz,raw))
    def va(self,off):
        for rva,sz,raw in self.sec:
            if raw<=off<raw+sz:return self.base+rva+off-raw
        return None
    def off(self,va,n=4):
        rva=va-self.base
        for begin,sz,off in self.sec:
            if begin<=rva and rva+n<=begin+sz and off+rva-begin+n<=len(self.b):return off+rva-begin
        raise ValueError(hex(va))
    def u32(self,va):return struct.unpack_from('<I',self.b,self.off(va))[0]
    def read(self,va,n):p=self.off(va,n);return self.b[p:p+n]
    def text(self,va):
        i=self.off(va,1);stop=self.b.find(b'\0',i,i+350)
        if stop<0:raise ValueError(hex(va))
        return self.b[i:stop].decode('ascii')
    def descriptor(self,rec):
        typ=self.u32(rec);name=self.text(self.u32(rec+4));off=self.u32(rec+8)
        extva=self.u32(rec+20)
        ext=self.text(extva) if 0x22400000<extva<0x22490000 else None
        handler=self.u32(rec+28)
        return {'metadata_address':f'0x{rec:08X}','type_id':typ,'internal_name':name,
                'entity_offset':f'0x{off:X}','external_name':ext,
                'handler_va':f'0x{handler:08X}' if handler else None}
    def factory_for(self,name):
        hits=[];start=0
        while True:
            i=self.b.find(name.encode('ascii')+b'\0',start)
            if i<0:break
            nameva=self.va(i);start=i+1
            if not nameva:continue
            needle=b'\x68'+struct.pack('<I',nameva);off=0
            while True:
                j=self.b.find(needle,off)
                if j<0:break
                off=j+1
                if self.b[j+5:j+6]!=b'\x68':continue
                obj=struct.unpack_from('<I',self.b,j+6)[0]
                try:
                    vt=self.u32(obj);col=self.u32(vt-4);td=self.u32(col+12)
                    rtti=self.text(td+8)
                    m=re.fullmatch(r'\.\?AV\?\$CEntityFactory@V(.+)@@@@',rtti)
                    if not m:continue
                    hits.append({'classname':name,'original_cxx_class':m.group(1),
                                 'registration_va':f'0x{self.va(j):08X}',
                                 'factory_object_va':f'0x{obj:08X}',
                                 'rtti':rtti})
                except (ValueError,UnicodeDecodeError,struct.error):continue
        if len(hits)!=1:raise AssertionError(f'Expected one valid factory registration for {name}: {hits}')
        return hits[0]

def keyvalue_sound_definitions(z):
    pat=re.compile(r'(?m)^\s*"([^"\r\n]+)"[^\r\n]*\r?\n(?:(?:\s*//[^\r\n]*|\s*)\r?\n)*\s*\{')
    definitions=collections.defaultdict(list)
    for name in z.namelist():
        if not (name.lower().endswith('.txt') and 'sound' in name.lower()):continue
        text=z.read(name).decode('cp1252','replace')
        for k in pat.findall(text):definitions[k.casefold()].append(name)
    return definitions

def audit(folder,server_path):
    server=PE(server_path)
    client_file=server_path.parent/'client.dll'
    client_bytes=client_file.read_bytes()
    client_sha=hashlib.sha256(client_bytes).hexdigest()
    assert client_sha==EXPECTED_CLIENT
    assert b'.?AVCHudPoliceRadio@@' in client_bytes
    assert b'KeypadScheme.res' in client_bytes
    logs={};counts=collections.Counter();all_types=set()
    for fn in LOG_FILES:
        src=(folder/fn).read_text(encoding='utf-8',errors='replace')
        names=re.findall(r'Attempted to create unknown entity type ([A-Za-z0-9_]+)!',src)
        per=collections.Counter(names)
        counts.update(per);all_types|=set(names)
        logs[fn]={'attempts':len(names),'distinct_types':len(per),
                  'top':per.most_common(12),'sha256':hashlib.sha256((folder/fn).read_bytes()).hexdigest()}
    factories=[server.factory_for(k) for k in sorted(all_types)]
    classgroups=collections.defaultdict(list)
    for f in factories:classgroups[f['original_cxx_class']].append(f['classname'])
    factory_aliases={k:v for k,v in classgroups.items() if len(v)>1}
    bundles={}
    for fn in BUNDLES:
        z=zipfile.ZipFile(folder/fn)
        bundles[fn]={'sha256':hashlib.sha256((folder/fn).read_bytes()).hexdigest(),
                     'archive_entries':len(z.namelist()),
                     'files':sum(not x.endswith('/') for x in z.namelist())}

    with zipfile.ZipFile(folder/'scenes.zip') as zs,zipfile.ZipFile(folder/'scripts.zip') as zscr,zipfile.ZipFile(folder/'resource.zip') as zr:
        scfiles={x.casefold() for x in zs.namelist()}
        allscenes=[x for x in zs.namelist() if x.lower().endswith('.vcd')]
        scene_presence={x:x.casefold() in scfiles for x in SCENE_ERRORS}
        assert all(scene_presence.values())
        events=collections.Counter();speak=[]
        for sc in allscenes:
            text=zs.read(sc).decode('cp1252','replace')
            events.update(k.lower() for k in re.findall(r'(?m)^\s*event\s+(\w+)\s+"',text))
            cues=re.findall(r'(?ms)^\s*event\s+speak\s+"[^"\r\n]*"\s*\{.*?^\s*param\s+"([^"]+)"',text)
            speak.extend({'scene':sc,'sound':cue} for cue in cues)
        sounddefs=keyvalue_sound_definitions(zscr)
        resolved=sum(ref['sound'].casefold() in sounddefs for ref in speak)
        unresolved=sorted({ref['sound'] for ref in speak if ref['sound'].casefold() not in sounddefs})
        important_sounds={cue:sounddefs.get(cue.casefold(),[]) for cue in ['highrise.jess_shakeoff','highrise.jess_rodhole','finale.jc_uplink','jessica.jess_doihavesmbutt']}
        manifest=zscr.read('scripts/game_sounds_manifest.txt').decode('cp1252','replace')
        manifest_inc=re.findall(r'(?m)^\s*"precache_file"\s*"([^"]+)"',manifest)
        scrfiles={x.casefold() for x in zscr.namelist()}
        absent_from_archive=[x for x in manifest_inc if x.casefold() not in scrfiles]
        radio=zscr.read('scripts/radio.txt').decode('cp1252','replace')
        scanner=re.findall(r'(?m)^\s*"Scanner"\s*"([^"]+)"',radio)
        phone=zscr.read('scripts/phonebook.txt').decode('cp1252','replace')
        phone_numbers=re.findall(r'(?m)^\s*"([0-9]+)"\s*"([^"\r\n]+)"',phone)
        hud=zscr.read('scripts/HudAnimations.txt').decode('cp1252','replace')
        assert 'HudPoliceRadio' in hud
        assert 'resource/KeypadScheme.res' in zr.namelist()
        loc=zr.read('resource/se1_english.txt').decode('utf-16')
        tokens={}
        for key in LOCALIZED_KEYS:
            m=re.search(r'"'+re.escape(key)+r'"\s*"([^"\r\n]+)"',loc,re.IGNORECASE)
            assert m,key
            tokens[key]=m.group(1)
        scene_stats={'vcd_files':len(allscenes),'previously_erroring_files_present':scene_presence,
                     'event_types':dict(events.most_common()),'speak_events':len(speak),
                     'speak_events_with_script_definition':resolved,
                     'speak_events_without_definition_in_supplied_scripts_zip':len(speak)-resolved,
                     'unresolved_sound_names':unresolved,
                     'examples_definition_locations':important_sounds,
                     'scene_cache_present_in_scenes_archive':any('scene.cache'==x.split('/')[-1].casefold() for x in zs.namelist())}
        script_stats={'client_hud_police_radio_animation_present':True,'client_keypad_scheme_present':True,'manifest_entries':len(manifest_inc),
                      'manifest_references_absent_from_scripts_zip':absent_from_archive,
                      'sound_definitions_in_scripts_zip':len(sounddefs),
                      'radio_scanner_entries':scanner,
                      'phonebook_number_entries':len(phone_numbers),
                      'phonebook_first_five':phone_numbers[:5]}

    # Recovery of actual original C++ datamap inputs and saved entity fields.
    vehicle=[server.descriptor(0x22531604+0x34*i) for i in range(23)]
    assert vehicle[0]['external_name']=='EnableUse'
    assert vehicle[-1]['external_name']=='SetRadioStation'
    assert all(x['internal_name'].startswith('Input') for x in vehicle)
    assert len({x['handler_va'] for x in vehicle})==23
    radio=[server.descriptor(0x225242cc+0x34*i) for i in range(5)]
    msg=[server.descriptor(0x22524aec+0x34*i) for i in range(9)]
    assert [x['internal_name'] for x in msg[3:8]]==['InputSetMessage','InputSetSpeaker','InputSetPriority','InputDispatch','InputRemove']
    assert msg[8]['external_name']=='OnDispatch'
    # Original server Dispatch method resolves a local player before FireOutput.
    assert server.read(0x222e4bfe,5)==bytes.fromhex('e8 fd 1c e7 ff')
    assert server.read(0x2232f500,6)==bytes.fromhex('8b 81 68 07 00 00')
    assert server.text(0x22462b28)=='scripts/radio.txt'
    assert server.text(0x22462dd0)=='env_radio'
    assert server.text(0x224639c0)=='env_radio_message'
    assert server.text(0x2243caf4)=='trigger_dynamic_difficulty'
    for key in ('prop_vehicle_sin','env_radio','env_radio_message','trigger_dynamic_difficulty'):
        assert key in all_types
    return {'binary':{'server_sha256':server.sha,'client_sha256':client_sha,'client_hud_police_radio_rtti_present':True,'client_keypad_scheme_reference_present':True},'console':{'logs':logs,
             'total_failed_creation_attempts':sum(counts.values()),'distinct_classnames':len(counts),
             'top_25':counts.most_common(25),'all_factory_mappings_verified':len(factories),
             'original_factories':factories,'shared_cxx_factory_class_groups':factory_aliases},
             'archives':bundles,'scenes':scene_stats,'scripts':script_stats,
             'localization':{'original_file':'resource/se1_english.txt','dock_panel_tokens':tokens},
             'original_dll_entity_datamaps':{'CPropSinVehicle_inputs':vehicle,'CRadio_fields':radio,'CRadioMessage_fields_and_inputs':msg,
                 'CRadioMessage_Dispatch_FireOutput_call':'0x222E4BFE',
                 'CPropSinVehicle_RadioOn_handle_check':'0x2232F500',
                 'CTriggerDynamicDifficulty_status':'Factory verified; behavior/datamap not reconstructed in this audit'},
             'scope_notes':['No game binaries or copyrighted script contents are redistributed by the audit output.',
                            'Log counts are repeated creation attempts, not unique map placements.',
                            'Archive-level presence does not prove files were extracted/mounted in the tested HL2 build.',
                            'Sound references absent from scripts.zip might be provided by another mounted Source game.',
                            'Runtime output handler semantics beyond verified call sites require further tracing.']}

if __name__=='__main__':
    root=pathlib.Path(sys.argv[1]) if len(sys.argv)>1 else pathlib.Path('/mnt/data')
    server=pathlib.Path(sys.argv[2]) if len(sys.argv)>2 else root/'sin_private/server.dll'
    result=audit(root,server)
    out=root/'sin_drive_crosswalk_evidence_09.json'
    out.write_text(json.dumps(result,indent=2,ensure_ascii=False)+'\n',encoding='utf-8')
    print(f'PASS: {result["console"]["distinct_classnames"]} original factory registrations, '
          f'{result["console"]["total_failed_creation_attempts"]} logged failures, '
          f'{len(result["original_dll_entity_datamaps"]["CPropSinVehicle_inputs"])} vehicle inputs, '
          f'{result["scenes"]["speak_events"]} scene voice cues; output {out}')