#!/usr/bin/env python3
"""Phase 487: original SHA-pinned seven-file soundscape dependency DAG.

PRIVATE --scripts: read-only validate the user's retail scripts.zip and
derive node/edge/topological/expansion counts without exporting scripts.
PUBLIC --self-test: invented KeyValues only, no original game content.
This audits structural dependency edges, NOT sound level, DSP, random
selection, engine audio playback or actual map loading.
"""
import argparse
import collections
import hashlib
import json
import sys
import zipfile
from pathlib import Path
from audit_sin_original_soundscape_catalog import EXPECTED, parse
from audit_sin_original_soundscape_manifest import (
    FILES_OBSERVED, MANIFEST_PATH, inspect_manifest,
)

# These values were measured from all seven separately SHA256-verified
# original 2006 game resources, not a mock graph or later Source engine.
PINNED_GRAPH = {
    "documents": 7,
    "definitions": 301,
    "reference_occurrences": 242,
    "resolved_occurrences": 239,
    "unresolved_occurrences": 3,
    "unresolved_distinct_targets": 2,
    "distinct_resolved_targets": 83,
    "definitions_with_resolved_outgoing": 120,
    "definitions_without_resolved_outgoing": 181,
    "cross_file_resolved_occurrences": 4,
    "maximum_dependency_depth": 5,
    "maximum_single_root_expansion": 10,
    "all_roots_expansion_sum": 602,
    "maximum_resolved_outgoing_degree": 5,
    "directed_acyclic": True,
}
PINNED_DEFINITIONS_PER_FILE = (14,26,95,81,40,38,7)

def graph_inventory(named_scripts):
    definitions = {}
    references = []
    definitions_by_file = []
    for path,text in named_scripts:
        roots=parse(text)
        definitions_by_file.append(len(roots))
        for name,children in roots:
            folded=name.lower()
            if not name or folded in definitions:
                raise ValueError("missing/duplicated case-insensitive soundscape name")
            definitions[folded] = path

            def visit(nodes):
                for key,value in nodes:
                    if not isinstance(value,list):
                        continue
                    if key.lower()=="playsoundscape":
                        targets=[v for k,v in value
                                 if k.lower()=="name" and
                                 isinstance(v,str)]
                        if len(targets)==0:
                            raise ValueError("playsoundscape without a name")
                        references.append((folded,targets[0].lower()))
                    visit(value)
            visit(children)

    n=len(definitions)
    adjacency={name:[] for name in definitions}
    indegree={name:0 for name in definitions}
    seen_targets=set()
    missing=[]
    cross_file=0
    for src,tgt in references:
        if tgt not in definitions:
            missing.append((src,tgt))
            continue
        adjacency[src].append(tgt)
        indegree[tgt]+=1
        seen_targets.add(tgt)
        if definitions[src]!=definitions[tgt]:
            cross_file+=1
    queue=collections.deque(node for node in definitions if indegree[node]==0)
    order=[]
    depth={name:1 for name in definitions}
    while queue:
        node=queue.popleft()
        order.append(node)
        for target in adjacency[node]:
            depth[target]=max(depth[target],depth[node]+1)
            indegree[target]-=1
            if indegree[target]==0:
                queue.append(target)
    acyclic=len(order)==n
    report={
        "documents":len(named_scripts),
        "definitions":n,
        "definitions_by_manifest_order":definitions_by_file,
        "reference_occurrences":len(references),
        "resolved_occurrences":sum(map(len,adjacency.values())),
        "unresolved_occurrences":len(missing),
        "unresolved_distinct_targets":len({tgt for _,tgt in missing}),
        "distinct_resolved_targets":len(seen_targets),
        "definitions_with_resolved_outgoing":sum(bool(v) for v in adjacency.values()),
        "definitions_without_resolved_outgoing":sum(not v for v in adjacency.values()),
        "cross_file_resolved_occurrences":cross_file,
        "maximum_resolved_outgoing_degree":max(map(len,adjacency.values()),default=0),
        "directed_acyclic":acyclic,
    }
    if acyclic:
        # Dynamic expansion counts preserve repeated references. Reverse
        # DAG order guarantees every child's count is available first.
        expansions={}
        for node in reversed(order):
            expansions[node]=1+sum(expansions[t] for t in adjacency[node])
        report.update(
            maximum_dependency_depth=max(depth.values(),default=0),
            maximum_single_root_expansion=max(expansions.values(),default=0),
            all_roots_expansion_sum=sum(expansions.values()),
        )
    return report

