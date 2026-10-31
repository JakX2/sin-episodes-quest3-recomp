#!/usr/bin/env python3
"""Verify SiN Episode 1 activity IDs and per-weapon selectors against the user-supplied original Windows DLL archives. No DLLs are distributed."""
import argparse
import hashlib
import json
import struct
import zipfile
from pathlib import Path

EXPECTED_SIN = {
    'server.dll':'568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650',
    'client.dll':'6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d',
}
EXPECTED_HL2 = {
    'server.dll':'e1984ba7ea6c344be55a91c3b9914f43550dcac0670e09ad9346f2c2d8f0406d',
    'client.dll':'2095adcf963a3eff8dd575056bf1e1ca840ab67c2b847181e291d0ccac093aae',
}
ACTIVITIES = {
    0x9b:'ACT_VM_DRAW',
    0x9d:'ACT_VM_IDLE',
    0xa4:'ACT_VM_PRIMARYATTACK',
    0xa5:'ACT_VM_SECONDARYATTACK',
    0xb8:'ACT_VM_IDLE_TO_LOWERED',
    0xb9:'ACT_VM_IDLE_LOWERED',
    0xba:'ACT_VM_LOWERED_TO_IDLE',
    0xbb:'ACT_VM_RECOIL1',
    0xbc:'ACT_VM_RECOIL2',
    0xbd:'ACT_VM_RECOIL3',
    0xc0:'ACT_VM_ZOOMED_IDLE',
    0xc1:'ACT_VM_ZOOMED_PRIMARYATTACK',
    0xc2:'ACT_VM_ZOOMED_SECONDARYATTACK',
}
VTABLES = {
    'CBaseHLCombatWeapon':0x22459194,
    'CWeaponMagnum':0x2247875c,
    'CWeaponAssaultRifle':0x224776ac,
    'CWeaponScatterGun':0x2247a1e4,
}
# Instruction bytes at independent sites, not merely isolated constants in data.
SIGNATURES = {
    'enter_zoom_activity':(0x222cbba0,'68 c0 00 00 00'),
    'exit_zoom_activity':(0x222cbc04,'68 9d 00 00 00'),
    'zoom_activity_maintenance_flag':(0x222cb963,'80 be 78 05 00 00 00'),
    'maintenance_zoom_idle_compare':(0x222cb972,'3d c0 00 00 00'),
    'maintenance_zoom_primary_compare':(0x222cb979,'3d c1 00 00 00'),
    'maintenance_zoom_secondary_compare':(0x222cb980,'3d c2 00 00 00'),
    'maintenance_virtual_finish_condition':(0x222cb98b,'ff 92 20 03 00 00'),
    'maintenance_request_zoom_idle':(0x222cb99b,'68 c0 00 00 00'),
    'maintenance_request_activity_virtual':(0x222cb9a2,'ff 90 14 03 00 00'),
    'base_primary_read_zoom_flag':(0x222cb700,'8a 81 78 05 00 00'),
    'base_primary_nonzero_to_mask':(0x222cb706,'f6 d8 1b c0 83 e0 1d'),
    'base_primary_activity_base':(0x222cb70d,'05 a4 00 00 00 c3'),
    'base_secondary_read_zoom_flag':(0x222cb720,'8a 81 78 05 00 00'),
    'base_secondary_nonzero_to_mask':(0x222cb726,'f6 d8 1b c0 83 e0 1d'),
    'base_secondary_activity_base':(0x222cb72d,'05 a5 00 00 00 c3'),
    'rifle_zoom_flag':(0x22313880,'80 b9 78 05 00 00 00'),
    'rifle_zoomed_primary':(0x22313889,'b8 c1 00 00 00 c3'),
    'rifle_recoil_state_read':(0x2231388f,'8b 81 7c 05 00 00'),
    'rifle_recoil_first_threshold':(0x22313895,'83 f8 02'),
    'rifle_normal_primary':(0x2231389a,'b8 a4 00 00 00 c3'),
    'rifle_recoil_second_threshold':(0x223138a0,'83 f8 03'),
    'rifle_recoil1':(0x223138a5,'b8 bb 00 00 00 c3'),
    'rifle_recoil_third_threshold':(0x223138ad,'83 f8 04'),
    'rifle_recoil2_or_recoil3':(0x223138b0,'0f 9d c1 81 c1 bc 00 00 00'),
    'server_activity_lookup_call':(0x222afc19,'e8 b2 ef e3 ff'),
}

class PE:
    def __init__(self,data):
        self.data=data
        h=struct.unpack_from('<I',data,0x3c)[0]
        assert data[h:h+4]==b'PE\0\0'
        opt=struct.unpack_from('<H',data,h+20)[0]
        self.base=struct.unpack_from('<I',data,h+52)[0]
        self.sections=[]
        for i in range(struct.unpack_from('<H',data,h+6)[0]):
            p=h+24+opt+i*40
            name=data[p:p+8].split(b'\0')[0].decode('ascii')
            vs,rva,rawsz,raw=struct.unpack_from('<IIII',data,p+8)
            self.sections.append((name,self.base+rva,rawsz,raw))
    def read(self,addr,count):
        for name,va,size,raw in self.sections:
            if va<=addr and addr+count<=va+size:
                return self.data[raw+addr-va:raw+addr-va+count]
        raise AssertionError('unmapped VA '+hex(addr))
    def u32(self,addr):
        return struct.unpack('<I',self.read(addr,4))[0]
    def find_section_string(self, section, text):
        needle=text.encode('ascii')+b'\0'
        matches=[]
        for name,va,size,raw in self.sections:
            if name!=section: continue
            pos=0
            blob=self.data[raw:raw+size]
            while True:
                i=blob.find(needle,pos)
                if i<0:break
                matches.append(va+i)
                pos=i+1
        return matches
    def code_search(self, needle):
        matches=[]
        for name,va,size,raw in self.sections:
            if name!='.text':continue
            blob=self.data[raw:raw+size]
            pos=0
            while True:
                i=blob.find(needle,pos)
                if i<0:break
                matches.append(va+i)
                pos=i+1
        return matches
    def rtti_name(self,vtable):
        locator=self.u32(vtable-4)
        typ=self.u32(locator+12)
        ptr=typ+8
        return self.read(ptr,128).split(b'\0')[0].decode('ascii')

