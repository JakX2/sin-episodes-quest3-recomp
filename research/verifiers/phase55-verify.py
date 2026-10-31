#!/usr/bin/env python3
"""Reproduce Phase55 findings from exact user-supplied SiN DLLs.
Static PE verification only. Not live original-game execution.
"""
import hashlib,struct,json,zipfile,argparse
from pathlib import Path
HASHES={
 'sin':{'server.dll':'568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650', 'client.dll':'6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d'},
 'hl2':{'server.dll':'e1984ba7ea6c344be55a91c3b9914f43550dcac0670e09ad9346f2c2d8f0406d', 'client.dll':'2095adcf963a3eff8dd575056bf1e1ca840ab67c2b847181e291d0ccac093aae'}
}
class PE:
 def __init__(self,raw):
  self.raw=raw
  hdr=struct.unpack_from('<I',raw,0x3c)[0]
  assert raw[hdr:hdr+4]==b'PE\0\0'
  count=struct.unpack_from('<H',raw,hdr+6)[0]
  opt=struct.unpack_from('<H',raw,hdr+20)[0]
  self.base=struct.unpack_from('<I',raw,hdr+52)[0]
  self.sections=[]
  for n in range(count):
   at=hdr+24+opt+40*n
   vsize,rva,rsize,off=struct.unpack_from('<IIII',raw,at+8)
   name=raw[at:at+8].split(b'\0')[0].decode()
   self.sections.append((name,self.base+rva,rsize,off))
 def read(self,va,count):
  for name,start,size,off in self.sections:
   if start<=va and va+count<=start+size:return self.raw[off+va-start:off+va-start+count]
  raise AssertionError(f'Unmapped VA {va:#x}')
 def u32(self,va):return struct.unpack('<I',self.read(va,4))[0]
 def cstr(self,va):return self.read(va,96).split(b'\0')[0].decode()
 def rtti(self,vt):return self.cstr(self.u32(self.u32(vt-4)+12)+8)
 def call(self,va):
  raw=self.read(va,5)
  assert raw[0] in (0xe8,0xe9)
  return (va+5+struct.unpack('<i',raw[1:])[0])&0xffffffff
 def section(self,name):
  for n,s,l,p in self.sections:
   if n==name:return self.raw[p:p+l]
  raise AssertionError(name)

def load(path,name):
 with zipfile.ZipFile(path) as z:
  assert z.testzip() is None
  raw={k:z.read(k) for k in HASHES[name]}
 digests={k:hashlib.sha256(v).hexdigest() for k,v in raw.items()}
 assert digests==HASHES[name],f'{name} digests differ: {digests}'
 return {k:PE(v) for k,v in raw.items()},digests

# Anchored instruction starts: frozen bytes independently checked by objdump.
SERVER={
 0x222ae408:'88 9e 08 05 00 00',
 0x223168b3:'e8 d8 7a f9 ff',
 0x223168c0:'88 86 08 05 00 00',
 0x2230b353:'e8 38 30 fa ff',
 0x22313c13:'e8 38 77 ff ff',
 0x2231ba93:'e8 f8 28 f9 ff',
 0x222b45d3:'6a ff 68 74 c7 45 22',
 0x222b45e7:'6a ff 68 68 c7 45 22',
 0x222b45f0:'89 83 60 01 00 00',
 0x222b45fb:'89 83 64 01 00 00',
 0x222b4608:'68 58 c7 45 22',
 0x222b4621:'68 48 c7 45 22',
 0x222b4739:'e8 62 08 fb ff',
 0x222b474f:'88 83 c8 06 00 00',
 0x222b4e2d:'88 9a c8 06 00 00',
 0x222abf5d:'8b 80 60 01 00 00',
 0x222abf7d:'8b 80 64 01 00 00',
 0x222abffd:'8a 80 c8 06 00 00',
 0x222af094:'80 b8 c8 06 00 00 00',
 0x222af8e6:'80 b8 c8 06 00 00 00',
 0x222af457:'38 9e 08 05 00 00',
 0x222af7f5:'80 be 08 05 00 00 00',
 0x222af4e7:'f7 87 38 09 00 00 01 08 00 00',
 0x222af559:'e8 12 b1 ff ff',
 0x222af74f:'e8 1c af ff ff',
 0x222af7f0:'e8 7b ae ff ff',
 0x222af6b7:'ff 92 38 04 00 00',
 0x222af6d5:'ff 90 04 04 00 00',
 0x222af758:'ff 92 3c 04 00 00',
 0x222af776:'ff 90 08 04 00 00',
 0x222af73f:'89 96 ec 04 00 00',
 0x222af7e0:'89 96 f0 04 00 00',
 0x222af8ef:'c7 86 a8 04 00 00 00 00 00 00',
 0x2231400e:'e8 bd b3 f9 ff',
 0x22317d7e:'88 9e 08 05 00 00',
}
CLIENT={
 0x241e68c9:'e8 b2 ef f7 ff',
 0x241e68df:'88 83 c8 06 00 00',
 0x2413f705:'0f b6 8a c8 06 00 00',
}
VTABLES={
 'CBaseHLCombatWeapon':0x22459194,
 'CWeaponMagnum':0x2247875c,
 'CWeaponAssaultRifle':0x224776ac,
 'CWeaponScatterGun':0x2247a1e4,
}
SHARED_SLOTS={221:0x222af450,222:0x222af660,223:0x222af810,257:0x222abf50,258:0x222abf70,270:0x222abfd0,271:0x222ac010}

