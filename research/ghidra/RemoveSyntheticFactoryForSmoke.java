// @category SiN
// SYNTHETIC CI ONLY: remove one preexisting synthetic function label so the
// subsequent reviewed-address seed pass is forced to create its candidate.
// It refuses every program except a locally built harmless "synthetic" ELF.
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;

public class RemoveSyntheticFactoryForSmoke extends GhidraScript {
    @Override
    public void run() throws Exception {
        if (!currentProgram.getName().equals("synthetic")) {
            throw new IllegalStateException("Synthetic-only smoke may not touch original game programs");
        }
        String[] args = getScriptArgs();
        if (args.length != 1 || !args[0].matches("(?i)[0-9a-f]{1,16}")) {
            throw new IllegalArgumentException("Expected one synthetic ELF function VA");
        }
        long bits = Long.parseUnsignedLong(args[0], 16);
        Address address = currentProgram.getAddressFactory().getDefaultAddressSpace().getAddress(bits);
        Function function = currentProgram.getFunctionManager().getFunctionAt(address);
        if (function == null) {
            throw new IllegalStateException("Synthetic helper was not auto-detected before seed recovery");
        }
        removeFunctionAt(address);
        if (currentProgram.getFunctionManager().getFunctionAt(address) != null) {
            throw new IllegalStateException("Synthetic function was not removed for smoke");
        }
        println("SIN_SYNTHETIC_FACTORY_REMOVED " + address);
    }
}
