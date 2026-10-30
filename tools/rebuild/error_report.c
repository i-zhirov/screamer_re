/* The rebuilt `error_report` module.
 *
 * The original sits at linear 0x10978 in STARTH.EXE and holds 16 dispatch instructions
 * plus one message body per code:
 *
 *     push ebp; mov ebp,esp; sub esp,4
 *     mov [ebp-4],eax          ; the code argument
 *     mov eax,[ebp-4]
 *     inc eax                   ; the Watcom jump-table bias
 *     cmp eax,0x17              ; 24 entries, one per code -1 to 22
 *     ja 0x10b87                ; the default
 *     jmp dword ptr cs:[eax*4 + 0x918]
 *
 * Each body holds its own `push` of a format-string address and the stream object `0x2fd2`,
 * a call to the printer at `0x4bf64`, `add esp`, and the full epilogue.  The original does
 * not share a body or the epilogue.  The codes 1, 2, 3, 4, 5, 7, and 10, and the default,
 * push a second argument `0x7fe8` as well, so the printer is a printf-style call.  The
 * offsets are format-string addresses in the data segment: `0x44` points at "Cannot free
 * memory of a duplicated object!", `0x6f` at "OK.\n", and so on.  The source passes string
 * literals, so the compiler emits the long `push imm32` with a relocation; an integer
 * constant that fits in a signed byte would become the short `push imm8`.
 *
 * The `inc eax` biases the table by one: the original switch holds the codes -1 through 22,
 * so the table at the linear 0x10918 holds 24 entries.  The codes 19, 20, and 21 point at
 * the default body, and the code 22 prints the offset 0x369.  A period Watcom 10.0a build
 * emits `inc eax` with `cmp eax,0x17` only for a range that starts at -1 and ends at 22;
 * a range that starts at 0 emits no `inc`.  Two callers in the game pass the constant 22.
 *
 * The parameter is volatile.  The original stores the argument to the frame and reloads it
 * before the switch (`sub esp,4`, `mov [ebp-4],eax`, `mov eax,[ebp-4]`); a plain parameter
 * stays in eax and produces no local.  The volatile qualifier reproduces the store and the
 * reload, and the same frame-local pattern appears 32 times in the original text.
 *
 * The module builds with `-of+ -ot`.  The frame is forced, and the time optimization keeps
 * one full body per code: the size optimization merges the common call tails, and the
 * original does not merge them.
 */
extern void __cdecl report(char *table, ...);
/* The original saves no register around the call: the printer declaration reports that
 * only eax changes.  The auxiliary pragma also keeps the C name mapping (`_*`), because a
 * pragma without a name string maps the symbol to its default object name. */
#pragma aux report "_*" modify exact [eax];
extern char message_table[];

void error_report(volatile int code)
{
    switch (code) {
    case -1: report(message_table, "Cannot free memory of a duplicated object!"); break;
    case 0: report(message_table, "OK.\n"); break;
    case 1: report(message_table, "Error while searching for object file: %s.3dx\n", 0x7fe8); break;
    case 2: report(message_table, "An error occurred while opening file: %s.3dx\n", 0x7fe8); break;
    case 3: report(message_table, "Not enough memory for storing object: %s.3dx\n", 0x7fe8); break;
    case 4: report(message_table, "File %s is corrupted.\n", 0x7fe8); break;
    case 5: report(message_table, "Not enough memory for storing object: %s.\n", 0x7fe8); break;
    case 6: report(message_table, "Not enough memory for creating new objects.\n"); break;
    case 7: report(message_table, "Object %s not found in list\n", 0x7fe8); break;
    case 8: report(message_table, "Maximum number of objects reached. Modify MAX_OBJS.\n"); break;
    case 9: report(message_table, "Cannot free an object that has not been allocated, yet!"); break;
    case 10: report(message_table, "Cannot put in list %s. Maximum number of objects reached!", 0x7fe8); break;
    case 11: report(message_table, "Cannot free a visible object !!"); break;
    case 12: report(message_table, "Object not present in visual-list, cannot get out it."); break;
    case 13: report(message_table, "Object already in visual-list."); break;
    case 14: report(message_table, "Cannot toggle: object not present in visual-list"); break;
    case 15: report(message_table, "Did not succeed in freeing an object. Aborted!"); break;
    case 16: report(message_table, "Cannot create sprite: object not loaded."); break;
    case 17: report(message_table, "Cannot animate object: object not loaded."); break;
    case 18: report(message_table, "Out of memory."); break;
    case 22: report(message_table, "General failure: aborted."); break;
    default: report(message_table, "Unknown error in engine interface <%s>.\n", 0x7fe8); break;
    }
}
