#!/usr/bin/env python3
"""Original 2006 SiN client VGUI/HUD startup call path authentication.

Only metadata and source-level instruction witnesses. Owner original DLLs
must be provided privately. No commercial DLL bytes are emitted.
"""
from __future__ import annotations
import argparse, hashlib, json, struct, zipfile
from pathlib import Path
from audit_sin_original_client_init_interface_manifest import (
    PE32, SERVER_SHA, CLIENT_SHA, INIT_VA, INIT_LEN, INIT_SHA,
)

METHODS=(
 (0x241EDFB0,123,'64cbfa079f093b1e577a59721396d73742eb4543b836e7fb0cc55d724783417d','vgui_client_bootstrap'),
 (0x24009340,101,'a0851e6f97bc087734723394de37dfb925db41c1764087f4c9e94ede1a1b384e','hud_handle_named_registration'),
 (0x2422ECD0,1,'ae3f4619b0413d70d3004b9131c3752153074e45725be13b9a148978895e359e','vgui_noop'),
 (0x2422ECF0,14,'d1f9d41fb9e6c8628fa934c2ac6a848a9aae39e604b44f7b4f1ddab5ceed29b3','vgui_check_wrapped'),
)
# Query block start, name pointer, original store VA, original name.
QUERIES=(
 (0x241EDFCB,0x242AC50C,0x24378AB8,'VGUI_InputInternal001'),
 (0x241EDFE2,0x242D32C0,0x24378ABC,'VGUI_System010'),
)
class Refused(ValueError): pass

def require(ok,why):
 if not ok: raise Refused(why)

def relcall(pe,site,target):
 v=pe.at(site,5)
 require(v[0]==0xE8 and site+5+struct.unpack_from('<i',v,1)[0]==target,
         f'rel32 call changed {site:#x}->{target:#x}')

def shortje(pe,site,target):
 b=pe.at(site,2)
 require(b[0]==0x74 and site+2+struct.unpack('<b',b[1:2])[0]==target,
         f'short JE changed at {site:#x}')

