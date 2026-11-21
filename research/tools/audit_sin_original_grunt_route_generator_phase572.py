#!/usr/bin/env python3
"""Authenticate original 2006 SiN NPC navigation virtual+0x54 candidate builder.
Owner-private original binaries required for --bin-zip; original-free self-test.
"""
import argparse,hashlib,struct,zipfile
from pathlib import Path
from audit_sin_original_grunt_group_phase560 import PE32,CLIENT_HASH,SERVER_HASH,edge
NEW_METHODS={
 0x220774E0:(894,'799f3f619354b5398526851b212f4aa662322f52f86886b2d4cb1b02f6f21e01','candidate route builder, nav virtual +0x54'),
 0x220748B0:(5,'f4c6d7ae520f88aecb3ea65952e885437fa4a6ce4b5c3439a161d1c5d8e42863','nav virtual+0x50 constant true'),
}
CALLS={
 0x220774F4:0x220B2CF0,
 0x22077545:0x2206D9F0,
 0x22077567:0x2206D9F0,
 0x22077581:0x22066270,
 0x2207762F:0x2206D9F0,
 0x22077659:0x2226EE80,
 0x22077742:0x2228B220,
 0x220777BC:0x22071C50,
 0x220777CC:0x2226EE80,
 0x220777E8:0x220B2AB0,
 0x2207784D:0x220B2A10,
}
WITNESSES={
 0x220774E0:'83ec70',0x220774F4:'e8f7b70300',
 0x220774F9:'8b4724',0x220774FC:'8b28',
 0x22077508:'8b4524',0x2207750B:'bb03000000',
 0x22077514:'83f801',0x22077523:'8b4f14',
 0x22077529:'ff5258',0x2207753D:'55',
 0x22077545:'e8a664ffff',0x22077556:'8b4528',
 0x22077567:'e88464ffff',0x22077581:'e8eaecfeff',
 0x2207758F:'d8542414',0x22077595:'f6c441',
 0x220775A4:'dc1de88f3d22',0x220775AC:'f6c441',
 0x220775C6:'dc1dd0e13d22',0x220775E9:'f6c441',
 0x22077612:'83f903',0x22077628:'55',
 0x2207762F:'e8bc63ffff',0x2207763F:'dc05d0e13d22',
 0x22077652:'6a30',0x22077654:'b9f4af5422',
 0x22077659:'e822781f00',0x22077670:'8bf5',
 0x22077674:'f3a5',0x22077678:'c74014000080bf',
 0x2207768E:'8b4820',0x220776A5:'895010',
 0x220776C9:'89442418',0x220776F0:'8b6d28',
 0x22077735:'ff15d0b15122',0x22077742:'e8d93a2100',
 0x22077769:'680000c842',0x22077773:'680b400202',
 0x22077792:'f3ab',0x220777BC:'e88fa4ffff',
 0x220777C5:'6a30',0x220777CC:'e8af761f00',
 0x220777E8:'e8c3b20300',0x220777F1:'3bde',
 0x2207781C:'c744241400000000',
 0x22077824:'d9442414',0x2207782E:'dfe0',
 0x2207783F:'0f846cfdffff',0x2207784D:'e8beb10300',
 0x22077855:'b001',0x2207785B:'c20400',
 0x220748B0:'b001c20400',
 0x220754BC:'ff5254',0x220747B2:'c70654933e22',
}
VTABLE=0x223E9354
CONST_DOUBLE={0x223D8FE8:0.1,0x223DE1D0:0.01}

def audit(client,server):
 assert hashlib.sha256(client).hexdigest()==CLIENT_HASH, "not owner-original client.dll"
 assert hashlib.sha256(server).hexdigest()==SERVER_HASH, "not owner-original server.dll"
 pe=PE32(server);assert pe.base==0x22000000
 for va,(n,hashval,name) in NEW_METHODS.items():
  assert pe.section(va)==b'.text'
  body=pe.read(va,n)
  assert hashlib.sha256(body).hexdigest()==hashval, name
  assert pe.read(va+n,1)==b'\xcc', 'original method boundary'
  assert body[-3:]==b'\xc2\x04\x00', 'native ret4'
 for src,dst in CALLS.items():edge(pe,src,dst)
 for va,h in WITNESSES.items():
  raw=bytes.fromhex(h)
  assert pe.read(va,len(raw))==raw, f'bad opcode witness {va:#x}'
 for slot,target in [(0x50,0x220748B0),(0x54,0x220774E0)]:
  assert struct.unpack('<I',pe.read(VTABLE+slot,4))[0]==target,'wrong original nav vtable'
 for va,val in CONST_DOUBLE.items():
  assert struct.unpack('<d',pe.read(va,8))[0]==val, 'wrong native double threshold'
 assert struct.unpack('<I',pe.read(0x22077769+1,4))[0]==0x42C80000, 'wrong original 100.f'
 return dict(new_complete_functions=len(NEW_METHODS),new_distinct_server_bytes=sum(x[0] for x in NEW_METHODS.values()),
             original_E8_direct_calls=len(CALLS),native_vtable_slots=2,
             original_double_thresholds=list(CONST_DOUBLE.values()),original_opcode_field_witnesses=len(WITNESSES))

def self_test():
 class PE:
  def __init__(self,at,dat):self.at=at;self.dat=dat
  def read(self,addr,n):return self.dat[addr-self.at:addr-self.at+n]
 for src,dst in CALLS.items():
  raw=b'\xe8'+struct.pack('<i',dst-src-5)
  edge(PE(src,raw),src,dst)
  for i in range(5):
   bad=bytearray(raw);bad[i]^=1
   try:edge(PE(src,bad),src,dst)
   except AssertionError:pass
   else:raise AssertionError('mutated call edge accepted')
 try:audit(b'incorrect',b'incorrect')
 except AssertionError:pass
 else:raise AssertionError('incorrect original binary pair accepted')
 return 'PHASE572_SYNTHETIC_OK'

def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('--self-test',action='store_true')
 p.add_argument('--bin-zip',type=Path)
 a=p.parse_args()
 if not (a.self_test or a.bin_zip):p.error('use --self-test and/or --bin-zip')
 if a.self_test:print(self_test())
 if a.bin_zip:
  with zipfile.ZipFile(a.bin_zip) as z:
   print('PHASE572_ORIGINAL_VERIFIED',audit(z.read('client.dll'),z.read('server.dll')))
if __name__=='__main__':main()