#!/usr/bin/env python3
"""SHA-pinned 2006 client.dll VoiceMask 32+32+8 bit-reader decompilation.

The full original handler was already counted in Phase552. This phase counts
only newly authenticated low-level read helpers and verifies three original
handler callsites against the unchanged private 2006 client/server pair.
"""
import argparse
import hashlib
from pathlib import Path
import struct
import zipfile
from audit_sin_original_voice_block_hash import PE32, CLIENT_HASH, SERVER_HASH, check_edge

METHODS = {
 0x240A4760: (118,'1e35e98014bd1194e61f7304aea1064290c9e380acc4945687c0c763745450c2', 'unsigned bit-range reader'),
 0x240A5080: (98,'5d3803e84d3a3f68627ba0cb9a57cfcb4421840e76baee1bc33a4a967e1a9e1a', 'sign-extended bit reader'),
 0x240A55F0: (8,'a3195220e8ee7a63daeaa179d3081e3f75e76b5e964cc9ce33090534ebadbc2c', '8-bit unsigned reader wrapper'),
 0x240A5620: (8,'888dfba4d3941fcd6d2922391ae091bdde162a21986da7d6d4a71b6e4da05a68', '32-bit signed reader wrapper'),
}
CALLS={
 0x240A508D:0x240A4760,
 0x240A55F2:0x240A4760,
 0x240A5622:0x240A5080,
 0x241E52BA:0x240A5620, # two native full 32-bit words
 0x241E52C4:0x240A5620,
 0x241E5305:0x240A55F0, # one original 8-bit word
}
# Witnesses include the already SHA-authenticated original Phase552 handler.
WITNESSES={
 0x240A4760:'8bc1',           # reader owner
 0x240A4762:'8b480c',         # reader bit cursor +12
 0x240A476b:'8b7008',         # total bit count +8
 0x240A4775:'89700c',         # clamp cursor on overflow
 0x240A4779:'c6401001',       # overflow flag byte +16
 0x240A477d:'33c0',           # overflow yields zero
 0x240A4783:'8b30',           # original dword bit storage pointer +0
 0x240A4789:'c1ff05',         # bit cursor /32
 0x240A4791:'83e11f',         # bit cursor within dword
 0x240A4794:'d3ee',           # shift consumed dword
 0x240A4799:'c1f905',         # last-bit dword boundary
 0x240A47AA:'8b04ad28e23224', # original width-to-mask table
 0x240A47B3:'23c6',           # read mask
 0x240A47BA:'83e21f',         # split dword carry
 0x240A47C4:'234304',         # second dword carry
 0x240A47CB:'d3e0',           # combine partial value
 0x240A47CD:'0bc6',           # accumulated unsigned value
 0x240A47D3:'c20400',         # width one argument
 0x240A5087:'83c3ff',         # signed width minus 1
 0x240A508A:'53',             # request width-1 unsigned bits
 0x240A508D:'e8cef6ffff',     # original unsigned bit-reader call
 0x240A5092:'8b560c',         # sign-bit cursor
 0x240A50B4:'8b0e',           # underlying dword buffer
 0x240A50CB:'897e0c',         # advance one more sign bit
 0x240A50D3:'83caff',         # signed sign extension if set
 0x240A50D8:'d3e2',           # sign bit mask
 0x240A50DA:'03c2',           # signed completion
 0x240A50DF:'c20400',         # width argument
 0x240A55F0:'6a08',           # original 8-bit unsigned wrapper
 0x240A55F2:'e869f1ffff',     # direct to unsigned reader
 0x240A55F7:'c3',             # wrapper ret
 0x240A5620:'6a20',           # original 32-bit signed wrapper
 0x240A5622:'e859faffff',     # direct to signed reader
 0x240A5627:'c3',             # wrapper ret
 0x241E52BA:'e86103ecff',     # first 32-bit mask
 0x241E52C1:'894614',         # store audible mask at +0x14
 0x241E52C4:'e85703ecff',     # second 32-bit mask
 0x241E52C9:'89461c',         # store server-banned mask +0x1C
 0x241E5305:'e8e602ecff',     # 8-bit status
 0x241E530A:'894608',         # 8-bit status as 32-bit value
 0x241E530F:'c20400',         # one message reader pointer
}
NATIVE_NAMES={
 0x242A9F50:b'CVoiceStatus::HandleVoiceMaskMsg\n',
 0x242A9F2C:b'    - m_AudiblePlayers[%d] = %lu\n',
 0x242A9F04:b'    - m_ServerBannedPlayers[%d] = %lu\n',
 0x242AA144:b'VoiceMask',
}

