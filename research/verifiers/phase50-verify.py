#!/usr/bin/env python3
"""Phase 50: fixed-binary evidence for SiN weapon-script -> live viewmodel transfer.
Only operates on user-provided archives. No game binaries are packaged.
"""
import argparse,hashlib,json,struct,zipfile
from pathlib import Path
EXPECTED={
 'sin': {'server.dll':'568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650',
         'client.dll':'6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d'},
 'hl2': {'server.dll':'e1984ba7ea6c344be55a91c3b9914f43550dcac0670e09ad9346f2c2d8f0406d',
         'client.dll':'2095adcf963a3eff8dd575056bf1e1ca840ab67c2b847181e291d0ccac093aae'}}
class PE:
 def __init__(self,data):
  self.data=data; p=struct.unpack_from('<I',data,0x3c)[0]; assert data[p:p+4]==b'PE\0\0'
  count=struct.unpack_from('<H',data,p+6)[0];size=struct.unpack_from('<H',data,p+20)[0]
  self.base=struct.unpack_from('<I',data,p+52)[0];self.sects=[]
  for i in range(count):
   q=p+24+size+40*i; nm=data[q:q+8].split(b'\0')[0].decode()
   vs,rva,sz,off=struct.unpack_from('<IIII',data,q+8)
   self.sects.append((nm,self.base+rva,sz,off))
 def read(self,va,n):
  for nm,b,sz,off in self.sects:
   if b<=va and va+n<=b+sz:return self.data[off+va-b:off+va-b+n]
  raise AssertionError(f'missing VA {va:#x}')
 def u32(self,va):return struct.unpack('<I',self.read(va,4))[0]
 def rtti_name(self,vt):
  loc=self.u32(vt-4);typ=self.u32(loc+12)
  return self.read(typ+8,150).split(b'\0')[0].decode('ascii')
 def near_calls(self,target):
  out=[]
  for nm,va,sz,off in self.sects:
   if nm!='.text':continue
   dat=self.data[off:off+sz]
   for i in range(len(dat)-4):
    if dat[i]==0xe8:
     rel=struct.unpack_from('<i',dat,i+1)[0]
     if (va+i+5+rel)&0xffffffff==target:out.append(va+i)
  return out

def load(archive,kind):
 with zipfile.ZipFile(archive) as z:
  assert z.testzip() is None,'bad archive CRC'
  raw={n:z.read(n) for n in EXPECTED[kind]}
 hashes={n:hashlib.sha256(x).hexdigest() for n,x in raw.items()}
 assert hashes==EXPECTED[kind],f'{kind} archive contains different binaries: {hashes}'
 return {k:PE(v) for k,v in raw.items()},hashes
# These constants were transcribed from original fixed-address 32-bit disassembly,
# not generated from the current input. A match alone is not proof of calling context.
SIGNATURES={
 'server.dll':{
  'script_parser_key':(0x222b4827,'68 c8 c6 45 22 68 58 c6 45 22 8b cd e8 c8 17 fb ff'),
  'script_parser_triplet':(0x222b4838,'8d 93 d4 06 00 00 52 8d 8b d0 06 00 00 51 8d 93 cc 06 00 00 52'),
  'weapon_method_header':(0x222ad5e0,'57 8b f9 8b 87 94 04 00 00 83 f8 ff'),
  'weapon_owner_handle':(0x222ad5f2,'8b 15 20 56 4f 22 8b c8 81 e1 ff 0f 00 00 c1 e1 04'),
  'weapon_player_resolution':(0x222ad615,'8b 01 56 ff 90 fc 00 00 00 8b f0 85 f6 74 61'),
  'weapon_viewmodel_lookup':(0x222ad646,'8b 87 98 04 00 00 8b 17 53 8b 1e 57 50 8b cf ff 92 f8 03 00 00'),
  'weapon_config_vector_transfer':(0x222ad664,'66 8b 87 1c 05 00 00 8b 1e 50 e8 5d 85 00 00 83 c4 04 05 cc 06 00 00 50 8b ce ff 93 e8 02 00 00'),
  'weapon_config_pointer_and_virtual_call':(0x222ad676,'05 cc 06 00 00 50 8b ce ff 93 e8 02 00 00'),
  'viewmodel_vtable_thunk':(0x222412a0,'81 c1 b0 04 00 00 e9 f5 fb ff ff'),
  'viewmodel_vector_compare':(0x22240ea0,'d9 01 8b 54 24 04 d9 02 da e9 df e0 f6 c4 44 7a 1e'),
  'viewmodel_vector_compare_z':(0x22240ec0,'d9 41 08 d9 42 08 da e9 df e0 f6 c4 44 7b 2f'),
  'viewmodel_dirty_change':(0x22240ecf,'80 b9 40 fd ff ff 00 74 09 c6 81 50 fd ff ff 01'),
  'viewmodel_vector_copy':(0x22240eee,'8b 02 89 01 8b 42 04 89 41 04 8b 52 08 89 51 08 8b c1 c2 04'),
  'weapon_vcall_199_a':(0x222ad4db,'8b 07 8b cf ff 90 1c 03 00 00'),
  'weapon_vcall_199_b':(0x222ad56b,'8b 07 8b cf ff 90 1c 03 00 00'),
  'enter_ads_fov_activity':(0x222cbb86,'8b 16 68 00 00 80 3e 8b ce ff 92 40 04 00 00 50 56 8b cf e8 52 2b f8 ff 8b 06 68 c0 00 00 00'),
 },
 'client.dll':{
  'client_weapon_parser':(0x241e69b7,'68 58 a4 2a 24 68 e8 a3 2a 24 8b cd e8 f8 fd f7 ff'),
  'client_weapon_info_copy':(0x2413f5b0,'8b c1 8b 54 24 04 c7 00 90 af 29 24 0f b6 4a 04'),
  'client_info_zoomfov_copy':(0x2413f6ce,'8b 8a bc 01 00 00 89 88 bc 01 00 00'),
  'client_info_vector_copy':(0x2413f72c,'8d 8a cc 06 00 00 8d b0 cc 06 00 00 8b 39 89 3e 8b 79 04 89 7e 04 8b 49 08 89 4e 08'),
  'client_recv_viewmodel_origin':(0x2409cd3c,'6a 0c 68 a8 07 00 00 68 88 5d 28 24'),
 }}
