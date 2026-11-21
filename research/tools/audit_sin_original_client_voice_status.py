#!/usr/bin/env python3
"""Read-only original 2006 SiN client CVoiceStatus code authentication.

Authenticates four complete original methods, player-mask instructions,
real console/diagnostic literals, and original CALL edges. Does not load
or execute original DLLs or emit proprietary native code.
"""
from __future__ import annotations
import argparse,hashlib,json,struct,zipfile
from pathlib import Path
from audit_sin_original_client_init_interface_manifest import PE32, CLIENT_SHA, SERVER_SHA
METHODS=(
 ('CVoiceStatus_UpdateServerState',0x241E5880,516,'5cba2a0d5d5050b037efaadbf8e4f2d1b3301be419a1a8123a7752d3eacbddbe'),
 ('CVoiceStatus_HandleReqStateMsg',0x241E5A90,41,'c65f98bb8d7dec1aa9f5d3e5a51f22845a4e5550a1063a0bf0aa3b864a7f40bd'),
 ('voice_bitfield_at_plus14',0x241E5AC0,39,'d6c9929c6d2dfe4663f62022a23270004b1873a578397bdd27baf4240ad48bda'),
 ('voice_bitfield_at_plus10',0x241E5AF0,39,'cc8e3aacf17ca10ebfaabc63b34fd1a8b1e7c7b4bdd286819f6aa38610c73a7d'),
)
CALLS=((0x241E5979,0x241E4F00),(0x241E5AB0,0x241E5880))
STRINGS=(
 (0x242AA038,'CVoiceStatus::UpdateServerState: g_bLevelInitialized\n'),
 (0x242AA028,'VModEnable %d'),
 (0x242A9FF8,"CVoiceStatus::UpdateServerState: Sending '%s'\n"),
 (0x242A9FF0,'vban'),
 (0x242A9FEC,' %x'),
 (0x242A9FC0,'CVoiceStatus::UpdateServerState: no change\n'),
 (0x242AA070,'CVoiceStatus::HandleReqStateMsg\n'),
)
WITNESSES=(
 (0x241E5880,'81ec0c0b0000','0xB0C-byte temporary stack'),
 (0x241E5886,'803d80f6332400','read original CHLClient map active byte'),
 (0x241E5891,'7526','active map branch'),
 (0x241E5893,'a1c0863724','logging context global'),
 (0x241E5898,'83782c00','log enabled test'),
 (0x241E58A2,'6838a02a24','literal map-active diagnostic'),
 (0x241E58B6,'c20400','early return one argument'),
 (0x241E58B9,'8b1578863724','mod-state global'),
 (0x241E58C6,'397a2c','read mod-state flag'),
 (0x241E58E1,'894108','store prior VModEnable state this+8'),
 (0x241E58DC,'6828a02a24','literal VModEnable formatter'),
 (0x241E5906,'ff5218','engine virtual byte +0x18 send command'),
 (0x241E5927,'6800080000','vban command assembly buffer 0x800'),
 (0x241E5933,'68f09f2a24','literal vban prefix'),
 (0x241E5950,'83ff20','32 player slots upper bound'),
 (0x241E5966,'ff5220','engine virtual byte +0x20 player-info check'),
 (0x241E5987,'8bce','bit index stored in ecx'),
 (0x241E59A2,'85448a1c','test original this+0x1c mask'),
 (0x241E59B1,'09442418','merge player bit into output flags'),
 (0x241E59BF,'395c2418','compare new and remembered voice masks'),
 (0x241E59CC,'68ec9f2a24','hex formatter string'),
 (0x241E5A68,'ff5218','engine virtual byte +0x18 send voice ban data'),
 (0x241E5A78,'895004','publish engine snapshot at this+4'),
 (0x241E5A81,'c20400','method ret 4'),
 (0x241E5A90,'a1c0863724','handle request state log-global'),
 (0x241E5AAE,'8bce','restore this before forced state update'),
 (0x241E5AB0,'e8cbfdffff','HandleReqStateMsg directly invokes update'),
 (0x241E5AB6,'c20400','handle request ret 4'),
 (0x241E5AC0,'8b542404','first 1-indexed player argument'),
 (0x241E5AC4,'83c2ff','subtract one before bit lookup'),
 (0x241E5ACC,'83e11f','bit position low five bits'),
 (0x241E5AD6,'c1fa05','word index shift five'),
 (0x241E5AD9,'23449614','bitmask array offset +0x14'),
 (0x241E5AE4,'c20400','first accessor ret 4'),
 (0x241E5AF0,'8b542404','second 1-indexed player argument'),
 (0x241E5AF4,'83c2ff','subtract one before bit lookup'),
 (0x241E5AFC,'83e11f','bit position low five bits'),
 (0x241E5B06,'c1fa05','word index shift five'),
 (0x241E5B09,'23449610','bitmask array offset +0x10'),
 (0x241E5B14,'c20400','second accessor ret 4')
)
class AuditError(ValueError):pass
def require(cond,why):
 if not cond:raise AuditError(why)
