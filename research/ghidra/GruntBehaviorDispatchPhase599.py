#@category SiN.Episodes
# Phase599 Ghidra Jython (Python 2.7-compatible), read-only, original 2006 SiN server.dll.
# Select original server.dll program with autoanalysis complete.
# Script args in headless mode: Phase598 JSON, original server.dll, output JSON.
# In GUI prompts ask for those three files.
# Only addresses, hashes, counters, function and mnemonic metadata exported.
# NO original executable code bytes or disassembly dumps are exported.

import hashlib
import json
import re
import struct

SERVER_SHA = "568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650"
CLIENT_SHA = "6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d"
MAX_NATIVE_SPAN = 0x20000
RELEVANT_EVENT_IDS = (0x54, 0x43)
MARKER_OFFSET_PATTERN = re.compile(r"\[[^\]]*\+\s*(?:0x0*8|0*8)\s*\]",re.I)

def refuse(cond, message):
    if not cond:
        raise ValueError(message)

def parse_addr(value):
    refuse(isinstance(value, basestring) and value.lower().startswith("0x"),
           "Expected hexadecimal VA")
    return int(value,16)

def addr(va):
    return currentProgram.getAddressFactory().getDefaultAddressSpace().getAddress(long(va))

def get_program_u32(va):
    val = currentProgram.getMemory().getInt(addr(va))
    return int(val) & 0xFFFFFFFF

class OriginalPE:
    def __init__(self, raw):
        refuse(len(raw)>0x100 and raw[:2]=="MZ","MZ missing")
        pe=struct.unpack_from("<I",raw,0x3C)[0]
        refuse(raw[pe:pe+4]=="PE\x00\x00","PE missing")
        opt=pe+24
        refuse(struct.unpack_from("<H",raw,pe+4)[0]==0x14C and
               struct.unpack_from("<H",raw,opt)[0]==0x10B,
               "Selected original is not x86 PE32")
        self.image_base=struct.unpack_from("<I",raw,opt+28)[0]
        n=struct.unpack_from("<H",raw,pe+6)[0]
        sec_start=opt+struct.unpack_from("<H",raw,pe+20)[0]
        self.sections=[]
        self.raw=raw
        for i in range(n):
            pos=sec_start+i*40
            refuse(pos+40<=len(raw),"Truncated PE section table")
            name=raw[pos:pos+8].split("\x00")[0]
            vs,rva,size,off=struct.unpack_from("<IIII",raw,pos+8)
            refuse(off+size<=len(raw),"Truncated PE section")
            self.sections.append((name,self.image_base+rva,min(vs or size,size),off))

    def raw_span(self,entry,length):
        for name,start,size,off in self.sections:
            if name==".text" and start<=entry and entry+length<=start+size:
                return self.raw[off+entry-start:off+entry-start+length]
        return None

def inspect_method(va,roles,pe):
    listing=currentProgram.getListing()
    a=addr(va)
    inst_at=listing.getInstructionAt(a)
    function=currentProgram.getFunctionManager().getFunctionAt(a)
    containing=currentProgram.getFunctionManager().getFunctionContaining(a)
    record={
      "original_target":"0x%08X"%va,
      "roles":roles,
      "ghidra_instruction_at_entry":inst_at is not None,
      "ghidra_function_starts_at_entry":function is not None,
      "ghidra_function_contains_entry":containing is not None,
      "native_method_boundary_independently_authenticated":False,
      "marker_plus_0x08_write_proven":False,
      "event_0x54_consumption_proven":False,
      "active_behavior_owner_pointer_proven":False,
      "event_immediate_0x54_sites":[],
      "event_immediate_0x43_sites":[],
      "component_plus_0x08_operand_candidates":[],
      "direct_call_targets":[],
      "ghidra_span_candidate_sha256":None,
    }
    if function is None:
        if containing is not None:
            record["ghidra_containing_function_start"]="0x%08X"%containing.getEntryPoint().getOffset()
        return record
    body=function.getBody()
    start=body.getMinAddress().getOffset()
    end=body.getMaxAddress().getOffset()+1
    record["ghidra_function_label"]=str(function.getName())
    record["ghidra_function_start"]="0x%08X"%start
    record["ghidra_function_end_exclusive"]="0x%08X"%end
    record["ghidra_span_bytes"]=end-start
    try:
        one_range=(body.getNumAddressRanges()==1)
    except Exception:
        # Older Ghidra APIs expose just an AddressRangeIterator.
        ranges=body.getAddressRanges()
        number=0
        while ranges.hasNext():
            ranges.next()
            number+=1
        one_range=(number==1)
    record["ghidra_contiguous_function_body"]=one_range
    # Entire-function authentication requires independently verified boundaries.
    # This is only the hash of the contiguous Ghidra-selected raw PE span.
    if one_range and start==va and 0<end-start<=MAX_NATIVE_SPAN:
        raw=pe.raw_span(start,end-start)
        if raw is not None:
            record["ghidra_span_candidate_sha256"]=hashlib.sha256(raw).hexdigest()
    calls=set()
    steps=0
    it=listing.getInstructions(body,True)
    while it.hasNext():
        monitor.checkCancelled()
        inst=it.next()
        steps+=1
        if steps>50000:
            record["scan_truncated_at_50000_instructions"]=True
            break
        here=inst.getAddress().getOffset()
        mnemonic=str(inst.getMnemonicString()).upper()
        nops=inst.getNumOperands()
        for i in range(nops):
            sc=inst.getScalar(i)
            if sc is not None:
                value=int(sc.getUnsignedValue()) & 0xFFFFFFFF
                if value in RELEVANT_EVENT_IDS:
                    key="event_immediate_0x54_sites" if value==0x54 else "event_immediate_0x43_sites"
                    record[key].append({
                        "va":"0x%08X"%here,
                        "mnemonic":mnemonic,
                        "operand_index":i,
                        "candidate_only_not_event_proof":True,
                    })
            operand=str(inst.getDefaultOperandRepresentation(i))
            if (MARKER_OFFSET_PATTERN.search(operand) and
                mnemonic in ("MOV","CMP","TEST","AND","OR","XOR","INC","DEC","SETZ","SETNZ")):
                record["component_plus_0x08_operand_candidates"].append({
                    "va":"0x%08X"%here,
                    "mnemonic":mnemonic,
                    "operand_index":i,
                    "receiver_proven":False,
                    "write_proven":False,
                })
        if inst.getFlowType().isCall():
            for target in inst.getFlows():
                target_va=target.getOffset()
                if 0x22000000<=target_va<0x23000000:
                    calls.add(target_va)
    record["ghidra_instruction_count"]=steps
    record["direct_call_targets"]=["0x%08X"%x for x in sorted(calls)]
    return record

