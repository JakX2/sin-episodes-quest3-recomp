#!/usr/bin/env python3
"""Original 2006 SiN client CVoiceStatus vtable, lifecycle and callback SHA audit.

Depends on the sibling Phase551 read-only PE32 verifier. Original retail DLLs
are private owner inputs, never checked in or redistributed.
"""
import argparse
import hashlib
import struct
import zipfile
from audit_sin_original_voice_block_hash import PE32, CLIENT_HASH, SERVER_HASH, check_edge

# Complete 2006 original client methods; each whole-body SHA has been verified.
METHODS={
 0x241E5050:(27,'b46aa3159ec436d2a49e839506758499c47fd13919dc8de46790e75fe01c58b6','singleton cleanup'),
 0x241E5070:(79,'c23b52c5dc6061d3a1b011cc2156349fabfb16ea4140c084bd74612b84ecb536','CVoiceStatus destructor body'),
 0x241E50F0:(1,'ae3f4619b0413d70d3004b9131c3752153074e45725be13b9a148978895e359e','vtable 8 no-op'),
 0x241E5240:(71,'992e840f91b5204c5040eb7fc61331082371192c4dd099b7507075a4a3928bb7','CVoiceStatus constructor'),
 0x241E5290:(30,'f5fa2ed765b07194937ec896864b8cb8bcf98276a164063393dd3a26fab062b8','deleting destructor wrapper'),
 0x241E5C00:(47,'aeeddef32d803938f3edbbb3bb022bbc3f2f0b0cada06bfd1ed6f4bb93f42e0c','lazy voice singleton factory'),
 0x241E5C30:(48,'8d70a352bdc6edd025a7586d33dbb7691dc3d17f1fe5dd480b18b1b5189ee820','registered RequestState callback relay'),
 0x241E5C60:(150,'fef610655478bc1259d0657447c73c845faa98440a40909c1e71a5fb98dc70d7','vtable 4 VGUI and command initialization'),
 0x241E5D00:(34,'33c2ff01ef064e8d4e3dffd5847bf8b2d3721e5b908233153e6150ffec02614f','one-second voice status tick gate'),
 0x241E5370:(21,'a71f0350df22aaf4588c13071c7805011d4ec9f4cda37ee143f185a32ca60907','registered VoiceMask callback relay'),
 0x241E52B0:(98,'5b6dd5061f8bf30ee046727ffb8a399f341d17a54b58198fcc16f0152fc69d61','registered voice mask setup handler'),
 0x241E51B0:(70,'73d1e226ed8a9911a557a4eed32a2ad0ae39dd3fb8c5cdfa8f024a524e652085','player identity hash presence query'),
 0x241E5D30:(73,'b82b3fe8d1b4abb93c0c100e9eda3805749b9b44fd52a250eccd51a772851115','VGUI texture panel show/hide dispatch'),
}
CALLS={
 0x241E50B1:0x241E4D00,
 0x241E525B:0x241E4EB0,
 0x241E5293:0x241E5070,
 0x241E52A0:0x2416FD60,
 0x241E5C0E:0x2416FCF0,
 0x241E5C1C:0x241E5240,
 0x241E5C59:0x241E5880,
 0x241E5C79:0x241E4F90,
 0x241E5CD3:0x241CFF70,
 0x241E5CE8:0x241CFF70,
 0x241E5D1A:0x241E5880,
 0x241E537F:0x241E52B0,
 0x241E52BA:0x240A5620,
 0x241E52C4:0x240A5620,
 0x241E5305:0x240A55F0,
 0x241E51E7:0x241E4F00,
}
JUMPS={0x241E50BA:0x241E4F80}
WITNESSES={
 0x241E5050:'8b0d58863724', # singleton global
 0x241E505C:'6a01', # deleting virtual call literal 1
 0x241E5060:'c7055886372400000000', # global nulling
 0x241E5073:'8b8e28280000', # pointer at +0x2828
 0x241E507B:'c706f89e2a24', # original vptr restore
 0x241E5085:'ff5034', # texture close/release virtual +0x34
 0x241E5090:'c7055c86372400000000', # clear active singleton pointer
 0x241E50AD:'50',
 0x241E50B6:'8d4e28', # hash embedded +0x28
 0x241E50F0:'c3', # literal no-op
 0x241E5244:'c706f89e2a24', # CVoiceStatus vptr
 0x241E524F:'895e14', # initial zero first mask
 0x241E5255:'8d4e28', # hash table +0x28
 0x241E5260:'899e30280000', # +0x2830 zero
 0x241E5266:'889e2c280000', # +0x282c flag false
 0x241E5275:'899e28280000', # +0x2828 zero
 0x241E527B:'c74608ffffffff', # original -1 flag
 0x241E5298:'f644240801', # delete flag
 0x241E5C00:'a158863724', # own global pointer
 0x241E5C09:'6834280000', # total 0x2834 bytes
 0x241E5C21:'a358863724', # store ctor result
 0x241E5C30:'a15c863724', # active global pointer
 0x241E5C55:'6a01', # force network refresh
 0x241E5C63:'8b0d84f63324', # engine global
 0x241E5C6B:'ff9098000000', # virtual +0x98
 0x241E5C76:'8d4e28', # hash subobject
 0x241E5C7E:'c6862c28000001', # has-loaded-hash flag
 0x241E5C91:'680c5b2924', # VGUI textures string
 0x241E5C96:'6850a12a24', # voice icon name
 0x241E5C9B:'89355c863724', # publish active instance
 0x241E5CA4:'898628280000', # icon handle pointer
 0x241E5CAE:'ff5230', # icon handle virtual +0x30
 0x241E5CBE:'c6462400', # callback state flag zero
 0x241E5CC2:'894620', # +0x20 argument
 0x241E5CC5:'894e0c', # +0x0c argument
 0x241E5CCE:'6844a12a24', # VoiceMask name
 0x241E5CDE:'68305c1e24', # RequestState callback pointer
 0x241E5CE3:'6834a12a24', # RequestState name
 0x241E5CED:'b801000000', # return true
 0x241E5CF3:'c20800', # ret 8
 0x241E5D00:'a158a42f24', # engine globals current time pointer
 0x241E5D05:'d9400c', # current time +0x0C
 0x241E5D08:'d86104', # minus this+0x04
 0x241E5D0B:'d81d08e32d24', # compare 1.0f
 0x241E5D18:'6a00', # unforced update
 0x241E5370:'8b0d5c863724', # active singleton
 0x241E537F:'e82cffffff', # setup callback
 0x241E51CE:'ff5020', # player info virtual +0x20
 0x241E51DF:'8d442428', # 32B key slice at player info+0x24
 0x241E51E4:'8d4e28', # hash embedded
 0x241E5D55:'ff90b0000000', # GUI state virtual +0xB0
 0x241E5D69:'ff5230', # GUI show
 0x241E5D72:'ff5034', # GUI hide
}
STRING_WITNESSES={
 0x242AA150:b'voice/icntlk_pl',
 0x242AA144:b'VoiceMask',
 0x242AA134:b'RequestState',
 0x24295B0C:b'VGUI textures',
 0x242AA070:b'CVoiceStatus::HandleReqStateMsg\n',
}
EXPECTED_TYPE=b'.?AVCVoiceStatus@@\0'

