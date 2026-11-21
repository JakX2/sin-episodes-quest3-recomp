#!/usr/bin/env python3
"""Original SiN 2006 owner-linked group construction/membership audit (Phase562).

Read-only exact original PE32 SHA authentication. Never emit native code bytes.
"""
import argparse
import hashlib
import struct
import zipfile
from pathlib import Path
from audit_sin_original_grunt_group_phase560 import PE32,CLIENT_HASH,SERVER_HASH,edge

METHODS={
 0x220AA6A0:(99,'838977b93fb2de5eaae51669e06519562ef484ecf7f0e9d783817be2cd2ff4b8','named 0x78-byte linked-group constructor'),
 0x220AA710:(90,'7f9a809484db8d69b0213f9f3881beed3ee5abcf0844b457d04d25bffed62248','default linked-group constructor'),
 0x220AA810:(60,'361b80733d736860c791353349994cd66cecc580f3813de6933b7b6706e6a7c6','allocate and link group in registry'),
 0x220AA850:(82,'c68ad6c711e9bc018a03750f8c69fb8293e2ba6c547e780b82f735b8509bf90a','unlink group from registry and release storage'),
 0x220AA0D0:(219,'39cc26906ddebebf3dcec5762cda698a71457dcd0140a493c51eade127776f65','remove linked member, optional notify survivors, owner clear'),
 0x220AA1B0:(273,'26cbea4607e8daf71d0a6bc36920e34509b94e863aea48d2070215b5a3c3b061','insert owner into linked group, cardinality/previous group gate'),
 0x220AA8C0:(94,'cf454aef73b3b7ceb1108ed5b60275ae142503b8c059d433f35644111e639a32','compact tree-node insertion used in cache-rebuild context'),
}
CALLS={
 0x220AA6D9:0x221F3DE0,0x220AA741:0x221F3DE0,
 0x220AA816:0x2226E790,0x220AA829:0x220AA6A0,
 0x220AA883:0x220A98B0,0x220AA896:0x2226E800,
 0x220AA0F8:0x220A97E0,0x220AA12F:0x223A5D10,
 0x220AA1E2:0x220AA0D0,0x220AA209:0x220A8670,
 0x220AA226:0x220A9790,0x220AA247:0x22060CC0,
 0x220AA261:0x2206B7B0,0x220AA2B1:0x2206B130,
 0x220AA8DB:0x220A8A30,0x220AA8E2:0x220A8BA0,0x220AA8F6:0x220AA350,
}
CROSS_EDGES={
 0x22054CFD:0x220AA810, # Phase560 group registry fallback
 0x22054D20:0x220AA0D0, # Phase560 Grunt +0x9c4 clear
 0x220AA9E0:0x220AA8C0, # Phase561 timed refresh path
}
WITNESSES={
 0x220AA6AD:'897e48', # zero group member count
 0x220AA6B3:'895b44', # fixed array storage initial marker
 0x220AA6B6:'c74658ffffffff', # unused index sentinel
 0x220AA6BD:'897e60', # record ptr null
 0x220AA6C6:'897e6c', # records count zero
 0x220AA6F5:'897e74', # deadline 0
 0x220AA6F8:'897e5c', # last record ptr 0
 0x220AA717:'895e48', # default group's count 0
 0x220AA721:'c74658ffffffff',
 0x220AA75E:'895e74', # default deadline zero
 0x220AA812:'6a78', # literal 120-byte group allocation
 0x220AA824:'740a', # conditional allocation return path
 0x220AA850:'8b01', # registry head
 0x220AA880:'8d4e60', # cached-record cleanup
 0x220AA0D5:'85db', # member nonnull
 0x220AA0EE:'894c2410', # converted native handle
 0x220AA0F2:'8d7508', # handles +8
 0x220AA137:'834640ff', # decremented group count through embedded array +0x40
 0x220AA13B:'807c241400', # optional notify mode
 0x220AA17D:'ff9050070000', # surviving member vtable +0x750
 0x220AA195:'ff9290060000', # removed member virtual +0x690
 0x220AA19B:'c783c809000000000000', # owner +0x9c8 clears
 0x220AA1C0:'83be8c00000000', # reject special owner state
 0x220AA1CD:'8b8ec4090000', # existing owner group
 0x220AA1E7:'837f4810', # group member limit 16
 0x220AA209:'e862e4ffff', # full-group evict helper
 0x220AA230:'ff9290060000', # member +0x690 attach callback
 0x220AA239:'8986c8090000', # member group-name field +0x9c8
 0x220AA23F:'837f4801', # further group processing only if >1
 0x220AA2A4:'ff9278060000', # related entity virtual +0x678
 0x220AA8C0:'8b442408', # second arg
 0x220AA8F6:'e855faffff', # tree assignment helper
 0x220AA8FB:'6683461201', # 16-bit node-generation increment
}

def audit(client,server):
 assert hashlib.sha256(client).hexdigest()==CLIENT_HASH,'original client changed'
 assert hashlib.sha256(server).hexdigest()==SERVER_HASH,'original server changed'
 pe=PE32(server);assert pe.base==0x22000000
 regions=[]
 for va,(n,digest,role) in METHODS.items():
  assert pe.section(va)==b'.text',role
  assert hashlib.sha256(pe.read(va,n)).hexdigest()==digest,role
  assert pe.read(va+n,1)==b'\xcc',f'boundary {va:#x}'
  regions.append((va,va+n))
 regions.sort();assert all(a[1]<=b[0] for a,b in zip(regions,regions[1:]))
 for site,target in {**CALLS,**CROSS_EDGES}.items():edge(pe,site,target)
 for site,hx in WITNESSES.items():
  assert pe.read(site,len(bytes.fromhex(hx)))==bytes.fromhex(hx),f'witness {site:#x}'
 assert struct.unpack('<B',pe.read(0x220AA813,1))[0]==0x78 # exact allocation size
 assert sum(n for n,_,_ in METHODS.values())==917
 return dict(new_complete_server_methods=7,authenticated_original_bytes=917,
             native_calls=len(CALLS),original_cross_calls=len(CROSS_EDGES),
             opcode_witnesses=len(WITNESSES),group_object_allocated_bytes=120,
             native_group_member_limit=16)

def self_test():
 class Stub:
  def __init__(self,site,data):self.site,self.data=site,data
  def read(self,addr,n):return self.data[addr-self.site:addr-self.site+n]
 for site,target in {**CALLS,**CROSS_EDGES}.items():
  data=b'\xe8'+struct.pack('<i',target-site-5)
  edge(Stub(site,data),site,target)
  for i in range(5):
   bad=bytearray(data);bad[i]^=1
   try:edge(Stub(site,bad),site,target)
   except AssertionError:pass
   else:raise AssertionError('E8 mutation not detected')
 try:audit(b'MZfake',b'MZfake')
 except AssertionError:pass
 else:raise AssertionError('fake DLL approved')
 return 'SELF_TEST_OK'

def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('--self-test',action='store_true')
 p.add_argument('--bin-zip',type=Path)
 a=p.parse_args()
 if not(a.self_test or a.bin_zip):p.error('select --self-test and/or --bin-zip')
 if a.self_test:print(self_test())
 if a.bin_zip:
  with zipfile.ZipFile(a.bin_zip) as z:
   print('SIN_ORIGINAL_GRUNT_GROUP_LIFECYCLE_PHASE562_AUTHENTICATED',audit(z.read('client.dll'),z.read('server.dll')))
if __name__=='__main__':main()