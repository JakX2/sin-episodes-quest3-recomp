#!/usr/bin/env python3
"""Phase596: rigorously triage original-free Phase595 candidate address reports.

No PE/DLL bytes are required. This program never equates offset equality with
class ownership; it exports candidate priorities and known provenance only.
"""
import argparse
import collections
import json
from pathlib import Path

ORIGINAL_SERVER_SHA = '568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650'
GATE_SHA = '969cc5659f53449812b567734994fbb656edc52fabffaa810dd15c0152f8f5a2'
GATE_CALLS_VERIFIED = {0x2235C282, 0x2235C436}  # Phase592 E8 direct edges
VERIFIED_METHOD_SITES = {
    0x2202D93E: ('Phase592', 'optional-component event gate; true owner +0xC18 pointer read'),
    0x2235A375: ('Phase587', 'Grunt activity method +0x338; owner +0xC18 pointer read'),
    0x2236473F: ('Phase588', 'Jetpack ACT remapper; attached entity +0xC18 vector (not owner component pointer)'),
}
HIGH_PRIORITY_WRITES = (0x223527AC, 0x22353C3C, 0x22353D4C)
GRUNT_NEARBY_READS = (0x223596E9, 0x22359FC7, 0x2235A0CA, 0x2235A375,
                      0x2235AABC, 0x2235DBA3, 0x2235DF97)

def require_hex(value):
    if not isinstance(value, str) or not value.lower().startswith('0x'):
        raise ValueError('Expected original virtual address as 0xHEX')
    parsed = int(value, 16)
    if parsed < 0x22000000 or parsed >= 0x23000000:
        raise ValueError('Unexpected non-SiN code address: %s' % value)
    return parsed

def triage(data):
    if data.get('status') != 'CANDIDATE_SITES_ONLY':
        raise ValueError('Not a Phase595 candidate scan')
    if data.get('server_sha256') != ORIGINAL_SERVER_SHA:
        raise ValueError('Retail server SHA not authenticated')
    if data.get('authenticated_gate') != '0x2202D930' or data.get('authenticated_gate_sha256') != GATE_SHA:
        raise ValueError('Previously verified gate method changed')
    if data.get('x86_instruction_boundaries_proven') is not False:
        raise ValueError('This triage requires unresolved instruction boundaries')
    if data.get('owner_class_identity_proven') is not False:
        raise ValueError('Unexpected owner identity claim')
    if data.get('virtual_plus_0x60_implementation_proven') is not False:
        raise ValueError('Unexpected handler implementation claim')
    sites = data.get('candidate_member_uses')
    calls = data.get('candidate_gate_e8_callers')
    if not isinstance(sites, list) or not isinstance(calls, list):
        raise ValueError('Missing candidate arrays')
    addresses = [require_hex(s['candidate_va']) for s in sites]
    parsed_calls = [require_hex(c) for c in calls]
    if len(set(addresses)) != len(addresses) or len(set(parsed_calls)) != len(parsed_calls):
        raise ValueError('Duplicated candidate addresses')
    writes = [x for x in sites if x.get('could_assign_pointer') is True]
    if len(sites) != data.get('candidate_count') or len(writes) != data.get('candidate_pointer_assignments'):
        raise ValueError('Reported counts do not match actual data')
    addr_to_site = dict(zip(addresses, sites))

    def summarize_region(name, start, stop):
        items = [x for va, x in zip(addresses, sites) if start <= va < stop]
        return {'label': name, 'start_va': '0x%08X' % start,
                'end_exclusive_va': '0x%08X' % stop,
                'candidate_uses': len(items),
                'candidate_writes': sum(x.get('could_assign_pointer') is True for x in items)}

    def describe(va, label, evidence):
        hit = addr_to_site.get(va)
        return {
            'va': '0x%08X' % va,
            'in_uploaded_phase595_scan': hit is not None,
            'opcode_class': None if hit is None else hit['opcode_class'],
            'register': None if hit is None else hit['base_register'],
            'reason': label,
            'confidence': evidence,
        }

    gate_calls = []
    for va in parsed_calls:
        gate_calls.append({'va': '0x%08X' % va,
                           'source': 'Phase592 E8 verified' if va in GATE_CALLS_VERIFIED else 'Phase595 byte-pattern candidate, Ghidra confirmation needed'})

    ranked = []
    for va in HIGH_PRIORITY_WRITES:
        ranked.append(describe(va, 'candidate +0xC18 store near Grunt/Elite code; constructor/owner not proven', 'candidate only'))
    for va in GRUNT_NEARBY_READS:
        ranked.append(describe(va, 'candidate +0xC18 read near Grunt behavior; check actual receiver',
                               'verified original Grunt method' if va == 0x2235A375 else 'candidate only'))
    for va, (phase, reason) in VERIFIED_METHOD_SITES.items():
        if va not in GRUNT_NEARBY_READS:
            ranked.append(describe(va, reason, phase + ' method-proven'))
    if any(not x['in_uploaded_phase595_scan'] for x in ranked):
        raise ValueError('Expected anchored site absent from uploaded scan')
    return {
        'status': 'TRIAGE_ONLY_NO_NEW_NATIVE_METHODS',
        'matched_original_server': True,
        'matched_client': data.get('matched_client_sha256') is not None,
        'phase595_total_candidates': len(sites),
        'phase595_potential_writes': len(writes),
        'opcode_breakdown': dict(sorted(collections.Counter(s['opcode_class'] for s in sites).items())),
        'base_register_breakdown': dict(sorted(collections.Counter(s['base_register'] for s in sites).items())),
        'direct_gate_caller_candidates': gate_calls,
        'regions': [summarize_region(*item) for item in (
            ('native optional gate', 0x2202D930, 0x2202D978),
            ('near Elite Grunt', 0x22352000, 0x22356000),
            ('near inherited Grunt activity methods', 0x22359000, 0x2235E000),
            ('Jetpack activity and nearby methods', 0x22360000, 0x22370000),
            ('later NPC/attachment neighborhood', 0x22370000, 0x22390000),
        )],
        'prioritized_candidate_sites': ranked,
        'caveat': 'Opcode-pattern matches are not necessarily instruction boundaries. +0xC18 is reused across unrelated classes.',
        'component_class_proven': False,
        'virtual_plus_0x60_target_proven': False,
        'proprietary_bytes_exported': False,
    }