def verify(server,client):
 require(hashlib.sha256(server).hexdigest()==SERVER_SHA,'wrong server.dll hash')
 require(hashlib.sha256(client).hexdigest()==CLIENT_SHA,'wrong client.dll hash')
 pe=PE32(client)
 require(pe.base==0x24000000,'wrong client base')
 require(hashlib.sha256(pe.at(INIT_VA,INIT_LEN)).hexdigest()==INIT_SHA,
         'Init full-function hash mismatch')
 for va,n,digest,name in METHODS:
  require(pe.section(va)=='.text','not executable original '+name)
  require(hashlib.sha256(pe.at(va,n)).hexdigest()==digest,'complete function hash differs: '+name)
  require(pe.at(va+n,1)==b'\xcc','complete function boundary differs: '+name)
 relcall(pe,0x240FE3C7,0x241EDFB0)
 # Original parent Init short-circuits to common failure when this helper fails.
 require(pe.at(0x240FE3CC,3)==b'\x83\xc4\x04','Init helper argument cleanup')
 require(pe.at(0x240FE3CF,8)==b'\x84\xc0\x0f\x84\xc7\x01\x00\x00',
         'Init VGUI helper failure branch')
 # The VGUI helper first calls through an imported loader with literal CLIENT.
 require(pe.at(0x242AC524,7)==b'CLIENT\0', 'CLIENT interface context label changed')
 require(pe.at(0x241EDFB7,5)==b'\x68'+struct.pack('<I',0x242AC524),
         'client context literal pointer changed')
 relcall(pe,0x241EDFBC,0x240010D0)
 require(pe.at(0x241EDFC4,4)==b'\x84\xc0\x75\x03',
         'loader failure path changed')
 require(pe.at(0x241EDFC8,3)==b'\x32\xc0\xc3',
         'early false return changed')
 # Both engine queries share return-if-null, storing exact globals.
 for site,nptr,globalptr,name in QUERIES:
  require(pe.at(nptr,len(name)+1)==name.encode()+b'\0',name+' name changed')
  require(pe.at(site,7)==b'\x6a\x00\x68'+struct.pack('<I',nptr),
          name+' factory argument changed')
  require(pe.at(site+7,4)==b'\xff\x54\x24\x0c',
          name+' indirect factory call changed')
  require(pe.at(site+11,5)==b'\x83\xc4\x08\x85\xc0',
          name+' stack cleanup/null check changed')
  require(pe.at(site+16,5)==b'\xa3'+struct.pack('<I',globalptr),
          name+' original pointer global changed')
  shortje(pe,site+21,0x241EDFC8)
 # Guarded one-time CHudTextureHandle registration: NOT a generic plugin load.
 require(pe.at(0x242AC4F8,len('CHudTextureHandle')+1)==b'CHudTextureHandle\0',
         'HUD registration name mismatch')
 require(pe.at(0x241EDFF9,7)==b'\x80\x3d'+struct.pack('<I',0x24378B28)+b'\x00',
         'one-time HUD registration flag mismatch')
 require(pe.at(0x241EE00C,7)==b'\xc6\x05'+struct.pack('<I',0x24378B28)+b'\x01',
         'HUD registration flag write missing')
 relcall(pe,0x241EE013,0x24009340)
 require(pe.at(0x241EE007,5)==b'\x68'+struct.pack('<I',0x242AC4F8),
         'HUD named registration argument missing')
 # Two VGUI follow-up functions: first is actually a no-op RET.
 relcall(pe,0x241EE01B,0x2422ECD0)
 relcall(pe,0x241EE020,0x2422ECF0)
 require(pe.at(0x2422ECD0,1)==b'\xc3','first follow-up is no longer a no-op')
 require(pe.at(0x2422ECF0,6)==b'\x8b\x0d'+struct.pack('<I',0x2433F6B0),
         'second follow-up provider changed')
 require(pe.at(0x2422ECF8,5)==b'\x6a\x02\xff\x50\x04',
         'second follow-up virtual+4 argument changed')
 require(pe.at(0x241EE025,6)==b'\x85\xc0\x0f\x95\xc0\xc3',
         'VGUI bootstrap nonzero-result Boolean return changed')
 # Named HUD handle registration helper makes these original subcalls.
 relcall(pe,0x2400935A,0x241D4A80)
 relcall(pe,0x24009371,0x2400A4D0)
 relcall(pe,0x2400939B,0x24009F40)
 return {
  'status':'ORIGINAL_CLIENT_VGUI_HUD_BOOTSTRAP_AUTHENTICATED',
  'original_client_sha256':CLIENT_SHA,
  'new_complete_method_count':len(METHODS),
  'new_complete_method_bytes':sum(n for _,n,_,_ in METHODS),
  'original_interface_names':[v[3] for v in QUERIES],
  'original_one_time_hud_registration_label':'CHudTextureHandle',
  'confirmed_noop_function_va':'0x2422ecd0',
  'limits':'No VGUI implementation, complete HUD rendering, rebuilt client startup or game boot.'
 }

def self_test():
 class Fake:
  def __init__(self,at,b):self.addr=at;self.raw=b
  def at(self,a,n):return self.raw[:n]
 a=0x24002000;b=0x24003000
 raw=b'\xe8'+struct.pack('<i',b-a-5)
 relcall(Fake(a,raw),a,b)
 for i in range(5):
  m=bytearray(raw);m[i]^=1
  try:relcall(Fake(a,bytes(m)),a,b)
  except Refused:pass
  else:raise AssertionError('corrupt CALL accepted')
 shortje(Fake(a,b'\x74\x10'),a,a+0x12)
 for raw2 in (b'\x75\x10',b'\x74\x11'):
  try:shortje(Fake(a,raw2),a,a+0x12)
  except Refused:pass
  else:raise AssertionError('corrupt JE accepted')
 try:verify(b'fake',b'fake')
 except Refused:pass
 else:raise AssertionError('untrusted DLL accepted')
 print('PASS original-client VGUI startup synthetic CALL/JE mutations and SHA refusal')

def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('--bin-zip',type=Path)
 p.add_argument('--self-test',action='store_true')
 a=p.parse_args()
 if not a.self_test and a.bin_zip is None:p.error('provide --self-test or --bin-zip')
 if a.self_test:self_test()
 if a.bin_zip:
  with zipfile.ZipFile(a.bin_zip) as z:
   paths={Path(name).name.lower():name for name in z.namelist()}
   require('client.dll' in paths and 'server.dll' in paths,'original pair missing')
   result=verify(z.read(paths['server.dll']),z.read(paths['client.dll']))
  print(json.dumps(result,indent=2))

if __name__=='__main__':main()