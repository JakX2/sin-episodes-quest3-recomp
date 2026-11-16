#!/usr/bin/env python3
"""Fail-closed metadata-only reconciliation of reviewed Ghidra factory seed results."""
import argparse
import csv
import json
from pathlib import Path

ALLOWED = {'already_inferred', 'seeded_candidate', 'non_executable', 'overlaps_another_inferred_function', 'unresolved_candidate'}
MODULES = ('client.dll', 'server.dll')

def read_csv(path, fields):
    with path.open(newline='', encoding='utf-8') as stream:
        reader = csv.DictReader(stream)
        if reader.fieldnames != fields:
            raise ValueError(f'{path.name}: unexpected CSV schema')
        return list(reader)

def audit(seed_dir, result_dir):
    output = {'schema': 1, 'evidence_tier': 'ghidra_inferred_or_seeded_candidates_only', 'modules': {}}
    for module in MODULES:
        seeds = read_csv(seed_dir / f'{module}_factory_seeds.csv', ['entry', 'interface', 'source'])
        results = read_csv(result_dir / f'{module}_factory_seed_results.csv', ['entry', 'interface', 'outcome', 'result_function_entry'])
        expected = {(r['entry'].lower().lstrip('0') or '0', r['interface']) for r in seeds}
        actual = {(r['entry'].lower().lstrip('0') or '0', r['interface']) for r in results}
        if len(expected) != len(seeds) or len(actual) != len(results) or expected != actual:
            raise ValueError(f'{module}: missing, duplicated or unexpected reviewed seed')
        if any(r['source'] != 'reviewed_original_static_registration' for r in seeds):
            raise ValueError(f'{module}: invalid seed provenance')
        counts = {}
        for r in results:
            outcome = r['outcome']
            if outcome not in ALLOWED and not (outcome.startswith('seed_error_') and outcome[11:].isidentifier()):
                raise ValueError(f'{module}: unknown outcome {outcome!r}')
            if outcome in ('already_inferred', 'seeded_candidate') and not r['result_function_entry']:
                raise ValueError(f'{module}: missing candidate function entry')
            if outcome not in ('already_inferred', 'seeded_candidate') and r['result_function_entry']:
                raise ValueError(f'{module}: unresolved seed has function entry')
            counts[outcome] = counts.get(outcome, 0) + 1
        output['modules'][module] = {'reviewed_seed_count': len(seeds), 'outcomes': dict(sorted(counts.items())), 'newly_verified_original_methods': 0, 'independently_engine_tested_original_methods': 0}
    return output

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--seed-dir', type=Path, required=True)
    parser.add_argument('--result-dir', type=Path, required=True)
    parser.add_argument('--out', type=Path, required=True)
    args = parser.parse_args()
    report = audit(args.seed_dir, args.result_dir)
    args.out.write_text(json.dumps(report, indent=2, sort_keys=True) + '\n', encoding='utf-8')
    print(json.dumps(report, sort_keys=True))

if __name__ == '__main__':
    main()
