// Print the instructions of the functions that contain the given addresses.
//
// The LX/LE loader applies the fixups, so the operands of a `push` show the full
// resolved address of a string or a data object, which the decompiler hides.
//
// Run it through analyzeHeadless with -postScript:
//
//   analyzeHeadless work/ghidra/proj Screamer \
//     -process STARTH.EXE -noanalysis \
//     -scriptPath tools/ghidra \
//     -postScript DisassembleFunctions.java 0x131a8

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.Listing;

public class DisassembleFunctions extends GhidraScript {

    @Override
    public void run() throws Exception {
        String[] arguments = getScriptArgs();
        if (arguments.length == 0) {
            println("usage: DisassembleFunctions <address> ...");
            return;
        }
        Listing listing = currentProgram.getListing();
        for (String argument : arguments) {
            Address address = currentProgram.getAddressFactory()
                    .getDefaultAddressSpace().getAddress(argument);
            Function function = getFunctionContaining(address);
            if (function == null) {
                println("=== " + argument + " is not in a function");
                continue;
            }
            println("=== " + function.getName() + " @ " + function.getEntryPoint());
            Instruction instruction = listing.getInstructionAt(function.getEntryPoint());
            while (instruction != null && function.getBody().contains(instruction.getAddress())) {
                println("  " + instruction.getAddress() + "  " + instruction.toString());
                instruction = instruction.getNext();
            }
        }
    }
}
