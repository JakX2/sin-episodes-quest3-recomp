#!/usr/bin/env python3
"""Phase599: audit the owner's eight original Grunt behavior vtable target report.

Only original addresses, RTTI class labels and public metadata are processed.
The Phase598 report's method boundaries and runtime behavior identities remain
unknown. This source-free audit does not decompile or load original DLLs.
"""
import argparse
from collections import defaultdict
import json
from pathlib import Path

ORIGINAL_SERVER = "568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650"
ORIGINAL_CLIENT = "6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d"
# Phase535 native RTTI-verified class names, owner offsets, vtable addresses.
KNOWN = (
    ("CAI_RappelBehavior",0xCFC,0x223F956C,0x220DEA60,0x220CD400),
    ("CAI_FuncTankBehavior",0xCD8,0x224ABEC4,0x22392970,0x22392700),
    ("CAI_TakecoverBehavior",0xD20,0x224AE624,0x22399650,0x22398670),
    ("CAI_BeserkBehavior",0xDA0,0x224AB88C,0x22391C90,0x220CD400),
    ("CAI_ActBusyBehavior",0xF88,0x224AB494,0x2238EB40,0x220CD400),
    ("CAI_AssaultBehavior",0xDB8,0x223F5544,0x220CF190,0x220CD400),
    ("CGruntStandoffBehavior",0xDE8,0x22495344,0x220E0290,0x220E1EE0),
    ("CAI_FollowBehavior",0xEB8,0x223F6B7C,0x220D6FF0,0x220D33B0),
)
class Refusal(ValueError):
    pass

def verify(cond,msg):
    if not cond: raise Refusal(msg)

def parse_va(value,what):
    verify(isinstance(value,str) and value.startswith("0x"),"Bad address field: "+what)
    try: v=int(value,16)
    except ValueError: raise Refusal("Malformed address: "+what)
    verify(0x22000000 <= v < 0x23000000, "Unexpected virtual address "+what)
    return v

def parse_owner(value):
    verify(isinstance(value,str) and value.startswith("0x"),"Bad owner member offset")
    try:return int(value,16)
    except ValueError: raise Refusal("Bad owner member offset")

def audit(data):
    verify(data.get("status")=="PHASE598_ORIGINAL_EIGHT_BEHAVIOR_VTABLE_TARGETS",
           "Expected uploaded Phase598 owner report")
    verify(data.get("source_server_sha256")==ORIGINAL_SERVER,"Wrong original server SHA")
    verify(data.get("source_client_sha256_authenticated")==ORIGINAL_CLIENT,
           "This follow-up requires authenticated original client+server pair")
    verify(data.get("authenticated_older_method")=="0x2202D930",
           "Original NPC gate witness does not match")
    verify(data.get("original_vtable_rtti_count_verified")==8,"Expected eight RTTI vtables")
    verify(data.get("original_virtual_entries_at_known_vtables")==16,
           "Expected sixteen original virtual slot entries")
    for k in ("method_function_boundaries_proven",
              "active_behavior_member_plus_0xC18_identity_proven",
              "original_virtual_semantics_proven",
              "behavior_event_0x54_consumption_proven"):
        verify(data.get(k) is False, "Unsupported claim in owner metadata: "+k)
    verify(data.get("proprietary_machine_code_exported") is False,"Raw binary metadata flag changed")
    entries=data.get("behaviors")
    verify(isinstance(entries,list) and len(entries)==8,"Missing eight behavior records")
    expected={name:(member,vt,event,activity) for name,member,vt,event,activity in KNOWN}
    seen=set()
    event_groups=defaultdict(list)
    activity_groups=defaultdict(list)
    for obj in entries:
        name=obj.get("class_name")
        verify(name in expected and name not in seen,"Unknown or duplicate behavior class")
        seen.add(name)
        member,vt,event,activity=expected[name]
        verify(parse_owner(obj.get("owner_embedded_offset"))==member,"Embedded object mismatch "+name)
        verify(parse_va(obj.get("authenticated_vtable"),name)==vt,"Original vtable mismatch "+name)
        verify(obj.get("native_rtti_verified") is True and
               obj.get("original_owner_binding_plus_0x04_verified") is True,
               "Missing authenticated class-owner binding "+name)
        verify(obj.get("candidate_event_target_section")==".text" and
               obj.get("candidate_activity_target_section")==".text",
               "Original vtable target outside .text "+name)
        verify(parse_va(obj.get("candidate_event_plus_0x60"),name+" event")==event,
               "Event virtual +0x60 differs "+name)
        verify(parse_va(obj.get("candidate_activity_plus_0x7C"),name+" activity")==activity,
               "Activity virtual +0x7C differs "+name)
        verify(obj.get("runtime_active_behavior_proven") is False and
               obj.get("event_0x54_consumption_proven") is False,
               "Unsupported runtime behavior claim "+name)
        event_groups[event].append(name)
        activity_groups[activity].append(name)
    verify(len(event_groups)==8 and len(activity_groups)==5,
           "Original virtual target cardinality changed")
    verify(activity_groups[0x220CD400]==[
          "CAI_RappelBehavior","CAI_BeserkBehavior",
          "CAI_ActBusyBehavior","CAI_AssaultBehavior"],
          "Unexpected common activity delegate classes")
    expected_targets={
        ("event_plus_0x60",event):tuple(names) for event,names in event_groups.items()
    }
    expected_targets.update({
        ("activity_plus_0x7C",target):tuple(names) for target,names in activity_groups.items()
    })
    unique=data.get("unique_virtual_targets")
    verify(isinstance(unique,list) and len(unique)==13,"Original unique target group count wrong")
    detected={}
    for obj in unique:
        k=(obj.get("vslot_role"),parse_va(obj.get("native_target"),"group target"))
        verify(k in expected_targets and k not in detected,"Unrecognized/duplicate grouped target")
        names=tuple(obj.get("behavior_classes",[]))
        verify(names==expected_targets[k] and obj.get("shared_by_classes")==len(names),
               "Grouped virtual target attribution wrong")
        detected[k]=names
    verify(len(detected)==len(expected_targets),"Missing grouped original target")
    return {
      "status":"PHASE599_SOURCE_PINNED_ORIGINAL_BEHAVIOR_DISPATCH_TRIAGE",
      "validated_source_pair":True,
      "class_vtables_authenticated_from_phase598":8,
      "virtual_slots_checked":16,
      "unique_event_handler_entry_candidates":8,
      "unique_activity_delegate_entry_candidates":5,
      "unique_original_method_address_candidates":13,
      "event_handler_targets":[
          {"native_va":"0x%08X"%va,"class_name":names[0],
           "needs_instruction_boundary_and_handler_code":True,
           "event_0x54_behavior_proven":False}
          for va,names in sorted(event_groups.items())
      ],
      "activity_delegate_targets":[
          {"native_va":"0x%08X"%va,"classes":names,
           "shares_one_native_entry":len(names)>1,
           "method_body_proven":False}
          for va,names in sorted(activity_groups.items())
      ],
      "key_common_activity_virtual":"0x220CD400",
      "important_original_grunt_gate":"0x2202D930",
      "native_event_gate_virtual_offset":"0x60",
      "native_activity_gate_virtual_offset":"0x7C",
      "owner_plus_0xC18_is_active_behavior_proven":False,
      "event_0x54_consumption_proven":False,
      "original_method_boundaries_proven":False,
      "native_method_bytes_exported":False,
    }

