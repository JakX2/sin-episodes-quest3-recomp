#!/usr/bin/env python3
"""Read-only SHA-verified 2006 SiN client/server shutdown+cache x86 audit."""
from __future__ import annotations
import argparse,hashlib,json,struct
from pathlib import Path
from audit_sin_subsystem_startup import audit as previous_audit
from audit_sin_original_factories import PE32,VerificationError

CLIENT='client.dll';SERVER='server.dll'
REGIONS={
CLIENT:{
 'client_interface_slot_1':(0x240ff3d0,204,'4cab24e5b8920392fdcf69020e709da740e06f2963f01ca2b374a6c3755f92d1')},
SERVER:{
 'server_interface_slot_8':(0x221e79b0,210,'f6c9415aaacd6601867d81610fd6ea162a588012dc71e105a6de05660fe01e8c'),
 'server_interface_slot_1':(0x221e7b00,41,'eef931750a7ee8975e75405e8b2fc4607904eeae15aec74a356035f129363c35'),
 'server_interface_slot_7':(0x221e7b30,5,'bf4cd45b2447cdeca904e392cfceb2525169471f83eba87ad88280d24ab4d71c'),
 'server_global_reset':(0x221f4980,69,'7c13db622884650619706d474b2f5165bb01bbf31bbf70574dad5519d0710c16'),
 'modelsounds_cache_close':(0x22149710,89,'2815f0d5793c8cc5a8e82318a19d8e6083a832f545217d81aa31235818bd0e59'),
 'scene_cache_close':(0x221ac910,89,'16e8e4cbcc11f001a469b61a8c0c6e86cbc8bedebd9836fa60d9a10baceeb2fe'),
}}
def parse_pairs(s):
    return {int(z.split('=')[0],16):z.split('=')[1] for z in s.split()}
OPCODES={
CLIENT:parse_pairs("""
240ff3d0=5657 240ff3d2=8bf9 240ff3d9=a170f42f24
240ff3ec=ff5628 240ff403=ff5628 240ff41a=ff5628
240ff427=8b0da0f83724 240ff42f=ff5018 240ff43a=ff5210
240ff445=ff5004 240ff45a=ff5208 240ff45d=8b7710
240ff460=83ee01 240ff463=7810 240ff465=8b4704
240ff468=8b0cb0 240ff46b=8b11 240ff46d=ff5234
240ff470=83ee01 240ff473=79f0 240ff475=33f6
240ff477=897710 240ff494=8935d0f63324 240ff49b=c3
"""),
SERVER:parse_pairs("""
221e79b0=56 221e79bb=a180ba5122
221e79ce=ff5628 221e79e5=ff5628 221e79fc=ff5628
221e7a12=ff5628 221e7a29=ff5628
221e7a67=8b0d88b85422 221e7a6d=85c9 221e7a6f=7410
221e7a73=6a01 221e7a75=ff10 221e7a77=c70588b8542200000000
221e7a81=c3 221e7b05=8b0d98215722
221e7b0d=68742a4422 221e7b12=ff9090000000
221e7b20=ff9294000000 221e7b26=b001 221e7b28=c3
22149710=803dc5034f2200 22149719=803dc4034f2200
22149720=c605c5034f2200 2214973d=803dd6034f2200
22149750=84c0 22149752=750a 22149768=c3
221ac910=803d1d3f502200 221ac919=803d1c3f502200
221ac920=c6051d3f502200 221ac93d=803d2e3f502200
221ac950=84c0 221ac952=750a 221ac968=c3
221ad488=3b4638 22149968=3b4638
221f4980=833d782c5722ff 221f498c=c7057c2c572200000000
221f49a7=a3702c5722 221f49ac=c705742c572200000000
"""),
}
CALLS={
CLIENT:{
0x240ff3d4:0x240fc900,0x240ff3e0:0x241dab30,
0x240ff3f7:0x2418d1c0,0x240ff40e:0x241acf50,
0x240ff41d:0x241e5050,0x240ff422:0x241640f0,
0x240ff448:0x24053ab0,0x240ff44d:0x24085220,
0x240ff47a:0x241566c0,0x240ff484:0x24143e80,
0x240ff489:0x241ee0c0,0x240ff48e:0x240fc100},
SERVER:{
0x221e79b1:0x22149710,0x221e79b6:0x221ac910,
0x221e79c2:0x22091790,0x221e79d9:0x221c2f90,
0x221e79f0:0x2209df20,0x221e7a06:0x2222eca0,
0x221e7a1d:0x22273be0,0x221e7a31:0x22283e80,
0x221e7a4a:0x22283e90,0x221e7a5d:0x22283fc0,
0x221e7a62:0x221f5570,0x221e7b00:0x221f4980,
0x2214972e:0x22146410,0x2214974b:0x2213a7b0,
0x22149759:0x22144000,0x221ac92e:0x221aa5e0,
0x221ac94b:0x221a3550,0x221ac959:0x221a8600,
0x221f499d:0x2226e3e0}}
JMPS={CLIENT:{},SERVER:{
0x221e7b30:0x221f4980,0x22149738:0x22143f50,
0x22149763:0x22143f50,0x221ac938:0x221a80c0,
0x221ac963:0x221a80c0,0x221f49c0:0x2228f400}}
VTABLE_SLOTS={
CLIENT:{0x24296c70:0x240ff3d0},
SERVER:{0x22442f00:0x221e7b00,
0x22442f18:0x221e7b30,0x22442f1c:0x221e79b0}}
CACHE={
'scene':{'instance':0x22503ee0,'filename_va':0x2242ed68,'filename':b'scene.cache\x00','format_marker':3,'close':0x221ac910},
'model_sounds':{'instance':0x224f0388,'filename_va':0x2240bc9c,'filename':b'modelsounds.cache\x00','format_marker':2,'close':0x22149710},
}
def check(pe,va,raw,label):
    if pe.at(va,len(raw))!=raw:
        raise VerificationError('Phase88 %s evidence mismatch at %#x'%(label,va))
