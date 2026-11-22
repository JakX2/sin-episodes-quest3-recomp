#!/usr/bin/env python3
"""Phase600: validate and sort Phase599 original Grunt Ghidra metadata.

The output is only address/role metadata; Ghidra-estimated boundaries,
literal 0x54 sightings and member+8 operands never prove original semantics.
"""
import argparse
from copy import deepcopy
import json
from pathlib import Path

SERVER='568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650'
CLIENT='6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d'
EVENTS={
  0x220DEA60:'CAI_RappelBehavior',
  0x22392970:'CAI_FuncTankBehavior',
  0x22399650:'CAI_TakecoverBehavior',
  0x22391C90:'CAI_BeserkBehavior',
  0x2238EB40:'CAI_ActBusyBehavior',
  0x220CF190:'CAI_AssaultBehavior',
  0x220E0290:'CGruntStandoffBehavior',
  0x220D6FF0:'CAI_FollowBehavior',
}
ACTS={
  0x220CD400:('CAI_RappelBehavior','CAI_BeserkBehavior','CAI_ActBusyBehavior','CAI_AssaultBehavior'),
  0x22392700:('CAI_FuncTankBehavior',),
  0x22398670:('CAI_TakecoverBehavior',),
  0x220E1EE0:('CGruntStandoffBehavior',),
  0x220D33B0:('CAI_FollowBehavior',),
}
class Refusal(ValueError):pass

def check(ok, message):
    if not ok:raise Refusal(message)

def parse_address(s):
    check(isinstance(s,str) and s.startswith('0x'),'Expected original hex address')
    try:return int(s,16)
    except ValueError:raise Refusal('Malformed address')

def inspect(source):
    check(source.get('status')=='PHASE599_GHIDRA_ESTIMATED_NATIVE_FUNCTION_BOUNDS',
          'Expected Phase599 Ghidra original-source report')
    check(source.get('source_server_sha256')==SERVER and
          source.get('original_client_sha256_report_authenticated')==CLIENT and
          source.get('original_pe_verified_against_disk') is True,
          'Original DLL provenance mismatch')
    check(source.get('source_phase598_vtable_pointers_matched_loaded_program')==16 and
          source.get('unique_original_vtable_targets')==13,
          'Original vtable counts invalid')
    for flag in ('new_native_method_boundaries_sha_authenticated',
                 'component_plus_0x08_proven_as_override_marker',
                 'event_0x54_handling_proven','exported_proprietary_machine_code'):
        check(source.get(flag) is False,'Unsupported native certainty claim: '+flag)
    records=source.get('functions')
    check(isinstance(records,list) and len(records)==13,'Expected 13 function records')
    seen=set()
    rows=[]
    for rec in records:
        address=parse_address(rec.get('original_target'))
        check(address in EVENTS or address in ACTS,'Unknown original method pointer')
        check(address not in seen,'Duplicate original target')
        seen.add(address)
        roles=rec.get('roles')
        expected=([{'class':EVENTS[address],'slot':'event+0x60'}] if address in EVENTS else
                  [{'class':name,'slot':'activity+0x7C'} for name in ACTS[address]])
        check(roles==expected,'Incorrect Grunt behavior vtable role')
        start=rec.get('ghidra_function_starts_at_entry') is True
        digest=rec.get('ghidra_span_candidate_sha256')
        if digest is not None:
            check(start and rec.get('ghidra_contiguous_function_body') is True,
                  'Ghidra candidate hash without contiguous method start')
            check(isinstance(digest,str) and len(digest)==64 and
                  all(c in '0123456789abcdef' for c in digest),
                  'Malformed Ghidra candidate digest')
        immediate=rec.get('event_immediate_0x54_sites',[])
        marker=rec.get('component_plus_0x08_operand_candidates',[])
        check(isinstance(immediate,list) and isinstance(marker,list),'Missing opcode evidence lists')
        for x in immediate:
            check(x.get('candidate_only_not_event_proof') is True,
                  'Immediate 0x54 promoted without control-flow proof')
        for x in marker:
            check(x.get('receiver_proven') is False and x.get('write_proven') is False,
                  'Original member write/receiver asserted without source proof')
        rows.append({
          'original_target':'0x%08X'%address,
          'event_behavior_class':EVENTS.get(address),
          'activity_behavior_classes':list(ACTS.get(address,())),
          'ghidra_function_entry':start,
          'ghidra_candidate_bytes':rec.get('ghidra_span_bytes') if start else None,
          'ghidra_candidate_sha256':digest,
          'immediate_0x54_site_candidates':len(immediate),
          'member_plus_0x08_operand_candidates':len(marker),
          'original_method_boundary_proven':False,
          'original_event_0x54_consumption_proven':False,
        })
    check(seen==set(EVENTS)|set(ACTS),'Incomplete exact original target set')
    check(sum(x['ghidra_function_entry'] for x in rows)==source.get('ghidra_function_starts'),
          'Ghidra method count mismatch')
    rows.sort(key=lambda x:(x['event_behavior_class'] is None,
                             -(bool(x['immediate_0x54_site_candidates']) and
                               bool(x['member_plus_0x08_operand_candidates'])),
                             x['original_target']))
    return {
       'status':'PHASE600_GHIDRA_SOURCE_METADATA_TRIAGE_ONLY',
       'original_2006_dll_pair_sha_authenticated':True,
       'event_entries':8,
       'activity_entries':5,
       'ghidra_candidate_function_starts':source.get('ghidra_function_starts'),
       'targets':rows,
       'source_function_bodies_authenticated':0,
       'active_behavior_pointer_proven':False,
       'event_0x54_consumption_proven':False,
       'proprietary_code_bytes_exported':False,
    }

