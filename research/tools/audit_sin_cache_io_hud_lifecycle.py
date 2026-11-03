#!/usr/bin/env python3
"""Read-only original SiN 2006 client/server cache I/O and HUD lifecycle audit.

Requires the owner-held exact original PE32 DLLs and chains Phase91's SHA,
RTTI and original vtable audit. Publishes metadata only, no game code bytes.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
from pathlib import Path
from audit_sin_rtti_cache_client_hud import audit as previous_audit
from audit_sin_original_factories import PE32, VerificationError

CLIENT=(
 ('CDebugOverlayPanel',0x241eca60,38,'f923966ea39aa30c84150b401978164d08bde5a0703c67333a66f0a44970e7ba','single'),
 ('CNetGraphPanelInterface',0x241ef9f0,38,'f923966ea39aa30c84150b401978164d08bde5a0703c67333a66f0a44970e7ba','single'),
 ('CFPS',0x241ed1a0,38,'f923966ea39aa30c84150b401978164d08bde5a0703c67333a66f0a44970e7ba','single'),
 ('CMessageChars',0x241eea30,38,'f923966ea39aa30c84150b401978164d08bde5a0703c67333a66f0a44970e7ba','single'),
 ('CLoadingDisc',0x241ee480,71,'a76c48a785305d420349c2fddf8b0162abcfefceacfafd92c7746e18c7cd705c','double'),
 ('CCenterPrint',0x241ec510,43,'f0d8ceffc860e87d455d2b6781db940593e046b7ad67f7339d5ee4e7857c2b0b','nulling'),
 ('CTextMessage',0x241f2320,38,'f923966ea39aa30c84150b401978164d08bde5a0703c67333a66f0a44970e7ba','single'),
 ('CStatsPanelInterface',0x24207670,43,'f0d8ceffc860e87d455d2b6781db940593e046b7ad67f7339d5ee4e7857c2b0b','nulling'),
)
SERVER=(
 ('CSceneCache','write',0x221a7360,88,'a6c16128d7c518f90d8f923a78bbcd578faee93791ae2bc67cf0101c554e66fc'),
 ('CModelSoundCache','write',0x221437e0,72,'2fee958faf2870edecab5be14269be1760efb27f1f82b3b29dccd61ba4ca28a8'),
 ('CSceneCache','read',0x221ab0a0,125,'c8d169ad3901aa82139fa5af25a315aa0199b4e9367b125e88c9f1c1380b511a'),
 ('CModelSoundCache','read',0x221453e0,119,'fbd9ae15a440354cd372ce784b7d8ef816994e9d503bec185a48a297b5dbd268'),
)
SERVER_OPCODE_ANCHORS={
 0x221a736a:'8b4704',0x221a7375:'668b6f14',0x221a7390:'8b4708',
 0x221a739f:'ff5218',0x221437ea:'668b5f10',0x22143800:'8b4704',
 0x2214380f:'ff5218',0x221ab0c3:'d95e04',0x221ab0d6:'6800020000',
 0x221ab0f4:'ff520c',0x221ab0f7:'83f8ff',0x221453f4:'c7470c00000000',
 0x22145410:'6800020000',0x2214542e:'ff520c',0x22145431:'83f8ff',
 0x221ab0b7:'c7430c00000000',0x221a739e:'50',0x2214380e:'50',
 0x221ab0cd:'0fb7c0',0x22145400:'0fb7c0',
}
SERVER_CALLS={
 0x221a7370:0x221a73c0,0x221a737c:0x22143830,
 0x221a73a5:0x2228dce0,0x221437f1:0x22143830,
 0x22143815:0x2228dce0,0x221ab0be:0x221ab120,
 0x221ab0c8:0x22145460,0x221ab0e2:0x2228d810,
 0x221ab107:0x22097e10,0x221453fb:0x22145460,
 0x2214541c:0x2228d810,0x22145441:0x22097e10,
}
CLIENT_PATTERNS={
 'single':((0x10,'ff9098000000'),(0x21,'ff5274')),
 'double':((0x10,'ff9098000000'),(0x21,'ff5274'),
           (0x31,'ff9098000000'),(0x42,'ff5274')),
 'nulling':((0x0e,'ff9098000000'),(0x1f,'ff5274'),
            (0x22,'c7460400000000')),
}
def exact(pe,at,raw,reason):
 if pe.at(at,len(raw))!=raw:
  raise VerificationError('Phase92 '+reason+' differs at %#x'%at)

def verify_client(pe,full_hash=True):
 observed=[]
 for name,start,size,digest,kind in CLIENT:
  if full_hash and hashlib.sha256(pe.at(start,size)).hexdigest()!=digest:
   raise VerificationError('Phase92 original HUD code SHA mismatch: '+name)
  exact(pe,start+size,b'\xcc','function boundary')
  for offset,raw in CLIENT_PATTERNS[kind]:
   exact(pe,start+offset,bytes.fromhex(raw),'HUD release virtual-call sequence')
  observed.append(dict(class_name=name,original_va=hex(start),bytes=size,
                       code_sha256=digest if full_hash else None,shape=kind,
                       unbind_virtual_offset=0x98,unbind_argument=0,
                       release_virtual_offset=0x74,release_argument=1,
                       second_pointer_offset=8 if kind=='double' else None,
                       clears_first_pointer=(kind=='nulling')))
 return dict(methods=observed,original_function_count=8,
             distinct_binary_implementations=3,
             checked_virtual_opcodes=sum(len(CLIENT_PATTERNS[x[-1]]) for x in CLIENT))

def verify_server(pe,full_hash=True):
 observed=[]
 for name,operation,start,length,digest in SERVER:
  if full_hash and hashlib.sha256(pe.at(start,length)).hexdigest()!=digest:
   raise VerificationError('Phase92 original server code SHA mismatch: '+name+operation)
  exact(pe,start+length,b'\xcc','function boundary')
  observed.append(dict(class_name=name,operation=operation,original_va=hex(start),
                       bytes=length,code_sha256=digest if full_hash else None))
 for site,raw in SERVER_OPCODE_ANCHORS.items():
  exact(pe,site,bytes.fromhex(raw),'cache opcode')
 for site,target in SERVER_CALLS.items():
  exact(pe,site,b'\xe8'+struct.pack('<i',target-site-5),'cache CALL target')
 return dict(methods=observed,scene_has_float_prefix=True,
             model_sound_has_float_prefix=False,
             item_count_format='unsigned 16-bit',
             temporary_buffer_capacity=0x200,
             unresolved_key_sentinel=0xffffffff,
             identifier_lookup_virtual_offset=0x0c,
             identifier_writer_virtual_offset=0x18,
             server_opcode_anchors=len(SERVER_OPCODE_ANCHORS),
             server_direct_calls=len(SERVER_CALLS),
             limitation='NOT a complete on-disk record schema or bytewise cache codec')

def audit(client,server):
 previous_audit(client,server)  # original dual-DLL SHA + phase91 RTTI/vtables
 return dict(client=verify_client(PE32(client)),
             server=verify_server(PE32(server)),
             scope='Original x86 evidence and mock contracts, NOT Android game modules')

def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('--client',required=True,type=Path)
 p.add_argument('--server',required=True,type=Path)
 p.add_argument('--output',required=True,type=Path)
 a=p.parse_args()
 result=audit(a.client.read_bytes(),a.server.read_bytes())
 a.output.parent.mkdir(parents=True,exist_ok=True)
 a.output.write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
 print('Verified 8 original client HUD shutdown bodies and 4 server cache I/O bodies')
if __name__=='__main__':main()