def verify_module(pe,module,hash_check=True):
    regions={}
    for name,(va,size,digest) in REGIONS[module].items():
        if hash_check and hashlib.sha256(pe.at(va,size)).hexdigest()!=digest:
            raise VerificationError('Phase88 SHA-256 region mismatch: '+name)
        regions[name]={'address':hex(va),'size':size,'sha256':digest if hash_check else None}
    for va,raw in OPCODES[module].items():check(pe,va,bytes.fromhex(raw),'opcode')
    for va,target in CALLS[module].items():
        check(pe,va,b'\xe8'+struct.pack('<i',target-va-5),'CALL')
    for va,target in JMPS[module].items():
        check(pe,va,b'\xe9'+struct.pack('<i',target-va-5),'JMP')
    for va,target in VTABLE_SLOTS[module].items():
        check(pe,va,struct.pack('<I',target),'vtable')
    report={'module':module,'functions':regions,'opcode_anchors':len(OPCODES[module]),
            'direct_calls':len(CALLS[module]),'direct_jumps':len(JMPS[module]),
            'vtable_slots':{hex(v):hex(t) for v,t in VTABLE_SLOTS[module].items()}}
    if module==CLIENT:
        report.update({'slot_1_va':'0x240FF3D0','array_member':'this+0x04',
          'count_member':'this+0x10','reverse_system_virtual_offset':'0x34',
          'registration_cleanup_call':'0x240FF422',
          'shutdown':'three helper+virtual0x28 pairs; registry cleanup; additional callbacks; reverse systems; zero count; further helpers'})
    else:
        caches={}
        for name,item in CACHE.items():
            check(pe,item['instance']+0x34,
                  struct.pack('<II',item['filename_va'],item['format_marker']),name+' singleton')
            check(pe,item['filename_va'],item['filename'],name+' literal')
            caches[name]={'filename':item['filename'][:-1].decode('ascii'),
                'instance_va':hex(item['instance']),'name_member':'+0x34',
                'format_marker_member':'+0x38','format_marker':item['format_marker'],
                'dirty_member':'+0x3C','started_member':'+0x3D',
                'conditional_close_member':'+0x4E','close_va':hex(item['close'])}
        check(pe,0x22442a74,b'exec game.cfg\n\x00','game cfg command')
        report.update({'cache_singletons':caches,'slot_1_va':'0x221E7B00',
             'slot_7_va':'0x221E7B30','slot_8_va':'0x221E79B0',
             'slot_1_command':'exec game.cfg\n',
             'cache_close_order':['modelsounds.cache','scene.cache'],
             'slot_8_virtual_0x28_dispatch_count':5,
             'slot_8_optional_object_global':'0x2254B888',
             'slot_8_optional_release_virtual_offset':'0x00',
             'slot_8_optional_release_argument':1,
             'slot_7_global_reset_va':'0x221F4980'})
    report['scope']='Exact original x86 state flow. No complete cache parser, DLL recompile or native Android engine ABI.'
    return report
def audit(client,server):
    previous_audit(client,server)
    return {'client':verify_module(PE32(client),CLIENT),
            'server':verify_module(PE32(server),SERVER)}
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--client',required=True,type=Path)
    p.add_argument('--server',required=True,type=Path)
    p.add_argument('--output',required=True,type=Path)
    a=p.parse_args()
    report=audit(a.client.read_bytes(),a.server.read_bytes())
    a.output.parent.mkdir(parents=True,exist_ok=True)
    a.output.write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print(json.dumps({m:{'regions':len(report[m]['functions']),
             'opcodes':report[m]['opcode_anchors'],'calls':report[m]['direct_calls']}
             for m in (CLIENT,SERVER)},indent=2))
if __name__=='__main__':main()
