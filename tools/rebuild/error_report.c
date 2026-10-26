/* The rebuilt `error_report` module.
 *
 * The original sits at linear 0x10978 in STARTH.EXE and holds 16 dispatch instructions
 * plus 21 message bodies, one per code:
 *
 *     push ebp; mov ebp,esp; sub esp,4
 *     mov [ebp-4],eax          ; the code argument
 *     mov eax,[ebp-4]
 *     inc eax                   ; the Watcom jump-table bias
 *     cmp eax,0x17              ; 23 entries
 *     ja 0x10b87                ; the default
 *     jmp dword ptr cs:[eax*4 + 0x918]
 *
 * Each body pushes a message offset and the message-table base `0x2fd2`, calls the printer
 * at `0x4bf64`, and returns.  The codes 2, 3, 4, 5, 6, 8, 11, and the default push a
 * format offset `0x7fe8` as well, so the printer is a printf-style call.
 *
 * The jump table at the linear 0x10918 holds 23 object-relative entries, and the codes 20,
 * 21, and 22 point at the default body.
 */
extern void __cdecl report(char *table, ...);
extern char message_table[];

void error_report(int code)
{
    switch (code) {
    case 0: report(message_table, 0x44); break;
    case 1: report(message_table, 0x6f); break;
    case 2: report(message_table, 0x74, 0x7fe8); break;
    case 3: report(message_table, 0xa3, 0x7fe8); break;
    case 4: report(message_table, 0xd1, 0x7fe8); break;
    case 5: report(message_table, 0xff, 0x7fe8); break;
    case 6: report(message_table, 0x116, 0x7fe8); break;
    case 7: report(message_table, 0x141); break;
    case 8: report(message_table, 0x16e, 0x7fe8); break;
    case 9: report(message_table, 0x18b); break;
    case 10: report(message_table, 0x1c0); break;
    case 11: report(message_table, 0x1f8, 0x7fe8); break;
    case 12: report(message_table, 0x232); break;
    case 13: report(message_table, 0x252); break;
    case 14: report(message_table, 0x288); break;
    case 15: report(message_table, 0x2a7); break;
    case 16: report(message_table, 0x2d8); break;
    case 17: report(message_table, 0x307); break;
    case 18: report(message_table, 0x330); break;
    case 19: report(message_table, 0x35a); break;
    default: report(message_table, 0x383, 0x7fe8); break;
    }
}
