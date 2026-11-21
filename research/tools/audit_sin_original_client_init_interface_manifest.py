#!/usr/bin/env python3
"""SHA-authenticate original 2006 SiN VClient011::Init interface request order.

Inspects owner-local exact original PE32 DLLs. Reports only original metadata,
never raw commercial bytes. Does not execute original code or claim startup.
"""
from __future__ import annotations
import argparse, hashlib, json, struct, zipfile
from pathlib import Path

SERVER_SHA='568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650'
CLIENT_SHA='6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d'
INIT_VA=0x240FE0E0
INIT_LEN=0x4C4
INIT_SHA='14d81449866220aaef1f5f906b577179ea107df78a424a364c715261adbb35db'
VTABLE=0x24296C6C
FAIL=0x240FE59E
# (original query site, name C-string pointer, original global destination, name)
REQUIRED=(
 (0x240FE109,0x24296AEC,0x2433F684,'VEngineClient012'),
 (0x240FE127,0x24296ADC,0x2433F688,'VEngineModel012'),
 (0x240FE140,0x24296AC8,0x2433F68C,'VEngineEffects001'),
 (0x240FE159,0x24296AB8,0x2433F690,'VEngineCvar002'),
 (0x240FE172,0x24296AA0,0x2433F6D8,'EngineTraceClient003'),
 (0x240FE18B,0x24296A88,0x2433F694,'VEngineRenderView011'),
 (0x240FE1A4,0x24296A74,0x2433F698,'VDebugOverlay003'),
 (0x240FE1BD,0x24296A60,0x2433F69C,'VMaterialSystem076'),
 (0x240FE1D6,0x24296A50,0x2433F6A8,'VEngineCache001'),
 (0x240FE1EF,0x24296A3C,0x2433F6AC,'VModelInfoClient003'),
 (0x240FE208,0x24296A2C,0x2433F6B0,'VEngineVGui001'),
 (0x240FE221,0x24296A10,0x2433F6B4,'VEngineClientStringTable001'),
 (0x240FE23A,0x242969FC,0x2433F6B8,'SpatialPartition001'),
 (0x240FE253,0x242969E8,0x2433F6C0,'VEngineShadowMgr002'),
 (0x240FE26C,0x242969D0,0x2433F6C4,'StaticPropMgrClient004'),
 (0x240FE285,0x242969B8,0x2433F6C8,'IEngineSoundClient003'),
 (0x240FE29E,0x242D328C,0x2433F6BC,'VFileSystem012'),
 (0x240FE2B7,0x242969A4,0x2433F6CC,'VEngineRandom001'),
 (0x240FE2D0,0x24296984,0x2433F6DC,'VENGINE_GAMEUIFUNCS_VERSION004'),
 (0x240FE2E9,0x2429696C,0x2433F6E0,'GAMEEVENTSMANAGER002'),
 (0x240FE302,0x24296958,0x2433F6E4,'VSoundEmitter001'),
)
# This single query is not followed by a null-fail jump. It must remain
# distinct from the 21 required requests.
OPTIONAL=(0x240FE347,0x24296934,0x2433F6A0,'VMaterialSystemStub001')
# These names are requested through original virtual pointers rather than
# the same passed-in interface factory, and their results are checked.
DERIVED=(
 (0x240FE362,0x24296914,0x2433F6A4,'MaterialSystemHardwareConfig012'),
 (0x240FE3E1,0x24296900,0x2433F6D0,'MatSystemSurface003'),
)

class Refused(ValueError):pass

def require(cond,msg):
 if not cond:raise Refused(msg)

class PE32:
 def __init__(self,raw):
  self.raw=raw
  require(raw[:2]==b'MZ','MZ missing')
  e=struct.unpack_from('<I',raw,0x3C)[0]
  require(raw[e:e+4]==b'PE\0\0','PE missing')
  c=e+4
  require(struct.unpack_from('<H',raw,c)[0]==0x14C,'not Intel i386 PE')
  cnt,sz=struct.unpack_from('<H',raw,c+2)[0],struct.unpack_from('<H',raw,c+16)[0]
  opt=c+20
  require(struct.unpack_from('<H',raw,opt)[0]==0x10B,'not PE32')
  base=struct.unpack_from('<I',raw,opt+28)[0]
  self.sections=[]
  for i in range(cnt):
   p=opt+sz+i*40
   name=raw[p:p+8].split(b'\0')[0].decode('ascii','replace')
   vs,rva,rs,off=struct.unpack_from('<IIII',raw,p+8)
   require(off+rs<=len(raw),'section outside file')
   self.sections.append((name,base+rva,vs,rs,off))
  self.base=base
 def at(self,va,n):
  for name,start,vs,rs,off in self.sections:
   if start<=va and va+n<=start+min(vs,rs):return self.raw[off+va-start:off+va-start+n]
  raise Refused('unmapped original VA '+hex(va))
 def u32(self,va):return struct.unpack('<I',self.at(va,4))[0]
 def section(self,va):
  for name,start,vs,_,_ in self.sections:
   if start<=va<start+vs:return name
  return None

def check_jz_target(pe,site,dest):
 raw=pe.at(site,6)
 require(raw[:2]==b'\x0f\x84' and site+6+struct.unpack_from('<i',raw,2)[0]==dest,
         'fail-closed JE branch mismatch '+hex(site))

def check_ascii(pe,at,label):
 require(pe.at(at,len(label)+1)==label.encode('ascii')+b'\0','name mismatch: '+label)

