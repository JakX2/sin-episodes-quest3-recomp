#!/usr/bin/env python3
"""Read-only SHA-pinned original 2006 / native SourceVR 0.5.5 sound ABI audit.

Metadata only. No ARM64 method is called or considered ABI approved.
"""
from __future__ import annotations
import argparse,hashlib,json,struct
from pathlib import Path

ORIGINAL_SHA='8c92cdc58f366cde07a1e14b8d95bfc36abdb552242508993189479faa79dba5'
NATIVE_SHA='484e5bb2c1457ee6e62f39d47c83bc68b8cee491c2f1479886a8e0d93740c454'
REGIONS={
 'old_name_to_token':(0x2c00a830,23,'2b50c0b51f219bd9d15b2063185dc60ef20db971dd82d117aa5ff17cfa42ff06'),
 'old_token_to_name':(0x2c008fc0,71,'5f65434fafff1a5e839afbce905aea344f95a5f5deab14bd9b4a4cd43a84845d'),
 'native_name_to_token_candidate':(0x133a0,336,'f2fd0908b3a15993aa23cda25464009aaedd3a250d2a156e73657e13a86aa07d'),
 'native_token_to_name_candidate':(0x13570,88,'4ab11e12272f034525b4f4ea1b4f5603d0700263c141385a8bc62cf02a8ed55e'),
 'native_index_validator':(0x13520,76,'44f13012e9a0558361a5e4d8ea8fd9b01bddc8f6c04cded69214ea1719272a26')}
ARM={
 0x133b4:0xb40003c1,0x133f4:0x39400288,0x13498:0x94000fe2,
 0x1349c:0x3100041f,0x134ac:0x79400513,0x134b4:0x12800013,
 0x134d8:0x2a1303e0,0x13520:0xb9405009,0x13524:0x12003c28,
 0x13548:0x7940112a,0x1357c:0xf9400008,0x13588:0xf9402108,
 0x1358c:0xd63f0100,0x135a4:0x12003e69,0x135ac:0xf8696908,
 0x135b0:0xf9400108,0x135b8:0x9a880000}
X86={0x2c00a830:'8b442404',0x2c00a835:'83c15c',
     0x2c008fcb:'ff5010',0x2c008fd2:'b83c96012c'}
class AuditFailure(ValueError):pass
class PE32:
 def __init__(self,data):
  self.data=data
  if data[:2]!=b'MZ' or len(data)<512:raise AuditFailure('not a PE32 image')
  off=struct.unpack_from('<I',data,0x3c)[0]
  if off+24>len(data) or data[off:off+4]!=b'PE\0\0':raise AuditFailure('PE signature missing')
  count=struct.unpack_from('<H',data,off+6)[0]
  opts=struct.unpack_from('<H',data,off+20)[0];o=off+24
  if count>96 or o+opts+40*count>len(data):raise AuditFailure('bad PE section table')
  if struct.unpack_from('<H',data,o)[0]!=0x10b:raise AuditFailure('expected PE32 x86')
  self.base=struct.unpack_from('<I',data,o+28)[0];self.sections=[]
  for i in range(count):
   p=o+opts+i*40
   virtual_size,va,file_size,file_off=struct.unpack_from('<IIII',data,p+8)
   self.sections.append((va,virtual_size,file_off,file_size))
 def at(self,addr,count):
  rva=addr-self.base
  for va,vsize,off,size in self.sections:
   if va<=rva and rva+count<=va+size and off+(rva-va)+count<=len(self.data):
    return self.data[off+(rva-va):off+(rva-va)+count]
  raise AuditFailure('PE VA not file-backed: %#x'%addr)
class ELF64:
 def __init__(self,data):
  self.data=data
  if data[:6]!=b'\x7fELF\x02\x01' or len(data)<64:raise AuditFailure('not ELF64 little endian')
  if struct.unpack_from('<HH',data,16)!=(3,183):raise AuditFailure('not AArch64 ET_DYN')
  phoff=struct.unpack_from('<Q',data,32)[0];entsize,count=struct.unpack_from('<HH',data,54)
  if count>128 or entsize<56 or phoff+count*entsize>len(data):raise AuditFailure('ELF phdr damaged')
  self.loads=[]
  for i in range(count):
   typ,flags,off,va,_,filesz,_=struct.unpack_from('<IIQQQQQ',data,phoff+i*entsize)
   if typ==1:self.loads.append((va,off,filesz,flags))
 def at(self,addr,count):
  for va,off,size,_ in self.loads:
   if addr>=va and addr+count<=va+size and off+(addr-va)+count<=len(self.data):
    return self.data[off+(addr-va):off+(addr-va)+count]
  raise AuditFailure('ELF VA not file-backed: %#x'%addr)
def verify_bytes(pe,elf,strict_hash=True):
 out={}
 for name,(addr,length,sha) in REGIONS.items():
  src=pe if name.startswith('old_') else elf
  actual=hashlib.sha256(src.at(addr,length)).hexdigest()
  if strict_hash and actual!=sha:raise AuditFailure('Phase102 function SHA mismatch: '+name)
  out[name]={'address':hex(addr),'length':length,'sha256':actual}
 for addr,opcode in ARM.items():
  if elf.at(addr,4)!=struct.pack('<I',opcode):raise AuditFailure('Phase102 AArch64 opcode mismatch at %#x'%addr)
 for addr,code in X86.items():
  raw=bytes.fromhex(code)
  if pe.at(addr,len(raw))!=raw:raise AuditFailure('Phase102 x86 opcode mismatch at %#x'%addr)
 return out
def audit(old,native):
 if hashlib.sha256(old).hexdigest()!=ORIGINAL_SHA:raise AuditFailure('original 2006 SoundEmitterSystem.dll SHA mismatch')
 if hashlib.sha256(native).hexdigest()!=NATIVE_SHA:raise AuditFailure('SourceVR 0.5.5 ARM64 sound emitter SHA mismatch')
 return {
  'provenance':{'original_sound_sha256':ORIGINAL_SHA,'native_sound_sha256':NATIVE_SHA},
  'verified_regions':verify_bytes(PE32(old),ELF64(native)),
  'native_instruction_anchors':len(ARM),'old_instruction_anchors':len(X86),
  'name_to_token_candidate':{'original_slot':3,'native_slot':7,'native_address':'0x133a0',
    'observed_inputs':'x0 native object, x1 string pointer','observed_result':'w0 u16 index or -1',
    'native_abi_approved':False},
  'token_to_name_candidate':{'original_slot':6,'native_slot':10,'native_address':'0x13570',
    'observed_inputs':'x0 object, w1 token, call native validator slot8',
    'observed_result':'x0 from internal name table or fallback',
    'pointer_lifetime_verified':False,'native_abi_approved':False},
  'not_claimed':['complete signatures','safe native pointers','real native provider','Quest gameplay']}
def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('--original-sound',required=True,type=Path)
 p.add_argument('--native-sound',required=True,type=Path)
 p.add_argument('--output',required=True,type=Path)
 a=p.parse_args()
 result=audit(a.original_sound.read_bytes(),a.native_sound.read_bytes())
 a.output.parent.mkdir(parents=True,exist_ok=True)
 a.output.write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
 print('Verified sound slots 7/10 from exact original/native binaries; ABI NOT approved')
if __name__=='__main__':main()
