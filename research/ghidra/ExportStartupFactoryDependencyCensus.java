// @category SiN
// PRIVATE original-only, two-method startup dependency census. Metadata only.
// Requires the SHA-authenticated vtable seeds from Phase 280. Does not emit
// instruction bytes, disassembly, C pseudocode, arbitrary string contents,
// engine addresses outside two reviewed method entrypoints, or code paths.
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.InstructionIterator;
import ghidra.program.model.symbol.Reference;
import ghidra.program.model.symbol.RefType;
import ghidra.program.model.data.DataType;
import ghidra.program.model.listing.Data;
import java.io.File;
import java.io.PrintWriter;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.StandardOpenOption;
import java.util.List;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.Set;

public class ExportStartupFactoryDependencyCensus extends GhidraScript {
    private static final String SOURCE =
        "sha256_authenticated_original_pe32_vtable_prefix";
    private static final int MAX_SEED_ROWS = 200;
    private static final int MAX_METHOD_BYTES = 65536;
    private static final int MAX_INTERFACE_LITERALS = 32;
    private static final int MAX_INSTRUCTIONS = 25000;
    private static final String VERSION =
        "(?:V[A-Za-z][A-Za-z0-9_]{1,48}|ServerGame[A-Za-z0-9_]{1,48}|" +
        "GameClientExports)[0-9]{3}";

    private static void csv(PrintWriter out, Object... cells) {
        for (int i = 0; i < cells.length; i++) {
            if (i != 0) out.print(",");
            String cell = String.valueOf(cells[i]).replace("\"", "\"\"")
                .replace("\n", " ").replace("\r", " ");
            out.print("\"" + cell + "\"");
        }
        out.println();
    }

    @Override
    public void run() throws Exception {
        if (!"1".equals(System.getenv("SIN_PRIVATE_DECOMP_ACK"))) {
            throw new SecurityException("Private original inspection must be acknowledged");
        }
        String[] args = getScriptArgs();
        if (args.length != 2) {
            throw new IllegalArgumentException("Expected reviewed seed directory and private output directory");
        }
        String module = currentProgram.getName();
        final String iface;
        final String sdkCandidate;
        if ("client.dll".equals(module)) {
            iface = "VClient011";
            sdkCandidate = "Init";
        } else if ("server.dll".equals(module)) {
            iface = "ServerGameDLL003";
            sdkCandidate = "DLLInit";
        } else {
            throw new SecurityException("Original-only program allowlist");
        }
        File seeds = new File(args[0], module + "_vtable_method_seeds.csv").getCanonicalFile();
        List<String> rows = Files.readAllLines(seeds.toPath(), StandardCharsets.UTF_8);
        if (rows.size() < 2 || rows.size() > MAX_SEED_ROWS + 1 ||
            !"entry,interface,slot,source".equals(rows.get(0))) {
            throw new IllegalArgumentException("Unreviewed original seed CSV");
        }
        long methodVa = -1;
        for (int i = 1; i < rows.size(); i++) {
            String[] fields = rows.get(i).split(",", -1);
            if (fields.length != 4 || !fields[0].matches("(?i)[0-9a-f]{1,8}") ||
                !fields[1].matches("[A-Za-z0-9_]+") ||
                !fields[2].matches("[0-9]{1,3}") ||
                !SOURCE.equals(fields[3])) {
                throw new IllegalArgumentException("Unsafe vtable seed");
            }
            if (iface.equals(fields[1]) && "0".equals(fields[2])) {
                if (methodVa != -1) throw new IllegalArgumentException("Duplicate slot zero");
                methodVa = Long.parseLong(fields[0], 16);
            }
        }
        if (methodVa <= 0) throw new IllegalArgumentException("Missing startup method seed");
        Address at = currentProgram.getAddressFactory().getDefaultAddressSpace()
            .getAddress(methodVa);
        Function method = currentProgram.getFunctionManager().getFunctionAt(at);
        String status = "missing_exact_ghidra_function";
        long extent = 0;
        int direct = 0;
        int indirect = 0;
        int instructions = 0;
        Set<String> interfaceNames = new HashSet<>();
        if (method != null && !method.isExternal()) {
            extent = method.getBody().getNumAddresses();
            if (extent == 0 || extent > MAX_METHOD_BYTES) {
                status = "unbounded_inferred_function";
            } else {
                status = "bounded_inferred_function";
                InstructionIterator iter = currentProgram.getListing()
                    .getInstructions(method.getBody(), true);
                while (iter.hasNext()) {
                    monitor.checkCancelled();
                    Instruction ins = iter.next();
                    if (++instructions > MAX_INSTRUCTIONS) {
                        status = "excessive_instruction_count";
                        break;
                    }
                    if (ins.getFlowType().isCall()) {
                        if (ins.getFlowType().isComputed()) indirect++;
                        else direct++;
                    }
                    for (Reference ref : ins.getReferencesFrom()) {
                        if (!ref.getReferenceType().isData()) continue;
                        Data data = currentProgram.getListing().getDataAt(ref.getToAddress());
                        if (data == null) continue;
                        Object value = data.getValue();
                        if (!(value instanceof String)) continue;
                        String name = (String)value;
                        if (name.length() <= 64 && name.matches(VERSION)) {
                            if (interfaceNames.size() < MAX_INTERFACE_LITERALS)
                                interfaceNames.add(name);
                        }
                    }
                }
            }
        }
        File root = new File(args[1]).getCanonicalFile();
        if (!root.isDirectory() && !root.mkdirs()) {
            throw new IllegalStateException("Unable to create private output directory");
        }
        File output = new File(root, module + "_startup_factory_census.csv");
        if (output.exists()) throw new IllegalStateException("Refusing overwrite");
        List<String> names = new ArrayList<>(interfaceNames);
        java.util.Collections.sort(names);
        try (PrintWriter out = new PrintWriter(Files.newBufferedWriter(output.toPath(),
                StandardCharsets.UTF_8, StandardOpenOption.CREATE_NEW))) {
            csv(out, "interface", "slot", "entry", "sdk_method_hypothesis",
                "outcome", "inferred_body_bytes", "instruction_count",
                "direct_call_sites", "indirect_call_sites", "interface_literal_count",
                "interface_version_literals");
            csv(out, iface, "0", String.format("0x%08X", methodVa), sdkCandidate,
                status, extent, instructions, direct, indirect, names.size(),
                String.join(";", names));
        }
        println("SIN_STARTUP_FACTORY_CENSUS " + module + " method=" + iface +
            "[0] status=" + status + " direct_calls=" + direct +
            " indirect_calls=" + indirect + " version_literals=" + names.size() +
            " verified_factory_argument_types=0 first_failure_branch_verified=0");
    }
}
