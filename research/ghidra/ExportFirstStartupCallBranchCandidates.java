// @category SiN
// PRIVATE, SHA-gated owner run only. Candidate call/branch sites, NOT proof
// of factory identity, request order, or original method semantics.
// Exports no original instructions, decompiled C, or raw strings.
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.InstructionIterator;
import ghidra.program.model.symbol.Reference;
import ghidra.program.model.listing.Data;
import java.io.File;
import java.io.PrintWriter;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.StandardOpenOption;
import java.util.ArrayList;
import java.util.List;

public class ExportFirstStartupCallBranchCandidates extends GhidraScript {
    private static final int MAX_BYTES = 65536;
    private static final int MAX_INSTRUCTIONS = 25000;
    private static final int MAX_CANDIDATES = 8;
    private static final int PRECEDING_WINDOW = 32;
    private static final int FOLLOWING_WINDOW = 16;
    private static final String VERSION =
        "(?:V[A-Za-z][A-Za-z0-9_]{1,48}|ServerGame[A-Za-z0-9_]{1,48}|" +
        "GameClientExports)[0-9]{3}";
    private static void cell(PrintWriter writer, String value) {
        writer.print("\"" + value.replace("\"", "\"\"").replace("\n", " ") + "\"");
    }
    private static void row(PrintWriter writer, String... values) {
        for (int i = 0; i < values.length; i++) {
            if (i != 0) writer.print(",");
            cell(writer, values[i]);
        }
        writer.println();
    }
    private static String relative(Address start, Address at) {
        return Long.toString(at.subtract(start));
    }
    private static String literalCandidate(Instruction ins) {
        for (Reference ref : ins.getReferencesFrom()) {
            if (!ref.getReferenceType().isData()) continue;
            Data data = currentData(ins, ref);
            if (data == null) continue;
            Object value = data.getValue();
            if (value instanceof String && ((String)value).matches(VERSION))
                return (String)value;
        }
        return "";
    }
    // Instance helper, so Ghidra's currentProgram is accessed only in script context.
    private static Data currentData(Instruction ins, Reference ref) {
        return ins.getProgram().getListing().getDataAt(ref.getToAddress());
    }
    @Override
    public void run() throws Exception {
        if (!"1".equals(System.getenv("SIN_PRIVATE_DECOMP_ACK")))
            throw new SecurityException("Private original analysis not acknowledged");
        String[] args = getScriptArgs();
        if (args.length != 2)
            throw new IllegalArgumentException("Expected authenticated seed directory and private output directory");
        String module = currentProgram.getName();
        String iface;
        if ("client.dll".equals(module)) iface = "VClient011";
        else if ("server.dll".equals(module)) iface = "ServerGameDLL003";
        else throw new SecurityException("Original PE32 startup modules only");
        File seed = new File(args[0], module + "_vtable_method_seeds.csv").getCanonicalFile();
        List<String> lines = Files.readAllLines(seed.toPath(), StandardCharsets.UTF_8);
        if (lines.size() < 2 || lines.size() > 201 ||
            !"entry,interface,slot,source".equals(lines.get(0)))
            throw new SecurityException("Missing bounded SHA-reviewed vtable seeds");
        long entry = -1;
        for (int i = 1; i < lines.size(); i++) {
            String[] fields = lines.get(i).split(",", -1);
            if (fields.length != 4 || !fields[0].matches("(?i)[0-9a-f]{1,16}") ||
                !fields[1].matches("[A-Za-z0-9_]+") ||
                !fields[2].matches("[0-9]{1,3}") ||
                !"sha256_authenticated_original_pe32_vtable_prefix".equals(fields[3]))
                throw new SecurityException("Invalid reviewed seed row");
            long value = Long.parseUnsignedLong(fields[0], 16);
            if (value == 0 || Long.compareUnsigned(value, 0xffffffffL) > 0)
                throw new SecurityException("Not a PE32 seed");
            if (iface.equals(fields[1]) && "0".equals(fields[2])) {
                if (entry != -1) throw new SecurityException("Duplicate startup entry");
                entry = value;
            }
        }
        long expected = "client.dll".equals(module) ? 0x240FE0E0L : 0x221E7640L;
        if (entry != expected) throw new SecurityException("Unreviewed startup entry");
        Address start = currentProgram.getAddressFactory().getDefaultAddressSpace().getAddress(entry);
        Function fn = currentProgram.getFunctionManager().getFunctionAt(start);
        if (fn == null || fn.isExternal() || fn.getBody().getNumAddresses() == 0 ||
            fn.getBody().getNumAddresses() > MAX_BYTES)
            throw new IllegalStateException("Missing or unbounded exact Ghidra startup function");
        List<Instruction> instructions = new ArrayList<>();
        InstructionIterator iterator = currentProgram.getListing().getInstructions(fn.getBody(), true);
        while (iterator.hasNext()) {
            monitor.checkCancelled();
            if (instructions.size() >= MAX_INSTRUCTIONS)
                throw new IllegalStateException("Startup method instruction cap exceeded");
            instructions.add(iterator.next());
        }
        File directory = new File(args[1]).getCanonicalFile();
        if (!directory.isDirectory() && !directory.mkdirs())
            throw new IllegalStateException("Cannot create private output directory");
        File output = new File(directory, module + "_first_factory_branch_candidates.csv");
        if (output.exists()) throw new IllegalStateException("Refuse private output overwrite");
        try (PrintWriter writer = new PrintWriter(Files.newBufferedWriter(
                output.toPath(), StandardCharsets.UTF_8, StandardOpenOption.CREATE_NEW))) {
            row(writer, "interface", "slot", "entry", "candidate_rank",
                "computed_call_relative_offset", "nearest_prior_version_literal",
                "prior_literal_relative_offset", "next_conditional_relative_offset",
                "next_conditional_distance_in_instructions", "evidence_tier");
            int count = 0;
            for (int i = 0; i < instructions.size() && count < MAX_CANDIDATES; i++) {
                Instruction call = instructions.get(i);
                if (!call.getFlowType().isCall() || !call.getFlowType().isComputed()) continue;
                String literal = "";
                String literalOffset = "";
                for (int j = i - 1; j >= Math.max(0, i - PRECEDING_WINDOW); j--) {
                    String candidate = literalCandidate(instructions.get(j));
                    if (!candidate.isEmpty()) {
                        literal = candidate;
                        literalOffset = relative(start, instructions.get(j).getAddress());
                        break;
                    }
                }
                String branchOffset = "";
                String branchDistance = "";
                for (int j = i + 1; j < instructions.size() &&
                        j <= i + FOLLOWING_WINDOW; j++) {
                    Instruction candidate = instructions.get(j);
                    if (candidate.getFlowType().isConditional()) {
                        branchOffset = relative(start, candidate.getAddress());
                        branchDistance = Integer.toString(j - i);
                        break;
                    }
                }
                row(writer, iface, "0", String.format("0x%08X", entry),
                    Integer.toString(++count), relative(start, call.getAddress()),
                    literal, literalOffset, branchOffset, branchDistance,
                    "ghidra_inferred_nearby_sites_not_verified_factory_or_failure");
            }
        }
        println("SIN_FIRST_FACTORY_BRANCH_CANDIDATES " + module +
            " private_bounded_metadata_only=1 verified_factory_calls=0 verified_failure_edges=0");
    }
}