def main():
    args=getScriptArgs()
    if len(args)>=3:
        report_file=str(args[0]); dll_file=str(args[1]); outfile=str(args[2])
    else:
        report_file=str(askFile("Select owner Phase598 behavior targets JSON","Open").getAbsolutePath())
        dll_file=str(askFile("Select untouched original SiN server.dll","Open").getAbsolutePath())
        outfile=str(askFile("Save Phase599 Ghidra address-only report","Save").getAbsolutePath())
    with open(report_file,"rb") as fh:
        report=json.load(fh)
    refuse(report.get("status")=="PHASE598_ORIGINAL_EIGHT_BEHAVIOR_VTABLE_TARGETS",
           "Invalid owner Phase598 report")
    refuse(report.get("source_server_sha256")==SERVER_SHA,"Wrong source server SHA metadata")
    refuse(report.get("source_client_sha256_authenticated")==CLIENT_SHA,
           "Original pair not authenticated by Phase598")
    refuse(report.get("original_vtable_rtti_count_verified")==8,"Expected eight source vtables")
    with open(dll_file,"rb") as fh:
        original=fh.read()
    refuse(hashlib.sha256(original).hexdigest()==SERVER_SHA,
           "Selected local server.dll bytes are not the authenticated 2006 release")
    pe=OriginalPE(original)
    refuse(pe.image_base==0x22000000 and
           currentProgram.getImageBase().getOffset()==0x22000000,
           "Wrong preferred server.dll image base")
    refuse(len(report["behaviors"])==8,"Incomplete behavior list")
    rolemap={}
    allclasses=set()
    event_targets=set()
    activity_targets=set()
    for obj in report["behaviors"]:
        name=obj["class_name"]
        refuse(name not in allclasses,"Duplicate class in report")
        allclasses.add(name)
        vt=parse_addr(obj["authenticated_vtable"])
        event=parse_addr(obj["candidate_event_plus_0x60"])
        activity=parse_addr(obj["candidate_activity_plus_0x7C"])
        refuse(get_program_u32(vt+0x60)==event and
               get_program_u32(vt+0x7C)==activity,
               "Imported program vtable does not match original Phase598 "+name)
        event_targets.add(event);activity_targets.add(activity)
        rolemap.setdefault(event,[]).append({"class":name,"slot":"event+0x60"})
        rolemap.setdefault(activity,[]).append({"class":name,"slot":"activity+0x7C"})
    refuse(len(event_targets)==8 and len(activity_targets)==5,
           "Unexpected original slot sharing")
    entries=[inspect_method(va,rolemap[va],pe) for va in sorted(rolemap)]
    output={
      "status":"PHASE599_GHIDRA_ESTIMATED_NATIVE_FUNCTION_BOUNDS",
      "source_server_sha256":SERVER_SHA,
      "original_client_sha256_report_authenticated":CLIENT_SHA,
      "original_pe_verified_against_disk":True,
      "source_phase598_vtable_pointers_matched_loaded_program":16,
      "unique_original_vtable_targets":len(entries),
      "ghidra_function_starts":sum(1 for e in entries if e["ghidra_function_starts_at_entry"]),
      "missing_ghidra_functions":[e["original_target"] for e in entries if not e["ghidra_function_starts_at_entry"]],
      "functions":entries,
      "new_native_method_boundaries_sha_authenticated":False,
      "component_plus_0x08_proven_as_override_marker":False,
      "event_0x54_handling_proven":False,
      "exported_proprietary_machine_code":False,
      "limitations":"Function sizes/hashes follow Ghidra candidate bodies and are not standalone source-authenticated method boundaries; immediate 0x54 and +8 syntactic sites need dataflow validation."
    }
    with open(outfile,"w") as fd:
        json.dump(output,fd,indent=2,sort_keys=True)
    println("Phase599 result: "+outfile)
    println("Ghidra starts %d/%d targets. Event 0x54 remains unproven."%(
            output["ghidra_function_starts"],len(entries)))

main()
