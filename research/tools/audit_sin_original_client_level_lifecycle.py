#!/usr/bin/env python3
"""Original SiN Episodes 2006 client lifecycle vtable read-only verifier.

Authenticates original DLL hashes, five full original method digests, exact
IBaseClientDLL-style vtable addresses, original rel32 calls, data literals,
and selected x86 instruction witnesses. Does not execute retail code.
"""
from __future__ import annotations
import argparse, hashlib, json, struct, zipfile
from pathlib import Path
from audit_sin_original_client_init_interface_manifest import PE32, CLIENT_SHA, SERVER_SHA

VTABLE=0x24296C6C
# (name, native address, bytes, SHA256) filled from owner-original DLL.
METHODS=(
 ('whole_client_shutdown',0x240FF3D0,204,'4cab24e5b8920392fdcf69020e709da740e06f2963f01ca2b374a6c3755f92d1'),
 ('level_pre_entity',0x240FDB40,192,'28a4824a1a03c8b393e0551448dc062856eeff486cd11391de282e4f09a55511'),
 ('level_post_entity',0x240FDC00,21,'dce3dcce7372b6a641fe7c1bb20a17c1982b3c922ec798c6bcad85b3b4fd34b2'),
 ('level_shutdown',0x240FF4A0,202,'e33da70aa7dd66188da6a46dbca45f6cb80ec06afeaea5005a88d714ee43672a'),
 ('owned_global_cleanup',0x240FC900,27,'71721cdd2db6d9210f6b335faca52209973d792a0aa0b802c0c7306241303e88')
)
# Exact original direct near CALLs inside these five methods.
CALLS=(
 (0x240FF3D4,0x240FC900),
 (0x240FF3E0,0x241DAB30),(0x240FF3F7,0x2418D1C0),(0x240FF40E,0x241ACF50),
 (0x240FF41D,0x241E5050),(0x240FF422,0x241640F0),
 (0x240FF448,0x24053AB0),(0x240FF44D,0x24085220),
 (0x240FF47A,0x241566C0),(0x240FF484,0x24143E80),
 (0x240FF489,0x241EE0C0),(0x240FF48E,0x240FC100),
 (0x240FDB70,0x24053AB0),(0x240FDB97,0x241566D0),(0x240FDB9F,0x24128C50),
 (0x240FDBD8,0x24143240),(0x240FDBF7,0x24143240),
 (0x240FDC00,0x24156730),(0x240FDC05,0x24194F90),
 (0x240FF4BA,0x240C4570),(0x240FF4C2,0x24156740),
 (0x240FF4C7,0x241941A0),(0x240FF4DD,0x241061E0),
 (0x240FF4FD,0x24156750),(0x240FF51D,0x2417F750),
 (0x240FF527,0x241432C0)
)
# (original code VA, opcode hex, independently described behavior)
WITNESSES=(
 (0x240FF3D0,'56','whole shutdown prologue'),
 (0x240FF3D9,'a170f42f24','read three-request proxy global'),
 (0x240FF3EC,'ff5628','first proxy virtual +0x28 dispatch'),
 (0x240FF403,'ff5628','second proxy virtual +0x28 dispatch'),
 (0x240FF41A,'ff5628','third proxy virtual +0x28 dispatch'),
 (0x240FF42F,'ff5018','global virtual +0x18 before +0x10'),
 (0x240FF43A,'ff5210','global virtual +0x10 after +0x18'),
 (0x240FF45D,'8b7710','registered game system count'),
 (0x240FF460,'83ee01','reverse system index decrement'),
 (0x240FF46B,'8b11','retrieve system vptr'),
 (0x240FF46D,'ff5234','registered system virtual +0x34'),
 (0x240FF477,'897710','zero registered system count'),
 (0x240FF494,'8935d0f63324','zero global pointer after teardown'),
 (0x240FDB40,'803d80f6332400','map initialization active flag test'),
 (0x240FDB47,'0f85b0000000','skip map init on already active'),
 (0x240FDB53,'c60580f6332401','set map active flag to 1'),
 (0x240FDB6C,'56','pass map name to engine virtual +8'),
 (0x240FDB9F,'e8acb00200','map init call dependency'),
 (0x240FDBAA,'83791401','prediction conditional count > 1'),
 (0x240FDBB8,'ff9248010000','prediction virtual +0x148'),
 (0x240FDBC0,'751f','virtual nonzero goes to command at b0 cl_predict 0'),
 (0x240FDBCA,'68c0682924','original cl_predict 1 command'),
 (0x240FDBE9,'68b0682924','original cl_predict 0 command'),
 (0x240FDBFD,'c20400','map init ret 4'),
 (0x240FDC10,'8b01','post-init late vptr'),
 (0x240FDC12,'ff6014','post-init indirect tail JMP +0x14'),
 (0x240FF4A0,'803d80f6332400','map shutdown active flag test'),
 (0x240FF4AA,'0f84b8000000','map shutdown inactive skip'),
 (0x240FF4B3,'c60580f6332400','map shutdown clears active before callbacks'),
 (0x240FF4D4,'ff500c','map shutdown global virtual +0x0c'),
 (0x240FF4E2,'8b35f8f63324','map shutdown linked list head'),
 (0x240FF4F4,'ff12','map shutdown linked callback vslot zero'),
 (0x240FF534,'ff5014','map shutdown global virtual +0x14'),
 (0x240FF53F,'ff5220','map shutdown global virtual +0x20'),
 (0x240FF542,'8b7710','map shutdown system count'),
 (0x240FF558,'ff5234','map shutdown system callback +0x34'),
 (0x240FF560,'c7471000000000','map shutdown reset system count'),
 (0x240FC900,'8b0df4f33324','owned global pointer read'),
 (0x240FC90C,'6a01','owned global virtual destruction flag 1'),
 (0x240FC90E,'ff10','owned global virtual byte slot +0'),
 (0x240FC910,'c705f4f3332400000000','owned global zero after callback')
)
class AuditError(ValueError):pass
def need(cond,reason):
 if not cond: raise AuditError(reason)
