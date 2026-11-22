#!/usr/bin/env python3
"""Phase601: authenticate complete original 2006 Grunt animation event handlers.

Original owner-private DLL bytes are used only for read-only SHA, native slot,
callsite and dispatch-table validation. No game code bytes are exported.
"""
import argparse
import hashlib
import json
import struct
import zipfile
from pathlib import Path

SERVER='568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650'
CLIENT='6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d'
GATE=(0x2202D930,72,'969cc5659f53449812b567734994fbb656edc52fabffaa810dd15c0152f8f5a2')
FALLBACK=(0x220CD2F0,7,'1823d20cb51f1dda02bb824b26fb3f0b5d377c30ac6c844f2419d5cd71db5186')
# MSVC RTTI-backed 2006 vtable, actual event method start, WHOLE native
# extent including in-method dispatch tables, whole-span SHA256, default branch.
BEHAVIORS=(
 ('CAI_RappelBehavior',0x223F956C,0x220DEA60,387,'eff73b12af6f7b712a89cc88d56a1f27fd12cddb9e8b22cc6b2dbe68a1ec3446',0x220DEAA7,'E8',None),
 ('CAI_FuncTankBehavior',0x224ABEC4,0x22392970,1128,'fed2eda6f89634f075fd61cda6189a04f0dec3675c7a64bcf98ced101f3cdf3b',0x22392DA9,'E8',(0x22392DBC,7)),
 ('CAI_TakecoverBehavior',0x224AE624,0x22399650,840,'db6b11082b545915b8cc97007056bed4fe71a5f362731d00cfa1f25f63545d29',0x2239996D,'E8',(0x22399980,6)),
 ('CAI_BeserkBehavior',0x224AB88C,0x22391C90,5,'d9e092b8790db44da7e09f1e51e2fbc250ed845d6cdd63e39bb92d8a7a820932',0x22391C90,'E9',None),
 ('CAI_ActBusyBehavior',0x224AB494,0x2238EB40,1460,'7de85ad368cc51b040842a38460eeca8c56a3f10e94b3e16cdd1f542d074d09e',0x2238F0CE,'E8',(0x2238F0DC,6)),
 ('CAI_AssaultBehavior',0x223F5544,0x220CF190,1492,'7b91ac5bca99404555e57dc9aa7de45850fc0fd5c0934aab996d3e172c03057a',0x220CF73B,'E8',(0x220CF74C,6)),
 ('CGruntStandoffBehavior',0x22495344,0x220E0290,738,'dd6a9103d73cd60fa0c0ab87219dd860f08a9a02467d5ecb382dfe793953b5cc',0x220E0565,'E8',None),
 # Follow ends at 0x220D773C. 0x220D7740 is SEPARATE code after 4 INT3.
 ('CAI_FollowBehavior',0x223F6B7C,0x220D6FF0,1868,'c1a882ec7bf3e7e506a162eb7f1507776fdcab066a3055112550e62717ee5491',0x220D7713,'E8',(0x220D7724,6)),
)
# Compact instruction-site witnesses for event ID filters, fallback branch and
# byte +8 marker (NOT a bulk disassembly of original machine code).
WITNESSES={
 0x220DEA6A:'3da0860100',0x220DEA7B:'83f80d',0x220DEA7E:'7524',
 0x22392983:'056079feff',0x22392988:'83f806',0x2239298C:'0f8714040000',
 0x22399662:'056079feff',0x22399667:'83f805',0x2239966B:'0f87f9020000',
 0x2238EB4C:'056079feff',0x2238EB51:'83f805',0x2238EB55:'0f8770050000',
 0x220CF19D:'3da5860100',0x220CF1AF:'3da2860100',
 0x220CF1C0:'83e843',0x220CF1C3:'0f846f050000',
 0x220CF1C9:'2d5d860100',0x220CF1D3:'0f855f050000',
 0x220E029B:'83f843',0x220E02A6:'83f862',0x220E02A9:'0f85b3020000',
 0x220D7001:'3da4860100',0x220D7014:'3da1860100',
 0x220D7025:'83f843',0x220D702E:'83f862',
 0x220D7033:'3da0860100',0x220D7038:'0f85d2060000',
 0x220CD2F0:'c6410800c20400',
 0x2202D94D:'c6460801',0x2202D951:'ff5060',0x2202D954:'807e0800',
}
class Refusal(ValueError):pass
def require(ok,message):
    if not ok:raise Refusal(message)

