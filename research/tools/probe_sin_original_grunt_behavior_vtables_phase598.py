#!/usr/bin/env python3
"""Phase598: SHA-locked, read-only SiN 2006 Grunt behavior vtable slot explorer.

Enumerates +0x60/+0x7C virtual pointers for eight independently RTTI-verified
original Grunt behaviors (Phase535). No commercial bytes/disassembly exported.
These virtual slot semantics are candidate attributions: Phase587/592 original
Grunt calls them on owner+0xC18, but Phase597's identification of that pointer
as the active behavior still awaits native proof.
"""
import argparse
import hashlib
import json
import struct
import zipfile
from collections import defaultdict
from pathlib import Path

SERVER_SHA = "568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650"
CLIENT_SHA = "6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d"
GATE_SHA = "969cc5659f53449812b567734994fbb656edc52fabffaa810dd15c0152f8f5a2"
SPECS = (
    ("CAI_RappelBehavior",0xCFC,0x223F956C,".?AVCAI_RappelBehavior@@"),
    ("CAI_FuncTankBehavior",0xCD8,0x224ABEC4,".?AVCAI_FuncTankBehavior@@"),
    ("CAI_TakecoverBehavior",0xD20,0x224AE624,".?AVCAI_TakecoverBehavior@@"),
    ("CAI_BeserkBehavior",0xDA0,0x224AB88C,".?AVCAI_BeserkBehavior@@"),
    ("CAI_ActBusyBehavior",0xF88,0x224AB494,".?AVCAI_ActBusyBehavior@@"),
    ("CAI_AssaultBehavior",0xDB8,0x223F5544,".?AVCAI_AssaultBehavior@@"),
    ("CGruntStandoffBehavior",0xDE8,0x22495344,".?AVCGruntStandoffBehavior@CNPC_GruntBase@@"),
    ("CAI_FollowBehavior",0xEB8,0x223F6B7C,".?AVCAI_FollowBehavior@@"),
)
class Refusal(ValueError):
    pass

def check(test, message):
    if not test:
        raise Refusal(message)

class PE32:
    def __init__(self, data):
        check(len(data)>=0x100 and data[:2]==b'MZ',"MZ header missing")
        off=struct.unpack_from("<I",data,0x3C)[0]
        check(off+24<=len(data) and data[off:off+4]==b'PE\x00\x00',"PE signature missing")
        machine,n=struct.unpack_from("<HH",data,off+4)
        option_size=struct.unpack_from("<H",data,off+20)[0]
        optional=off+24
        check(machine==0x14C and optional+32<=len(data) and
              struct.unpack_from("<H",data,optional)[0]==0x10B,"Requires x86 PE32")
        self.base=struct.unpack_from("<I",data,optional+28)[0]
        self.data=data
        self.sections=[]
        start=optional+option_size
        for i in range(n):
            pos=start+40*i
            check(pos+40<=len(data),"Truncated PE section table")
            name=data[pos:pos+8].split(b'\x00')[0].decode('ascii','replace')
            virtual,relative,size,file_off=struct.unpack_from("<IIII",data,pos+8)
            check(file_off+size<=len(data),"PE section truncated")
            self.sections.append((name,self.base+relative,min(virtual or size,size),file_off))
    def section(self,addr,length=1):
        for name,start,size,off in self.sections:
            if start<=addr and addr+length<=start+size:return name
        return None
    def read(self,addr,length):
        check(length>=0,"Negative read length")
        for name,start,size,off in self.sections:
            if start<=addr and addr+length<=start+size:
                return self.data[off+addr-start:off+addr-start+length]
        raise Refusal("Unmapped original address 0x%08X"%addr)
    def u32(self,addr):
        return struct.unpack("<I",self.read(addr,4))[0]

def verify_rtti(pe,vt,name):
    check(pe.section(vt,0x80)=='.rdata',"Vtable outside .rdata")
    col=pe.u32(vt-4)
    check(pe.section(col,0x10)=='.rdata',"RTTI COL missing at 0x%08X"%vt)
    descriptor=pe.u32(col+0xC)
    check(pe.section(descriptor+8,len(name)+1) in ('.data','.rdata'),
          "RTTI type descriptor out of data")
    check(pe.read(descriptor+8,len(name)+1)==name.encode('ascii')+b'\x00',
          "Wrong original RTTI type for 0x%08X"%vt)

def inspect(pe):
    check(pe.base==0x22000000,"Unexpected original server.dll preferred base")
    check(hashlib.sha256(pe.read(0x2202D930,72)).hexdigest()==GATE_SHA,
          "The Phase592 original event gate has changed")
    check(pe.u32(0x2249AAA4+0x444)==0x2235BFD0,"Grunt parent event override changed")
    behaviors=[]
    for name,member,vt,rtti in SPECS:
        verify_rtti(pe,vt,rtti)
        check(pe.u32(vt+4)==0x220CDBD0,"Original behavior owner binder changed: "+name)
        event=pe.u32(vt+0x60)
        activity=pe.u32(vt+0x7C)
        behaviors.append({
          'class_name':name,
          'owner_embedded_offset':'0x%03X'%member,
          'original_vtable':'0x%08X'%vt,
          'rtti_authenticated':True,
          'owner_binding_slot_verified':True,
          'candidate_event_slot_plus_0x60':'0x%08X'%event,
          'candidate_event_target_section':pe.section(event),
          'candidate_activity_slot_plus_0x7C':'0x%08X'%activity,
          'candidate_activity_target_section':pe.section(activity),
          'active_at_event_0x54_proven':False
        })
    return behaviors