def check_call(read,site,target):
 v=read(site,5)
 need(v[:1]==b'\xe8' and site+5+struct.unpack_from('<i',v,1)[0]==target,
      f'original rel32 call changed {site:#x}->{target:#x}')
def check_jump(read,site,target,opcode):
 v=read(site,6)
 need(v[:2]==opcode and site+6+struct.unpack_from('<i',v,2)[0]==target,
      'original rel32 branch changed '+hex(site))
def verify(server,client):
 need(hashlib.sha256(server).hexdigest()==SERVER_SHA,'non-original server SHA')
 need(hashlib.sha256(client).hexdigest()==CLIENT_SHA,'non-original client SHA')
 pe=PE32(client)
 need(pe.base==0x24000000,'unrecognized client PE32 base')
 need(pe.u32(VTABLE)==0x240FE0E0 and pe.section(VTABLE)=='.rdata',
      'original VClient011 vtable base changed')
 # Original MSVC RTTI is .?AVCHLClient@@, not guessed from Source SDK.
 need(pe.u32(VTABLE-4)==0x242E5818, 'CHLClient RTTI COL pointer changed')
 need(pe.u32(0x242E5818+12)==0x242FA618, 'CHLClient RTTI type descriptor changed')
 need(pe.at(0x242FA618+8,len(b'.?AVCHLClient@@'))==b'.?AVCHLClient@@', 'CHLClient RTTI name changed')
 # +0 original Init; +4/+8/+12/+16 native lifecycle entries.
 for i,(name,va,n,sha) in enumerate(METHODS):
  need(pe.section(va)=='.text',name+' not in original .text')
  need(hashlib.sha256(pe.at(va,n)).hexdigest()==sha,name+' method digest changed')
  if i<4:need(pe.u32(VTABLE+(i+1)*4)==va, 'original lifecycle vtable entry changed: '+name)
  else:need(pe.at(va+n,1)==b'\xcc','owned global destructor boundary changed')
 # +8 level pre-entity has NO padding before +12 level-post entry.
 need(pe.at(0x240FDC00,1)==b'\xe8' and pe.u32(VTABLE+0x0c)==0x240FDC00,
      'original adjacent level lifecycle method boundary changed')
 for name,va,n,sha in METHODS[:1]+METHODS[2:4]:
  need(pe.at(va+n,1)==b'\xcc',name+' original INT3 boundary changed')
 for site,target in CALLS:check_call(pe.at,site,target)
 check_jump(pe.at,0x240FDB47,0x240FDBFD,b'\x0f\x85')
 check_jump(pe.at,0x240FF4AA,0x240FF568,b'\x0f\x84')
 for at,raw,label in WITNESSES:need(pe.at(at,len(bytes.fromhex(raw)))==bytes.fromhex(raw), 'original witness changed: '+label)
 for va,word in ((0x242968B0,b'cl_predict 0\0'),(0x242968C0,b'cl_predict 1\0')):
  need(pe.at(va,len(word))==word,'prediction command literal changed')
 return {'status':'ORIGINAL_CLIENT_VTABLE_LIFECYCLE_AUTHENTICATED',
  'original_client_sha256':CLIENT_SHA,'new_verified_methods':len(METHODS),
  'new_native_bytes':sum(x[2] for x in METHODS),
  'original_direct_calls':len(CALLS),'opcode_witnesses':len(WITNESSES),
  'original_msvc_type_descriptor':'CHLClient','vtable':hex(VTABLE),'slots':[{'byte_offset':hex(4*(i+1)),'VA':hex(x[1]),'role':x[0]} for i,x in enumerate(METHODS[:4])],
  'same_state_flag':hex(0x2433F680),'native_prediction_commands':['cl_predict 0','cl_predict 1'],
  'limitation':'Exact original machine-code evidence and bounded C-like flow; not full translated game DLL.'}
