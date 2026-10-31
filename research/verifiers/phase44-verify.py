#!/usr/bin/env python3
"""Offline verification of SiN 2006 zoom input, shared ZoomFOV getter and FOV calls.
No proprietary binary files are included in the distributed bundle.
"""
import argparse, hashlib, json, struct, zipfile
from pathlib import Path

SHA = {'server.dll':'568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650',
       'client.dll':'6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d'}

class PE:
 def __init__(self,b):
  self.b=b;p=struct.unpack_from('<I',b,0x3c)[0]
  assert b[p:p+4]==b'PE\0\0'
  self.base=struct.unpack_from('<I',b,p+52)[0]
  self.sections=[]
  for i in range(struct.unpack_from('<H',b,p+6)[0]):
   o=p+24+struct.unpack_from('<H',b,p+20)[0]+40*i
   name=b[o:o+8].split(b'\0')[0].decode('ascii')
   vs,rva,rs,raw=struct.unpack_from('<IIII',b,o+8)
   self.sections.append((name,self.base+rva,rs,raw))
 def read(self,va,n):
  for name,start,rs,raw in self.sections:
   if start<=va and va+n<=start+rs: return self.b[raw+va-start:raw+va-start+n]
  raise ValueError(f'Invalid VA {va:#x}')
 def u32(self,va):return struct.unpack('<I',self.read(va,4))[0]
 def string(self,va):
  for _,start,rs,raw in self.sections:
   if start<=va<start+rs:return self.b[raw+va-start:raw+rs].split(b'\0',1)[0].decode('ascii')
  raise ValueError(hex(va))
 def rtti(self,vt):return self.string(self.u32(self.u32(vt-4)+12)+8)

# Exact instruction sequences; do not infer meaning from signatures alone.
SIGNATURES={
 'server.dll':{
  'zoom_script_key':(0x222b4906,'68 30 c6 45 22'),
  'zoom_info_store_1bc':(0x222b4918,'89 83 bc 01 00 00'),
  'weapon_zoom_getter':(0x222ac030,'66 8b 81 1c 05 00 00'),
  'weapon_zoom_info_field':(0x222ac03d,'8b 80 bc 01 00 00'),
  'zoom_active_guard':(0x222cbb63,'80 be 78 05 00 00 00'),
  'zoom_fov_getter_vcall':(0x222cbb8f,'ff 92 40 04 00 00'),
  'zoom_in_duration':(0x222cbb88,'68 00 00 80 3e'),
  'zoom_in_fov_player_call':(0x222cbb99,'e8 52 2b f8 ff'),
  'zoom_active_set':(0x222cbbad,'c6 86 78 05 00 00 01'),
  'zoom_out_duration':(0x222cbbe6,'68 9a 99 19 3e'),
  'zoom_out_target_zero':(0x222cbbeb,'6a 00'),
  'zoom_out_fov_player_call':(0x222cbbf0,'e8 fb 2a f8 ff'),
  'zoom_active_clear':(0x222cbc11,'c6 86 78 05 00 00 00'),
  'zoom_common_branch_call_enable':(0x222ccdb9,'e8 a2 ed ff ff'),
  'zoom_common_branch_call_disable':(0x222ccd98,'e8 23 ee ff ff'),
 },
 'client.dll':{
  'zoom_plus_command_handler':(0x2426b296,'68 c0 1d 16 24'),
  'zoom_minus_command_handler':(0x2426b2c6,'68 d0 1d 16 24'),
  'zoom_plus_kbutton_pointer':(0x24161dc2,'68 38 80 36 24'),
  'zoom_minus_kbutton_first_holder':(0x24161ded,'8b 0d 38 80 36 24'),
  'zoom_down_sets_three':(0x24160f98,'83 c9 03'),
  'zoom_key_mask':(0x2416269d,'81 ce 00 00 08 00'),
  'zoom_input_state_test':(0x24162695,'84 05 40 80 36 24'),
 },
}
VT={'CBaseHLCombatWeapon':0x22459194,'CWeaponMagnum':0x2247875c,
    'CWeaponAssaultRifle':0x224776ac,'CWeaponScatterGun':0x2247a1e4}

def audit(original):
 for k,b in original.items():assert hashlib.sha256(b).hexdigest()==SHA[k],f'wrong {k} SHA-256'
 pe={k:PE(b) for k,b in original.items()}
 assert pe['server.dll'].base==0x22000000 and pe['client.dll'].base==0x24000000
 verified=[]
 for name,group in SIGNATURES.items():
  for label,(va,hx) in group.items():
   assert pe[name].read(va,len(bytes.fromhex(hx)))==bytes.fromhex(hx),(name,label,hex(va))
   verified.append({'dll':name,'name':label,'va':hex(va),'bytes':hx})
 s,c=pe['server.dll'],pe['client.dll']
 assert s.string(0x2245c630)=='ZoomFOV'
 assert c.string(0x2429fb60)=='+zoom' and c.string(0x2429fb68)=='-zoom'
 assert s.u32(0x222b4907)==0x2245c630
 assert c.u32(0x2426b29c)==0x2429fb60 and c.u32(0x2426b2cc)==0x2429fb68
 assert c.u32(0x2426b297)==0x24161dc0 and c.u32(0x2426b2c7)==0x24161dd0
 target=s.u32(VT['CBaseHLCombatWeapon']+4*272)
 assert target==0x222ac030
 classes={}
 for name,vt in VT.items():
  assert s.rtti(vt)=='.?AV'+name+'@@',(name,s.rtti(vt))
  assert s.u32(vt+272*4)==target,name
  classes[name]={'vtable':hex(vt),'zoom_virtual_slot':272,'getter':hex(target)}
 # Confirm numeric constants exactly as pushed by FOV callsites.
 assert struct.unpack('<f',s.read(0x222cbb89,4))[0]==0.25
 f=struct.unpack('<f',s.read(0x222cbbe7,4))[0]
 assert abs(f-0.15)<1e-7
 return {'hashes':{n:hashlib.sha256(b).hexdigest() for n,b in original.items()},
         'instruction_signatures':verified,
         'shared_weapon_zoom_getter':classes,
         'client_zoom':{'down_handler':'0x24161dc0','up_handler':'0x24161dd0',
                        'kbutton_state':'0x24368038','kbutton_flags':'0x24368040',
                        'button_mask':'0x00080000'},
         'zoom_fov':{'script_key':'ZoomFOV','weapon_info_offset':'0x1bc',
                     'getter':'0x222ac030','player_fov_function':'0x2224e6f0',
                     'zoom_in_duration_seconds':0.25,'zoom_out_duration_seconds':f,
                     'zoom_out_target':0},
         'limitations':['Not yet proven whether every firearm invokes these zoom-in/out routines under every condition.',
                        'Not yet identified exact server writer of m_bZooming / m_bZoomScope network properties.',
                        'Not yet recovered sight/viewmodel pose offsets or optic overlays.',
                        'FOV transition model omits player ownership/weapon eligibility checks present in original code.']}

if __name__=='__main__':
 a=argparse.ArgumentParser();a.add_argument('binzip',type=Path);a.add_argument('--output',type=Path,default=Path('verified.json'));args=a.parse_args()
 with zipfile.ZipFile(args.binzip) as z:original={k:z.read(k) for k in SHA}
 report=audit(original);args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(report,indent=2)+'\n')
 print(f"PASS {len(report['instruction_signatures'])} x86 signatures; 2 DLL hashes; 4 weapon vtable getters; zoom key/FOV numeric constants")
