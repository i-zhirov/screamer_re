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

import java.util.Iterator;

public class LeIdentity extends GhidraScript {

    private static final long PAGE0_MODULE = 0x10000L;
    private static final String UNKNOWN = "unknown";
    private static final String IMAGE_BLOCK = ".image";
    private static final long MZ_LFANEW_OFFSET = 0x3cL;
    private static final long LE_PAGE_SIZE_OFFSET = 0x28L;
    private static final long LE_OBJECT_TABLE_OFFSET = 0x40L;
    private static final long LE_OBJECT_PAGE_TABLE_OFFSET = 0x48L;
    private static final long LE_DATA_PAGES_OFFSET = 0x80L;
    private static final long OBJECT_PAGE_TABLE_INDEX_OFFSET = 0x0cL;

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

        println("=== identity");
        println("entry=" + entryValue());
        println("page0_module=" + format(PAGE0_MODULE));
        println("page0_file=" + page0FileValue());
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

    private String page0FileValue() {
        try {
            MemoryBlock image =
                    currentProgram.getMemory().getBlock(IMAGE_BLOCK);
            if (image == null || !image.isInitialized()) {
                return UNKNOWN;
            }
            Address base = image.getStart();
            long lfanew = readLe32(base, MZ_LFANEW_OFFSET);
            long pageSize = readLe32(base, lfanew + LE_PAGE_SIZE_OFFSET);
            long objectTable =
                    readLe32(base, lfanew + LE_OBJECT_TABLE_OFFSET);
            long pageTable =
                    readLe32(base, lfanew + LE_OBJECT_PAGE_TABLE_OFFSET);
            long dataPages = readLe32(base, lfanew + LE_DATA_PAGES_OFFSET);
            long pageTableIndex = readLe32(
                    base,
                    lfanew + objectTable + OBJECT_PAGE_TABLE_INDEX_OFFSET);
            if (pageTableIndex < 1) {
                return UNKNOWN;
            }
            long entryOffset = lfanew + pageTable + (pageTableIndex - 1) * 4;
            long raw = readBe32(base, entryOffset);
            long filePage = raw >>> 8;
            if (filePage < 1) {
                return UNKNOWN;
            }
            long fileOffset = dataPages + (filePage - 1) * pageSize;
            if (fileOffset < 0 || fileOffset >= image.getSize()) {
                return UNKNOWN;
            }
            return format(fileOffset);
        } catch (Exception error) {
            return UNKNOWN;
        }
    }

    private long readLe32(Address base, long offset) throws Exception {
        byte[] buffer = new byte[4];
        currentProgram.getMemory().getBytes(base.add(offset), buffer);
        return (buffer[0] & 0xffL)
                | ((buffer[1] & 0xffL) << 8)
                | ((buffer[2] & 0xffL) << 16)
                | ((buffer[3] & 0xffL) << 24);
    }

    private long readBe32(Address base, long offset) throws Exception {
        byte[] buffer = new byte[4];
        currentProgram.getMemory().getBytes(base.add(offset), buffer);
        return ((buffer[0] & 0xffL) << 24)
                | ((buffer[1] & 0xffL) << 16)
                | ((buffer[2] & 0xffL) << 8)
                | (buffer[3] & 0xffL);
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
