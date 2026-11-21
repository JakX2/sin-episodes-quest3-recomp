#!/usr/bin/env python3
"""Read-only original 2006 SiN server VoiceMask producer, cross-DLL payload audit.

Original owner DLL bytes are never stored in this source. Uses Phase551's
PE32 reader and CALL verifier to authenticate both the original server sender
and the previously checked client handler. Audit against private bin.zip only.
"""
import argparse
import hashlib
from pathlib import Path
import struct
import zipfile
from audit_sin_original_voice_block_hash import PE32, CLIENT_HASH, SERVER_HASH, check_edge

METHODS = {
 0x22293CF0: (619,'4b9346aca0bdbdf927aa5932929c452c1b10a1021d9973a2bbe8ce25fcceb8c8','server voice recipient update and VoiceMask send'),
 0x22293F60: (31,'6ac092bb3d7ca051f8340b9c10db7f90a75f6d9eb80091a6d1a231bd2ea3e619','server voice periodic threshold helper'),
 0x221E86D0: (68,'3709a3e1d6548cfab75e1c2a036a3983e795036dae5c66542fa1a6520d6c7642','begin named user message'),
 0x221E8720: (25,'6621698cfb136a2fa1a7184901662316a923e84481fc32a525305c00e9760f2d','finish user message'),
 0x221E8740: (40,'e20a47c6e04e4bc52e739107ac04d4dacd26ea7507d160dd97985d438de0a275','message write 8-bit wrapper'),
 0x221E8800: (40,'a6a02d0c8fb9ed8b6b8ac07c79a09b288e9372ff14b09bd24e66634d7fbf0ab8','message write 32-bit wrapper'),
 0x22258EC0: (17,'feb386edf6daf296630273762eb63d35673fcfafa9afabaec4c70816cb180abb','low-level 8-bit bit writer'),
 0x2225A170: (15,'659ae1684a01183326c1249adad18958db0475afc4490097147cc6645292529c','low-level 32-bit bit writer'),
}
CALLS = {
 0x22293D38:0x22286C70, # player lookup
 0x22293D64:0x222701C0, # recipient message filter temporary
 0x22293D76:0x222706B0,
 0x22293DA2:0x221E86D0, # RequestState begin
 0x22293DAA:0x221E8720, # RequestState finish, no payload writes
 0x22293DDF:0x22286C70,
 0x22293EA7:0x221E86D0, # VoiceMask begin
 0x22293EB1:0x221E8800, # first 32-bit value
 0x22293EBD:0x221E8800, # second 32-bit value
 0x22293ECF:0x221E8740, # third 8-bit value
 0x22293ED7:0x221E8720, # VoiceMask end
 0x22293F39:0x222701F0,
 0x22293F77:0x22293CF0, # periodic timer gate -> update
 0x221E86DD:0x22022710, # name -> registered message ID lookup
 0x221E8762:0x22258EC0, # byte wrapper delegates 8-bit writer
 0x221E8822:0x2225A170, # long wrapper delegates 32-bit writer
 0x22258EC9:0x221E74E0, # low-level bit writer 8 bits
 0x2225A177:0x22259B00, # low-level bit writer 32 bits
 0x222CFD17:0x220229A0, # registered VoiceMask, size 9 bytes
 0x222CFD29:0x220229A0, # registered RequestState, size 0 bytes
}
WITNESSES={
 0x22293CF0:'83ec40',             # local recipient masks
 0x22293D10:'396b0c',             # owner player count
 0x22293D30:'8d7d01',             # 1-based recipient player
 0x22293D7D:'83e11f',             # bit offset %32
 0x22293D82:'c1fb05',             # dword index /32
 0x22293D90:'85b388055922',       # request-state mask
 0x22293D9C:'68d06b4522',         # RequestState string
 0x22293DA2:'e82949f5ff',         # message begin RequestState
 0x22293DAA:'e87149f5ff',         # finish request with zero writes
 0x22293DB3:'218b88055922',       # clear request-state bit
 0x22293DC6:'85b300045922',       # current client voice-state mask
 0x22293E19:'8bcf',               # computed mask bit logic
 0x22293E2E:'0910',               # accumulate per-player audible bits
 0x22293E41:'8d3cad00000000',     # dword offset
 0x22293E50:'8bca',               # compare with old mask
 0x22293E58:'7531',               # changed path
 0x22293E8F:'890a',               # publish recomputed word
 0x22293E9B:'68c46b4522',         # VoiceMask literal
 0x22293EA7:'e82448f5ff',         # begin VoiceMask
 0x22293EAC:'8b4c241c',           # 32-bit local computed audible word
 0x22293EB1:'e84a49f5ff',         # first 32-bit writer
 0x22293EB6:'8b9708045922',       # server masked/blocked array
 0x22293EBD:'e83e49f5ff',         # second 32-bit writer
 0x22293EC2:'23b300045922',       # server flag mask
 0x22293EC8:'f7de',               # Boolean normalization
 0x22293ECE:'56',                 # third field
 0x22293ECF:'e86c48f5ff',         # byte writer
 0x22293ED7:'e84448f5ff',         # end VoiceMask
 0x22293F57:'83c440',             # result frame cleanup
 0x22293F60:'dd442404',           # elapsed double argument
 0x22293F67:'dd5110',             # owner timestamp accumulator
 0x22293F6A:'dc1da0a43d22',       # compare exact native 0.3 double
 0x22293F77:'e874fdffff',         # threshold triggers update
 0x221E86D0:'8b0d98b14d22',       # user-message name registry object
 0x221E8704:'56',                 # message ID
 0x221E8706:'ff90ac000000',       # engine BeginMessage virtual +0xac
 0x221E8728:'ff90b0000000',       # engine EndMessage virtual +0xb0
 0x221E872E:'c705f821572200000000', # clear current writer global
 0x221E8740:'833df821572200',     # current writer present?
 0x221E8762:'e859070700',         # low-level bit writer 8
 0x221E8800:'833df821572200',     # current writer present?
 0x221E8822:'e849190700',         # low-level bit writer 32
 0x22258EC4:'6a01',               # unsigned writer mode 1
 0x22258EC6:'6a08',               # 8-bit count
 0x2225A174:'6a20',               # 32-bit count
 0x222CFD10:'6a09',               # VoiceMask fixed 9 bytes
 0x222CFD12:'68c46b4522',         # VoiceMask name pointer
 0x222CFD22:'6a00',               # RequestState fixed zero bytes
 0x222CFD24:'68d06b4522',         # RequestState pointer
}
STRINGS={0x22456BC4:b'VoiceMask',0x22456BD0:b'RequestState',0x22456BE0:b'voice_serverdebug'}
CLIENT_CONSUMER={
 0x241E52B0:(98,'5b6dd5061f8bf30ee046727ffb8a399f341d17a54b58198fcc16f0152fc69d61'),
 0x240A55F0:(8,'a3195220e8ee7a63daeaa179d3081e3f75e76b5e964cc9ce33090534ebadbc2c'),
 0x240A5620:(8,'888dfba4d3941fcd6d2922391ae091bdde162a21986da7d6d4a71b6e4da05a68'),
}

