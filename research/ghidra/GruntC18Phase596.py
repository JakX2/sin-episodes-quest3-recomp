#@category SiN.Episodes
# Ghidra 12.x Jython script. Run with original 2006 server.dll selected.
# First script arg: Phase595 candidate JSON. Second: metadata-only output JSON.
# In GUI, file prompts appear when script arguments are not provided.
# This exports addresses, function labels and instruction mnemonics, not bytes.

import json
from java.io import File

EXPECTED_BASE = 0x22000000
GATE_ADDR = 0x2202D930
AUTHENTICATED_CALL_SITES = set([0x2235C282, 0x2235C436])
SOURCE_SHA = '568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650'

def address_of(va):
    return currentProgram.getAddressFactory().getDefaultAddressSpace().getAddress(long(va))

def describe_function(addr):
    fun = currentProgram.getFunctionManager().getFunctionContaining(addr)
    if fun is None:
        return None
    return {'entry_va': '0x%08X' % fun.getEntryPoint().getOffset(),
            'ghidra_label': str(fun.getName()),
            'kind': 'Ghidra function identification; class ownership NOT proven'}

def describe_instruction(va, original):
    addr = address_of(va)
    listing = currentProgram.getListing()
    exact = listing.getInstructionAt(addr)
    containing = listing.getInstructionContaining(addr)
    rec = {'candidate_va': '0x%08X' % va,
           'scan_opcode_class': original.get('opcode_class'),
           'scan_base_register': original.get('base_register'),
           'could_assign_pointer_heuristic': original.get('could_assign_pointer'),
           'ghidra_exact_instruction_boundary': exact is not None,
           'inside_disassembled_instruction': containing is not None,
           'ghidra_function': describe_function(addr),
           'class_owner_proven': False,
           'component_vtable_proven': False}
    if exact is not None:
        rec['ghidra_mnemonic'] = str(exact.getMnemonicString())
        rec['instruction_length'] = exact.getLength()
    elif containing is not None:
        rec['containing_instruction_va'] = '0x%08X' % containing.getMinAddress().getOffset()
    return rec

def describe_call(site):
    addr = address_of(site)
    inst = currentProgram.getListing().getInstructionAt(addr)
    is_call = inst is not None and str(inst.getMnemonicString()).upper() == 'CALL'
    to_gate = False
    if is_call:
        for ref in currentProgram.getReferenceManager().getReferencesFrom(addr):
            if ref.getToAddress().getOffset() == GATE_ADDR:
                to_gate = True
                break
    return {'candidate_va': '0x%08X' % site,
            'phase592_native_e8_edge_authenticated': site in AUTHENTICATED_CALL_SITES,
            'ghidra_call_instruction': is_call,
            'ghidra_target_is_2202D930': to_gate,
            'ghidra_containing_function': describe_function(addr),
            'owner_class_proven': False}

def main():
    args = getScriptArgs()
    if args is not None and len(args) >= 2:
        source_file = str(args[0])
        output_file = str(args[1])
    else:
        source_file = str(askFile('Select Phase595 candidate JSON', 'Open').getAbsolutePath())
        output_file = str(askFile('Save Phase596 Ghidra metadata JSON', 'Save').getAbsolutePath())
    with open(source_file, 'r') as fd:
        source = json.load(fd)
    if source.get('server_sha256') != SOURCE_SHA or source.get('status') != 'CANDIDATE_SITES_ONLY':
        raise ValueError('Wrong Phase595 source JSON or server SHA')
    program_base = currentProgram.getImageBase().getOffset()
    if program_base != EXPECTED_BASE:
        raise ValueError('Open the original server.dll at PE preferred image base 0x22000000')
    if 'server.dll' not in str(currentProgram.getName()).lower():
        raise ValueError('Select the original server.dll program before running the script')
    sites = []
    for item in source['candidate_member_uses']:
        monitor.checkCancelled()
        sites.append(describe_instruction(int(item['candidate_va'], 16), item))
    calls = []
    for raw_site in source['candidate_gate_e8_callers']:
        monitor.checkCancelled()
        calls.append(describe_call(int(raw_site, 16)))
    report = {
        'status': 'GHIDRA_INSTRUCTION_BOUNDARY_AND_FUNCTION_TRIAGE_ONLY',
        'scan_source_original_sha256': SOURCE_SHA,
        'ghidra_selected_program': str(currentProgram.getName()),
        'program_preferred_image_base': '0x%08X' % program_base,
        'candidate_records': sites,
        'gate_callers': calls,
        'candidate_exact_instructions': sum(1 for x in sites if x['ghidra_exact_instruction_boundary']),
        'candidate_interior_bytes': sum(1 for x in sites if x['inside_disassembled_instruction'] and not x['ghidra_exact_instruction_boundary']),
        'native_component_class_identified': False,
        'native_virtual_plus_0x60_implementation_identified': False,
        'raw_binary_bytes_exported': False,
    }
    with open(output_file, 'w') as fd:
        json.dump(report, fd, indent=2)
    println('Phase596 wrote candidate provenance metadata: ' + output_file)
    println('Instruction starts: %d of %d candidates' % (report['candidate_exact_instructions'], len(sites)))
    println('Gate callsites: ' + str([(c['candidate_va'], c['ghidra_target_is_2202D930']) for c in calls]))

main()
