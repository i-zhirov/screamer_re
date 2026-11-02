// Extract the identity facts from the loaded program.
//
// Run it through identity.sh, which imports the executable and supplies the
// `--main` and `--call-site` addresses:
//
//     analyzeHeadless work/proj Screamer -import STARTH.EXE -overwrite \
//         -scriptPath tools/ghidra \
//         -postScript LeIdentity.java --main 0x10020 --call-site 0xd4364
//
// The output starts with `=== identity` and holds one key per line.  An
// unresolved key prints `key=unknown`.  A fatal condition prints
// `error: <message>` to stderr and throws.

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.mem.MemoryBlock;
import ghidra.program.model.mem.MemoryBlockSourceInfo;

import java.util.Iterator;

public class LeIdentity extends GhidraScript {

    private static final long PAGE0_MODULE = 0x10000L;
    private static final String UNKNOWN = "unknown";

    @Override
    public void run() throws Exception {
        String[] arguments = getScriptArgs();
        String mainArgument = null;
        String callSiteArgument = null;
        for (int i = 0; i < arguments.length; i++) {
            if ("--main".equals(arguments[i]) && i + 1 < arguments.length) {
                mainArgument = arguments[++i];
            } else if ("--call-site".equals(arguments[i])
                    && i + 1 < arguments.length) {
                callSiteArgument = arguments[++i];
            } else {
                fail("unknown argument: " + arguments[i]);
            }
        }
        if (mainArgument == null) {
            fail("no --main address");
        }
        if (callSiteArgument == null) {
            fail("no --call-site address");
        }
        if (currentProgram == null) {
            fail("no program is open");
        }

        Address mainAddress = toAddr(mainArgument);
        Address callSite = toAddr(callSiteArgument);
        Address pageAddress = toAddr(PAGE0_MODULE);

        println("=== identity");
        println("entry=" + entryValue());
        println("page0_module=" + format(PAGE0_MODULE));
        println("page0_file=" + page0FileValue(pageAddress));
        println("main=" + functionEntry(mainAddress));
        println("caller=" + functionEntry(callSite));
        println("call_site=" + format(callSite));
        println("call_target=" + callTargetValue(callSite));
    }

    private String entryValue() {
        Iterator<Address> entries =
                currentProgram.getSymbolTable().getExternalEntryPointIterator();
        Address only = null;
        int count = 0;
        while (entries.hasNext()) {
            Address address = entries.next();
            count++;
            if (count == 1) {
                only = address;
            }
        }
        if (count != 1) {
            return UNKNOWN;
        }
        return format(only);
    }

    private String page0FileValue(Address pageAddress) {
        MemoryBlock block = currentProgram.getMemory().getBlock(pageAddress);
        if (block == null) {
            return UNKNOWN;
        }
        for (MemoryBlockSourceInfo info : block.getSourceInfos()) {
            if (info.contains(pageAddress)) {
                long offset = info.getFileBytesOffset(pageAddress);
                if (offset < 0) {
                    return UNKNOWN;
                }
                return format(offset);
            }
        }
        return UNKNOWN;
    }

    private String functionEntry(Address address) {
        Function function = getFunctionContaining(address);
        if (function == null) {
            return UNKNOWN;
        }
        return format(function.getEntryPoint());
    }

    private String callTargetValue(Address callSite) {
        Instruction instruction =
                currentProgram.getListing().getInstructionAt(callSite);
        if (instruction == null || !instruction.getFlowType().isCall()) {
            return UNKNOWN;
        }
        Address[] flows = instruction.getFlows();
        if (flows == null || flows.length == 0) {
            return UNKNOWN;
        }
        return format(flows[0]);
    }

    private static String format(long value) {
        return "0x" + Long.toHexString(value);
    }

    private static String format(Address address) {
        return format(address.getOffset());
    }

    private void fail(String message) throws Exception {
        printerr("error: " + message);
        throw new IllegalStateException(message);
    }
}
