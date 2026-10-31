#!/usr/bin/env python3
"""Phase 54 static x86 evidence verifier for exact user-provided DLL archives.
No game binaries are redistributed. Only fixed, reviewed VA signatures are checked.
"""
import argparse,hashlib,json,struct,zipfile
from pathlib import Path
HASHES={
'sin':{'server.dll':'568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650','client.dll':'6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d'},
'hl2':{'server.dll':'e1984ba7ea6c344be55a91c3b9914f43550dcac0670e09ad9346f2c2d8f0406d','client.dll':'2095adcf963a3eff8dd575056bf1e1ca840ab67c2b847181e291d0ccac093aae'}}
class PE:
 def __init__(self,data):
  self.data=data;pos=struct.unpack_from('<I',data,0x3c)[0]
  assert data[pos:pos+4]==b'PE\0\0';count=struct.unpack_from('<H',data,pos+6)[0]
  opt=struct.unpack_from('<H',data,pos+20)[0];self.base=struct.unpack_from('<I',data,pos+52)[0]
  self.sections=[]
  for i in range(count):
   at=pos+24+opt+40*i
   virtual_size,rva,raw_size,raw_ptr=struct.unpack_from('<IIII',data,at+8)
   self.sections.append((self.base+rva,raw_size,raw_ptr))
 def read(self,va,n):
  for start,size,off in self.sections:
   if start<=va and va+n<=start+size:return self.data[off+va-start:off+va-start+n]
  raise AssertionError(f'Unmapped virtual address {va:#x}')
 def u32(self,va):return struct.unpack('<I',self.read(va,4))[0]
 def rtti(self,vt):return self.read(self.u32(self.u32(vt-4)+12)+8,120).split(b'\0')[0].decode()
 def direct(self,va):
  b=self.read(va,5);assert b[0] in (0xe8,0xe9),f'not relative call/jump at {va:#x}'
  return (va+5+struct.unpack('<i',b[1:])[0])&0xffffffff
 def jcc32(self,va):
  b=self.read(va,6);assert b[0]==0x0f and 0x80<=b[1]<=0x8f
  return (va+6+struct.unpack('<i',b[2:])[0])&0xffffffff

def load(path,kind):
 with zipfile.ZipFile(path) as z:
  assert z.testzip() is None
  binaries={k:z.read(k) for k in HASHES[kind]}
 sha={k:hashlib.sha256(b).hexdigest() for k,b in binaries.items()}
 assert sha==HASHES[kind],f'{kind} mismatched SHA256: {sha}'
 return {k:PE(v) for k,v in binaries.items()},sha
WEAPONS={'CBaseHLCombatWeapon':0x22459194,'CWeaponMagnum':0x2247875c,'CWeaponAssaultRifle':0x224776ac,'CWeaponScatterGun':0x2247a1e4}
SLOTS={221:0x222af450,222:0x222af660,223:0x222af810}
# Instruction starts and bytes identified in the original x86 code. The
# report explains why these are material to the conclusions.
SIGNATURES={
 0x222ae390:'53 56 8b f1 e8',
 0x222ae408:'88 9e 08 05 00 00',
 0x223168b3:'e8 d8 7a f9 ff',
 0x223168c0:'88 86 08 05 00 00',
 0x2230b353:'e8 38 30 fa ff',
 0x22313c13:'e8 38 77 ff ff',
 0x2231ba93:'e8 f8 28 f9 ff',
 0x222aef5c:'ff 92 38 04 00 00',
 0x222aef6a:'ff 90 74 03 00 00',
 0x222aed84:'80 be ac 04 00 00 00',
 0x222aed8f:'ff 90 7c 03 00 00',
 0x222aed9d:'c7 86 a8 04 00 00 00 00 00 00',
 0x222aedae:'68 9c 00 00 00',
 0x222cbdb3:'e8 08 fe ff ff',
 0x222cbdbf:'e8 bc 2f fe ff',
 0x222ccd6b:'80 bf ac 04 00 00 00',
 0x222ccd98:'e8 23 ee ff ff',
 0x222cbb60:'56 8b f1 80 be 78 05 00 00 00',
 0x222cbbc0:'56 8b f1 80 be 78 05 00 00 00',
 0x222af457:'38 9e 08 05 00 00',
 0x222af4e7:'f7 87 38 09 00 00 01 08 00 00',
 0x222af4f3:'39 9e ec 04 00 00',
 0x222af4f9:'0f 8f 4f 01 00 00',
 0x222af53a:'8d 4c 24 0c 51 8b cb',
 0x222af559:'e8 12 b1 ff ff',
 0x222af562:'ff 90 80 03 00 00',
 0x222af5a5:'38 9e ac 04 00 00',
 0x222af5d1:'83 be a8 04 00 00 02',
 0x222af5dc:'68 74 af 45 22',
 0x222af5e1:'c7 86 a8 04 00 00 03 00 00 00',
 0x222af64e:'88 9e ac 04 00 00',
 0x222af810:'56 8b f1 8b 06 6a 06',
 0x222af835:'c6 86 ac 04 00 00 00'
}
EDGES={
 0x223168b3:0x222ae390,
 0x2230b353:0x222ae390,
 0x22313c13:0x2230b350,
 0x2231ba93:0x222ae390,
 0x222cbdb3:0x222cbbc0,
 0x222cbdbf:0x222aed80,
 0x222ccd98:0x222cbbc0,
 0x222af559:0x222aa670,
}
JCC={0x222af4f9:0x222af64e}