def fixture():
    records=[]
    uniq={}
    for name,member,vt,event,activity in KNOWN:
        records.append({
          "class_name":name, "owner_embedded_offset":hex(member),
          "authenticated_vtable":hex(vt),"native_rtti_verified":True,
          "original_owner_binding_plus_0x04_verified":True,
          "candidate_event_plus_0x60":hex(event),
          "candidate_event_target_section":".text",
          "candidate_activity_plus_0x7C":hex(activity),
          "candidate_activity_target_section":".text",
          "runtime_active_behavior_proven":False,
          "event_0x54_consumption_proven":False
        })
        for slot,va in (("event_plus_0x60",event),("activity_plus_0x7C",activity)):
            uniq.setdefault((slot,va),[]).append(name)
    return {
       "status":"PHASE598_ORIGINAL_EIGHT_BEHAVIOR_VTABLE_TARGETS",
       "source_server_sha256":ORIGINAL_SERVER,
       "source_client_sha256_authenticated":ORIGINAL_CLIENT,
       "authenticated_older_method":"0x2202D930",
       "original_vtable_rtti_count_verified":8,
       "original_virtual_entries_at_known_vtables":16,
       "method_function_boundaries_proven":False,
       "active_behavior_member_plus_0xC18_identity_proven":False,
       "original_virtual_semantics_proven":False,
       "behavior_event_0x54_consumption_proven":False,
       "behaviors":records,
       "unique_virtual_targets":[
           {"vslot_role":role,"native_target":hex(addr),
            "behavior_classes":classes,"shared_by_classes":len(classes)}
           for (role,addr),classes in uniq.items()
       ],
       "proprietary_machine_code_exported":False,
    }

def self_test():
    import copy
    test=fixture()
    result=audit(test)
    assert result["unique_original_method_address_candidates"]==13
    assert len(result["event_handler_targets"])==8
    assert len(result["activity_delegate_targets"])==5
    checks=[
      lambda obj:obj.update(source_server_sha256="invalid"),
      lambda obj:obj["behaviors"][6].update(candidate_event_plus_0x60="0x22000010"),
      lambda obj:obj["behaviors"][7].update(authenticated_vtable="0x22495344"),
      lambda obj:obj["behaviors"][0].update(runtime_active_behavior_proven=True),
      lambda obj:obj.update(behavior_event_0x54_consumption_proven=True),
      lambda obj:obj["unique_virtual_targets"][0].update(shared_by_classes=99),
      lambda obj:obj["behaviors"][3].update(candidate_event_target_section=".rdata"),
    ]
    for mutation in checks:
        altered=copy.deepcopy(test)
        mutation(altered)
        try: audit(altered)
        except Refusal: pass
        else: raise AssertionError("Corrupt synthetic report was accepted")
    print("PHASE599_BEHAVIOR_VTABLE_TRIAGE_SYNTHETIC_PASS (7 mutations refused)")

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--input",type=Path,help="Phase598 original metadata JSON")
    p.add_argument("--out",type=Path,help="Address-only Phase599 sorted target summary JSON")
    p.add_argument("--self-test",action="store_true")
    args=p.parse_args()
    if args.self_test:self_test()
    if args.input is None:
        if args.self_test:return
        p.error("Pass --input or --self-test")
    try:
        result=audit(json.loads(args.input.read_text(encoding="utf8")))
    except (OSError,Refusal,ValueError,KeyError,TypeError) as ex:
        p.exit(2,"PHASE599 METADATA REFUSED: %s\n"%ex)
    result_text=json.dumps(result,indent=2)+"\n"
    if args.out:
        args.out.write_text(result_text,encoding="utf8")
        print("Phase599 triaged eight authentic Grunt event handler addresses:",args.out)
    else:
        print(result_text)

if __name__=="__main__":
    main()
