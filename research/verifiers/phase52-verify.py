#!/usr/bin/env python3
"""Phase 52 anchored proof checks, on user's original Windows x86 SiN DLL archive.
No game binaries are included in the evidence ZIP. All VAs refer to this one build.
"""
import argparse, hashlib, json, struct, zipfile
from pathlib import Path

EXPECTED = {
 'sin': {'server.dll':'568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650',
         'client.dll':'6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d'},
 'hl2': {'server.dll':'e1984ba7ea6c344be55a91c3b9914f43550dcac0670e09ad9346f2c2d8f0406d',
         'client.dll':'2095adcf963a3eff8dd575056bf1e1ca840ab67c2b847181e291d0ccac093aae'}
}

class PE:
 def __init__(self, data):
  self.data=data; p=struct.unpack_from('<I',data,0x3c)[0]
  assert data[p:p+4]==b'PE\x00\x00'
  n=struct.unpack_from('<H',data,p+6)[0]
  opt=struct.unpack_from('<H',data,p+20)[0]
  self.base=struct.unpack_from('<I',data,p+52)[0]
  self.sections=[]
  for i in range(n):
   q=p+24+opt+40*i
   name=data[q:q+8].split(b'\x00')[0].decode('ascii')
   virt_size,rva,raw_size,raw_offset=struct.unpack_from('<IIII',data,q+8)
   self.sections.append((name,self.base+rva,raw_size,raw_offset))
 def read(self,va,n):
  for name,start,size,off in self.sections:
   if start<=va and va+n<=start+size:
    return self.data[off+va-start:off+va-start+n]
  raise ValueError(f'VA not mapped: {va:#x}+{n}')
 def u32(self,va):return struct.unpack('<I',self.read(va,4))[0]
 def rtti(self,vt):
  col=self.u32(vt-4);td=self.u32(col+12)
  return self.read(td+8,130).split(b'\x00')[0].decode('ascii')
 def rel_call(self,addr):
  ins=self.read(addr,5)
  assert ins[0] in (0xe8,0xe9),hex(addr)
  return (addr+5+struct.unpack_from('<i',ins,1)[0])&0xffffffff

def archive(path,variant):
 with zipfile.ZipFile(path) as z:
  assert z.testzip() is None,'Bad ZIP CRC'
  d={k:z.read(k) for k in EXPECTED[variant]}
 h={k:hashlib.sha256(v).hexdigest() for k,v in d.items()}
 assert h==EXPECTED[variant],f'{variant} SHA-256 mismatch: {h}'
 return {k:PE(v) for k,v in d.items()},h

# Fixed signatures transcribed from objdump -Mintel output of the known binary build.
# Each is a fixed address and independent, immutable expected byte sequence.
SIG={
 'draw_activity_registration':(0x220e780e,'68 9b 00 00 00 68 1c 82 3d 22'),
 'holster_activity_registration':(0x220e781d,'68 9c 00 00 00 68 0c 82 3d 22'),
 'default_draw_getter':(0x222ac340,'b8 9b 00 00 00 c3'),
 'draw2_lookup':(0x222cb831,'53 68 68 e7 45 22 8b ce e8 02 13 e2 ff'),
 'draw_activity_fallback':(0x222cb89f,'8b ce 5f 5e e9'),
 'shared_deploy_helper':(0x222ac303,'57 8b 3e ff 97 00 04 00 00 50 8b 06 8b ce ff 90 98 03 00 00'),
 'shared_deploy_helper_vcall207':(0x222ac323,'8b 06 6a 00 8b ce ff 90 f8 03 00 00 50 8b ce ff 97 3c 03 00 00'),
 'slot207_eligibility_begin':(0x222aebd0,'56 8b f1 8b 06 ff 90 2c 03 00 00 84 c0 75 22'),
 'slot207_secondary_condition':(0x222aebdf,'8b 16 8b ce ff 92 1c 04 00 00 84 c0 74 14'),
 'slot207_owner_guard':(0x222aec50,'83 bf 8c 00 00 00 00 74 07 5f 32 c0 5e c2 10 00'),
 'slot207_player_context':(0x222aec60,'8b 44 24 18 50 8b cf e8 34 e6 f8 ff'),
 'slot207_viewmodel_setup_first':(0x222aec6c,'8b 16 8b ce ff 92 1c 03 00 00'),
 'slot207_request_activity_second':(0x222aec76,'8b 4c 24 14 8b 06 51 8b ce ff 90 14 03 00 00'),
 'slot207_duration':(0x222aec85,'8b 86 ec 03 00 00 50 8b ce e8 4d 01 e4 ff'),
 'slot207_player_timing':(0x222aec9c,'51 8b cf d9 1c 24 e8 69 1e d9 ff'),
 'slot207_weapon_timing_primary':(0x222aecce,'80 be f0 01 00 00 00 74 0f c6 86 00 02 00 00 01'),
 'slot207_weapon_timing_secondary':(0x222aed25,'80 be f0 01 00 00 00 74 0f c6 86 00 02 00 00 01'),
 'slot207_success':(0x222aed54,'8b 16 6a 01 8b ce ff 92 50 03 00 00'),
 'holster_wrapper_zoom_reset':(0x222cbdb0,'56 8b f1 e8 08 fe ff ff'),
 'holster_wrapper_base_call':(0x222cbdb8,'8b 44 24 08 50 8b ce e8 bc 2f fe ff'),
 'holster_wrapper_success_state':(0x222cbdc8,'b0 01 88 86 6c 05 00 00 8b 0d 90 21 57 22 8b 51 0c 89 96 74 05 00 00'),
 'holster_base_activity_request':(0x222aedac,'8b 16 68 9c 00 00 00 8b ce ff 92 14 03 00 00'),
 'holster_base_activity_compare':(0x222aedbb,'81 be bc 04 00 00 9c 00 00 00'),
 'holster_base_hide_event':(0x222aee59,'8b 44 24 08 6a 00 6a 00 50 68 68 af 45 22'),
 'fallback_from_lowered_branch':(0x222cc437,'8b ce c6 86 6c 05 00 00 00 e8 bb fe fd ff'),
 'rifle_slot209_state_reset':(0x2230ae50,'33 c0 89 81 84 05 00 00 89 81 7c 05 00 00 e9 dd 14 fc ff'),
 'scattergun_slot209_copy':(0x2231bb00,'8b 81 ec 04 00 00 89 81 7c 05 00 00 e9 2f 08 fb ff'),
}
WEAPONS={'CBaseHLCombatWeapon':0x22459194,'CWeaponMagnum':0x2247875c,
'CWeaponAssaultRifle':0x224776ac,'CWeaponScatterGun':0x2247a1e4}

