/* The rebuilt `memory_error_report` module.
 *
 * The original sits at linear 0x2edad in STARTH.EXE and holds 97 instructions
 * and 291 bytes: the five register saves, the frame, the two frame locals, the
 * status read at `+0xc`, the six message bodies, the binary dispatch tree, and
 * the final `exit(-1)` call at `0x300be`.
 *
 * The record at the argument pointer holds the slot count at `+0x08`, the
 * status at `+0x0c`, and the handler name at `+0x10`.  The function prints one
 * message per status through the printer at `0x4bf64` with the stream
 * `0x2fd2`, and then stops the process with the code -1:
 *
 *     1        "Cannot allocate memory for: %s. Out of memory.\n" with the name
 *     2        the no-slots message with the name, then the slot count
 *     3        "Handler not present in memory management list."
 *     7        "Memory control blocks damaged. System corrupted."
 *     9        "Memory handler structures corrupted. Aborted"
 *     other    "Unknown error in memory management"
 *
 * The status values are sparse, so the compiler builds a comparison tree and
 * not a jump table.  The dispatch tests the status local at `[ebp-8]`, so the
 * local is volatile.  The handler pointer is volatile too: the original stores
 * the argument at `[ebp-4]` and loads it again for each body.  The original
 * holds the EBP frame, so the module builds with `-of+`.
 */
extern void __cdecl report(char *stream, char *format, ...);
extern void exit(int status);

typedef struct memory_handler {
    int reserved_00;
    int reserved_04;
    int slots;
    int status;
} memory_handler;

void memory_error_report(memory_handler * volatile handler)
{
    volatile int status = handler->status;

    switch (status) {
    case 1:
        report((char *)0x2fd2, "Cannot allocate memory for: %s. Out of memory.\n",
               (char *)handler + 0x10);
        break;
    case 2:
        report((char *)0x2fd2, "No slots for allocating: %s. Resize memory handler.\n",
               (char *)handler + 0x10);
        report((char *)0x2fd2, "Actual number of slots: %d\n", handler->slots);
        break;
    case 3:
        report((char *)0x2fd2, "Handler not present in memory management list.");
        break;
    case 7:
        report((char *)0x2fd2, "Memory control blocks damaged. System corrupted.");
        break;
    case 9:
        report((char *)0x2fd2, "Memory handler structures corrupted. Aborted");
        break;
    default:
        report((char *)0x2fd2, "Unknown error in memory management");
        break;
    }
    exit(-1);
}
