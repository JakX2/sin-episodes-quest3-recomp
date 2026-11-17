// @category SiN
// Recover Ghidra function-start CANDIDATES only at SHA256-authenticated
// PE32 vtable slot targets. No original code, disassembly or pseudocode exported.
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.mem.MemoryBlock;
import java.io.File;
import java.io.PrintWriter;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

public class SeedAuthenticatedVtablePrefixMethods extends GhidraScript {
    private static final int MAX_RECORDS = 200;
    private static final String ORIGINAL_SOURCE =
        "sha256_authenticated_original_pe32_vtable_prefix";
    private static final String SYNTHETIC_SOURCE =
        "synthetic_vtable_test_fixture";

    private static String safe(String value) {
        return "\"" + value.replace("\"", "\"\"")
            .replace("\r", " ").replace("\n", " ") + "\"";
    }

    private static void row(PrintWriter writer, String... fields) {
        for (int i = 0; i < fields.length; i++) {
            if (i != 0) writer.print(",");
            writer.print(safe(fields[i]));
        }
        writer.println();
    }

    private static long parseVa(String input) {
        if (!input.matches("(?i)[a-f0-9]{1,16}")) {
            throw new IllegalArgumentException("Malformed vtable method VA");
        }
        long bits = Long.parseUnsignedLong(input, 16);
        if (bits == 0 || Long.compareUnsigned(bits, 0xffffffffL) > 0) {
            throw new IllegalArgumentException("Vtable method VA outside PE32");
        }
        return bits;
    }

    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length != 2) {
            throw new IllegalArgumentException("Expected vetted vtable seed directory and output directory");
        }
        String program = currentProgram.getName();
        boolean synthetic = program.equals("synthetic");
        if (!synthetic && !program.equals("client.dll") && !program.equals("server.dll")) {
            throw new IllegalStateException("Unexpected Ghidra program for SiN PE32 vtable seeding");
        }
        File file = new File(args[0], program + "_vtable_method_seeds.csv").getCanonicalFile();
        if (!file.isFile()) {
            throw new IllegalStateException("No authenticated vtable seed CSV for " + program);
        }
        List<String> lines = Files.readAllLines(file.toPath(), StandardCharsets.UTF_8);
        if (lines.isEmpty() || !lines.get(0).equals("entry,interface,slot,source")
                || lines.size() < 2 || lines.size() > MAX_RECORDS + 1) {
            throw new IllegalStateException("Invalid or unbounded vtable seed CSV");
        }
        File directory = new File(args[1]).getCanonicalFile();
        if (!directory.isDirectory() && !directory.mkdirs()) {
            throw new IllegalStateException("Cannot create vtable seed status directory");
        }
        File result = new File(directory, program + "_vtable_seed_results.csv");
        Set<Long> addresses = new HashSet<>();
        Set<String> ownerSlots = new HashSet<>();
        int prior = 0, seeded = 0, unresolved = 0;
        try (PrintWriter writer = new PrintWriter(
                Files.newBufferedWriter(result.toPath(), StandardCharsets.UTF_8))) {
            row(writer, "entry", "interface", "slot", "outcome", "result_function_entry");
            for (int i = 1; i < lines.size(); ++i) {
                monitor.checkCancelled();
                String[] parts = lines.get(i).split(",", -1);
                if (parts.length != 4 || !parts[1].matches("[A-Za-z0-9_]+")
                        || !parts[2].matches("[0-9]{1,3}")
                        || Integer.parseInt(parts[2]) > 255
                        || !(synthetic ? SYNTHETIC_SOURCE : ORIGINAL_SOURCE).equals(parts[3])) {
                    throw new IllegalArgumentException("Invalid vetted vtable seed CSV row " + i);
                }
                long va = parseVa(parts[0]);
                if (!addresses.add(va) || !ownerSlots.add(parts[1] + ":" + parts[2])) {
                    throw new IllegalArgumentException("Duplicate vtable method address or owner slot");
                }
                Address entry = currentProgram.getAddressFactory()
                    .getDefaultAddressSpace().getAddress(va);
                Function known = currentProgram.getFunctionManager().getFunctionAt(entry);
                String status;
                if (known != null) {
                    prior++;
                    status = "already_inferred";
                } else {
                    MemoryBlock block = currentProgram.getMemory().getBlock(entry);
                    if (block == null || !block.isExecute()) {
                        unresolved++;
                        status = "non_executable";
                    } else if (currentProgram.getFunctionManager().getFunctionContaining(entry) != null) {
                        unresolved++;
                        status = "overlaps_another_inferred_function";
                    } else {
                        try {
                            if (currentProgram.getListing().getInstructionAt(entry) == null) {
                                disassemble(entry);
                            }
                            known = createFunction(entry, null);
                            if (known == null) {
                                unresolved++;
                                status = "unresolved_candidate";
                            } else {
                                seeded++;
                                status = "seeded_candidate";
                            }
                        } catch (Exception error) {
                            unresolved++;
                            status = "seed_error_" + error.getClass().getSimpleName();
                        }
                    }
                }
                row(writer, parts[0], parts[1], parts[2], status,
                    known == null ? "" : known.getEntryPoint().toString());
            }
        }
        println("SIN_VTABLE_METHOD_SEEDS " + program + " prior=" + prior
            + " new_ghidra_candidates=" + seeded + " unresolved=" + unresolved
            + " independently_verified_original_methods=0");
    }
}