def verify(sin,hl2=None):
 pe,hs=archive(sin,'sin'); s=pe['server.dll'];c=pe['client.dll']
 assert s.base==0x22000000 and c.base==0x24000000
 found=[]
 for label,(addr,pattern) in SIG.items():
  expected=bytes.fromhex(pattern)
  assert s.read(addr,len(expected))==expected, f'{label} bytes mismatch at {addr:#x}'
  found.append({'name':label,'VA':hex(addr),'pattern':pattern})
 assert s.read(0x223d821c,12)==b'ACT_VM_DRAW\x00'
 assert s.read(0x223d820c,15)==b'ACT_VM_HOLSTER\x00'
 assert s.read(0x2245e768,13)==b'ACT_VM_DRAW2\x00'
 assert s.read(0x2245af68,11)==b'HideWeapon\x00'
 # Strings appearing in the module don't prove any particular weapon has corresponding model sequences.
 tables={}
 for name,vt in WEAPONS.items():
  assert s.rtti(vt)==f'.?AV{name}@@'
  expected={197:0x222b07c0,199:0x222ad5e0,207:0x222aebd0,
            209:{'CWeaponAssaultRifle':0x2230ae50,'CWeaponScatterGun':0x2231bb00}.get(name,0x222cc340),
            210:0x222cbdb0,230:0x222cb7f0}
  got={i:s.u32(vt+4*i) for i in expected}
  assert got==expected,f'{name} slot mismatch: {got}'
  tables[name]={'vtable':hex(vt),'slots':{str(i):hex(addr) for i,addr in got.items()}}
 assert s.rtti(0x2244f2b4)=='.?AVCBaseViewModel@@'
 assert s.u32(0x2244f2b4+4*186)==0x222412a0
 # Every checked link below is direct E8/E9 and the named site is instruction-aligned.
 calls={
 0x222cbdb3:0x222cbbc0,   # zoom-out before holster base
 0x222cbdbf:0x222aed80,
 0x222cc440:0x222ac300,   # lowered-state branch falls back to draw/deploy helper
 0x222cb839:0x220ecb40,   # activity lookup by name
 0x222cb8a3:0x222ac340,   # fallback to ACT_VM_DRAW
 0x222aec67:0x2223d2a0,
 0x222aec8e:0x220eede0,   # sequence duration request
 0x222aeca2:0x22040b10,
 0x2230ae5e:0x222cc340,
 0x2231bb0c:0x222cc340,
 }
 for addr,expected in calls.items():
  assert s.rel_call(addr)==expected,f'relative call at {addr:#x} mismatch'
 result={
  'phase':52, 'analysis':'Windows x86 static binary evidence only',
  'original_sha256':hs,'signature_count':len(found),'signatures':found,
  'weapon_vtables':tables,
  'activity_names':{'155':'ACT_VM_DRAW','156':'ACT_VM_HOLSTER','draw2':'ACT_VM_DRAW2 (name lookup, runtime activity ID not asserted)'},
  'verified_direct_links':{hex(x):hex(y) for x,y in calls.items()},
  'evidence_summary':[
   'slot230 returns regular draw or may choose a name-resolved ACT_VM_DRAW2 using state/time/randomness checks',
   '0x222ac300 constructs arguments and calls weapon virtual slot207 (0x222aebd0)',
   'slot207 guarded player path calls slot199 to refresh viewmodel before slot197 to request an activity',
   'slot207 then computes sequence-duration-derived time and updates player/weapon attack time fields',
   'slot210 calls ZoomOut before shared holster, whose activity request is ACT_VM_HOLSTER',
   'slot210 writes fields +0x56C and +0x574 only following nonzero holster result',
   'slot209 for rifle clears +0x584/+0x57C, for Scattergun copies +0x4EC to +0x57C, before shared slot209 logic'],
  'limits':[
   'No original model files, so ACT_VM_DRAW2 presence per weapon and numerical sequence duration remain unknown.',
   'Exact engine-level ordering of inventory selection against these virtual calls is not fully reconstructed.',
   'The alternate draw condition and all engine side effects are not fully modeled by the C++ tests.',
   'No original game or Quest runtime execution tests performed.']}
 if hl2:
  _,result['hl2_sha256']=archive(hl2,'hl2')
 return result

if __name__=='__main__':
 a=argparse.ArgumentParser();a.add_argument('sin',type=Path);a.add_argument('--hl2',type=Path);a.add_argument('--json',type=Path)
 args=a.parse_args();r=verify(args.sin,args.hl2)
 if args.json:args.json.write_text(json.dumps(r,indent=2)+'\n')
 print(f'PASS Phase 52: {r["signature_count"]} anchored signatures, 4 RTTI weapon tables, activity strings, 10 anchored control-flow edges and archive hashes.')
