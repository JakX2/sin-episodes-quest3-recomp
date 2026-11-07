#!/usr/bin/env python3
"""Verify original 2006 SiN PE32 client/server startup interface contracts.

Read-only on the owner's genuine bin.zip. Never executes or exports original
DLL bytes. Decodes the actual x86 PUSH name, factory CALL, null-test, global
pointer store and common failure branch. CI tests use invented synthetic
instruction windows only. This is NOT a full DLLInit decompilation.
"""
import argparse
import hashlib
import json
import struct
import zipfile
from pathlib import Path

ORIGINAL = {
 "server.dll":("568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650",0x22000000,0x221e7640,0x221e79a1,16),
 "client.dll":("6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d",0x24000000,0x240fe0e0,0x240fe59e,12),
}
SERVER = """
221e764b VEngineServer021 engine 22572198
221e7664 VoiceServer001 engine 2257219c
221e767d VEngineCvar002 engine 225721a0
221e7696 VEngineServerStringTable001 engine 225721a8
221e76af StaticPropMgrServer002 engine 225721ac
221e76c8 VEngineRandom001 engine 225721b0
221e76e1 IEngineSoundServer003 engine 225721b4
221e76fa SpatialPartition001 engine 225721b8
221e7713 VModelInfoServer001 engine 225721bc
221e772c EngineTraceServer003 engine 225721c0
221e7749 VFileSystem012 filesystem 225721a4
221e7762 GAMEEVENTSMANAGER002 engine 225721c4
221e777b VEngineCache001 engine 225721c8
221e7794 VSoundEmitter001 engine 225721d0
221e77ad ServerUploadGameStats001 engine 22572194
221e77c6 ISERVERPLUGINHELPERS001 engine 225721d4
"""
CLIENT = """
240fe10b VEngineClient012 engine 2433f684
240fe129 VEngineModel012 engine 2433f688
240fe142 VEngineEffects001 engine 2433f68c
240fe15b VEngineCvar002 engine 2433f690
240fe174 EngineTraceClient003 engine 2433f6d8
240fe18d VEngineRenderView011 engine 2433f694
240fe1a6 VDebugOverlay003 engine 2433f698
240fe1bf VMaterialSystem076 engine 2433f69c
240fe1d8 VEngineCache001 engine 2433f6a8
240fe1f1 VModelInfoClient003 engine 2433f6ac
240fe20a VEngineVGui001 engine 2433f6b0
240fe223 VEngineClientStringTable001 engine 2433f6b4
240fe23c SpatialPartition001 engine 2433f6b8
240fe255 VEngineShadowMgr002 engine 2433f6c0
240fe26e StaticPropMgrClient004 engine 2433f6c4
240fe287 IEngineSoundClient003 engine 2433f6c8
240fe2a0 VFileSystem012 engine 2433f6bc
240fe2b9 VEngineRandom001 engine 2433f6cc
240fe2d2 VENGINE_GAMEUIFUNCS_VERSION004 engine 2433f6dc
240fe2eb GAMEEVENTSMANAGER002 engine 2433f6e0
240fe304 VSoundEmitter001 engine 2433f6e4
"""
# Required, but obtained through other already-acquired object vtables.
CLIENT_DERIVED = [
 (0x240fe362,"MaterialSystemHardwareConfig012",0x20,0x2433f6a4),
 (0x240fe3e1,"MatSystemSurface003",0x08,0x2433f6d0),
]
OPTIONAL = {
 "server.dll":(0x221e7944,"VDebugOverlay003",0x225721cc),
 "client.dll":(0x240fe349,"VMaterialSystemStub001",0x2433f6a0),
}

def rows(text):
 return [(int(a,16),b,c,int(d,16))
         for a,b,c,d in (row.split() for row in text.splitlines() if row.strip())]
SERVER_REQUIRED = rows(SERVER)
CLIENT_REQUIRED = rows(CLIENT)

class AuditError(ValueError): pass
def require(condition,why):
 if not condition: raise AuditError(why)

class PEView:
 def __init__(self,blob):
  self.blob=blob
  require(blob[:2]==b"MZ","not DOS/PE")
  header=struct.unpack_from("<I",blob,0x3c)[0]
  require(blob[header:header+4]==b"PE\x00\x00","not PE")
  machine,count=struct.unpack_from("<HH",blob,header+4)
  opts=struct.unpack_from("<H",blob,header+20)[0]
  opt=header+24
  require(machine==0x14c and 1<=count<=96 and
          struct.unpack_from("<H",blob,opt)[0]==0x10b,"not x86 PE32")
  self.base=struct.unpack_from("<I",blob,opt+28)[0]
  self.sections=[]
  for i in range(count):
   at=opt+opts+40*i
   require(at+40<=len(blob),"truncated PE sections")
   name=blob[at:at+8].split(b"\x00")[0].decode("ascii","replace")
   virtual_size,rva,raw_size,raw=struct.unpack_from("<IIII",blob,at+8)
   require(raw+raw_size<=len(blob),"invalid section file bounds")
   self.sections.append((name,self.base+rva,virtual_size,raw_size,raw))
  require({".text",".rdata"} <= {s[0] for s in self.sections},
          "original PE code/data sections missing")
 def at(self,va,size):
  for name,start,virtual_size,raw_size,raw in self.sections:
   if start<=va and va+size<=start+min(virtual_size,raw_size):
    at=raw+va-start
    return self.blob[at:at+size]
  raise AuditError("unmapped PE virtual address "+hex(va))
 def section_at(self,va):
  for name,start,virtual_size,raw_size,raw in self.sections:
   if start<=va<start+virtual_size:return name
  return None