def private_audit(archive):
    with zipfile.ZipFile(archive) as z:
        inspect_manifest(z.read(MANIFEST_PATH),set(z.namelist()))
        originals=[]
        for path in FILES_OBSERVED:
            data=z.read(path)
            if hashlib.sha256(data).hexdigest()!=EXPECTED[path]:
                raise ValueError("original soundscape SHA mismatch: "+path)
            originals.append((path,data.decode("latin1")))
    observed=graph_inventory(originals)
    for key,required in PINNED_GRAPH.items():
        if observed.get(key)!=required:
            raise ValueError("private SHA-pinned graph anomaly: "+key)
    if tuple(observed["definitions_by_manifest_order"])!=PINNED_DEFINITIONS_PER_FILE:
        raise ValueError("private SHA-pinned per-document definition mismatch")
    observed.update(
        original_script_files_sha_verified=7,
        original_manifest_sha_verified=True,
        original_audio_bytes_exported=False,
        actual_engine_audio_playback_verified=False,
    )
    return observed

def self_test():
    first='''"A" {
        "playsoundscape" { "name" "B" }
        "playsoundscape" { "name" "missing" }
        "playsoundscape" { "name" "b" }
    } "D" {}'''
    second='''"B" { "playsoundscape" { "name" "C" } } "C" {}'''
    s=graph_inventory([("scripts/syntheticA.txt",first),
                       ("scripts/syntheticB.txt",second)])
    expected={
        "definitions":4,
        "reference_occurrences":4,
        "resolved_occurrences":3,
        "unresolved_occurrences":1,
        "unresolved_distinct_targets":1,
        "distinct_resolved_targets":2,
        "definitions_with_resolved_outgoing":2,
        "definitions_without_resolved_outgoing":2,
        "cross_file_resolved_occurrences":2,
        "maximum_dependency_depth":3,
        "maximum_single_root_expansion":5,
        "all_roots_expansion_sum":9,
        "directed_acyclic":True,
    }
    for key,wanted in expected.items():
        if s.get(key)!=wanted:
            raise ValueError("synthetic DAG metric wrong: "+key)
    cyclic=second.replace('"name" "C"','"name" "A"')
    if graph_inventory([("a",first),("b",cyclic)])["directed_acyclic"]:
        raise ValueError("synthetic A -> B -> A cycle accepted")
    for changed in (
        first+' "a" {}',
        first.replace('"name" "B"','"wrong" "B"',1),
    ):
        try:graph_inventory([("a",changed),("b",second)])
        except ValueError:pass
        else:raise ValueError("synthetic malformed graph accepted")
    return {
        "original_free_graph_dag_selftest":"PASS",
        "no_original_game_assets_read":True,
        "repeated_reference_occurrences_preserved":True,
        "cycle_and_duplicate_rejected":True,
    }

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    group=ap.add_mutually_exclusive_group(required=True)
    group.add_argument("--self-test",action="store_true")
    group.add_argument("--scripts",type=Path)
    args=ap.parse_args()
    print(json.dumps(self_test() if args.self_test
                     else private_audit(args.scripts),sort_keys=True))

if __name__=="__main__":
    try:main()
    except (AssertionError,ValueError,UnicodeError,FileNotFoundError,
            KeyError,zipfile.BadZipFile) as exc:
        print("FAIL soundscape dependency graph SHA audit: "+str(exc),
              file=sys.stderr)
        sys.exit(1)