def grouped(behaviors):
    groups=defaultdict(list)
    for item in behaviors:
        for slot,key in (('event+0x60','candidate_event_slot_plus_0x60'),
                         ('activity+0x7C','candidate_activity_slot_plus_0x7C')):
            groups[(slot,item[key])].append(item['class_name'])
    return [{'slot':slot,'target_address':address,'behavior_classes':names,
             'class_count':len(names)} for (slot,address),names in sorted(groups.items())]

def analyze(server,client=None):
    check(hashlib.sha256(server).hexdigest()==SERVER_SHA,
          "Unsupported original server.dll full-file SHA256")
    if client is not None:
        check(hashlib.sha256(client).hexdigest()==CLIENT_SHA,
              "Unsupported original client.dll full-file SHA256")
    behaviors=inspect(PE32(server))
    return {
      'status':'PHASE598_EIGHT_ORIGINAL_GRUNT_BEHAVIOR_VIRTUAL_TARGETS',
      'original_server_sha256':SERVER_SHA,
      'original_client_sha256_verified':CLIENT_SHA if client is not None else None,
      'phase535_behavior_rtti_vtables_checked':len(behaviors),
      'candidate_original_vtable_pointers_read':len(behaviors)*2,
      'method_boundaries_authenticated':False,
      'owner_plus_0xC18_active_behavior_identity_proven':False,
      'component_plus_0x08_marker_semantic_name_proven':False,
      'event_0x54_handler_behavior_proven':False,
      'behavior_vtables':behaviors,
      'grouped_original_virtual_targets':grouped(behaviors),
      'proprietary_code_bytes_exported':False
    }

def self_test():
    assert len(SPECS)==8 and len(set(vt for name,offset,vt,rtti in SPECS))==8
    assert [spec[1] for spec in SPECS]==[0xCFC,0xCD8,0xD20,0xDA0,0xF88,0xDB8,0xDE8,0xEB8]
    fake=[
        {'class_name':'A','candidate_event_slot_plus_0x60':'0x22001000',
         'candidate_activity_slot_plus_0x7C':'0x22001200'},
        {'class_name':'B','candidate_event_slot_plus_0x60':'0x22001000',
         'candidate_activity_slot_plus_0x7C':'0x22001400'},
    ]
    assert any(r['slot']=='event+0x60' and r['class_count']==2 for r in grouped(fake))
    try: analyze(b'bad')
    except Refusal: pass
    else: raise AssertionError('Corrupt original DLL was accepted')
    # Synthetic section mapping and RTTI COL, not taken from original retail.
    data=bytearray(0x8000)
    data[:2]=b'MZ';struct.pack_into('<I',data,0x3C,0x80)
    data[0x80:0x84]=b'PE\x00\x00'
    struct.pack_into('<HH',data,0x84,0x14C,1)
    struct.pack_into('<H',data,0x94,0xE0)
    struct.pack_into('<H',data,0x98,0x10B)
    struct.pack_into('<I',data,0x98+28,0x22000000)
    sh=0x98+0xE0
    data[sh:sh+6]=b'.rdata'
    struct.pack_into('<IIII',data,sh+8,0x4000,0x1000,0x4000,0x200)
    def put(va,raw):data[0x200+(va-0x22001000):0x200+(va-0x22001000)+len(raw)]=raw
    vt,col,td=0x22001100,0x22002000,0x22002100
    put(vt-4,struct.pack('<I',col))
    put(col+0xC,struct.pack('<I',td))
    put(td+8,b'.?AVTestBehavior@@\x00')
    pe=PE32(bytes(data))
    verify_rtti(pe,vt,'.?AVTestBehavior@@')
    try:verify_rtti(pe,vt,'.?AVNotTest@@')
    except Refusal:pass
    else:raise AssertionError('Wrong RTTI type was accepted')
    print('PHASE598_ORIGINAL_FREE_EIGHT_BEHAVIOR_VTABLE_SYNTHETIC_PASS')

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    group=ap.add_mutually_exclusive_group()
    group.add_argument('--server',type=Path)
    group.add_argument('--bin-zip',type=Path)
    ap.add_argument('--client',type=Path)
    ap.add_argument('--out',type=Path,default=Path('grunt_behavior_phase598_targets.json'))
    ap.add_argument('--self-test',action='store_true')
    args=ap.parse_args()
    if args.self_test:
        self_test()
        if args.server is None and args.bin_zip is None:return
    if args.server is None and args.bin_zip is None:
        ap.error('Use --server, --bin-zip, or --self-test')
    if args.client is not None and args.bin_zip is not None:
        ap.error('Cannot use --client with --bin-zip')
    try:
        if args.bin_zip is not None:
            with zipfile.ZipFile(args.bin_zip) as z:
                server,client=z.read('server.dll'),z.read('client.dll')
        else:
            server=args.server.read_bytes()
            client=args.client.read_bytes() if args.client is not None else None
        result=analyze(server,client)
        args.out.write_text(json.dumps(result,indent=2)+'\n',encoding='utf8')
    except (Refusal,OSError,KeyError,zipfile.BadZipFile,struct.error) as ex:
        ap.exit(2,'PHASE598 ORIGINAL EVIDENCE REFUSED: %s\n'%ex)
    print('Phase598 verified 8 original behavior vtables and 16 native slot pointers')
    print('Wrote address-only report:',args.out)

if __name__=='__main__':main()