def selftest():
    base={
      'status':'PHASE599_GHIDRA_ESTIMATED_NATIVE_FUNCTION_BOUNDS',
      'source_server_sha256':SERVER,
      'original_client_sha256_report_authenticated':CLIENT,
      'original_pe_verified_against_disk':True,
      'source_phase598_vtable_pointers_matched_loaded_program':16,
      'unique_original_vtable_targets':13,
      'new_native_method_boundaries_sha_authenticated':False,
      'component_plus_0x08_proven_as_override_marker':False,
      'event_0x54_handling_proven':False,
      'exported_proprietary_machine_code':False,
      'ghidra_function_starts':13,
      'functions':[],
    }
    for a in sorted(set(EVENTS)|set(ACTS)):
        roles=([{'class':EVENTS[a],'slot':'event+0x60'}] if a in EVENTS else
               [{'class':name,'slot':'activity+0x7C'} for name in ACTS[a]])
        base['functions'].append({
          'original_target':'0x%08X'%a,'roles':roles,
          'ghidra_function_starts_at_entry':True,
          'ghidra_contiguous_function_body':True,
          'ghidra_span_candidate_sha256':'0'*64,
          'event_immediate_0x54_sites':[],
          'component_plus_0x08_operand_candidates':[],
        })
    assert len(inspect(base)['targets'])==13
    mutations=[
      lambda x:x.update(ghidra_function_starts=12),
      lambda x:x['functions'][0].update(original_target='0x22000000'),
      lambda x:x['functions'][0].update(roles=[]),
      lambda x:x['functions'][0].update(ghidra_span_candidate_sha256='bad'),
      lambda x:x['functions'][0].update(ghidra_function_starts_at_entry=False),
      lambda x:x.update(event_0x54_handling_proven=True),
      lambda x:x['functions'][0]['event_immediate_0x54_sites'].append({'candidate_only_not_event_proof':False}),
    ]
    for mutate in mutations:
        broken=deepcopy(base)
        mutate(broken)
        try:inspect(broken)
        except Refusal:pass
        else:raise AssertionError('Accepted source-unsupported mutated Ghidra report')
    print('PHASE600_GHIDRA_OUTPUT_TRIAGE_SYNTHETIC_PASS (7 mutations rejected)')

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--input',type=Path)
    p.add_argument('--out',type=Path)
    p.add_argument('--self-test',action='store_true')
    a=p.parse_args()
    if a.self_test:
        selftest()
        if not a.input:return
    if not a.input:p.error('Pass --input Phase599 Ghidra report or --self-test')
    try:result=inspect(json.loads(a.input.read_text(encoding='utf-8')))
    except (OSError,ValueError,TypeError,KeyError) as exc:
        p.exit(2,'PHASE600 OUTPUT REFUSED: %s\n'%exc)
    encoded=json.dumps(result,indent=2)+'\n'
    if a.out:
        a.out.write_text(encoded,encoding='utf-8')
        print('Wrote original-address-only triage:',a.out)
    else:print(encoded)

if __name__=='__main__':main()