def self_test():
 need(sum(x[2] for x in METHODS)==646 and len(CALLS)==26,'method/edge inventory drift')
 at,tar=0x24008000,0x24012345
 good=b'\xe8'+struct.pack('<i',tar-at-5)
 check_call(lambda _,n:good[:n],at,tar)
 for i in range(5):
  mutated=bytearray(good);mutated[i]^=1
  try:check_call(lambda _,n:bytes(mutated[:n]),at,tar)
  except AuditError:pass
  else:raise AssertionError('changed original rel32 accepted')
 for opcode,dest in ((b'\x0f\x84',0x240080aa),(b'\x0f\x85',0x240080bb)):
  good=opcode+struct.pack('<i',dest-at-6)
  check_jump(lambda _,n:good[:n],at,dest,opcode)
  for i in range(6):
   bad=bytearray(good);bad[i]^=1
   try:check_jump(lambda _,n:bytes(bad[:n]),at,dest,opcode)
   except AuditError:pass
   else:raise AssertionError('changed branch accepted')
 try:verify(b'fake',b'fake')
 except AuditError:pass
 else:raise AssertionError('fake original hashes accepted')
 print('PASS synthetic mutation and provenance refusal for VClient011 original lifecycle')
def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('--self-test',action='store_true')
 p.add_argument('--bin-zip',type=Path)
 a=p.parse_args()
 if not a.self_test and not a.bin_zip:p.error('select --self-test and/or --bin-zip')
 if a.self_test:self_test()
 if a.bin_zip:
  with zipfile.ZipFile(a.bin_zip) as z:
   names={Path(n).name.lower():n for n in z.namelist()}
   need('client.dll' in names and 'server.dll' in names,'owner original DLL pair missing')
   print(json.dumps(verify(z.read(names['server.dll']),z.read(names['client.dll'])),indent=2))
if __name__=='__main__':main()