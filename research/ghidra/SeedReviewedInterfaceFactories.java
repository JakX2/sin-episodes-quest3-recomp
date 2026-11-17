// @category SiN
// Recover function-start candidates only at previously reviewed static interface
// factory addresses. This is ANALYSIS SEEDING, NOT independent body SHA verification.
// Never reads or exports original bytes, instructions, decompiler text or assets.
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.mem.MemoryBlock;
import java.io.File;
import java.io.PrintWriter;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.List;

public class SeedReviewedInterfaceFactories extends GhidraScript {

    private static String safe(String value) {
        return "\"" + value.replace("\"", "\"\"").replace("\n", " ").replace("\r", " ") + "\"";
    }

    private static void row(PrintWriter output, String... fields) {
        for (int i = 0; i < fields.length; i++) {
            if (i > 0) output.print(",");
            output.print(safe(fields[i]));
        }
        output.println();
    }

    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length != 2) {
            throw new IllegalArgumentException("Expected vetted factory CSV directory and private output directory");
        }
        String name = currentProgram.getName().replace("/", "_").replace("\\", "_");
        File input = new File(args[0], name + "_factory_seeds.csv").getCanonicalFile();
        if (!input.isFile()) {
            throw new IllegalStateException("Missing reviewed seed file: " + input.getName());
        }
        List<String> lines = Files.readAllLines(input.toPath(), StandardCharsets.UTF_8);
        if (lines.isEmpty() || !lines.get(0).equals("entry,interface,source")) {
            throw new IllegalStateException("Invalid reviewed factory seed CSV header");
        }
        File outputDir = new File(args[1]).getCanonicalFile();
        if (!outputDir.isDirectory() && !outputDir.mkdirs()) {
            throw new IllegalStateException("Unable to make private metadata output directory");
        }
        File report = new File(outputDir, name + "_factory_seed_results.csv");
        int discovered = 0, seeded = 0, blocked = 0;
        try (PrintWriter writer = new PrintWriter(
                Files.newBufferedWriter(report.toPath(), StandardCharsets.UTF_8))) {
            row(writer, "entry", "interface", "outcome", "result_function_entry");
            for (int i = 1; i < lines.size(); ++i) {
                monitor.checkCancelled();
                String[] parts = lines.get(i).split(",", -1);
                if (parts.length != 3 || !parts[0].matches("(?i)[0-9a-f]{1,16}") ||
                    !parts[1].matches("[A-Za-z0-9_]+") ||
                    !parts[2].equals("reviewed_original_static_registration")) {
                    throw new IllegalArgumentException("Bad reviewed seed CSV record " + i);
                }
                long bits = Long.parseUnsignedLong(parts[0], 16);
                Address entry = currentProgram.getAddressFactory()
                    .getDefaultAddressSpace().getAddress(bits);
                String status;
                Function located = currentProgram.getFunctionManager().getFunctionAt(entry);
                if (located != null) {
                    status = "already_inferred";
                    discovered++;
                } else {
                    MemoryBlock block = currentProgram.getMemory().getBlock(entry);
                    if (block == null || !block.isExecute()) {
                        status = "non_executable";
                        blocked++;
                    } else if (currentProgram.getFunctionManager().getFunctionContaining(entry) != null) {
                        status = "overlaps_another_inferred_function";
                        blocked++;
                    } else {
                        try {
                            if (currentProgram.getListing().getInstructionAt(entry) == null) {
                                disassemble(entry);
                            }
                            located = createFunction(entry, null);
                            if (located != null) {
                                status = "seeded_candidate";
                                seeded++;
                            } else {
                                status = "unresolved_candidate";
                                blocked++;
                            }
                        } catch (Exception error) {
                            // The unresolved seed is evidence for manual review, not
                            // permission to clear existing Ghidra instructions/functions.
                            status = "seed_error_" + error.getClass().getSimpleName();
                            blocked++;
                        }
                    }
                }
                row(writer, parts[0], parts[1], status,
                    located == null ? "" : located.getEntryPoint().toString());
            }
        }
        println("SIN_REVIEWED_FACTORY_SEEDS " + name + " prior=" + discovered +
                " created_candidates=" + seeded + " unresolved=" + blocked +
                " (NO original function verification)");
    }
}