class PE32:
    def __init__(self,raw):
        require(len(raw)>0x100 and raw[:2]==b'MZ','MZ signature')
        pe=struct.unpack_from('<I',raw,0x3c)[0]
        require(pe+24<len(raw) and raw[pe:pe+4]==b'PE\x00\x00','PE signature')
        opt=pe+24
        require(struct.unpack_from('<H',raw,pe+4)[0]==0x14C and
                struct.unpack_from('<H',raw,opt)[0]==0x10B,'Expected x86 PE32')
        self.base=struct.unpack_from('<I',raw,opt+28)[0]
        require(self.base==0x22000000,'Original preferred image base')
        self.raw=raw
        self.sections=[]
        end=opt+struct.unpack_from('<H',raw,pe+20)[0]
        for i in range(struct.unpack_from('<H',raw,pe+6)[0]):
            off=end+40*i
            require(off+40<=len(raw),'PE truncated section header')
            name=raw[off:off+8].split(b'\x00')[0].decode('ascii','replace')
            vsize,rva,rsize,foff=struct.unpack_from('<IIII',raw,off+8)
            require(foff+rsize<=len(raw),'PE truncated section data')
            self.sections.append((name,self.base+rva,min(vsize or rsize,rsize),foff))
    def read(self,va,n):
        for name,base,length,off in self.sections:
            if base<=va and va+n<=base+length:
                return self.raw[off+va-base:off+va-base+n]
        raise Refusal('Unmapped original VA: '+hex(va))
    def u32(self,va):return struct.unpack('<I',self.read(va,4))[0]
    def section(self,va):
        for name,base,length,off in self.sections:
            if base<=va<base+length:return name
        return None

def check_edge(read,site,target,opcode):
    data=read(site,5)
    require(len(data)==5 and data[0]==opcode,'Wrong E8/E9 opcode '+hex(site))
    require(site+5+struct.unpack_from('<i',data,1)[0]==target,
            'Wrong native rel32 target '+hex(site))

def takes_default_for_54(name):
    """Symbolic evaluation of the independently full-hashed first-level
    partition, not an emulator or claim about arbitrary other event IDs.
    """
    event=0x54
    if name=='CAI_RappelBehavior':
        return event!=0x0d and event not in (0x186a0,0x186a1,0x186a2)
    if name in ('CAI_FuncTankBehavior','CAI_TakecoverBehavior','CAI_ActBusyBehavior'):
        switch_count=7 if name=='CAI_FuncTankBehavior' else 6
        return ((event-0x186a0)&0xffffffff)>=switch_count
    if name=='CAI_AssaultBehavior':
        return event!=0x43 and not 0x186a0<=event<=0x186a9
    if name=='CGruntStandoffBehavior':return event not in (0x43,0x62)
    if name=='CAI_FollowBehavior':return event not in (0x43,0x62) and not 0x186a0<=event<=0x186aa
    if name=='CAI_BeserkBehavior':return True
    raise Refusal('Unknown behavior')