def verify(sin,hl2=None):
 d,sh=load(sin,'sin');s=d['server.dll'];c=d['client.dll']
 assert s.base==0x22000000 and c.base==0x24000000
 for va,expected in SIGNATURES.items():
  actual=s.read(va,len(bytes.fromhex(expected)))
  assert actual==bytes.fromhex(expected),(hex(va),actual.hex(),expected)
 for va,target in EDGES.items():assert s.direct(va)==target,(hex(va),hex(s.direct(va)),hex(target))
 for va,target in JCC.items():assert s.jcc32(va)==target
 tables={}
 for name,vt in WEAPONS.items():
  assert s.rtti(vt)==f'.?AV{name}@@'
  slots={n:s.u32(vt+n*4) for n in SLOTS}
  assert slots==SLOTS,(name,slots)
  tables[name]={'vtable':hex(vt),'slots':{str(k):hex(v) for k,v in slots.items()}}
 assert s.read(0x222af5dc,5)==bytes.fromhex('68 74 af 45 22')
 assert s.read(0x2245af74,len('ACT_VM_RELOAD_END')+1)==b'ACT_VM_RELOAD_END\0'
 out={'phase':54,'kind':'Static analysis; no game execution','hashes':{'sin':sh},'checked_signatures':len(SIGNATURES),
 'signatures':{hex(k):v for k,v in SIGNATURES.items()},'direct_edges':{hex(k):hex(v) for k,v in EDGES.items()},
 'conditional_edges':{hex(k):hex(v) for k,v in JCC.items()},'rtTI_weapon_vtables':tables,
 'conclusions':[
 'Base constructor stores zero to reload mode byte +0x508',
 'Magnum writes +0x508 from AL=0; rifle constructor chain reaches base zero; ScatterGun constructor chain reaches base zero',
 'Shared weapon frame update invokes slot221 after a clip-eligibility virtual query',
 'Shared holster code conditionally calls slot223 on active reload, resets reload stage, and requests ACT_VM_HOLSTER',
 'SiN zoomed holster wrapper invokes zoom exit before shared holster',
 'Zoom eligibility tests reload-active +0x4AC but zoom-in/out routines have no observed reload cleanup',
 'One-round mode branch checks attack buttons and nonempty clip; can clear reload active before inserting another round'],
 'limits':[
 'All three constructors default +0x508=0; no conclusive proof this mode is later enabled for any named firearm',
 'Model events and precise per-firearm reload timing not recoverable from DLLs alone',
 'No live game or VR build tested']}
 if hl2:_,out['hashes']['hl2']=load(hl2,'hl2')
 return out
if __name__=='__main__':
 ap=argparse.ArgumentParser();ap.add_argument('sin',type=Path);ap.add_argument('--hl2',type=Path);ap.add_argument('--json',type=Path)
 a=ap.parse_args();v=verify(a.sin,a.hl2)
 if a.json:a.json.write_text(json.dumps(v,indent=2)+'\n')
 print(f'PASS Phase54: {v["checked_signatures"]} instruction signatures, {len(EDGES)} direct calls, {len(JCC)} conditional branches, 4 RTTI-verified weapon tables, hashes')
