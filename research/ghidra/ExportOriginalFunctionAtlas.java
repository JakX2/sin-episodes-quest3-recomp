// @category SiN
// Original-only bulk function/call/relocated-pointer metadata atlas.
// Java-based for Ghidra 12.2 standard analyzeHeadless (no PyGhidra required).
// NEVER exports original game bytes, disassembly or decompiled C.
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.address.AddressSpace;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.FunctionIterator;
import ghidra.program.model.listing.FunctionManager;
import ghidra.program.model.mem.Memory;
import ghidra.program.model.reloc.Relocation;
import ghidra.program.model.symbol.Reference;
import ghidra.program.model.symbol.RefType;
import java.io.File;
import java.io.PrintWriter;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.Iterator;

public class ExportOriginalFunctionAtlas extends GhidraScript {

    private static String safeField(Object raw) {
        String value = raw == null ? "" : raw.toString();
        value = value.replace("\"", "\"\"").replace("\n", " ")
                     .replace("\r", " ");
        return "\"" + value + "\"";
    }

    private static void csv(PrintWriter writer, Object... values) {
        for (int i = 0; i < values.length; ++i) {
            if (i != 0) {
                writer.print(",");
            }
            writer.print(safeField(values[i]));
        }
        writer.println();
    }

    @Override
    public void run() throws Exception {
        String[] arguments = getScriptArgs();
        if (arguments.length != 1) {
            throw new IllegalArgumentException(
                "Expected one output directory; CI prohibits emit-c");
        }
        File outputDirectory = new File(arguments[0]).getCanonicalFile();
        if (!outputDirectory.isDirectory() && !outputDirectory.mkdirs()) {
            throw new IllegalStateException("Unable to create metadata output directory");
        }
        String programName = currentProgram.getName()
            .replace("/", "_").replace("\\", "_");
        File functionsFile = new File(outputDirectory, programName + "_functions.csv");
        File callsFile = new File(outputDirectory, programName + "_callers.csv");
        File pointersFile = new File(outputDirectory,
                                     programName + "_relocated_code_pointers.csv");

        FunctionManager functionManager = currentProgram.getFunctionManager();
        Memory memory = currentProgram.getMemory();
        int functionCount = 0;
        int callCount = 0;
        int pointerCount = 0;
        try (
            PrintWriter functions = new PrintWriter(
                Files.newBufferedWriter(functionsFile.toPath(), StandardCharsets.UTF_8));
            PrintWriter callers = new PrintWriter(
                Files.newBufferedWriter(callsFile.toPath(), StandardCharsets.UTF_8));
            PrintWriter pointers = new PrintWriter(
                Files.newBufferedWriter(pointersFile.toPath(), StandardCharsets.UTF_8))
        ) {
            csv(functions, "entry", "name", "size_bytes", "prototype",
                "external", "ghidra_source", "direct_callers");
            csv(callers, "function_entry", "call_site", "caller_entry", "kind");
            csv(pointers, "source", "target", "target_function", "source_block");

            FunctionIterator iter = functionManager.getFunctions(true);
            while (iter.hasNext()) {
                monitor.checkCancelled();
                Function function = iter.next();
                if (function.isExternal()) {
                    continue;
                }
                Address entry = function.getEntryPoint();
                int directCallers = 0;
                for (Reference ref : currentProgram.getReferenceManager()
                        .getReferencesTo(entry)) {
                    RefType type = ref.getReferenceType();
                    if (!type.isCall() && !type.isJump()) {
                        continue;
                    }
                    Address source = ref.getFromAddress();
                    Function caller = functionManager.getFunctionContaining(source);
                    csv(callers, entry, source,
                        caller == null ? "" : caller.getEntryPoint(),
                        type);
                    callCount++;
                    if (type.isCall()) {
                        directCallers++;
                    }
                }
                String prototype;
                try {
                    prototype = function.getSignature().toString();
                } catch (Exception error) {
                    prototype = "";
                }
                csv(functions, entry, function.getName(),
                    function.getBody().getNumAddresses(), prototype,
                    function.isExternal(), function.getSymbol().getSource(),
                    directCallers);
                functionCount++;
            }

            // Relocations in data/rdata that point exactly into executable
            // memory are CANDIDATE function pointers, not verified starts.
            try {
                Object raw = currentProgram.getRelocationTable().getRelocations();
                if (raw instanceof Iterator<?>) {
                    Iterator<?> relocs = (Iterator<?>) raw;
                    AddressSpace space =
                        currentProgram.getAddressFactory().getDefaultAddressSpace();
                    while (relocs.hasNext()) {
                        monitor.checkCancelled();
                        Object next = relocs.next();
                        if (!(next instanceof Relocation)) {
                            continue;
                        }
                        Address location = ((Relocation) next).getAddress();
                        if (!memory.contains(location) ||
                            memory.getBlock(location) == null ||
                            memory.getBlock(location).isExecute()) {
                            continue;
                        }
                        long addressBits = memory.getInt(location) & 0xffffffffL;
                        Address target = space.getAddress(addressBits);
                        if (memory.contains(target) &&
                            memory.getBlock(target) != null &&
                            memory.getBlock(target).isExecute()) {
                            Function exact = functionManager.getFunctionAt(target);
                            csv(pointers, location, target,
                                exact == null ? "" : exact.getEntryPoint(),
                                memory.getBlock(location).getName());
                            pointerCount++;
                        }
                    }
                } else {
                    println("Relocation iterable not exposed by this Ghidra runtime");
                }
            } catch (Exception error) {
                println("Relocated pointer pass unavailable: " +
                        error.getClass().getName() + ": " + error.getMessage());
            }
        }

        println("SIN_ORIGINAL_ATLAS_CANDIDATES " + programName +
                " functions=" + functionCount +
                " references=" + callCount +
                " reloc_data_pointers=" + pointerCount);
        println("SIN_ORIGINAL_ATLAS_OUTPUT " + outputDirectory.getAbsolutePath());
    }
}
