#!/usr/bin/env python3
"""Phase 51 static verifier: proof chain from zoomed activity to viewmodel configuration.
User-supplied originals are inputs only. Does not ship or run proprietary DLLs.
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
  self.data=data; p=struct.unpack_from('<I',data,0x3c)[0];assert data[p:p+4]==b'PE\0\0'
  n=struct.unpack_from('<H',data,p+6)[0]; opt=struct.unpack_from('<H',data,p+20)[0]
  self.base=struct.unpack_from('<I',data,p+52)[0];self.sections=[]
  for i in range(n):
   q=p+24+opt+40*i;name=data[q:q+8].split(b'\x00')[0].decode('ascii')
   vs,rva,rs,ptr=struct.unpack_from('<IIII',data,q+8)
   self.sections.append((name,self.base+rva,rs,ptr))
 def read(self,va,n):
  for name,start,size,offset in self.sections:
   if start<=va and va+n<=start+size:return self.data[offset+va-start:offset+va-start+n]
  raise ValueError(f'VA not mapped: {va:08x} len {n}')
 def u32(self,va):return struct.unpack('<I',self.read(va,4))[0]
 def rtti(self,vtable):
  locator=self.u32(vtable-4);typeinfo=self.u32(locator+12)
  return self.read(typeinfo+8,150).split(b'\0')[0].decode('ascii')
 def near_calls(self,target):
  hits=[]
  for name,start,size,offset in self.sections:
   if name!='.text': continue
   data=self.data[offset:offset+size]
   for i in range(len(data)-4):
    if data[i]==0xe8 and (start+i+5+struct.unpack_from('<i',data,i+1)[0])&0xffffffff==target:
     hits.append(start+i)
  return hits

def get_arch(path,variant):
 with zipfile.ZipFile(path) as z:
  bad=z.testzip();assert bad is None,f'ZIP CRC failed: {bad}'
  contents={x:z.read(x) for x in EXPECTED[variant]}
 hashes={k:hashlib.sha256(b).hexdigest() for k,b in contents.items()}
 assert hashes==EXPECTED[variant],f'Unexpected {variant} DLL SHA256: {hashes}'
 return {k:PE(v) for k,v in contents.items()},hashes

# Hand-transcribed fixed byte ranges anchored at original VAs; never derive expected bytes from supplied binary.
SIGNATURES={
 'server.dll':{
  'zoom_requests_activity_C0':(0x222cbb9e,'8b 06 68 c0 00 00 00 8b ce ff 90 14 03 00 00'),
  'zoom_exit_condition_and_idle':(0x222cbbf5,'81 be bc 04 00 00 c0 00 00 00 5f 75 0f 8b 16 68 9d 00 00 00 8b ce ff 92 14 03 00 00'),
  'slot197_jump_to_dispatcher':(0x222b07c0,'e9 4b f4 ff ff'),
  'animation_lookup_fail_guard':(0x222afc10,'56 57 8b 7c 24 0c 57 8b f1 e8 b2 ef e3 ff 83 f8 ff'),
  'animation_lookup_fail_return':(0x222afc1e,'83 f8 ff 75 07 5f 32 c0 5e c2 04 00'),
  'animation_records_activity_sequence':(0x222afc35,'8b ce 89 be c4 04 00 00 89 86 c0 04 00 00'),
  'animation_vcall_slot198':(0x222afcc2,'50 8b 16 8b ce ff 92 18 03 00 00'),
  'slot198_header_and_negative_guard':(0x222ad470,'53 8b 5c 24 08 85 db 57 8b f9 7c 75'),
  'slot198_owner_handle':(0x222ad47c,'8b 87 94 04 00 00 83 f8 ff'),
  'slot198_vcall_slot199':(0x222ad4db,'8b 07 8b cf ff 90 1c 03 00 00'),
  'slot198_viewmodel_sequence':(0x222ad4e5,'8b 16 53 8b ce ff 92 e0 02 00 00'),
  'slot199_header':(0x222ad5e0,'57 8b f9 8b 87 94 04 00 00 83 f8 ff'),
  'slot199_owner_player_check':(0x222ad615,'8b 01 56 ff 90 fc 00 00 00 8b f0 85 f6 74 61'),
  'slot199_player_viewmodel_index':(0x222ad646,'8b 87 98 04 00 00 8b 17 53 8b 1e 57 50 8b cf'),
  'slot199_weapon_model_getter':(0x222ad655,'ff 92 f8 03 00 00 50 8b ce ff 93 e4 02 00 00'),
  'slot199_script_triplet_transfer':(0x222ad664,'66 8b 87 1c 05 00 00 8b 1e 50 e8 5d 85 00 00 83 c4 04 05 cc 06 00 00 50 8b ce ff 93 e8 02 00 00'),
  'weapon_model_getter_from_script':(0x222abed0,'66 8b 81 1c 05 00 00 50 e8 f3 9c 00 00 83 c4 04 05 a6 00 00 00'),
  'viewmodel_bind_weapon_and_model':(0x222412b0,'56 57 8b 7c 24 10 85 ff 8b f1'),
  'viewmodel_weapon_handle_field':(0x222412d3,'8d 54 24 10 52 8d 8e c8 04 00 00'),
  'viewmodel_model_change_guard':(0x222412f7,'3b 86 a8 04 00 00 74 25'),
  'viewmodel_model_assignment_call':(0x2224130e,'8b 16 50 8b ce ff 52 6c'),
  'viewmodel_script_vector_thunk':(0x222412a0,'81 c1 b0 04 00 00 e9 f5 fb ff ff'),
  'live_vector_compare':(0x22240ea0,'d9 01 8b 54 24 04 d9 02 da e9 df e0 f6 c4 44 7a 1e'),
  'live_vector_network_dirty':(0x22240ecf,'80 b9 40 fd ff ff 00 74 09 c6 81 50 fd ff ff 01'),
  'live_vector_copy':(0x22240eee,'8b 02 89 01 8b 42 04 89 41 04 8b 52 08 89 51 08'),
  'activity_duration_route_setup':(0x222ad56b,'8b 07 8b cf ff 90 1c 03 00 00'),
  'activity_missing_sequence_early_exit':(0x222afc23,'5f 32 c0 5e c2 04 00'),
 },
 'client.dll':{
  'viewmodel_origin_recv':(0x2409cd3c,'6a 0c 68 a8 07 00 00 68 88 5d 28 24'),
  'client_script_offset_copy':(0x2413f72c,'8d 8a cc 06 00 00 8d b0 cc 06 00 00 8b 39 89 3e')
 }
}
WEAPONS={'CBaseHLCombatWeapon':0x22459194,'CWeaponMagnum':0x2247875c,
 'CWeaponAssaultRifle':0x224776ac,'CWeaponScatterGun':0x2247a1e4}

def verify(sin,hl2=None):
 p,h=get_arch(sin,'sin');server,client=p['server.dll'],p['client.dll']
 assert (server.base,client.base)==(0x22000000,0x24000000)
 matched=[]
 for dll,items in SIGNATURES.items():
  for name,(addr,pattern) in items.items():
   exp=bytes.fromhex(pattern); got=p[dll].read(addr,len(exp))
   assert got==exp,f'{dll} {name} mismatch at {addr:#x}: {got.hex(" ")}'
   matched.append({'dll':dll,'name':name,'address':hex(addr),'bytes':pattern})
 tables={}
 for name,base in WEAPONS.items():
  assert server.rtti(base)==f'.?AV{name}@@'
  got={n:server.u32(base+4*n) for n in (197,198,199,254)}
  assert got=={197:0x222b07c0,198:0x222ad470,199:0x222ad5e0,254:0x222abed0}
  tables[name]={'vtable':hex(base),**{f'slot_{k}':hex(v) for k,v in got.items()}}
 vt=0x2244f2b4;assert server.rtti(vt)=='.?AVCBaseViewModel@@'
 assert {n:server.u32(vt+n*4) for n in (184,185,186)}=={
  184:0x22241330,185:0x222412b0,186:0x222412a0}
 # Direct E8 sites are not in themselves proof of all indirect callers. Validate known routes only.
 assert server.near_calls(0x222ad470)==[]
 assert server.near_calls(0x222ad5e0)==[]
 assert server.near_calls(0x222afc10)==[] # invoked via virtual-method trampoline
 assert sorted(server.near_calls(0x222ad590))==sorted([0x222aeea8,0x222adb78,0x2231592f])
 assert 0x222ad66e in server.near_calls(0x222b5bd0)
 assert 0x222ad65e not in server.near_calls(0x222412b0) # dispatched via slot185
 assert 0x222ad67e not in server.near_calls(0x222412a0) # dispatched via slot186
 result={
  'phase':51,'scope':'Static x86 DLL reconstruction, user-provided binaries; no game runtime simulation',
  'original_sha256':h,'signatures_count':len(matched),'signatures':matched,'weapon_vtables':tables,
  'viewmodel_vtable':{'address':hex(vt),'class':'CBaseViewModel','slot_184':'0x22241330','slot_185':'0x222412b0','slot_186':'0x222412a0'},
  'conditional_chain':[
   'ADS Enter 0x222cbb60 requests activity 0xC0 via weapon slot 197',
   'weapon slot197 0x222b07c0 jumps to 0x222afc10, which returns early for missing sequence (index -1)',
   'valid activity path calls weapon slot198 at 0x222afcc7 with sequence id',
   'weapon slot198 0x222ad470 requires nonnegative sequence, valid weapon-owner player and viewmodel',
   'slot198 calls slot199 0x222ad5e0 before viewmodel slot184 to send sequence',
   'slot199 validates owner/player/viewmodel and uses weapon slot254 0x222abed0 to obtain script model pointer at info+0xA6',
   'slot199 invokes viewmodel slot185 0x222412b0 to associate weapon and configure model',
   'slot199 gets weapon info+0x6CC and passes address to viewmodel slot186 0x222412a0, which forwards to change-aware setter 0x22240ea0'],
  'caveats':['This path occurs on successful animation requests, not necessarily only on deploy or equip.',
   'No proof that ADS changes the script vector values; the same vector is applied on valid animations.',
   'Sequence names, weapon model content, actual script offsets and the precise runtime frequency remain unknown.',
   'No full SourceVR/ARM64 build or in-game execution has been done.'],
  'near_calls_direct':{'slot198':[], 'slot199':[]},
 }
 if hl2:
  _,hh=get_arch(hl2,'hl2');result['hl2_sha256']=hh
 return result

def main():
 a=argparse.ArgumentParser();a.add_argument('sin',type=Path);a.add_argument('--hl2',type=Path);a.add_argument('--json',type=Path)
 args=a.parse_args();res=verify(args.sin,args.hl2)
 if args.json:args.json.write_text(json.dumps(res,indent=2)+'\n')
 print(f'PASS phase 51: {res["signatures_count"]} anchored signatures, 4 weapon RTTI/vtables, viewmodel RTTI/vtable, call-graph edges, supplied binary hashes')
if __name__=='__main__':main()