VTABLES={
 'CBaseHLCombatWeapon':0x22459194,
 'CWeaponMagnum':0x2247875c,
 'CWeaponAssaultRifle':0x224776ac,
 'CWeaponScatterGun':0x2247a1e4}

def verify(sin,hl2=None):
 (sind,sh)=load(sin,'sin');s,c=sind['server.dll'],sind['client.dll'];assert s.base==0x22000000 and c.base==0x24000000
 matches=[]
 for dll,spec in SIGNATURES.items():
  for name,(va,hexd) in spec.items():
   expected=bytes.fromhex(hexd)
   assert sind[dll].read(va,len(expected))==expected,f'{dll} {name} mismatch at {va:#x}'
   matches.append({'dll':dll,'name':name,'va':hex(va),'bytes':hexd})
 gun_slots={}
 for name,vt in VTABLES.items():
  assert s.rtti_name(vt)=='.?AV'+name+'@@'
  gun_slots[name]={'vtable':hex(vt),'slot_199':hex(s.u32(vt+199*4))}
  assert s.u32(vt+199*4)==0x222ad5e0
 assert s.rtti_name(0x2244f2b4)=='.?AVCBaseViewModel@@'
 assert s.u32(0x2244f2b4+186*4)==0x222412a0
 # Direct-caller census distinguishes inlined / virtual transfer from Phase 49's constructor-only scalar setter.
 assert s.near_calls(0x22241420)==[0x222415d3]
 assert s.near_calls(0x222ad5e0)==[]
 assert s.u32(0x2244f2b0)==0x224c7dec
 compare=None
 if hl2:
  (_,hh)=load(hl2,'hl2');compare={'hashes':hh,'note':'No HL2 function equivalence asserted; only archive identity checked.'}
 return {'phase':50,'kind':'Static analysis of user-supplied SiN Windows x86 binaries',
  'original_sha256':sh,'verified_signatures':len(matches),'signatures':matches,
  'weapon_vtables':gun_slots,'viewmodel_vtable':{'address':'0x2244f2b4','class':'CBaseViewModel','slot_186':'0x222412a0'},
  'dataflow':[
    'weapon routine at 0x222ad5e0 resolves player via weapon owner handle +0x494, then player viewmodel by index +0x498',
    'weapon script handle weapon+0x51c -> 0x222b5bd0 retrieves weapon-info object',
    'add eax, 0x6cc passes address of 3-float viewmodelOriginOffset',
    'viewmodel vtable slot 186 -> 0x222412a0 adjusts this to entity+0x4b0 and tail-jumps to 0x22240ea0',
    '0x22240ea0 checks triplet and writes three components, with entity network change marking',
    'client receives network property m_viewmodelOriginOffset at entity+0x7a8, used in client pose computation',
    'client weapon-info copy at 0x2413f5b0 explicitly copies info triplet +0x6cc..+0x6d4'],
  'direct_near_calls_to_weapon_method':[],'direct_near_calls_to_constructor_scalar_setter':['0x222415d3'],
  'bounds':['Does not establish that ADS entry/exit modifies the viewmodel vector.',
   'Does not establish which weapon lifecycle events invoke virtual slot 199 in game.',
   'Does not recover actual shipped weapon-script values, model sequences, or VR sight transforms.',
   'RTTI + instruction signatures and near-call scans are static evidence, not runtime testing.'],
  'provided_hl2':compare}

if __name__=='__main__':
 a=argparse.ArgumentParser();a.add_argument('sin',type=Path);a.add_argument('--hl2',type=Path);a.add_argument('--json',type=Path)
 q=a.parse_args();r=verify(q.sin,q.hl2)
 if q.json:q.json.write_text(json.dumps(r,indent=2)+'\n')
 print('PASS Phase 50:',r['verified_signatures'],'fixed instruction signatures, 4 weapon RTTI/vtables, viewmodel RTTI/slot, xrefs, original SHA256 and optional HL2 archive')
