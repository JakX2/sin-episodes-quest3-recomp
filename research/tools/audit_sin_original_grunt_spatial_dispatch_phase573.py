#!/usr/bin/env python3
"""Original SiN 2006 native 0x22071C50 waypoint-kind spatial dispatch audit.
Original owner-private binary identity; no proprietary raw assembly export.
"""
import argparse,hashlib,struct,zipfile
from pathlib import Path
from audit_sin_original_grunt_group_phase560 import PE32,CLIENT_HASH,SERVER_HASH,edge
VA=0x22071C50
SIZE=612
SHA='ce5242e9a2834f0ddf6287dd8d32549c32dc22e74a38eb3b6faaca4091fb7aaa'
CALLS={
 0x22071C90:0x2206D9F0,0x22071D52:0x2206E6B0,
 0x22071D8D:0x22071A20,0x22071DA9:0x2206E0D0,
 0x22071E42:0x2206E9E0,0x22071E5E:0x22070560,
 0x22071E72:0x2206EB40,0x22071E82:0x2206D9F0,
}
CALLER=0x220777BC
JT=0x22071EA4
DESTINATIONS=(0x22071CE7,0x22071E49,0x22071E2D,0x22071E65)
WITNESSES={
 0x22071C50:'81ecb4000000',0x22071C65:'85f6',
 0x22071C67:'b90e000000',0x22071C70:'f3ab',
 0x22071C90:'e85bbdffff',0x22071C95:'d95e20',
 0x22071C98:'33c9',0x22071C9A:'894e24',
 0x22071CCD:'8b8424d4000000',0x22071CD7:'83f803',
 0x22071CE0:'ff2485a41e0722',
 0x22071CE7:'8a8424e0000000',
 0x22071CF6:'c744241004000000',
 0x22071D02:'834c241008',0x22071D27:'c6430c01',
 0x22071D49:'ff92c4010000',
 0x22071D52:'e859c9ffff',0x22071D8D:'e88efcffff',
 0x22071D92:'c6430c00',0x22071DA9:'e822c3ffff',
 0x22071DCB:'c706feffffff',
 0x22071E1B:'ff15c4b15122',
 0x22071E2D:'8b8424d8000000',
 0x22071E42:'e899cbffff',
 0x22071E49:'8b9424d8000000',
 0x22071E5E:'e8fde6ffff',
 0x22071E65:'8b8c24d8000000',
 0x22071E72:'e8c9ccffff',
 0x22071E79:'55',0x22071E7C:'c706fcffffff',
 0x22071E82:'e869bbffff',
 0x22071E8D:'33c0',0x22071E8F:'3906',
 0x22071E94:'0f9dc0',0x22071E98:'81c4b4000000',
 0x22071E9E:'c22000',
}

def audit(client,server):
 assert hashlib.sha256(client).hexdigest()==CLIENT_HASH, 'unapproved client.dll'
 assert hashlib.sha256(server).hexdigest()==SERVER_HASH, 'unapproved server.dll'
 pe=PE32(server);assert pe.base==0x22000000
 assert pe.section(VA)==b'.text'
 assert hashlib.sha256(pe.read(VA,SIZE)).hexdigest()==SHA, 'whole 612B method/jump table mismatch'
 assert pe.read(0x22071E9E,3)==bytes.fromhex('c22000')
 assert pe.read(VA+SIZE,1)==b'\xcc', 'wrong native boundary after 4-case table'
 assert pe.read(0x22071EA1,3)==bytes.fromhex('8d4900'), 'jump-table boundary'
 for i,target in enumerate(DESTINATIONS):
  assert struct.unpack('<I',pe.read(JT+4*i,4))[0]==target, f'bad switch entry {i}'
 for va,target in CALLS.items():edge(pe,va,target)
 edge(pe,CALLER,0x22071C50)
 for va,h in WITNESSES.items():
  assert pe.read(va,len(bytes.fromhex(h)))==bytes.fromhex(h),f'bad original witness {va:#x}'
 return dict(new_complete_methods=1,new_distinct_original_bytes=SIZE,original_direct_E8_calls=len(CALLS),
             incoming_grunt_route_call=1,jump_table_destinations=len(DESTINATIONS),native_witnesses=len(WITNESSES))

def self_test():
 class PE:
  def __init__(self,va,data):self.va=va;self.data=data
  def read(self,va,n):return self.data[va-self.va:va-self.va+n]
 for va,target in {**CALLS,CALLER:0x22071C50}.items():
  raw=b'\xe8'+struct.pack('<i',target-va-5)
  edge(PE(va,raw),va,target)
  for i in range(5):
   changed=bytearray(raw);changed[i]^=1
   try:edge(PE(va,changed),va,target)
   except AssertionError:pass
   else:raise AssertionError('synthetic mutated E8 accepted')
 try:audit(b'bad',b'bad')
 except AssertionError:pass
 else:raise AssertionError('wrong original DLL accepted')
 return 'PHASE573_SYNTHETIC_OK'

def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('--self-test',action='store_true')
 p.add_argument('--bin-zip',type=Path)
 args=p.parse_args()
 if not(args.self_test or args.bin_zip):p.error('use --self-test and/or --bin-zip')
 if args.self_test:print(self_test())
 if args.bin_zip:
  with zipfile.ZipFile(args.bin_zip) as z:
   print('PHASE573_ORIGINAL_VERIFIED',audit(z.read('client.dll'),z.read('server.dll')))
if __name__=='__main__':main()