# Ghidra headless postScript (Jython): batch original SiN PE32 analysis.
# @category SiN
#
# analyzeHeadless ... -import client.dll server.dll
#   -scriptPath research/ghidra -postScript ExportOriginalFunctionAtlas.py OUTDIR [emit-c]
#
# Reports function metadata, caller edges, and relocated function pointers.
# Optional emit-c writes private local Ghidra pseudocode, NEVER commit it.
# Ghidra's inferred boundaries/names are not SHA-reviewed original methods.

import csv
import os
import traceback

from ghidra.app.decompiler import DecompInterface

args = getScriptArgs()
if len(args) < 1:
    raise ValueError("Usage: ExportOriginalFunctionAtlas.py OUTDIR [emit-c]")

outdir = os.path.abspath(args[0])
emit_c = len(args) > 1 and args[1] == "emit-c"
if not os.path.isdir(outdir):
    os.makedirs(outdir)
name = str(currentProgram.getName()).replace("/", "_").replace("\\", "_")
prefix = os.path.join(outdir, name)
functions = currentProgram.getFunctionManager()
references = currentProgram.getReferenceManager()
memory = currentProgram.getMemory()

def open_csv(suffix, columns):
    fp = open(prefix + "_" + suffix + ".csv", "wb")
    writer = csv.writer(fp)
    writer.writerow(columns)
    return fp, writer

f_function, w_function = open_csv(
    "functions",
    ["entry", "name", "size_bytes", "prototype", "external",
     "ghidra_source", "direct_callers"])
f_call, w_call = open_csv(
    "callers", ["function_entry", "call_site", "caller_entry", "kind"])
f_reloc, w_reloc = open_csv(
    "relocated_code_pointers",
    ["source", "target", "target_function", "source_block"])
f_decomp = None
decompiler = None
if emit_c:
    private_decomp_dir = os.path.join(outdir, name + "_PRIVATE_DECOMPILATION")
    if not os.path.isdir(private_decomp_dir):
        os.makedirs(private_decomp_dir)
    decompiler = DecompInterface()
    decompiler.openProgram(currentProgram)

try:
    count = 0
    for fn in functions.getFunctions(True):
        monitor.checkCanceled()
        if fn.isExternal():
            continue
        entry = fn.getEntryPoint()
        from_refs = references.getReferencesTo(entry)
        direct = 0
        for ref in from_refs:
            rt = ref.getReferenceType()
            if not (rt.isCall() or rt.isJump()):
                continue
            from_address = ref.getFromAddress()
            caller = functions.getFunctionContaining(from_address)
            w_call.writerow([
                str(entry), str(from_address),
                str(caller.getEntryPoint()) if caller else "",
                str(rt),
            ])
            if rt.isCall():
                direct += 1
        try:
            sig = str(fn.getSignature())
        except Exception:
            sig = ""
        w_function.writerow([
            str(entry), str(fn.getName()),
            str(fn.getBody().getNumAddresses()), sig,
            str(fn.isExternal()), str(fn.getSymbol().getSource()), direct,
        ])
        count += 1
        if decompiler is not None:
            try:
                result = decompiler.decompileFunction(fn, 20, monitor)
                decomp = result.getDecompiledFunction() if result else None
                if decomp is not None:
                    path = os.path.join(
                        private_decomp_dir, str(entry) + ".c")
                    with open(path, "wb") as output:
                        output.write(decomp.getC().encode("utf-8"))
            except Exception:
                print("DECOMPILATION_FAILED " + str(entry))

    # Data pointers to code: these are candidate vtables/callbacks,
    # never automatically a valid *function start*.
    try:
        relocations = currentProgram.getRelocationTable().getRelocations()
        for reloc in relocations:
            monitor.checkCanceled()
            at = reloc.getAddress()
            block = memory.getBlock(at)
            if block is None or block.isExecute():
                continue
            target_value = memory.getInt(at) & 0xffffffff
            addr = currentProgram.getAddressFactory().getDefaultAddressSpace().getAddress(
                target_value)
            if not memory.contains(addr):
                continue
            code_block = memory.getBlock(addr)
            if code_block is None or not code_block.isExecute():
                continue
            target_fn = functions.getFunctionAt(addr)
            w_reloc.writerow([
                str(at), str(addr),
                str(target_fn.getEntryPoint()) if target_fn else "",
                str(block.getName()),
            ])
    except Exception:
        print("RELOCATION_ENUMERATION_UNAVAILABLE " +
              traceback.format_exc()[-500:])
    print("SIN_ATLAS " + name + " GHIDRA_FUNCTION_CANDIDATES " + str(count))
    print("SIN_ATLAS_OUTPUT " + prefix)
finally:
    f_function.close()
    f_call.close()
    f_reloc.close()
    if decompiler is not None:
        decompiler.dispose()
