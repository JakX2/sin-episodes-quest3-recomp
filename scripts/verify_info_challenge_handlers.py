#!/usr/bin/env python3
"""SHA-pinned, read-only original 2006 info_challenge handler audit.

No original DLL/SDK byte content is published or executed. The underlying
challenge calculation at 0x222d3730 is NOT reconstructed here.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import zipfile

SERVER_SHA256='568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650'
FGD_SHA256='4e803f2e23e09ee2dd275064ee71be133536549d34321ff2c7237320ac8d60f9'
FGD_NAME='SinEpisodesSDK/bin/SinEpisodes.fgd'
HANDLERS={
 'GetInitialChallenge':(0x22527d40,0x224683b0,0x222f4390,'OnInitialChallenge'),
 'GetCurrentChallenge':(0x22527d74,0x22468380,0x222f43b0,'OnCurrentChallenge'),
 'GetChallengeDifference':(0x22527da8,0x2246834c,0x222f43e0,'OnChallengeDifference'),
}
INSTRUCTIONS={
 0x222f4393:'e80881fcff', 0x222f4398:'8b4008',
 0x222f439e:'8d8e94030000', 0x222f43a4:'e88798e3ff',
 0x222f43b4:'e877f3fdff', 0x222f43c4:'8d8eac030000',
 0x222f43e3:'e848f3fdff', 0x222f43e8:'d8a6dc030000',
 0x222f43f0:'d80d14633d22', 0x222f43f7:'8d8ec4030000',
}
class EvidenceError(ValueError): pass

def pe_read(blob,va,size):
 if len(blob)<128 or blob[:2]!=b'MZ':raise EvidenceError('not a DOS/PE file')
 pe=struct.unpack_from('<I',blob,0x3c)[0]
 if pe+24>len(blob) or blob[pe:pe+4]!=b'PE\0\0':raise EvidenceError('not a valid PE signature')
 machine,sections,_,_,_,optsize,_=struct.unpack_from('<HHIIIHH',blob,pe+4)
 opt=pe+24
 if machine!=0x14c or opt+optsize>len(blob) or optsize<96 or struct.unpack_from('<H',blob,opt)[0]!=0x10b:
  raise EvidenceError('expected PE32 x86 file')
 base=struct.unpack_from('<I',blob,opt+28)[0]
 rva=va-base
 if rva<0 or sections<1 or sections>96 or size<0:raise EvidenceError('invalid PE address/section count')
 start=opt+optsize
 if start+sections*40>len(blob):raise EvidenceError('truncated section table')
 for idx in range(sections):
  sh=start+idx*40
  virtual_size,addr,raw_size,raw_off=struct.unpack_from('<IIII',blob,sh+8)
  if addr<=rva and rva+size<=addr+min(virtual_size,raw_size):
   file_at=raw_off+rva-addr
   if file_at+size>len(blob):raise EvidenceError('truncated PE data')
   return blob[file_at:file_at+size]
 raise EvidenceError('VA outside backed sections')

def verify(binary,fgd,*,server_sha=SERVER_SHA256,fgd_sha=FGD_SHA256,
           handlers=HANDLERS,instructions=INSTRUCTIONS):
 if hashlib.sha256(binary).hexdigest()!=server_sha:raise EvidenceError('server.dll SHA mismatch')
 if hashlib.sha256(fgd).hexdigest()!=fgd_sha:raise EvidenceError('original FGD SHA mismatch')
 src=fgd.decode('latin1')
 block=re.search(r'@PointClass\s+base\(Targetname,\s*Parentname\)[^\r\n]*=\s*info_challenge\s*:[\s\S]*?\[([\s\S]*?)\]',src)
 if not block:raise EvidenceError('original info_challenge FGD block absent')
 for name,(desc,string_va,handler,out) in handlers.items():
  if not re.search(r'\binput\s+'+name+r'\(void\)',block.group(1)):
   raise EvidenceError('FGD input missing: '+name)
  if not re.search(r'\boutput\s+'+out+r'\(float\)',block.group(1)):
   raise EvidenceError('FGD float output missing: '+out)
  if pe_read(binary,string_va,len(name)+1)!=name.encode('ascii')+b'\0':
   raise EvidenceError('DLL handler name mismatch: '+name)
  ptr=struct.unpack('<I',pe_read(binary,desc,4))[0]
  code=struct.unpack('<I',pe_read(binary,desc+8,4))[0]
  if ptr!=string_va or code!=handler:raise EvidenceError('DLL handler pointer mismatch: '+name)
 for va,hexdump in instructions.items():
  if pe_read(binary,va,len(hexdump)//2)!=bytes.fromhex(hexdump):
   raise EvidenceError('DLL instruction mismatch: '+hex(va))
 if pe_read(binary,0x223d6314,4)!=struct.pack('<f',100.0):
  raise EvidenceError('difference multiplier is not 100.0f')
 return {'original_server_sha_verified':True,'original_fgd_sha_verified':True,
         'verified_handler_inputs':list(handlers),'verified_instruction_spans':len(instructions),
         'difference_baseline_object_offset':'0x3dc','difference_scale':100.0,
         'global_initial_value_offset':'0x8','current_challenge_function':'0x222d3730',
         'real_challenge_formula_verified':False,'original_code_executed':False,
         'quest_headset_tested':False}

def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('server_dll',type=Path);p.add_argument('sdk_zip',type=Path)
 args=p.parse_args()
 try:
  with zipfile.ZipFile(args.sdk_zip) as z:r=verify(args.server_dll.read_bytes(),z.read(FGD_NAME))
  print(json.dumps(r,indent=2))
 except (OSError,KeyError,zipfile.BadZipFile,ValueError) as ex:
  p.exit(1,'Phase92 original evidence verification failed: '+str(ex)+'\n')
if __name__=='__main__':main()
