// @category SiN
// Ghidra 12.2 metadata-only review of direct DATA references from known
// CreateInterface factory entrypoints. Original executable bytes, disassembly,
// pseudocode, class layouts and values at target objects are NEVER exported.
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.InstructionIterator;
import ghidra.program.model.mem.MemoryBlock;
import ghidra.program.model.symbol.Reference;
import java.io.File;
import java.io.PrintWriter;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

public class ExportReviewedFactoryDataReferences extends GhidraScript {
    private static final int MAX_INSTRUCTIONS_PER_FACTORY = 20000;
    private static final int MAX_DATA_REFERENCES_PER_FACTORY = 1000;

    private static void csv(PrintWriter output, Object... fields) {
        for (int index = 0; index < fields.length; index++) {
            if (index > 0) output.print(",");
            String text = fields[index] == null ? "" : fields[index].toString();
            output.print("\"" + text.replace("\"", "\"\"")
                .replace("\r", " ").replace("\n", " ") + "\"");
        }
        output.println();
    }

    private static long pe32(String value) {
        if (!value.matches("(?i)[0-9a-f]{1,8}")) {
            throw new IllegalArgumentException("Malformed reviewed factory VA");
        }
        long result = Long.parseUnsignedLong(value, 16);
        if (result == 0) throw new IllegalArgumentException("Null factory VA");
        return result;
    }

    @Override
    public void run() throws Exception {
        String[] arguments = getScriptArgs();
        if (arguments.length != 2) {
            throw new IllegalArgumentException("Expected reviewed seed dir and metadata output dir");
        }
        String programName = currentProgram.getName();
        if (!("client.dll".equals(programName) || "server.dll".equals(programName)
            || "synthetic".equals(programName))) {
            throw new IllegalArgumentException("Unexpected program, refusing factory reference export");
        }
        File seeds = new File(arguments[0], programName + "_factory_seeds.csv").getCanonicalFile();
        List<String> lines = Files.readAllLines(seeds.toPath(), StandardCharsets.UTF_8);
        if (lines.isEmpty() || !lines.get(0).equals("entry,interface,source")) {
            throw new IllegalArgumentException("Invalid reviewed factory CSV schema");
        }
        File outputDirectory = new File(arguments[1]).getCanonicalFile();
        if (!outputDirectory.isDirectory() && !outputDirectory.mkdirs()) {
            throw new IllegalStateException("Unable to create metadata-only output directory");
        }
        File statusFile = new File(outputDirectory, programName + "_factory_data_ref_status.csv");
        File refsFile = new File(outputDirectory, programName + "_factory_direct_data_refs.csv");
        int factories = 0;
        int resolved = 0;
        int referenceCount = 0;
        Set<String> names = new HashSet<>();
        try (
            PrintWriter status = new PrintWriter(
                Files.newBufferedWriter(statusFile.toPath(), StandardCharsets.UTF_8));
            PrintWriter references = new PrintWriter(
                Files.newBufferedWriter(refsFile.toPath(), StandardCharsets.UTF_8))
        ) {
            csv(status, "interface", "entry", "outcome", "inferred_function_bytes", "data_references");
            csv(references, "interface", "factory_entry", "instruction_va",
                "data_target_va", "target_block", "reference_kind");
            for (int index = 1; index < lines.size(); index++) {
                monitor.checkCancelled();
                String[] row = lines.get(index).split(",", -1);
                if (row.length != 3 || !row[1].matches("[A-Za-z0-9_]+")
                        || !row[2].equals("reviewed_original_static_registration")
                        || !names.add(row[1])) {
                    throw new IllegalArgumentException("Invalid or duplicated reviewed factory CSV row");
                }
                long bits = pe32(row[0]);
                Address entry = currentProgram.getAddressFactory()
                    .getDefaultAddressSpace().getAddress(bits);
                Function function = currentProgram.getFunctionManager().getFunctionAt(entry);
                String outcome = "missing_inferred_function";
                String extent = "";
                int found = 0;
                if (function != null && !function.isExternal()) {
                    resolved++;
                    outcome = "analyzed_direct_data_references";
                    extent = Long.toString(function.getBody().getNumAddresses());
                    InstructionIterator instructions =
                        currentProgram.getListing().getInstructions(function.getBody(), true);
                    int scanned = 0;
                    Set<String> uniqueReferences = new HashSet<>();
                    while (instructions.hasNext()) {
                        monitor.checkCancelled();
                        if (++scanned > MAX_INSTRUCTIONS_PER_FACTORY) {
                            throw new IllegalStateException("Factory instruction scan exceeds limit");
                        }
                        Instruction instruction = instructions.next();
                        for (Reference ref : instruction.getReferencesFrom()) {
                            if (ref.getReferenceType().isCall() || ref.getReferenceType().isJump()) {
                                continue;
                            }
                            Address destination = ref.getToAddress();
                            MemoryBlock block = currentProgram.getMemory().getBlock(destination);
                            if (block == null || block.isExecute()) {
                                continue;
                            }
                            String key = instruction.getAddress().toString()
                                + ":" + destination.toString() + ":" + ref.getReferenceType();
                            if (!uniqueReferences.add(key)) {
                                continue;
                            }
                            if (++found > MAX_DATA_REFERENCES_PER_FACTORY) {
                                throw new IllegalStateException("Factory reference count exceeds limit");
                            }
                            csv(references, row[1], entry, instruction.getAddress(),
                                destination, block.getName(), ref.getReferenceType());
                            referenceCount++;
                        }
                    }
                }
                csv(status, row[1], entry, outcome, extent, found);
                factories++;
            }
        }
        println("SIN_FACTORY_DIRECT_DATA_REFS " + programName
            + " reviewed_seeds=" + factories + " inferred_starts=" + resolved
            + " direct_data_refs=" + referenceCount
            + " reviewed_object_layouts=0 verified_vtables=0");
    }
}
