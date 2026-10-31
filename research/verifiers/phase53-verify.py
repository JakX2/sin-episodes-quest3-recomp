#!/usr/bin/env python3
"""Reproduce Phase 53 static checks against user's Windows-x86 SiN binaries.
All addresses and bytes are anchored to the SHA-256-identical original build.
"""
import argparse,hashlib,json,struct,zipfile
from pathlib import Path
HASHES={'sin':{'server.dll':'568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650','client.dll':'6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d'},'hl2':{'server.dll':'e1984ba7ea6c344be55a91c3b9914f43550dcac0670e09ad9346f2c2d8f0406d','client.dll':'2095adcf963a3eff8dd575056bf1e1ca840ab67c2b847181e291d0ccac093aae'}}
class PE:
 def __init__(self, data):
  self.data=data;pos=struct.unpack_from('<I',data,0x3c)[0]
  assert data[pos:pos+4]==b'PE\0\0'; n=struct.unpack_from('<H',data,pos+6)[0]
  opt=struct.unpack_from('<H',data,pos+20)[0]; self.base=struct.unpack_from('<I',data,pos+52)[0]
  self.sections=[]
  for i in range(n):
   q=pos+24+opt+40*i
   size,rva,raw,off=struct.unpack_from('<IIII',data,q+8)
   self.sections.append((self.base+rva,raw,off))
 def read(self,va,n):
  for start,size,off in self.sections:
   if start<=va and va+n<=start+size:return self.data[off+va-start:off+va-start+n]
  raise ValueError(f'Address {va:#x} unmapped')
 def u32(self,va):return struct.unpack('<I',self.read(va,4))[0]
 def rtti(self,vt):return self.read(self.u32(self.u32(vt-4)+12)+8,128).split(b'\0')[0].decode()
 def rel(self,addr):
  b=self.read(addr,5)
  assert b[0] in (0xe8,0xe9), f'Not E8/E9 at {addr:#x}'
  return (addr+5+struct.unpack_from('<i',b,1)[0]) & 0xffffffff

def load_archive(path,kind):
 with zipfile.ZipFile(path) as z:
  assert z.testzip() is None,'ZIP CRC error'
  binary={k:z.read(k) for k in HASHES[kind]}
 digests={k:hashlib.sha256(v).hexdigest() for k,v in binary.items()}
 assert digests==HASHES[kind],f'{kind} binary SHA-256 mismatch {digests}'
 return {k:PE(v) for k,v in binary.items()},digests
WEAPONS={'CBaseHLCombatWeapon':0x22459194,'CWeaponMagnum':0x2247875c,'CWeaponAssaultRifle':0x224776ac,'CWeaponScatterGun':0x2247a1e4}
SLOTS={221:0x222af450,222:0x222af660,223:0x222af810,224:0x222af3d0}
LINKS={
 0x222af3ec:0x220ecb40, # dynamically resolves ACT_VM_RELOAD_START
 0x222af3fd:0x220ecb40, # dynamically resolves ACT_VM_RELOAD_END
 0x222af435:0x222af240, # staged selector -> generic reload start
 0x222af5eb:0x220ecb40, # reload-end activity lookup
 0x222af610:0x222af240, # reload-end -> generic reload start
 0x2231400e:0x222af3d0, # rifle override -> shared stage selector
 0x222af383:0x220eede0, # selected reload animation sequence duration
 0x222af39c:0x22040b10, # player attack lockout timing
 0x222af559:0x222aa670, # decrement reserve ammo after single-shell step
 0x222af6e4:0x222aa770, # player reserve ammo query
 0x222af74f:0x222aa670, # reserve deduction on primary transfer
 0x222af7f0:0x222aa670, # reserve deduction on secondary transfer
}
STRINGS={0x223d8154:'ACT_VM_RELOAD',0x2245af88:'ACT_VM_RELOAD_START',0x2245af74:'ACT_VM_RELOAD_END'}

def verify(sin,hl2=None):
 pe,sh=load_archive(sin,'sin');s=pe['server.dll'];c=pe['client.dll']
 assert s.base==0x22000000 and c.base==0x24000000
 for addr,name in STRINGS.items():
  assert s.read(addr,len(name)+1)==name.encode()+b'\0',(addr,name)
 assert s.read(0x220e78b6,10)==bytes.fromhex('68 a6 00 00 00 68 54 81 3d 22')
 sigs={}
 for entry in (Path(__file__).parent/'SIGNATURES.txt').read_text().splitlines():
  address,hexdump=entry.split('|');addr=int(address,16);expected=bytes.fromhex(hexdump)
  assert s.read(addr,len(expected))==expected, f'Instruction signature mismatch {address}'
  sigs[address]=hexdump
 assert len(sigs)==35
 tables={}
 for name,vt in WEAPONS.items():
  assert s.rtti(vt)==f'.?AV{name}@@'
  expected=dict(SLOTS)
  if name=='CWeaponAssaultRifle':expected[224]=0x22314000
  got={i:s.u32(vt+4*i) for i in SLOTS}
  assert got==expected,(name,got,expected)
  tables[name]={'vtable':hex(vt),'slots':{str(k):hex(v) for k,v in got.items()}}
 for site,target in LINKS.items():
  assert s.rel(site)==target,(hex(site),hex(s.rel(site)),hex(target))
 # Check fixed assembler activity lookups & exact primary/secondary clip field references.
 assert s.read(0x222af3d0,2)==b'\x53\x56'
 assert s.read(0x222af3f8,5)==bytes.fromhex('68 74 af 45 22')
 assert s.read(0x222af3dd,5)==bytes.fromhex('68 88 af 45 22')
 assert s.read(0x222af416,5)==bytes.fromhex('b8 a6 00 00 00')
 result={'phase':53,'analysis':'Static x86 disassembly; clean-room model only','hashes':{'sin':sh},
 'signature_count':len(sigs),'signatures':sigs,'activity_names':STRINGS,'vtable_slots':tables,
 'direct_edges':{hex(a):hex(b) for a,b in LINKS.items()},
 'verified':[
 'ACT_VM_RELOAD is activity ID 166 / 0xA6 in server registration',
 'Shared slot 224 dynamically looks up ACT_VM_RELOAD_START and ACT_VM_RELOAD_END with a fallback to activity 166',
 'Shared slot 221 has separate state branches controlled by weapon+0x508; it tests active reload at +0x4AC and +0x4A8',
 'Shared slot 222 computes primary and secondary refill independently against player ammo and clip capacity and applies reserve deduction',
 'Shared slot 223 clears the reload-active field and normalizes reload-stage/timing state',
 'Assault rifle overrides slot 224 and calls shared staged selector, then conditionally restores prior attack deadline'],
 'limits':[
 'No weapon scripts/models: reload-start/end sequence presence and real per-weapon timing unknown',
 'The meaning of +0x508 for each firearm is not conclusively established without runtime/script evidence',
 'Per-shell reload processing and model test harness are deliberately narrower than all possible engine states',
 'No original gameplay execution or Quest 3 runtime tests performed']}
 if hl2:_,result['hashes']['hl2']=load_archive(hl2,'hl2')
 return result
if __name__=='__main__':
 parser=argparse.ArgumentParser();parser.add_argument('sin',type=Path);parser.add_argument('--hl2',type=Path);parser.add_argument('--json',type=Path)
 args=parser.parse_args();r=verify(args.sin,args.hl2)
 if args.json:args.json.write_text(json.dumps(r,indent=2)+'\n')
 print(f'PASS Phase53: {r["signature_count"]} binary signatures, 4 RTTI weapon tables, {len(LINKS)} anchored E8 calls, activity names and SHA-256s')