def check_call(read,site,target):
 v=read(site,5)
 require(v[0]==0xe8 and site+5+struct.unpack_from('<i',v,1)[0]==target,
         'native direct CALL changed '+hex(site))
def verify(server,client):
 require(hashlib.sha256(server).hexdigest()==SERVER_SHA,'server DLL source hash changed')
 require(hashlib.sha256(client).hexdigest()==CLIENT_SHA,'client DLL source hash changed')
 pe=PE32(client)
 require(pe.base==0x24000000,'unexpected client PE32 image base')
 for name,va,n,sha in METHODS:
  require(pe.section(va)=='.text',name+' not executable original client text')
  require(hashlib.sha256(pe.at(va,n)).hexdigest()==sha, name+' complete SHA changed')
  require(pe.at(va+n,1)==b'\xcc',name+' original next instruction boundary changed')
 for va,s in STRINGS:
  b=s.encode('ascii')+b'\0'
  require(pe.at(va,len(b))==b,'native literal changed '+hex(va))
 for site,target in CALLS:check_call(pe.at,site,target)
 for at,pattern,description in WITNESSES:
  pat=bytes.fromhex(pattern)
  require(pe.at(at,len(pat))==pat,'native instruction witness changed: '+description)
 return {'status':'ORIGINAL_SIN_CLIENT_VOICE_STATUS_DECOMP_AUTHENTICATED',
         'original_client_sha256':CLIENT_SHA,
         'new_complete_methods':len(METHODS),
         'new_nonoverlapping_native_bytes':sum(x[2] for x in METHODS),
         'verified_original_direct_calls':len(CALLS),
         'verified_original_literals':len(STRINGS),
         'verified_native_witnesses':len(WITNESSES),
         'map_active_flag':'0x2433f680',
         'player_bit_index':'(player_index - 1) & 0x1f',
         'player_word_index':'(player_index - 1) >> 5',
         'voice_mask_fields':['this+0x10','this+0x14'],
         'limit':'Only source-bounded static program flow. No real voice network runtime.'}
def self_test():
 require(len(METHODS)==4 and sum(x[2] for x in METHODS)==635,'method metadata drift')
 a,b=0x24001000,0x24002000
 good=b'\xe8'+struct.pack('<i',b-a-5)
 check_call(lambda _va,n:good[:n],a,b)
 for i in range(5):
  x=bytearray(good);x[i]^=1
  try:check_call(lambda _va,n:bytes(x[:n]),a,b)
  except AuditError:pass
  else:raise AssertionError('modified original CALL accepted')
 try:verify(b'fake',b'fake')
 except AuditError:pass
 else:raise AssertionError('fake original DLL accepted')
 print('PASS original-free voice status synthetic call mutations and original hash refusal')
def main():
 ap=argparse.ArgumentParser(description=__doc__)
 ap.add_argument('--self-test',action='store_true')
 ap.add_argument('--bin-zip',type=Path)
 args=ap.parse_args()
 if not args.self_test and not args.bin_zip:ap.error('choose --self-test and/or --bin-zip')
 if args.self_test:self_test()
 if args.bin_zip:
  with zipfile.ZipFile(args.bin_zip) as z:
   names={Path(p).name.lower():p for p in z.namelist()}
   require('server.dll' in names and 'client.dll' in names,'original DLL pair missing')
   print(json.dumps(verify(z.read(names['server.dll']),z.read(names['client.dll'])),indent=2))
if __name__=='__main__':main()