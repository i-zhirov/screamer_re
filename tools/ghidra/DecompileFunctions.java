// Decompile the functions that contain the given addresses.
//
// Run it through analyzeHeadless with -postScript:
//
//   analyzeHeadless work/ghidra/proj Screamer \
//     -import STARTH.EXE -overwrite \
//     -scriptPath tools/ghidra \
//     -postScript DecompileFunctions.java 0x10020 0x2ecf8
//
// The output is the decompiled C of each function, preceded by the requested
// address, the function name, and the function entry point.

import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.FunctionManager;

public class DecompileFunctions extends GhidraScript {

    @Override
    public void run() throws Exception {
        String[] arguments = getScriptArgs();
        if (arguments.length == 0) {
            println("usage: DecompileFunctions <address> ...");
            return;
        }
        println("entry point: " + currentProgram.getSymbolTable().getExternalEntryPointIterator());
        DecompInterface decompiler = new DecompInterface();
        decompiler.toggleCCode(true);
        decompiler.openProgram(currentProgram);
        FunctionManager functions = currentProgram.getFunctionManager();
        for (String argument : arguments) {
            Address address = currentProgram.getAddressFactory()
                    .getDefaultAddressSpace().getAddress(argument);
            Function function = functions.getFunctionContaining(address);
            if (function == null) {
                println("=== " + argument + ": no function");
                continue;
            }
            DecompileResults results =
                    decompiler.decompileFunction(function, 60, monitor);
            println("=== " + argument + " -> " + function.getName()
                    + " @ " + function.getEntryPoint());
            if (results.decompileCompleted()) {
                println(results.getDecompiledFunction().getC());
            } else {
                println("decompile failed: " + results.getErrorMessage());
            }
        }
        decompiler.dispose();
    }
}
