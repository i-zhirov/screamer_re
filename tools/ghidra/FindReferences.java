// Print every reference to the given addresses and the function that holds it.
//
// Run it through analyzeHeadless with -postScript:
//
//   analyzeHeadless work/ghidra/proj Screamer \
//     -import STARTH.EXE -overwrite \
//     -scriptPath tools/ghidra \
//     -postScript FindReferences.java 0x1000a3
//
// The LX/LE loader applies the fixups, so a data reference that is a low offset
// in the file resolves to the full address here.

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.symbol.Reference;
import ghidra.program.model.symbol.ReferenceManager;

public class FindReferences extends GhidraScript {

    @Override
    public void run() throws Exception {
        String[] arguments = getScriptArgs();
        if (arguments.length == 0) {
            println("usage: FindReferences <address> ...");
            return;
        }
        ReferenceManager references = currentProgram.getReferenceManager();
        for (String argument : arguments) {
            Address address = currentProgram.getAddressFactory()
                    .getDefaultAddressSpace().getAddress(argument);
            println("=== references to " + argument);
            int count = 0;
            for (Reference reference : references.getReferencesTo(address)) {
                Address from = reference.getFromAddress();
                Function function = getFunctionContaining(from);
                String name = function == null ? "(nofunc)" : function.getName();
                println("  " + name + " @ " + from + " [" + reference.getReferenceType() + "]");
                count++;
            }
            if (count == 0) {
                println("  none");
            }
        }
    }
}