KEYS={0x2245c774:'clip_size',0x2245c768:'clip2_size',0x2245c758:'default_clip',0x2245c748:'default_clip2',0x2245c6d0:'MeleeWeapon',0x242aa460:'MeleeWeapon'}
EDGES={0x223168b3:0x222ae390,0x2230b353:0x222ae390,0x22313c13:0x2230b350,0x2231ba93:0x222ae390,0x2231400e:0x222af3d0,0x222af559:0x222aa670,0x222af74f:0x222aa670,0x222af7f0:0x222aa670}

def verify(sin,hl2=None):
 d,sha=load(sin,'sin');s=d['server.dll'];c=d['client.dll']
 assert s.base==0x22000000 and c.base==0x24000000
 for pe,entries in ((s,SERVER),(c,CLIENT)):
  for va,seq in entries.items():
   expected=bytes.fromhex(seq);assert pe.read(va,len(expected))==expected,(hex(va),seq,pe.read(va,len(expected)).hex())
 for va,text in KEYS.items():
  pe=c if va>=0x24000000 else s
  assert pe.cstr(va)==text,(hex(va),pe.cstr(va))
 for a,b in EDGES.items():assert s.call(a)==b,(hex(a),hex(s.call(a)),hex(b))
 tables={}
 for name,vt in VTABLES.items():
  assert s.rtti(vt)==f'.?AV{name}@@', (name,s.rtti(vt))
  slots={key:s.u32(vt+4*key) for key in SHARED_SLOTS}
  assert slots==SHARED_SLOTS,(name,slots)
  stage=s.u32(vt+4*224)
  assert stage==(0x22314000 if name=='CWeaponAssaultRifle' else 0x222af3d0)
  tables[name]={'vtable':hex(vt),'shared_slots':{str(k):hex(v) for k,v in slots.items()},'stage_selector':hex(stage)}
 # The duplicate NPC pistol zero initializer is a scoped corroboration,
 # NOT an exhaustive proof about all possible runtime writes to +0x508.
 assert s.rtti(0x2247924c)=='.?AVCWeaponNPCPistol@@'
 out={'phase':55,'scope':'Static analysis of exact SiN binaries; no runtime game verification',
  'signatures':{'server':len(SERVER),'client':len(CLIENT)},'validated_key_strings':{hex(k):v for k,v in KEYS.items()},
  'checked_call_edges':len(EDGES),'weapon_vtables':tables,'sha256':{'sin':sha},
  'conclusions':['Four clip/default-clip keys mapped to WeaponInfo +0x160/+0x164/+0x168/+0x16c',
                 'MeleeWeapon script flag written at WeaponInfo +0x6c8 on both client and server',
                 'Primary and secondary reload loops call common reserve-removal function and update distinct clip offsets',
                 'All three named firearm constructors inherit one-round mode +0x508 initialized to zero',
                 'All three share slots 221-223, 257-258, 270-271; rifle alone overrides staged selector 224'],
  'limits':['No verified affirmative runtime write to weapon+0x508 for three named firearm classes',
            'No game weapon-script values or model assets in supplied DLL archives',
            'Negative writer search is bounded and cannot exclude indirect writes or save/load',
            'C++ reference-model tests are synthetic, not executed inside original game']}
 if hl2:_,out['sha256']['hl2']=load(hl2,'hl2')
 return out

if __name__=='__main__':
 ap=argparse.ArgumentParser();ap.add_argument('sin',type=Path);ap.add_argument('--hl2',type=Path);ap.add_argument('--json',type=Path)
 a=ap.parse_args();r=verify(a.sin,a.hl2)
 if a.json:a.json.write_text(json.dumps(r,indent=2)+'\n')
 print('Phase55 PASS: %s fixed instruction anchors, %s keys, %s call edges, 4 RTTI vtables, verified hashes'%(sum(r['signatures'].values()),len(KEYS),len(EDGES)))