def audit(client:bytes,server:bytes)->dict:
 assert hashlib.sha256(client).hexdigest()==CLIENT_HASH, 'wrong client SHA'
 assert hashlib.sha256(server).hexdigest()==SERVER_HASH, 'wrong server SHA'
 pe=PE32(client); assert pe.base==0x24000000
 ranges=[]
 for va,(size,digest,name) in METHODS.items():
  assert pe.section(va)==b'.text',f'{name}: not in text'
  assert hashlib.sha256(pe.read(va,size)).hexdigest()==digest,f'{name}: method SHA mismatch'
  assert pe.read(va+size,1)==b'\xcc',f'{name}: method boundary invalid'
  ranges.append((va,va+size))
 ranges.sort(); assert all(a[1]<=b[0] for a,b in zip(ranges,ranges[1:])), 'overlapping methods'
 for va,target in CALLS.items():check_edge(pe,va,target,0xE8)
 for va,h in WITNESSES.items():
  expected=bytes.fromhex(h)
  assert pe.read(va,len(expected))==expected, f'wrong opcode at {va:#x}'
 for va,s in NATIVE_NAMES.items():
  assert pe.read(va,len(s)+1)==s+b'\0',f'wrong original literal {va:#x}'
 assert sum(n for n,_,_ in METHODS.values())==232
 return {'new_complete_original_methods':len(METHODS),'new_original_code_bytes':232,
         'direct_call_edges':len(CALLS),'instruction_witnesses':len(WITNESSES),
         'native_message_strings':len(NATIVE_NAMES),'voice_mask_payload_bits':72,
         'fields_in_wire_order':['signed_bits32','signed_bits32','unsigned_bits8']}

def selftest():
 try:audit(b'MZ synthetic client',b'MZ synthetic server')
 except AssertionError:pass
 else:raise AssertionError('fake original SHA accepted')
 class FakePE:
  def __init__(self,addr,data):self.addr,self.data=addr,data
  def read(self,addr,n):return self.data[addr-self.addr:addr-self.addr+n]
 for a,t in CALLS.items():
  original=b'\xe8'+struct.pack('<i',t-(a+5))
  check_edge(FakePE(a,original),a,t,0xE8)
  for i in range(5):
   corrupt=bytearray(original);corrupt[i]^=1
   try:check_edge(FakePE(a,bytes(corrupt)),a,t,0xE8)
   except AssertionError:pass
   else:raise AssertionError(f'undetected mutated call byte {a:#x}+{i}')
 return 'SELF_TEST_OK'

def main():
 ap=argparse.ArgumentParser(description=__doc__)
 ap.add_argument('--bin-zip',type=Path)
 ap.add_argument('--self-test',action='store_true')
 opt=ap.parse_args()
 if not(opt.bin_zip or opt.self_test):ap.error('use --self-test and/or --bin-zip')
 if opt.self_test:print(selftest())
 if opt.bin_zip:
  with zipfile.ZipFile(opt.bin_zip) as z:r=audit(z.read('client.dll'),z.read('server.dll'))
  print('ORIGINAL_VOICE_MASK_WIRE_AUTHENTICATED',r)
if __name__=='__main__':main()