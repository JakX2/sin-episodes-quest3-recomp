// @category SiN
// Opt-in PRIVATE targeted Ghidra decompilation of SHA-reviewed PE32 vtable
// METHOD POINTER CANDIDATES. Never export original code into public artifacts.
// Decompiled C is NOT original source, verified body equivalence, or known ABI.
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
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

public class ExportTargetedVtableMethodPseudocode extends GhidraScript {
    private static final int MAX_REVIEWED_SEEDS = 200;
    private static final int MAX_SELECTED_PER_MODULE = 8;
    private static final long MAX_INFERRED_FUNCTION_EXTENT = 500000;
    private static final int MAX_DECOMPILED_CHARS = 262144;
    private static final String ORIGINAL_SOURCE =
        "sha256_authenticated_original_pe32_vtable_prefix";
    private static final String SYNTHETIC_SOURCE = "synthetic_vtable_test_fixture";

    // Explicit priority preserves meaningful IClientDLL/IServerGameDLL lifecycle
    // work rather than decompiling 105 arbitrary or unbounded addresses.
    private static List<String> selectedKeys(String module) {
        if (module.equals("client.dll")) return Arrays.asList(
            "VClient011:0", "VClient011:1", "VClient011:2", "VClient011:3",
            "GameClientExports001:0", "GameClientExports001:1");
        if (module.equals("server.dll")) return Arrays.asList(
            "ServerGameDLL003:0", "ServerGameDLL003:1",
            "ServerGameDLL003:2", "ServerGameDLL003:3",
            "ServerGameClients003:0", "ServerGameClients003:1",
            "ServerGameEnts001:0", "ServerGameEnts001:1");
        if (module.equals("synthetic")) return Arrays.asList("SyntheticVtable001:0");
        throw new IllegalStateException("Unexpected original program name");
    }

    private static long pe32(String raw) {
        if (!raw.matches("(?i)[0-9a-f]{1,16}")) {
            throw new IllegalArgumentException("Bad PE32 method seed address");
        }
        long value = Long.parseUnsignedLong(raw, 16);
        if (value == 0 || Long.compareUnsigned(value, 0xffffffffL) > 0) {
            throw new IllegalArgumentException("Method seed outside PE32 VA space");
        }
        return value;
    }

    private static void row(PrintWriter out, String... fields) {
        for (int i = 0; i < fields.length; i++) {
            if (i != 0) out.print(",");
            out.print("\"" + fields[i].replace("\"", "\"\"")
                .replace("\n", " ").replace("\r", " ") + "\"");
        }
        out.println();
    }

    private static void privatePermissions(Path path, boolean directory) throws Exception {
        try {
            Files.setPosixFilePermissions(path, PosixFilePermissions.fromString(
                directory ? "rwx------" : "rw-------"));
        } catch (UnsupportedOperationException windows) {
            // On hosted Linux runners the surrounding wrapper uses umask 077.
        }
    }

    private static final class Reviewed {
        final String name;
        final int slot;
        final long address;
        Reviewed(String interfaceName, int index, long entry) {
            name = interfaceName;
            slot = index;
            address = entry;
        }
    }