def verify(server,client):
 require(hashlib.sha256(server).hexdigest()==SERVER_SHA,'server original SHA mismatch')
 require(hashlib.sha256(client).hexdigest()==CLIENT_SHA,'client original SHA mismatch')
 pe=PE32(client)
 require(pe.base==0x24000000,'unexpected original client image base')
 require(pe.section(INIT_VA)=='.text','Init is not in .text')
 require(hashlib.sha256(pe.at(INIT_VA,INIT_LEN)).hexdigest()==INIT_SHA,
         'original complete VClient011 Init method digest changed')
 require(pe.at(INIT_VA+INIT_LEN,1)==b'\xcc','original Init ends before expected INT3 padding')
 require(pe.u32(VTABLE)==INIT_VA and pe.section(VTABLE)=='.rdata',
         'original VClient011 first virtual slot mismatch')
 # First query has a saved-factory-global write between push and call.
 at,ptr,glob,name=REQUIRED[0]
 check_ascii(pe,ptr,name)
 require(pe.at(at,2)==b'\x6a\x00' and pe.at(at+2,1)==b'\x68' and
         pe.u32(at+3)==ptr and pe.at(at+7,1)==b'\xa3' and
         pe.u32(at+8)==0x242FA458 and pe.at(at+12,2)==b'\xff\xd6',
         'first engine CreateInterface request contract changed')
 require(pe.at(0x240FE11C,1)==b'\xa3' and pe.u32(0x240FE11D)==glob,
         'first interface pointer store changed')
 check_jz_target(pe,0x240FE121,FAIL)
 # Remaining twenty queries share a distinct, exact 25-byte pattern.
 for i,(at,ptr,glob,name) in enumerate(REQUIRED[1:],2):
  check_ascii(pe,ptr,name)
  raw=pe.at(at,25)
  require(raw[:3]==b'\x6a\x00\x68' and struct.unpack_from('<I',raw,3)[0]==ptr,
          f'#{i} name push mismatch')
  require(raw[7:14]==b'\xff\xd6\x83\xc4\x08\x85\xc0',
          f'#{i} factory call and null check mismatch')
  require(raw[14]==0xA3 and struct.unpack_from('<I',raw,15)[0]==glob,
          f'#{i} global destination changed')
  check_jz_target(pe,at+19,FAIL)
 at,ptr,glob,name=OPTIONAL
 check_ascii(pe,ptr,name)
 require(pe.at(at,7)==b'\x6a\x00\x68'+struct.pack('<I',ptr),
         'optional VMaterialSystemStub001 query missing')
 require(pe.at(0x240FE35B,5)==b'\xa3'+struct.pack('<I',glob),
         'optional material stub global write missing')
 for at,ptr,glob,name in DERIVED:
  check_ascii(pe,ptr,name)
  require(pe.at(at,5)==b'\x68'+struct.pack('<I',ptr),
          'derived interface name push missing '+name)
  # These exact validated global stores are inspected independently below.
 require(pe.at(0x240FE36C,5)==b'\xa3'+struct.pack('<I',DERIVED[0][2]),
         'derived material configuration global changed')
 check_jz_target(pe,0x240FE371,FAIL)
 require(pe.at(0x240FE3EB,5)==b'\xa3'+struct.pack('<I',DERIVED[1][2]),
         'material surface global changed')
 check_jz_target(pe,0x240FE3F0,FAIL)
 require(pe.at(0x240FE595,9)==b'\xb8\x01\x00\x00\x00\x5e\xc2\x0c\x00',
         'original success return changed')
 require(pe.at(FAIL,6)==b'\x33\xc0\x5e\xc2\x0c\x00',
         'original failure return changed')
 return {
  'status':'ORIGINAL_2006_VCLIENT011_INIT_INTERFACE_ORDER_AUTHENTICATED',
  'client_method_va':hex(INIT_VA),
  'client_method_bytes':INIT_LEN,
  'client_method_sha256':INIT_SHA,
  'original_vtable_first_slot':hex(VTABLE),
  'required_engine_factory_queries':len(REQUIRED),
  'optional_engine_factory_queries':len((OPTIONAL,)),
  'derived_interface_queries_requiring_non_null':len(DERIVED),
  'common_failure_return':hex(FAIL),
  'initial_interface_names':[x[3] for x in REQUIRED],
  'optional_interface_name':OPTIONAL[3],
  'derived_interface_names':[x[3] for x in DERIVED],
  'limits':'Complete function bytes verified; only startup interface and return-gate segment decompiled, not every subsequent initializer/call.'
 }

def self_test():
 assert len(REQUIRED)==21 and len(DERIVED)==2
 assert len({x[0] for x in REQUIRED})==21
 assert len({x[3] for x in REQUIRED})==21
 site,target=0x12345000,0x12346540
 raw=b'\x0f\x84'+struct.pack('<i',target-site-6)
 class Small:
  def __init__(self,raw):self.raw=raw
  def at(self,va,n):assert va==site;return self.raw[:n]
 check_jz_target(Small(raw),site,target)
 for i in range(6):
  changed=bytearray(raw);changed[i]^=0x01
  try:check_jz_target(Small(bytes(changed)),site,target)
  except Refused:pass
  else:raise AssertionError('altered jz byte accepted '+str(i))
 try:verify(b'fake',b'fake')
 except Refused:pass
 else:raise AssertionError('modified original DLLs accepted')
 print('PASS client Init 21 required-order specs, JE mutation refusal and original SHA gate')

def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('--bin-zip',type=Path,help='owner-private original bin.zip')
 p.add_argument('--self-test',action='store_true')
 a=p.parse_args()
 if not a.bin_zip and not a.self_test:p.error('supply --self-test and/or --bin-zip')
 if a.self_test:self_test()
 if a.bin_zip:
  with zipfile.ZipFile(a.bin_zip) as z:s,c=z.read('server.dll'),z.read('client.dll')
  print(json.dumps(verify(s,c),indent=2,sort_keys=True))

if __name__=='__main__':main()