def audit(server,client):
    require(hashlib.sha256(server).hexdigest()==SERVER,'Wrong original server.dll SHA')
    require(hashlib.sha256(client).hexdigest()==CLIENT,'Wrong original client.dll SHA')
    pe=PE32(server)
    for addr,length,sha in (GATE,FALLBACK):
        require(pe.section(addr)=='.text','Original helper outside .text')
        require(hashlib.sha256(pe.read(addr,length)).hexdigest()==sha,
                'Complete original helper differs '+hex(addr))
        require(pe.read(addr+length,4)==b'\xcc'*4,
                'Method boundary/INT3 changed '+hex(addr))
    require(pe.u32(0x2249AAA4+0x444)==0x2235BFD0,
            'Original Grunt event slot +0x444 changed')
    rows=[]
    table_count=0
    for name,vt,start,n,sha,callsite,kind,table in BEHAVIORS:
        require(pe.section(vt)=='.rdata' and pe.u32(vt+0x60)==start,
                'Original class event virtual slot differs '+name)
        require(pe.section(start)=='.text','Method not executable '+name)
        require(hashlib.sha256(pe.read(start,n)).hexdigest()==sha,
                'Complete original event method SHA mismatch '+name)
        require(pe.read(start+n,4)==b'\xcc'*4,
                'Wrong next method/padding boundary '+name)
        check_edge(pe.read,callsite,FALLBACK[0],0xE8 if kind=='E8' else 0xE9)
        count=0
        if table is not None:
            first,count=table
            require(start<=first and first+4*count<=start+n,'Table outside method')
            targets=[pe.u32(first+4*i) for i in range(count)]
            require(all(start<=t<start+n for t in targets),
                    'Original case points outside method '+name)
        table_count+=count
        require(takes_default_for_54(name),'0x54 does not default '+name)
        rows.append({
            'name':name,'event_native_entry':'0x%08X'%start,
            'whole_method_length':n,'whole_method_sha256':sha,
            'default_original_callsite':'0x%08X'%callsite,
            'call_or_tailjump':kind,'verified_dispatch_entries':count,
            'event_0x54_reaches_shared_fallback':True,
        })
    for site,hexbytes in WITNESSES.items():
        raw=bytes.fromhex(hexbytes)
        require(pe.read(site,len(raw))==raw,
                'Original event partition/marker opcode witness changed '+hex(site))
    return {
        'status':'PHASE601_ORIGINAL_GRUNT_BEHAVIOR_EVENT54_NATIVE_AUTHENTICATED',
        'server_sha256':SERVER,'client_sha256':CLIENT,
        'new_complete_original_methods':len(BEHAVIORS)+1,
        'new_complete_original_method_bytes':sum(x[3] for x in BEHAVIORS)+FALLBACK[1],
        'verified_original_function_boundaries_with_int3':len(BEHAVIORS)+1,
        'native_2006_direct_fallback_e8_edges':7,
        'native_2006_tailjump_e9_edges':1,
        'opcode_field_witnesses':len(WITNESSES),
        'switch_pointer_entries_authenticated':table_count,
        'shared_original_marker_clear_entry':'0x%08X'%FALLBACK[0],
        'shared_marker_clear_length':FALLBACK[1],
        'shared_marker_clear_sha256':FALLBACK[2],
        'event_0x54_for_any_of_eight_registered_behaviors_falls_back':True,
        'current_behavior_only_eight_registered_types_proven':False,
        'original_animation_asset_emitting_0x54_proven':False,
        'original_event_handlers':rows,
        'proprietary_machine_code_exported':False,
    }

def selftest():
    class Synthetic:
        def __init__(self,site,raw):self.site,self.raw=site,raw
        def read(self,site,n):
            require(site==self.site and n==5,'Synthetic bad read')
            return self.raw
    mutations=0
    for name,vt,start,n,sha,site,kind,table in BEHAVIORS:
        op=0xE8 if kind=='E8' else 0xE9
        raw=bytes([op])+struct.pack('<i',FALLBACK[0]-site-5)
        check_edge(Synthetic(site,raw).read,site,FALLBACK[0],op)
        for i in range(5):
            changed=bytearray(raw);changed[i]^=1
            try:check_edge(Synthetic(site,bytes(changed)).read,site,FALLBACK[0],op)
            except Refusal:mutations+=1
            else:raise AssertionError('Accepted corrupt original rel32 encoding')
        assert takes_default_for_54(name)
    assert mutations==40
    assert sum(x[7][1] if x[7] else 0 for x in BEHAVIORS)==31
    try:audit(b'fake',b'fake')
    except Refusal:pass
    else:raise AssertionError('Incorrect game DLLs were accepted')
    print('PHASE601_ORIGINAL_FREE_EVENT54_SYNTHETIC_PASS (40 mutated branches refused)')

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--bin-zip',type=Path)
    p.add_argument('--server',type=Path)
    p.add_argument('--client',type=Path)
    p.add_argument('--out',type=Path)
    p.add_argument('--self-test',action='store_true')
    a=p.parse_args()
    if a.self_test:
        selftest()
        if a.bin_zip is None and a.server is None:return
    if a.bin_zip is None and (a.server is None or a.client is None):
        p.error('Use --self-test, --bin-zip or paired --server/--client')
    try:
        if a.bin_zip is not None:
            with zipfile.ZipFile(a.bin_zip) as z:
                result=audit(z.read('server.dll'),z.read('client.dll'))
        else:result=audit(a.server.read_bytes(),a.client.read_bytes())
    except (Refusal,OSError,KeyError,zipfile.BadZipFile,struct.error) as exc:
        p.exit(2,'PHASE601 SOURCE AUTHENTICATION REFUSED: %s\n'%exc)
    data=json.dumps(result,indent=2)+'\n'
    if a.out:
        a.out.write_text(data,encoding='utf-8')
        print('Authenticated Phase601 original gameplay metadata:',a.out)
    else:print(data)

if __name__=='__main__':main()
