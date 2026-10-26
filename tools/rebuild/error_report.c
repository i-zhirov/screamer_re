/* The rebuilt `error_report` module.
 *
 * The original sits at linear 0x10978 in STARTH.EXE and holds 16 dispatch instructions plus
 * 21 message bodies, one per code:
 *
 *     push ebp; mov ebp,esp; sub esp,4
 *     mov [ebp-4],eax          ; the code argument
 *     mov eax,[ebp-4]
 *     inc eax                   ; the Watcom jump-table bias
 *     cmp eax,0x17              ; 23 entries
 *     ja 0x10b87                ; the default
 *     jmp dword ptr cs:[eax*4 + 0x918]
 *
 * Each body pushes a message offset and the message-table base 0x2fd2, calls the printer
 * at 0x4bf64, and returns.  The codes 2, 3, 4, 5, 6, 8, 11, and the default push a
 * format offset 0x7fe8 as well, so the printer is a printf-style call.
 *
 * The compiler's own jump table does not work in the rebuilt image: the table lands in the
 * code object, its displacement and its entries keep the object-relative value 0 and 1, and the
 * loader does not relocate them, so the jump reaches offset 1 and faults.  This rebuild builds
 * the table at run time instead.  The handlers are real functions, so the compiler emits their
 * addresses as data that the loader relocates, the same way the `exit` module reaches its
 * function-pointer slots.
 */
extern void __cdecl report(char *table, ...);
extern char message_table[];

static void msg_0(void) { report(message_table, 0x44); }
static void msg_1(void) { report(message_table, 0x6f); }
static void msg_2(void) { report(message_table, 0x74, 0x7fe8); }
static void msg_3(void) { report(message_table, 0xa3, 0x7fe8); }
static void msg_4(void) { report(message_table, 0xd1, 0x7fe8); }
static void msg_5(void) { report(message_table, 0xff, 0x7fe8); }
static void msg_6(void) { report(message_table, 0x116, 0x7fe8); }
static void msg_7(void) { report(message_table, 0x141); }
static void msg_8(void) { report(message_table, 0x16e, 0x7fe8); }
static void msg_9(void) { report(message_table, 0x18b); }
static void msg_10(void) { report(message_table, 0x1c0); }
static void msg_11(void) { report(message_table, 0x1f8, 0x7fe8); }
static void msg_12(void) { report(message_table, 0x232); }
static void msg_13(void) { report(message_table, 0x252); }
static void msg_14(void) { report(message_table, 0x288); }
static void msg_15(void) { report(message_table, 0x2a7); }
static void msg_16(void) { report(message_table, 0x2d8); }
static void msg_17(void) { report(message_table, 0x307); }
static void msg_18(void) { report(message_table, 0x330); }
static void msg_19(void) { report(message_table, 0x35a); }

static void (*handlers[20])(void);

void error_report(int code)
{
    handlers[0] = msg_0;
    handlers[1] = msg_1;
    handlers[2] = msg_2;
    handlers[3] = msg_3;
    handlers[4] = msg_4;
    handlers[5] = msg_5;
    handlers[6] = msg_6;
    handlers[7] = msg_7;
    handlers[8] = msg_8;
    handlers[9] = msg_9;
    handlers[10] = msg_10;
    handlers[11] = msg_11;
    handlers[12] = msg_12;
    handlers[13] = msg_13;
    handlers[14] = msg_14;
    handlers[15] = msg_15;
    handlers[16] = msg_16;
    handlers[17] = msg_17;
    handlers[18] = msg_18;
    handlers[19] = msg_19;
    if (code >= 0 && code < 20) {
        handlers[code]();
    } else {
        report(message_table, 0x383, 0x7fe8);
    }
}
