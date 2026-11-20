/* The rebuilt `exit_with_message` module.
 *
 * The original sits at linear 0x30389 in STARTH.EXE and holds 25 instructions
 * and 62 bytes up to the DOS exit at 0x303bf.  The plan row counts 28
 * instructions and 68 bytes to the `ret` at 0x303cc, because the next record
 * starts at 0x4bee0 and the identity window crosses the small DS-restore stub.
 * The store record keeps the extent and sets no separate function size.
 *
 * The body is the Watcom C/C++ run-time routine `__do_exit_with_msg_` from
 * `bld/clib/startup/a/cstrt386.asm` of OpenWatcom V2.  The original takes the
 * message pointer in EAX and the exit code in EDX, opens the console device
 * `con`, writes the zero-terminated message, runs the finalizer routines with
 * the priority 0xff, and terminates the process with the exit code.  The
 * original version writes no newline after the message, unlike the current
 * OpenWatcom source.
 *
 * The direct callers pass the run-time messages "Not enough memory to allocate
 * file structures", "Floating-point support not loaded", and "Stack
 * Overflow!".  The source match, the console device, the message write, and the
 * terminator support the name `exit_with_message`.  The original has no symbol,
 * so the record keeps the name with the confidence inferred.
 *
 * The original has no EBP frame, so the module builds without `-of+`.  The
 * function uses no floating-point instruction, so the module links no FPU stub
 * and uses no assembly implementation.
 */
extern int exit_with_message_open(const char *name);
extern void exit_with_message_write(int handle, const char *text,
                                    unsigned int length);
extern void exit_with_message_finish(void);
extern void exit_with_message_terminate(int status);

void exit_with_message(const char *message, int status)
{
    int handle;
    unsigned int length;

    handle = exit_with_message_open("con");
    length = 0;
    while (message[length] != 0) {
        length = length + 1;
    }
    exit_with_message_write(handle, message, length);
    exit_with_message_finish();
    exit_with_message_terminate(status);
}
