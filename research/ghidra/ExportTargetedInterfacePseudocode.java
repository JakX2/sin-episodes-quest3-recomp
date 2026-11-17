// @category SiN
// Explicitly opted-in, owner-private C-like pseudocode export for a small,
// known interface-factory batch. Never publish proprietary pseudocode.
// Ghidra output is NOT original C++ source or verified method equivalence.
import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.mem.MemoryBlock;
import java.io.File;
import java.io.PrintWriter;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardOpenOption;
import java.nio.file.attribute.PosixFilePermissions;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

public class ExportTargetedInterfacePseudocode extends GhidraScript {
    private static final int MAX_FUNCTIONS_PER_MODULE = 8;
    private static final Set<String> PRIORITY = new HashSet<>(Arrays.asList(
        "ServerGameDLL003", "ServerGameClients003", "ServerGameEnts001",
        "VClient011", "VClientPrediction001", "VClientEntityList003",
        "GameMovement001", "GameClientExports001",
        "SyntheticEngine001"
    ));

    private static final class Seed {
        final String interfaceName;
        final long address;
        Seed(String name, long va) { interfaceName = name; address = va; }
    }

    private static void csv(PrintWriter stream, String... values) {
        for (int i = 0; i < values.length; ++i) {
            if (i != 0) stream.print(",");
            String value = values[i].replace("\r", " ").replace("\n", " ");
            stream.print("\"" + value.replace("\"", "\"\"") + "\"");
        }
        stream.println();
    }

    private static long parseAddress(String raw) {
        if (!raw.matches("(?i)[0-9a-f]{1,16}")) {
            throw new IllegalArgumentException("Invalid seed address format");
        }
        long parsed = Long.parseUnsignedLong(raw, 16);
        if (Long.compareUnsigned(parsed, 0xffffffffL) > 0) {
            throw new IllegalArgumentException("Seed address is not PE32");
        }
        return parsed;
    }

    private static void privatePermissions(Path path, boolean directory) throws Exception {
        try {
            Files.setPosixFilePermissions(path, PosixFilePermissions.fromString(
                directory ? "rwx------" : "rw-------"));
        } catch (UnsupportedOperationException unsupportedOnWindows) {
            // The owner-only runner is invoked under umask 077 on Unix.
        }
    }

    @Override
    public void run() throws Exception {
        if (!"1".equals(System.getenv("SIN_PRIVATE_DECOMP_ACK"))) {
            throw new SecurityException("Private pseudocode requires explicit local opt-in");
        }
        String[] arguments = getScriptArgs();
        if (arguments.length != 2) {
            throw new IllegalArgumentException("Expected reviewed seed directory and private output directory");
        }
        String programName = currentProgram.getName();
        if (!("client.dll".equals(programName) || "server.dll".equals(programName)
                || "synthetic".equals(programName))) {
            throw new IllegalStateException("Unexpected program for SiN interface pseudocode extraction");
        }

        File seedsFile = new File(arguments[0], programName + "_factory_seeds.csv").getCanonicalFile();
        if (!seedsFile.isFile()) {
            throw new IllegalStateException("Reviewed interface factory seed file missing");
        }
        List<String> lines = Files.readAllLines(seedsFile.toPath(), StandardCharsets.UTF_8);
        if (lines.isEmpty() || !lines.get(0).equals("entry,interface,source")) {
            throw new IllegalArgumentException("Invalid seed CSV schema");
        }
        List<Seed> batch = new ArrayList<>();
        Set<String> seen = new HashSet<>();
        for (int i = 1; i < lines.size(); ++i) {
            String[] columns = lines.get(i).split(",", -1);
            if (columns.length != 3 || !columns[1].matches("[A-Za-z0-9_]+")
                    || !columns[2].equals("reviewed_original_static_registration")) {
                throw new IllegalArgumentException("Invalid reviewed seed row");
            }
            long entry = parseAddress(columns[0]);
            if (!seen.add(columns[1])) {
                throw new IllegalArgumentException("Repeated reviewed interface name");
            }
            if (PRIORITY.contains(columns[1])
                    && ("synthetic".equals(programName) == "SyntheticEngine001".equals(columns[1]))) {
                batch.add(new Seed(columns[1], entry));
            }
        }
        if (batch.size() > MAX_FUNCTIONS_PER_MODULE) {
            throw new IllegalArgumentException("Refusing unbounded pseudocode export");
        }

        File root = new File(arguments[1]).getCanonicalFile();
        if (!root.isDirectory() && !root.mkdirs()) {
            throw new IllegalStateException("Cannot create private pseudocode output root");
        }
        privatePermissions(root.toPath(), true);
        Path privateDirectory = new File(root, programName + "_PRIVATE_TARGETED_DECOMPILATION").toPath();
        Files.createDirectories(privateDirectory);
        privatePermissions(privateDirectory, true);
        Path report = new File(root, programName + "_targeted_decomp_status.csv").toPath();

        int complete = 0;
        int unavailable = 0;
        DecompInterface decompiler = new DecompInterface();
        try {
            if (!decompiler.openProgram(currentProgram)) {
                throw new IllegalStateException("Ghidra decompiler could not open program");
            }
            try (PrintWriter status = new PrintWriter(Files.newBufferedWriter(
                    report, StandardCharsets.UTF_8, StandardOpenOption.CREATE_NEW))) {
                csv(status, "interface", "entry", "outcome", "inferred_function_bytes");
                for (Seed seed : batch) {
                    monitor.checkCancelled();
                    Address address = currentProgram.getAddressFactory()
                        .getDefaultAddressSpace().getAddress(seed.address);
                    MemoryBlock block = currentProgram.getMemory().getBlock(address);
                    Function function = currentProgram.getFunctionManager().getFunctionAt(address);
                    String outcome = "missing_inferred_function";
                    String extent = "";
                    if (block == null || !block.isExecute()) {
                        outcome = "not_executable";
                    } else if (function != null && !function.isExternal()) {
                        extent = Long.toString(function.getBody().getNumAddresses());
                        try {
                            DecompileResults result = decompiler.decompileFunction(function, 25, monitor);
                            if (result != null && result.decompileCompleted()
                                    && result.getDecompiledFunction() != null) {
                                String pseudocode = result.getDecompiledFunction().getC();
                                if (pseudocode != null && !pseudocode.isEmpty()) {
                                    String filename = seed.interfaceName + "_" +
                                        String.format("%08x", seed.address) + ".c";
                                    Path output = privateDirectory.resolve(filename);
                                    try (java.io.BufferedWriter stream = Files.newBufferedWriter(
                                            output, StandardCharsets.UTF_8, StandardOpenOption.CREATE_NEW)) {
                                        stream.write(pseudocode);
                                    }
                                    privatePermissions(output, false);
                                    outcome = "decompiled_candidate";
                                    complete++;
                                } else {
                                    outcome = "empty_decompilation";
                                }
                            } else {
                                outcome = "decompilation_failed";
                            }
                        } catch (Exception error) {
                            // Never include pseudocode or decompiler exception content in logs.
                            outcome = "decompilation_failed";
                        }
                    }
                    if (!outcome.equals("decompiled_candidate")) unavailable++;
                    csv(status, seed.interfaceName, String.format("0x%08X", seed.address),
                        outcome, extent);
                }
            }
            privatePermissions(report, false);
        } finally {
            decompiler.dispose();
        }
        println("SIN_TARGETED_DECOMP " + programName + " selected=" + batch.size() +
            " candidate_c_private=" + complete + " unavailable=" + unavailable +
            " original_methods_verified=0");
    }
}