def synthetic_test():
    base = {'status': 'CANDIDATE_SITES_ONLY', 'server_sha256': ORIGINAL_SERVER_SHA,
            'authenticated_gate': '0x2202D930', 'authenticated_gate_sha256': GATE_SHA,
            'x86_instruction_boundaries_proven': False, 'owner_class_identity_proven': False,
            'virtual_plus_0x60_implementation_proven': False,
            'candidate_gate_e8_callers': ['0x2235C282', '0x2235C436', '0x2208C00E']}
    needed = sorted(set(HIGH_PRIORITY_WRITES) | set(GRUNT_NEARBY_READS) | set(VERIFIED_METHOD_SITES))
    base['candidate_member_uses'] = [
        {'candidate_va': '0x%08X' % va, 'opcode_class': 'write-immediate' if va in HIGH_PRIORITY_WRITES else 'read-register',
         'base_register': 'esi', 'could_assign_pointer': va in HIGH_PRIORITY_WRITES} for va in needed]
    base['candidate_count'] = len(needed)
    base['candidate_pointer_assignments'] = len(HIGH_PRIORITY_WRITES)
    v = triage(base)
    assert v['phase595_total_candidates'] == len(needed)
    assert v['direct_gate_caller_candidates'][-1]['source'].startswith('Phase595')
    assert any(x['va'] == '0x2235A375' and x['confidence'] == 'verified original Grunt method' for x in v['prioritized_candidate_sites'])
    bad = dict(base, server_sha256='x')
    try: triage(bad)
    except ValueError: pass
    else: raise AssertionError('accepted mismatched original SHA')
    bad = dict(base, candidate_count=123)
    try: triage(bad)
    except ValueError: pass
    else: raise AssertionError('accepted inconsistent candidate count')
    print('PHASE596_TRIAGE_ORIGINAL_FREE_SYNTHETIC_PASS')

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--input', type=Path, help='Phase595 candidate JSON from local retail original')
    p.add_argument('--out', type=Path, help='Output addresses-only ranked summary JSON')
    p.add_argument('--self-test', action='store_true')
    a = p.parse_args()
    if a.self_test: synthetic_test()
    if a.input is None:
        if a.self_test: return
        p.error('Need --input or --self-test')
    result = triage(json.loads(a.input.read_text(encoding='utf-8')))
    output = json.dumps(result, indent=2, sort_keys=False) + '\n'
    if a.out:
        a.out.write_text(output, encoding='utf-8')
        print('Wrote triage:', a.out)
    else:
        print(output)

if __name__ == '__main__':
    main()
