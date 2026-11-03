#!/usr/bin/env python3
"""Verify original SiN Episodes' 14 named difficulty metrics and modifier math.

For private owner-held 2006 x86 PE32 client.dll/server.dll. The prerequisite
Phase 77 verifier pins original SHA-256 and checks original archive calls.
No original executable byte ranges are emitted by the report.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import struct

from audit_sin_manager_state import audit as original_archive_audit
from audit_sin_manager_state import PEImage, VerificationError

METRIC_NAMES=(
    'DAMAGE_TAKEN_SCALE','DAMAGE_DONE_SCALE','AMMO_QUANTITY_SCALE',
    'NUM_FORCED_HEALTH_KITS','PERCENT_HEALTH_KITS_DROP',
    'PERCENT_NPC_2ND_FIRE','PERCENT_NPC_MAKE_DIFFERENT',
    'PERCENT_NPC_CHANGE_SPAWN_NUM','PERCENT_NPC_WEARS_HELMET',
    'PERCENT_NPC_HELMET_SHOT_OFF','PERCENT_NPC_TAKES_COVER',
    'RANDOM_NPC_WAIT_BEFORE_SHOOT','PERCENT_NPC_ACCURATE',
    'PERCENT_NPC_MOBILE'
)
METRIC_PTR_TABLE=0x2245F04C
METRIC_DATA_BASE=0x22523478
MODIFIER_DATA_BASE=0x22521DE0
REGION_HASHES={
    (0x222d3490,181):'d4fd1d9de9e127ffd591c1436b62b791600b9bd874e6b07b51129822840d41da',
    (0x222d3370,98):'c7b66f642d54c3c951cc1522d974a58f02f5fb25f972b7346b167415b026b83e',
    (0x222d33e0,21):'38aa064ed75ca9b3f73f1a410f40f1c01182184eec3414ffdf2cb67e71ed3635',
    (0x222c07a0,26):'258bba31dd9f2eaf3bcf4d9ceab9225f7939fe62713683086c0f8abe7f6d7619',
    (0x222c07c0,76):'4577657594fa9eb10f4ddeb187fec7e490a0c9b36606a9de4c0458dbc421f844',
    (0x222c0810,80):'4103623924cac19a59a04bab0e0458c7a375275d6472946de127ef6776d97d43',
    (METRIC_PTR_TABLE,56):'a870baf7cbac30181833e0dfea0685de092f780d6074613330935abbc706cf2d',
    (METRIC_DATA_BASE,392):'9141ac4e8c37962e20289938cc10094ff6e232d27c6e5ef21c9bab4bde8df262',
    (MODIFIER_DATA_BASE,160):'a834ffb02981aa1f080d8686bd1f1f8f571e895ef8a6deef865cf90023a76e19',
}
# Small individual instruction checks independent of region SHA pins. These
# are also tested against mutation-only synthetic fixtures, not game files.
OPCODES={
    0x222d33c0:'83ff0e', # lookup scans precisely 14 names
    0x222d3490:'d905f8223d22', # FLD 1.0
    0x222d3496:'b880345222', # 14x28-byte data table iterator
    0x222d349b:'d8642404', # 1.0 - input t
    0x222d34a2:'05c4000000', # 7 metrics per unrolled loop
    0x222d34a7:'3d08365222', # end 14 metrics
    0x222d34ac:'d88838ffffff', # fmul input field +4
    0x222d34b2:'d9442404', # t
    0x222d34b6:'d8883cffffff', # fmul input field +8
    0x222d34bc:'dec1', # add two weighted values
    0x222d34be:'d99834ffffff', # store computed field +0
    0x222d353c:'0f8c5effffff', # repeat loop
    0x222c07a4:'c1e004', # 16-byte modifier stride
    0x222c07a7:'05e01d5222', # modifier table
    0x222c07ac:'837c240802', # split selector at 2
    0x222c07b3:'d900', # selector <2: offset +0
    0x222c07b6:'d94008', # >=2: offset +8
    0x222c07c4:'c1e104', # stride for additive updater
    0x222c07cd:'837c240802', # selector threshold
    0x222c07db:'d801', # fadd current modifier
    0x222c07dd:'d815c09c3d22', # compare with 2.0
    0x222c07ec:'c70100000040', # upper clamp 2.0
    0x222c07f3:'d815b09c3d22', # compare with 0.05
    0x222c0802:'c701cdcc4c3d', # lower clamp 0.05
    0x222c0809:'d919', # unclamped result
    0x222c0810:'d944240c', # absolute setter float
    0x222c0814:'d81dc09c3d22', # setter upper clamp
    0x222c083a:'d905b09c3d22', # setter lower clamp
    0x222c0852:'837c240802', # setter selector threshold
    0x222c0859:'d918', # assign first modifier
    0x222c085c:'d95808', # assign second modifier
}
DIRECT_CALLS={
    0x222b9194:0x222d3490, # difficulty parameter initialization
    0x222ba3ba:0x222d3490, # manager initialization
    0x222baef8:0x222c07c0, # manager modifier change
    0x222b926c:0x222c0810, # archive modifier restore selector 1
    0x222b9279:0x222c0810, # archive modifier restore selector 3
    0x222d366f:0x222d3370, # name-based lookup consumer
    0x222d3770:0x222d3370, # second name-based lookup consumer
}
CRITICAL_FLOATS={
    (0,0):1.075,(0,1):0.75,(0,2):1.4,
    (4,0):0.4,(4,1):0.5,(4,2):0.3,
    (13,0):0.5,(13,1):0.3,(13,2):0.7,
}

def check(pe,site,raw):
    if pe.at(site,len(raw))!=raw:
        raise VerificationError('original metric opcode/data mismatch at %#x'%site)

def verify_metric_math(pe, verify_hashes=True):
    for site,hexbytes in OPCODES.items():
        check(pe,site,bytes.fromhex(hexbytes))
    for site,target in DIRECT_CALLS.items():
        check(pe,site,b'\xe8'+struct.pack('<i',target-site-5))
    for (row,col),value in CRITICAL_FLOATS.items():
        check(pe,METRIC_DATA_BASE+row*28+col*4,struct.pack('<f',value))
    names=[]
    metric_rows=[]
    for i,expected in enumerate(METRIC_NAMES):
        ptr=struct.unpack('<I',pe.at(METRIC_PTR_TABLE+i*4,4))[0]
        check(pe,ptr,expected.encode('ascii')+b'\0')
        names.append(expected)
        metric_rows.append(list(struct.unpack('<7f',pe.at(METRIC_DATA_BASE+i*28,28))))
    if verify_hashes:
        for (site,length),expected_hash in REGION_HASHES.items():
            if hashlib.sha256(pe.at(site,length)).hexdigest()!=expected_hash:
                raise VerificationError('original metric code/table SHA-256 changed at %#x'%site)
    return {
        'metric_count':14,'metric_record_stride_bytes':28,
        'metric_names':names,
        'metric_initializers_float32':metric_rows,
        'metric_initializer_table_va':'0x22523478',
        'metric_name_pointer_table_va':'0x2245F04C',
        'blend_function_va':'0x222D3490',
        'blend_equation':'current=(1-t)*field1 + t*field2; no clamp in this routine',
        'modifier_count':10,'modifier_stride_bytes':16,
        'modifier_selector_rule':'selector < 2: first float at +0; selector >=2: second at +8',
        'modifier_add_function_va':'0x222C07C0',
        'modifier_set_function_va':'0x222C0810',
        'modifier_finite_clamp_bounds':[0.05000000074505806,2.0],
        'checked_opcode_sites':len(OPCODES),
        'checked_direct_call_sites':len(DIRECT_CALLS),
        'checked_region_sha256':len(REGION_HASHES) if verify_hashes else 0,
        'limitations':'Static initializers and finite-input math only; x87 rounding, metric event propagation, source ABI and save lifecycle not recovered.'
    }

def audit(client,server):
    previous=original_archive_audit(client,server)
    previous['metric_math']=verify_metric_math(PEImage(server),True)
    return previous

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--client',required=True,type=Path)
    parser.add_argument('--server',required=True,type=Path)
    parser.add_argument('--output',required=True,type=Path)
    args=parser.parse_args()
    report=audit(args.client.read_bytes(),args.server.read_bytes())
    args.output.write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(report['metric_math'],indent=2))
if __name__=='__main__':main()