def push_name(view,site,name):
 require(view.at(site,1)==b"\x68","factory PUSH opcode changed")
 pointer=struct.unpack("<I",view.at(site+1,4))[0]
 require(view.section_at(pointer)==".rdata","interface name outside .rdata")
 raw=view.at(pointer,80)
 end=raw.find(b"\x00")
 require(end>0,"interface name missing terminator")
 require(raw[:end]==name.encode("ascii"),
         "original factory name changed at "+hex(site))
 return pointer

def check_direct(view,record,failed_exit):
 site,name,provider,storage=record
 require(view.at(site-2,2)==b"\x6a\x00","nonzero return-code pointer argument")
 address=push_name(view,site,name)
 code=view.at(site,36)
 call_opcode=b"\xff\xd7" if provider=="filesystem" else b"\xff\xd6"
 call=code.find(call_opcode,5,18)
 require(call>=0,"different native factory callback for "+name)
 test=code.find(b"\x85\xc0",call+2,call+13)
 require(test>=0,"missing EAX null test for "+name)
 require(code[test+2:test+7]==b"\xa3"+struct.pack("<I",storage),
         "original interface global store changed for "+name)
 branch=code[test+7:test+13]
 require(len(branch)==6 and branch[:2]==b"\x0f\x84",
         "required interface no longer branches on missing value "+name)
 delta=struct.unpack("<i",branch[2:])[0]
 require(site+test+13+delta==failed_exit,
         "required interface common-failure target changed "+name)
 return {"name":name,"factory":provider,"request_va":hex(site),
         "name_va":hex(address),"global_storage_va":hex(storage),
         "call_va":hex(site+call),"on_null_fail_va":hex(failed_exit)}

def check_derived(view,record,failed_exit):
 site,name,slot,storage=record
 pointer=push_name(view,site,name)
 code=view.at(site,21)
 require(code[5:8]==b"\xff\x50"+bytes([slot]),
         "derived material/surface factory method changed "+name)
 require(code[8:10]==b"\x85\xc0","derived interface missing null test")
 require(code[10:15]==b"\xa3"+struct.pack("<I",storage),
         "derived interface global store changed")
 require(code[15:17]==b"\x0f\x84" and
         site+21+struct.unpack("<i",code[17:21])[0]==failed_exit,
         "derived interface common-failure edge changed")
 return {"name":name,"factory":"vtable+"+hex(slot),
         "request_va":hex(site),"name_va":hex(pointer),
         "global_storage_va":hex(storage),"on_null_fail_va":hex(failed_exit)}

def check_optional(view,record):
 site,name,storage=record
 pointer=push_name(view,site,name)
 code=view.at(site,30)
 require(b"\xff\xd6" in code[5:12],"optional factory call changed")
 require(b"\xa3"+struct.pack("<I",storage) in code[7:23],
         "optional interface destination changed")
 if name=="VDebugOverlay003":
  require(view.at(0x221e795f,2)==b"\x74\x21",
          "server optional debug-overlay branch changed")
  require(view.at(0x221e7979,2)==b"\xb0\x01" and
          view.at(0x221e7998,2)==b"\xb0\x01",
          "optional debug-overlay return paths no longer both succeed")
 else:
  require(view.at(0x240fe360,2)==b"\x8b\x01",
          "client optional material-stub continuation changed")
 return {"name":name,"request_va":hex(site),"name_va":hex(pointer),
         "global_storage_va":hex(storage),"null_directly_fails_init":False}

def audit_file(blob,name):
 sha,base,entry,failed_exit,arg_bytes=ORIGINAL[name]
 require(hashlib.sha256(blob).hexdigest()==sha,
         "original "+name+" SHA256 mismatch")
 pe=PEView(blob)
 require(pe.base==base and pe.section_at(entry)==".text",
         "original game startup method VA changed")
 if name=="server.dll":
  failure=b"\x5f\x32\xc0\x5e\x83\xc4\x0c\xc2\x10\x00\xcc"
 else:
  failure=b"\x33\xc0\x5e\xc2\x0c\x00\xcc"
 require(pe.at(failed_exit,len(failure))==failure,
         "original game method failure return/ABI changed")
 direct=[check_direct(pe,r,failed_exit) for r in
         (SERVER_REQUIRED if name=="server.dll" else CLIENT_REQUIRED)]
 derived=[check_derived(pe,r,failed_exit) for r in CLIENT_DERIVED] if name=="client.dll" else []
 optional=check_optional(pe,OPTIONAL[name])
 return {"module":name,"original_sha256":sha,"method_index":0,
         "method_va":hex(entry),"x86_thiscall_argument_bytes":arg_bytes,
         "failure_exit_va":hex(failed_exit),"required_direct":direct,
         "required_derived":derived,"optional_probes":[optional],
         "total_null_checked_requirements":len(direct)+len(derived)}

def audit(original_bin_zip):
 with zipfile.ZipFile(original_bin_zip) as z:
  results=[audit_file(z.read(name),name)
           for name in ("server.dll","client.dll")]
 return {"original_dllinit_dependencies":results,
         "not_proven":["full initialization body and side effects",
         "matching SourceVR ARM64 interface versions and C++ vtables",
         "engine-loaded game module or playable standalone Quest 3 APK"]}

if __name__=="__main__":
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument("--bin-zip",type=Path,required=True)
 args=p.parse_args()
 try:print(json.dumps(audit(args.bin_zip),indent=2))
 except (AuditError,OSError,zipfile.BadZipFile,KeyError,struct.error) as exc:
  p.exit(2,"Original SiN 2006 DLLInit audit FAILED: "+str(exc)+"\n")