    @Override
    public void run() throws Exception {
        if (!"1".equals(System.getenv("SIN_PRIVATE_DECOMP_ACK"))) {
            throw new SecurityException("Explicit private decompilation acknowledgement required");
        }
        String[] args = getScriptArgs();
        if (args.length != 2) {
            throw new IllegalArgumentException("Expected reviewed vtable seeds directory and private output root");
        }
        String module = currentProgram.getName();
        List<String> selection = selectedKeys(module);
        if (selection.size() > MAX_SELECTED_PER_MODULE) {
            throw new IllegalStateException("Unbounded priority method selection");
        }
        boolean synthetic = module.equals("synthetic");
        File seed = new File(args[0], module + "_vtable_method_seeds.csv").getCanonicalFile();
        if (!seed.isFile()) {
            throw new IllegalStateException("Authenticated method seed CSV missing");
        }
        List<String> lines = Files.readAllLines(seed.toPath(), StandardCharsets.UTF_8);
        if (lines.size() < 2 || lines.size() > MAX_REVIEWED_SEEDS + 1
                || !lines.get(0).equals("entry,interface,slot,source")) {
            throw new IllegalArgumentException("Invalid or oversized reviewed method seed CSV");
        }
        Map<String, Reviewed> reviewed = new HashMap<>();
        Set<Long> seenTargets = new HashSet<>();
        for (int i = 1; i < lines.size(); i++) {
            String[] col = lines.get(i).split(",", -1);
            if (col.length != 4 || !col[1].matches("[A-Za-z0-9_]+")
                    || !col[2].matches("[0-9]{1,3}")
                    || !(synthetic ? SYNTHETIC_SOURCE : ORIGINAL_SOURCE).equals(col[3])) {
                throw new IllegalArgumentException("Unreviewed method seed row");
            }
            int slot = Integer.parseInt(col[2]);
            if (slot > 255) throw new IllegalArgumentException("Invalid virtual slot");
            long va = pe32(col[0]);
            String key = col[1] + ":" + slot;
            if (reviewed.putIfAbsent(key, new Reviewed(col[1], slot, va)) != null
                    || !seenTargets.add(va)) {
                throw new IllegalArgumentException("Repeated interface/slot or target address");
            }
        }
        List<Reviewed> batch = new ArrayList<>();
        for (String key : selection) {
            Reviewed method = reviewed.get(key);
            if (method == null) {
                throw new IllegalArgumentException("Missing priority vtable seed " + key);
            }
            batch.add(method);
        }
        File privateRoot = new File(args[1]).getCanonicalFile();
        if (!privateRoot.isDirectory() && !privateRoot.mkdirs()) {
            throw new IllegalStateException("Cannot create private decompilation root");
        }
        privatePermissions(privateRoot.toPath(), true);
        Path pseudocodeDir = new File(privateRoot,
            module + "_PRIVATE_VTABLE_METHOD_DECOMPILATION").toPath();
        Files.createDirectories(pseudocodeDir);
        privatePermissions(pseudocodeDir, true);
        Path statusFile = new File(privateRoot,
            module + "_vtable_targeted_decomp_status.csv").toPath();

        int decompiled = 0;
        DecompInterface decompiler = new DecompInterface();
        try {
            if (!decompiler.openProgram(currentProgram)) {
                throw new IllegalStateException("Ghidra decompiler failed to open program");
            }
            try (PrintWriter statuses = new PrintWriter(Files.newBufferedWriter(
                statusFile, StandardCharsets.UTF_8,
                StandardOpenOption.CREATE_NEW, StandardOpenOption.WRITE))) {
                row(statuses, "interface", "slot", "entry", "outcome",
                    "inferred_function_bytes");
                for (Reviewed method : batch) {
                    monitor.checkCancelled();
                    Address at = currentProgram.getAddressFactory()
                        .getDefaultAddressSpace().getAddress(method.address);
                    MemoryBlock block = currentProgram.getMemory().getBlock(at);
                    Function function = currentProgram.getFunctionManager().getFunctionAt(at);
                    String outcome = "missing_inferred_function";
                    String length = "";
                    if (block == null || !block.isExecute()) {
                        outcome = "not_executable";
                    } else if (function != null && !function.isExternal()) {
                        long extent = function.getBody().getNumAddresses();
                        length = Long.toString(extent);
                        if (extent > MAX_INFERRED_FUNCTION_EXTENT) {
                            outcome = "excessive_inferred_function_extent";
                        } else {
                            try {
                                DecompileResults result = decompiler.decompileFunction(
                                    function, 25, monitor);
                                if (result == null || !result.decompileCompleted()
                                        || result.getDecompiledFunction() == null) {
                                    outcome = "decompilation_failed";
                                } else {
                                    String c = result.getDecompiledFunction().getC();
                                    if (c == null || c.isEmpty()) {
                                        outcome = "empty_decompilation";
                                    } else if (c.length() > MAX_DECOMPILED_CHARS) {
                                        outcome = "oversized_decompilation";
                                    } else {
                                        Path file = pseudocodeDir.resolve(
                                            method.name + "_slot" + method.slot + "_" +
                                            String.format("%08x", method.address) + ".c");
                                        try (java.io.BufferedWriter output =
                                            Files.newBufferedWriter(file,
                                                StandardCharsets.UTF_8,
                                                StandardOpenOption.CREATE_NEW,
                                                StandardOpenOption.WRITE)) {
                                            output.write(c);
                                        }
                                        privatePermissions(file, false);
                                        outcome = "decompiled_candidate";
                                        decompiled++;
                                    }
                                }
                            } catch (Exception error) {
                                // Never log exception text, disassembly or original bytes.
                                outcome = "decompilation_failed";
                            }
                        }
                    }
                    row(statuses, method.name, Integer.toString(method.slot),
                        String.format("0x%08X", method.address), outcome, length);
                }
            }
            privatePermissions(statusFile, false);
        } finally {
            decompiler.dispose();
        }
        println("SIN_TARGETED_VTABLE_METHOD_DECOMP " + module + " selected="
            + batch.size() + " private_candidate_c=" + decompiled
            + " unavailable=" + (batch.size()-decompiled)
            + " verified_original_methods=0 verified_abi_signatures=0");
    }
}