def audit(client:bytes,server:bytes)->dict:
 assert hashlib.sha256(client).hexdigest()==CLIENT_HASH,'wrong client original SHA'
 assert hashlib.sha256(server).hexdigest()==SERVER_HASH,'wrong server original SHA'
 s=PE32(server); c=PE32(client)
 assert s.base==0x22000000 and c.base==0x24000000
 ranges=[]
 for va,(size,digest,label) in METHODS.items():
  assert s.section(va)==b'.text',f'{label}: not text'
  assert hashlib.sha256(s.read(va,size)).hexdigest()==digest,f'wrong original server SHA {label}'
  assert s.read(va+size,1)==b'\xcc',f'{label}: missing expected padding'
  ranges.append((va,va+size))
 ranges.sort()
 assert all(a[1]<=b[0] for a,b in zip(ranges,ranges[1:])),'overlap in counted server methods'
 for va,t in CALLS.items():check_edge(s,va,t,0xE8)
 for va,h in WITNESSES.items():
  expected=bytes.fromhex(h)
  assert s.read(va,len(expected))==expected,f'original server witness {va:#x}'
 assert s.read(0x223DA4A0,8)==struct.pack('<d',0.3), 'native periodic threshold 0.3'
 assert s.read(0x223D8F98,8)==struct.pack('<d',0.0), 'native reset constant'
 for va,v in STRINGS.items():
  assert s.read(va,len(v)+1)==v+b'\0',f'original server string {va:#x}'
 for va,(length,digest) in CLIENT_CONSUMER.items():
  assert hashlib.sha256(c.read(va,length)).hexdigest()==digest,f'original client consumer {va:#x}'
 assert sum(n for n,_,_ in METHODS.values())==855
 return {'new_complete_original_server_functions':len(METHODS),
   'new_original_server_code_bytes':855,'direct_call_edges':len(CALLS),
   'instruction_witnesses':len(WITNESSES),'literal_names':len(STRINGS),
   'original_client_consumer_methods_rechecked':len(CLIENT_CONSUMER),
   'registered_VoiceMask_bytes':9,'registered_RequestState_bytes':0,
   'writer_widths_in_bits':[32,32,8],'periodic_seconds':0.3}

def selftest():
 try:audit(b'MZ fake client',b'MZ fake server')
 except AssertionError:pass
 else:raise AssertionError('fake original SHA accepted')
 class MockPE:
  def __init__(self,a,d):self.a,self.d=a,d
  def read(self,a,n):return self.d[a-self.a:a-self.a+n]
 for src,target in CALLS.items():
  raw=b'\xe8'+struct.pack('<i',target-(src+5));check_edge(MockPE(src,raw),src,target,0xE8)
  for k in range(5):
   bad=bytearray(raw);bad[k]^=1
   try:check_edge(MockPE(src,bad),src,target,0xE8)
   except AssertionError:pass
   else:raise AssertionError('mutated original call undetected')
 return 'SELF_TEST_OK'

def main():
 arg=argparse.ArgumentParser(description=__doc__)
 arg.add_argument('--bin-zip',type=Path);arg.add_argument('--self-test',action='store_true')
 args=arg.parse_args()
 if not(args.bin_zip or args.self_test):arg.error('use --bin-zip and/or --self-test')
 if args.self_test:print(selftest())
 if args.bin_zip:
  with zipfile.ZipFile(args.bin_zip) as z:r=audit(z.read('client.dll'),z.read('server.dll'))
  print('ORIGINAL_SERVER_VOICE_MASK_PRODUCER_AUTHENTICATED',r)
if __name__=='__main__':main()