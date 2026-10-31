#!/usr/bin/env python3
"""Static, fixed-address verification of SiN Episodes' viewmodel offset paths.

Expects original USER-SUPPLIED archives. Outputs no proprietary DLLs.
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
  self.data=data
  p=struct.unpack_from('<I',data,0x3c)[0]
  assert data[p:p+4]==b'PE\x00\x00'
  n=struct.unpack_from('<H',data,p+6)[0]
  s=struct.unpack_from('<H',data,p+20)[0]
  self.base=struct.unpack_from('<I',data,p+52)[0]
  self.sects=[]
  for i in range(n):
   q=p+24+s+i*40
   name=data[q:q+8].split(b'\0')[0].decode('ascii')
   virtual_size,rva,raw_size,raw_offset=struct.unpack_from('<IIII',data,q+8)
   self.sects.append((name,self.base+rva,raw_size,raw_offset))
 def read(self,va,n):
  for name,b,size,off in self.sects:
   if b<=va and va+n<=b+size: return self.data[off+va-b:off+va-b+n]
  raise AssertionError(f'VA {va:x} missing')
 def u32(self,va): return struct.unpack('<I',self.read(va,4))[0]
 def strings(self,word):
  needle=word.encode('ascii')+b'\0'; found=[]
  for name,b,n,off in self.sects:
   if name not in ('.rdata','.data'): continue
   blob=self.data[off:off+n];pos=0
   while (i:=blob.find(needle,pos))!=-1:
    # Must begin at a literal boundary, not inside 'm_viewmodelOriginOffset'.
    if i==0 or blob[i-1]==0:found.append(b+i)
    pos=i+1
  return found
 def direct_calls_to(self, target):
  # All near E8 rel32 operands (independent of alignment); useful as a scan, NOT a proof of absence of indirect calls.
  hits=[]
  for name,va,n,off in self.sects:
   if name!='.text':continue
   text=self.data[off:off+n]
   for i in range(len(text)-5):
    if text[i]!=0xe8:continue
    displacement=struct.unpack_from('<i',text,i+1)[0]
    if (va+i+5+displacement)&0xffffffff==target:hits.append(va+i)
  return hits

def load(path,kind):
 with zipfile.ZipFile(path) as z:
  assert z.testzip() is None,'ZIP corrupt'
  assert set(EXPECTED[kind]).issubset(z.namelist())
  bs={n:z.read(n) for n in EXPECTED[kind]}
 hashes={n:hashlib.sha256(b).hexdigest() for n,b in bs.items()}
 assert hashes==EXPECTED[kind],f'{kind} hashes mismatch {hashes}'
 return {n:PE(b) for n,b in bs.items()},hashes

# Machine-code bytes from observed 2006 user-provided DLLs. Never derived dynamically from input.
SIG = {
 'server.dll':{
  'weapon_script_key_lookup':(0x222b4827,'68 c8 c6 45 22 68 58 c6 45 22 8b cd e8 c8 17 fb ff'),
  'weapon_script_xyz_targets':(0x222b4838,'8d 93 d4 06 00 00 52 8d 8b d0 06 00 00 51 8d 93 cc 06 00 00 52'),
  'weapon_script_float3_scan':(0x222b484d,'68 44 4a 3d 22 50 e8 7e 14 0f 00'),
  'weapon_info_init_xyz':(0x222b4e82,'89 9a cc 06 00 00 89 9a d0 06 00 00 89 9a d4 06 00 00'),
  'viewmodel_registered_offset':(0x22240caa,'6a 0c 68 b0 04 00 00 68 10 f2 44 22'),
  'viewmodel_setter_head':(0x22241420,'d9 44 24 04 ba 01 00 00 00 d9 01 da e9 df e0'),
  'viewmodel_dirty_bit_for_x':(0x22241434,'80 b9 40 fd ff ff 00 74 08 88 91 50 fd ff ff eb 0c'),
  'viewmodel_setter_z':(0x2224149b,'80 b9 40 fd ff ff 00 74 10 88 91 50 fd ff ff'),
  'viewmodel_constructor_zero_call':(0x222415ca,'53 53 53 8d 8e b0 04 00 00 e8 48 fe ff ff'),
 },
 'client.dll':{
  'weapon_script_key_lookup':(0x241e69b7,'68 58 a4 2a 24 68 e8 a3 2a 24 8b cd e8 f8 fd f7 ff'),
  'weapon_script_xyz_targets':(0x241e69c8,'8d 93 d4 06 00 00 52 8d 8b d0 06 00 00 51 8d 93 cc 06 00 00 52'),
  'weapon_script_float3_scan':(0x241e69dd,'68 ec 50 28 24 50 e8 1e 2f 06 00'),
  'weapon_info_init_xyz':(0x241e6fe2,'89 9a cc 06 00 00 89 9a d0 06 00 00 89 9a d4 06 00 00'),
  'viewmodel_recv_offset':(0x2409cd3c,'6a 0c 68 a8 07 00 00 68 88 5d 28 24'),
  'client_vector_change_test':(0x2409d1b0,'d9 81 a8 07 00 00 8b 54 24 04 d9 02 da e9'),
  'client_vector_copy':(0x2409d1e9,'8b 02 89 81 a8 07 00 00 8b 42 04 89 81 ac 07 00 00 8b 52 08 89 91 b0 07 00 00'),
  'position_offset_component_z':(0x2409c9be,'d9 86 b0 07 00 00'),
  'position_offset_component_x':(0x2409c9ff,'d9 86 a8 07 00 00'),
  'position_offset_component_y':(0x2409ca3d,'d9 86 ac 07 00 00'),
 }
}

def verify(sin_path,hl2_path=None):
 bins,hashes=load(sin_path,'sin')
 count=0;sigout={}
 for n,signatures in SIG.items():
  for label,(va,hexstr) in signatures.items():
   expected=bytes.fromhex(hexstr); actual=bins[n].read(va,len(expected))
   assert actual==expected,f'{n} {label}: expected {hexstr}, got {actual.hex(" ")}'
   sigout[f'{n}:{label}']={'address':hex(va),'bytes':hexstr}
   count+=1
 s,c=bins['server.dll'],bins['client.dll']
 assert s.base==0x22000000 and c.base==0x24000000
 assert s.strings('viewmodelOriginOffset')==[0x2245c658]
 assert c.strings('viewmodelOriginOffset')==[0x242aa3e8]
 assert s.strings('m_viewmodelOriginOffset')==[0x2244f210]
 assert c.strings('m_viewmodelOriginOffset')==[0x24285d88]
 assert s.read(0x2245c6c8,6)==b'0 0 0\0'
 assert c.read(0x242aa458,6)==b'0 0 0\0'
 assert s.read(0x223d4a44,9)==b'%f %f %f\0'
 assert c.read(0x242850ec,9)==b'%f %f %f\0'
 assert s.u32(0x2244f2b0)==0x224c7dec # CBaseViewModel RTTI locator
 assert s.read(0x224efb88,20).startswith(b'.?AVCBaseViewModel@@')
 assert c.read(0x242f6eb4,21).startswith(b'.?AVC_BaseViewModel@@')
 assert c.u32(0x24285df4+200*4)==0x2409c970
 direct_calls=s.direct_calls_to(0x22241420)
 # Scan result is evidence of direct calls, not all potential indirect / external callers.
 assert direct_calls==[0x222415d3],f'unexpected direct calls: {direct_calls}'
 compare=None
 if hl2_path:
  hb,hh=load(hl2_path,'hl2')
  absence={n:{word:len(pe.strings(word))==0 for word in ['viewmodelOriginOffset','m_viewmodelOriginOffset','ZoomFOV']} for n,pe in hb.items()}
  assert all(all(v.values()) for v in absence.values())
  compare={'sha256':hh,'literal_absence_in_supplied_hl2':absence}
 return {
  'phase':49,'method':'static Windows x86 original SiN DLL verification, no runtime test',
  'original_sha256':hashes,'verified_fixed_machine_code_signatures':count,
  'signatures':sigout,
  'script_property':{'key':'viewmodelOriginOffset','default':'0 0 0','parse_format':'%f %f %f',
     'configuration_offsets':['0x6cc','0x6d0','0x6d4'],'server_parse':'0x222b4827', 'client_parse':'0x241e69b7'},
  'networked_viewmodel_field':{'name':'m_viewmodelOriginOffset','server_registration_offset':'0x4b0','client_registration_offset':'0x7a8',
    'size_bytes':12,'server_setter':'0x22241420','server_constructor':'0x22241570',
    'client_vector_setter':'0x2409d1b0','client_viewmodel_position':'0x2409c970'},
  'direct_near_calls_to_server_setter':[hex(x) for x in direct_calls],
  'boundaries':['The parsed weapon-info vector and live viewmodel entity vector are DIFFERENT objects; no direct copy path established.',
     'No proof the weapon script keys are populated by the released game assets; actual values remain unknown.',
     'Static near-call scan does not exclude indirect calls, inlined writes or external engine code.',
     'No proof ADS modifies either origin-offset vector. Do not narrow headset FOV.'],
  'provided_hl2_comparison':compare
 }

if __name__=='__main__':
 parser=argparse.ArgumentParser(description=__doc__)
 parser.add_argument('sin_archive',type=Path)
 parser.add_argument('--hl2',type=Path)
 parser.add_argument('--json',type=Path)
 a=parser.parse_args(); r=verify(a.sin_archive,a.hl2)
 if a.json:a.json.write_text(json.dumps(r,indent=2)+'\n')
 print(f'PASS: Phase 49, {r["verified_fixed_machine_code_signatures"]} fixed signatures; expected RTTI, script strings, offsets, caller scan and SHA-256 all match')