def verified_zip(path,expected):
    with zipfile.ZipFile(path) as f:
        assert f.testzip() is None, 'CRC mismatch'
        blobs={n:f.read(n) for n in expected}
    digests={n:hashlib.sha256(v).hexdigest() for n,v in blobs.items()}
    assert digests==expected, f'Unexpected file content in {path}: {digests}'
    return blobs,digests

def audit(sin_zip,hl2_zip=None):
    blobs,digests=verified_zip(sin_zip,EXPECTED_SIN)
    s,c=PE(blobs['server.dll']),PE(blobs['client.dll'])
    assert (s.base,c.base)==(0x22000000,0x24000000)
    signatures=[]
    for label,(addr,hex_bytes) in SIGNATURES.items():
        exp=bytes.fromhex(hex_bytes)
        assert s.read(addr,len(exp))==exp, f'{label} {hex(addr)}'
        signatures.append({'name':label,'address':hex(addr),'bytes':exp.hex(' ')})
    # Both code-registration paths begin with PUSH ID; PUSH STRING (one path also writes a global before the call).
    # The server has two independent initialization paths for these activity names.
    registration=[]
    for ident,label in ACTIVITIES.items():
        ptrs=s.find_section_string('.rdata',label)
        assert len(ptrs)==1,(label,ptrs)
        pattern=b'\x68'+struct.pack('<I',ident)+b'\x68'+struct.pack('<I',ptrs[0])
        sites=s.code_search(pattern)
        assert len(sites)==2, (label,sites)
        registration.append({'id':ident,'id_hex':hex(ident),'name':label,'name_va':hex(ptrs[0]),'registration_sites':[hex(p) for p in sites]})
    gun_virtuals={}
    for name,vt in VTABLES.items():
        assert s.rtti_name(vt)=='.?AV'+name+'@@',name
        actual={str(i):hex(s.u32(vt+4*i)) for i in [197,219,228,229,272]}
        assert s.u32(vt+4*197)==0x222b07c0
        assert s.u32(vt+4*219)==0x222cb960
        assert s.u32(vt+4*228)==(0x22313880 if name=='CWeaponAssaultRifle' else 0x222cb700)
        assert s.u32(vt+4*229)==0x222cb720
        assert s.u32(vt+4*272)==0x222ac030
        gun_virtuals[name]={'vtable':hex(vt),'slots':actual}
    assert s.read(0x222cbba0,5)==b'\x68'+struct.pack('<I',0xc0)
    assert s.read(0x222cbc04,5)==b'\x68'+struct.pack('<I',0x9d)
    assert c.find_section_string('.rdata','ACT_VM_ZOOMED_IDLE') or (b'ACT_VM_ZOOMED_IDLE\0' in blobs['client.dll'])
    hl2=None
    if hl2_zip:
        hblobs,hdigests=verified_zip(hl2_zip,EXPECTED_HL2)
        missing={n:{k:not (v.encode('ascii')+b'\0' in blob) for k,v in ACTIVITIES.items() if k in [0xc0,0xc1,0xc2]} for n,blob in hblobs.items()}
        assert all(all(z.values()) for z in missing.values()),missing
        hl2={'sha256':hdigests,'zoomed_activity_name_absent':missing,'scope':'absence of exact names in this HL2 build, not proof that equivalent gameplay is impossible'}
    return {
        'claim':'Phase 47 SiN original binaries symbolic activity mapping and selectors',
        'sin_sha256':digests,
        'signature_count':len(signatures),
        'signatures':signatures,
        'activity_registration':registration,
        'gun_virtuals':gun_virtuals,
        'zoom_flag_offset':'weapon+0x578',
        'rifle_nonzoomed_counter_offset':'weapon+0x57c',
        'shared_activity_update':'0x222cb960',
        'zoom_in_activity':'ACT_VM_ZOOMED_IDLE',
        'zoom_out_activity':'ACT_VM_IDLE (conditional)',
        'hl2_comparison':hl2,
        'limitations':['No weapon MDL or animation data supplied; cannot resolve particular sequence IDs or sight alignment.','Slot 229 activity selection does not prove a given weapon has a usable secondary-fire attack.','No Quest port or gameplay tests have occurred.'],
    }

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('binsin_zip',type=Path)
    parser.add_argument('--hl2',type=Path)
    parser.add_argument('--json',type=Path)
    args=parser.parse_args()
    r=audit(args.binsin_zip,args.hl2)
    if args.json:
        args.json.write_text(json.dumps(r,indent=2)+'\n',encoding='utf-8')
    print('PASS: original SiN DLL hashes; %d fixed machine-code signatures; %d activity registrations twice; four RTTI-verified weapon vtables'%(r['signature_count'],len(r['activity_registration'])))
    if args.hl2: print('PASS: original HL2 DLL hashes; three exact SiN zoom-activity names absent from both comparison DLLs')