def audit(client,server):
 if hashlib.sha256(client).hexdigest()!=CLIENT_HASH or hashlib.sha256(server).hexdigest()!=SERVER_HASH:
  raise AssertionError('wrong original DLL pair')
 pe=PE32(client)
 if pe.base!=0x24000000:raise AssertionError('wrong client image base')
 ranges=[]
 for addr,(count,expected,role) in METHODS.items():
  assert pe.section(addr)==b'.text',f'{role}: not in .text'
  assert hashlib.sha256(pe.read(addr,count)).hexdigest()==expected, f'{role}: full function SHA mismatch'
  if addr==0x241E5C30: # 48-byte function directly abuts initializer, no CC padding
   assert pe.read(addr+count,2)==b'\x56\x8b'
  else:
   assert pe.read(addr+count,1)==b'\xcc', f'{role}: no expected end padding'
  ranges.append((addr,addr+count))
 ranges.sort()
 assert all(a[1]<=b[0] for a,b in zip(ranges,ranges[1:])), 'counted methods overlap'
 for addr,target in CALLS.items():check_edge(pe,addr,target,0xE8)
 for addr,target in JUMPS.items():check_edge(pe,addr,target,0xE9)
 for addr,h in WITNESSES.items():
  assert pe.read(addr,len(bytes.fromhex(h)))==bytes.fromhex(h),f'offset witness {addr:#x} mismatch'
 for addr,s in STRING_WITNESSES.items():
  assert pe.read(addr,len(s)+1)==s+b'\0',f'original text {addr:#x} mismatch'
 # Direct proof of MSVC 32-bit Complete Object Locator -> TypeDescriptor.
 vtbl=0x242A9EF8
 assert pe.read(vtbl,12)==struct.pack('<III',0x241E5290,0x241E5C60,0x241E50F0)
 assert struct.unpack('<I',pe.read(vtbl-4,4))[0]==0x242EA63C
 col=struct.unpack('<5I',pe.read(0x242EA63C,20))
 assert col[0:3]==(0,0,0) and col[3]==0x2431D244
 assert pe.read(0x2431D244+8,len(EXPECTED_TYPE))==EXPECTED_TYPE
 assert pe.read(0x242DE308,4)==struct.pack('<f',1.0)
 count=sum(x[0] for x in METHODS.values());assert count==749
 return {'full_original_methods':len(METHODS),'unique_original_code_bytes':count,
         'direct_calls':len(CALLS),'direct_jumps':len(JUMPS),
         'native_instruction_witnesses':len(WITNESSES),
         'original_string_literals':len(STRING_WITNESSES),'rtti':'CVoiceStatus'}

def selftest():
 from audit_sin_original_voice_block_hash import selftest as parent_test
 assert parent_test()=='SELF_TEST_OK'
 assert sum(x[0] for x in METHODS.values())==749
 try:audit(b'MZ synthetic client',b'MZ synthetic server')
 except AssertionError: pass
 else:raise AssertionError('synthetic file incorrectly accepted')
 return 'SELF_TEST_OK'

def main():
 p=argparse.ArgumentParser();p.add_argument('--bin-zip');p.add_argument('--self-test',action='store_true');a=p.parse_args()
 if a.self_test:print(selftest())
 if a.bin_zip:
  with zipfile.ZipFile(a.bin_zip) as z:r=audit(z.read('client.dll'),z.read('server.dll'))
  print('ORIGINAL_CVOICE_STATUS_LIFECYCLE_AUTHENTICATED',r)
 if not (a.bin_zip or a.self_test):p.error('use --self-test or --bin-zip')
if __name__=='__main__':main()