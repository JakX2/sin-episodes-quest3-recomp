#!/usr/bin/env python3
"""Original SiN 2006 server CRecipientFilter voice-recipient proof, read-only.

Requires private original DLL pair only for opt-in --bin-zip SHA checks.
Uses the PE32 reader from the earlier original-free Phase551 auditor.
"""
import argparse
import hashlib
import struct
import zipfile
from pathlib import Path
from audit_sin_original_voice_block_hash import CLIENT_HASH, SERVER_HASH, PE32, check_edge

METHODS={
 0x222701C0:(41,'71cd95fee2a0a21a5ef5f62c48d06a9a3fba1b4ad5f7b92064fef5b7fc5c4ce2','CRecipientFilter constructor'),
 0x222701F0:(25,'7993bbe405b75946ccbdcb6a8057f7392b00e05d6da96bf05fb424de79009fbb','CRecipientFilter stack teardown'),
 0x222702A0:(45,'ef0da906d7a86f04913764f18e12c03f17356c724774f54516eb896306d4ad4e','CRecipientFilter virtual deleting destructor'),
 0x222706B0:(137,'375dfd003e35d9619921253deb1f78fa29b70d102e770b842e59ac37efb2f4c5','CRecipientFilter unique-player insertion'),
 0x22286C70:(70,'711bbc293e6224c764f1d535973921556ba395ac55b85910f3e81b75af962e55','original 1-based player to active entity resolver'),
}

CALLS={
 0x222701FC:0x2206F3D0, # destroy internal list
 0x222702AC:0x2206F3D0, # deleting destructor destroys internal list
 0x222702BF:0x2226E800, # optional object free
 0x22270717:0x22095600, # append unique recipient index
 0x2227072F:0x22095600, # append unique index via empty list case
 0x22293D64:0x222701C0, # Phase555 original temporary filter ctor
 0x22293D76:0x222706B0, # Phase555 add voice recipient
 0x22293F39:0x222701F0, # Phase555 cleanup original filter
}

WITNESSES={
 0x222701C4:'c700b0464522',  # original CRecipientFilter vtable
 0x222701CA:'894808',        # list pointer=0
 0x222701D3:'894814',        # list count=0
 0x222701E5:'88481d',        # filter field clear
 0x222701E8:'c3',            # constructor ends
 0x222701F6:'c706b0464522',  # filter vtable during cleanup
 0x22270201:'c706d0214322',  # IRecipientFilter vtable after cleanup
 0x222702A6:'c706b0464522',  # deleting destructor filter vtable
 0x222702B1:'f644240801',    # original delete flag
 0x222702B6:'c706d0214322',  # base interface vtable
 0x222706B6:'8b876c010000',  # player network link at +0x16c
 0x222706BE:'8b0d98215722',  # original service global
 0x222706C7:'ff5248',        # player index via service virtual +0x48
 0x222706CA:'807e1c00',      # special filter +0x1c branch
 0x222706EC:'8d4e08',        # internal recipient list object
 0x222706F4:'85f6',          # list count zero?
 0x22270700:'3907',          # exact duplicate match
 0x22270717:'e8e44ee2ff',    # insert into list
 0x2227072F:'e8cc4ee2ff',    # empty list insert
 0x22270736:'c20400',        # one-arg thiscall return
 0x22286C70:'8b442404',      # 1-based index
 0x22286C77:'85c0',          # reject non-positive
 0x22286C81:'3b4114',        # upper bound vs service global count
 0x22286C8F:'ff524c',       # engine service +0x4c player edict lookup
 0x22286C96:'8b08',         # edict bitfield
 0x22286C9A:'f6c101',       # test original flag bit after SHR
 0x22286C9F:'8b400c',       # edict +0xc owner pointer
 0x22286CAB:'ff6214',       # original owner virtual +0x14 tail
 0x22286CB5:'c3',           # invalid player return
}

RTTI={
 0x224546B0:(0x224C8B8C,0x224D87DC,b'.?AVCRecipientFilter@@'),
 0x224321D0:(0x224C2138,0x224D87BC,b'.?AVIRecipientFilter@@'),
}
PRIOR=(0x22293CF0,619,'4b9346aca0bdbdf927aa5932929c452c1b10a1021d9973a2bbe8ce25fcceb8c8')

def audit(client,server):
 assert hashlib.sha256(client).hexdigest()==CLIENT_HASH,'invalid original client SHA256'
 assert hashlib.sha256(server).hexdigest()==SERVER_HASH,'invalid original server SHA256'
 s=PE32(server); assert s.base==0x22000000
 intervals=[]
 for va,(size,digest,label) in METHODS.items():
  assert s.section(va)==b'.text',f'{label} outside text'
  assert hashlib.sha256(s.read(va,size)).hexdigest()==digest, f'{label} native body changed'
  assert s.read(va+size,1)==b'\xcc',f'{label} unexpected boundary'
  intervals.append((va,va+size))
 intervals.sort()
 assert all(a[1]<=b[0] for a,b in zip(intervals,intervals[1:]))
 for site,target in CALLS.items():check_edge(s,site,target,0xe8)
 for va,h in WITNESSES.items():assert s.read(va,len(bytes.fromhex(h)))==bytes.fromhex(h),f'instruction changed {va:#x}'
 for vtable,(col,typ,label) in RTTI.items():
  assert struct.unpack('<I',s.read(vtable-4,4))[0]==col
  assert struct.unpack('<I',s.read(col+12,4))[0]==typ
  assert s.read(typ+8,len(label)+1)==label+b'\0'
 assert struct.unpack('<I',s.read(0x224546B0,4))[0]==0x222702A0
 assert struct.unpack('<I',s.read(0x224321D0,4))[0]==0x221B9A80
 va,n,digest=PRIOR
 assert hashlib.sha256(s.read(va,n)).hexdigest()==digest
 assert sum(n for n,_,_ in METHODS.values())==318
 return {'new_full_original_server_methods':5,'unique_original_bytes':318,
         'call_edges':len(CALLS),'opcode_witnesses':len(WITNESSES),
         'rtti':['CRecipientFilter','IRecipientFilter'],
         'previous_VoiceMask_sender_verified':True}

def selftest():
 class Fake:
  def __init__(self,va,b):self.va,self.b=va,b
  def read(self,a,n):return self.b[a-self.va:a-self.va+n]
 for site,target in CALLS.items():
  good=b'\xe8'+struct.pack('<i',target-site-5)
  check_edge(Fake(site,good),site,target,0xe8)
  for i in range(5):
   bad=bytearray(good);bad[i]^=1
   try:check_edge(Fake(site,bad),site,target,0xe8)
   except AssertionError:pass
   else:raise AssertionError('bad rel32 mutation accepted')
 try:audit(b'MZ fake client',b'MZ fake server')
 except AssertionError:pass
 else:raise AssertionError('fake pair accepted')
 return 'SELF_TEST_OK'

def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('--bin-zip',type=Path);p.add_argument('--self-test',action='store_true');a=p.parse_args()
 if not(a.bin_zip or a.self_test):p.error('use --bin-zip and/or --self-test')
 if a.self_test:print(selftest())
 if a.bin_zip:
  with zipfile.ZipFile(a.bin_zip) as z:r=audit(z.read('client.dll'),z.read('server.dll'))
  print('ORIGINAL_SERVER_VOICE_RECIPIENT_FILTER_AUTHENTICATED',r)
if __name__=='__main__':main()